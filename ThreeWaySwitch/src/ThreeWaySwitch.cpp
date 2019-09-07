#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <CurrentSense.h>
#include <SinricSwitch3Way.h>
#include <WiFiManager.h>
#include <FirmwareUpdater.h>
#include <DeviceConfigurator.h>

#define LAN_HOSTNAME  "ESP_NoConfig"
#define CONFIG_FILE_NAME "/sinricSwitch1.txt"

void alertViaLed();
void toggleRelay();
void resetModule();
void rebootModule();
void updateButtonState()  ICACHE_RAM_ATTR;
void setLedState(boolean newState);

#define SKETCH_VERSION "v20190903-1605"
#define SKETCH_NAME "threeWaySwitch"

DeviceConfigurator* dev1 = nullptr;
SinricSwitch3Way *sinricSwitch = nullptr;
FirmwareUpdater* fwFetcher = nullptr;
IOTConfig swCfg;
volatile byte buttonPressed = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("************************ " SKETCH_VERSION " ***************************");

    Serial.print("Initializing WiFi Manager to capture config with AP Name: ");
    Serial.println(LAN_HOSTNAME);

    WiFiManager wiFiManager;
    wiFiManager.autoConnect(LAN_HOSTNAME);

    Serial.println("");
    Serial.print("WiFi connected (as client.) ");
    Serial.print("LAN IP addr: ");
    Serial.println(WiFi.localIP());

    fwFetcher = new FirmwareUpdater(SKETCH_NAME, SKETCH_VERSION, 3600000);
    
    dev1 = new DeviceConfigurator(CONFIG_FILE_NAME);
    swCfg = dev1->getConfig();

    attachInterrupt(digitalPinToInterrupt(swCfg.inputPin), updateButtonState, FALLING);
    sinricSwitch = new SinricSwitch3Way(swCfg.apiKey, swCfg.deviceID, 80, toggleRelay, alertViaLed, rebootModule, resetModule);
    Serial.printf("Pin config is: LED:%d INPUT:%d RELAY:%d ON-level:%d OFF-level:%d\n", swCfg.ledPin, swCfg.inputPin, swCfg.triggerPin, swCfg.onLevel, swCfg.offLevel);
    pinMode(A0, INPUT);
    pinMode(swCfg.ledPin, OUTPUT);
    pinMode(swCfg.triggerPin, OUTPUT);
    pinMode(swCfg.inputPin, INPUT_PULLUP);

    digitalWrite(swCfg.ledPin, HIGH);    //switch illumination to on
    digitalWrite(swCfg.triggerPin, swCfg.offLevel);   //high-voltage side off


}




void loop() {
  sinricSwitch -> loop();

  //check whether the light is on.  (samples for 250 msec so effectively debounces the momentary switch)
  double currentFlow = calcCurrentFlow(false);
  bool newState = (currentFlow > CURRENT_FLOW_NONZERO_THRESHOLD);
  sinricSwitch -> setPowerState(newState);
  setLedState(newState);
  
  if (buttonPressed > 0) {
      Serial.print("manual button press: " );
      Serial.println(buttonPressed);
      buttonPressed--;
      toggleRelay();
  }

  if (fwFetcher->isUpdateCheckDue()) {
      fwFetcher->checkServerForUpdate();
  }

}

void updateButtonState() {
        static unsigned long last_interrupt_time = 0;
        unsigned long interrupt_time = millis();
        // If interrupts come faster than 200ms, assume it's a bounce and ignore
        if (interrupt_time - last_interrupt_time > 200)
        {
            buttonPressed++;
        }
        last_interrupt_time = interrupt_time;
}

void toggleRelay() {
    Serial.print("Toggling switch..");
    int currentState = digitalRead(swCfg.triggerPin);
    if (currentState) {
        digitalWrite(swCfg.triggerPin, LOW);
        digitalWrite(swCfg.ledPin, HIGH);
    } else {
        digitalWrite(swCfg.triggerPin, HIGH);
        digitalWrite(swCfg.ledPin, LOW);
    }
}

void setLedState(bool currentFlowing) {
    if (currentFlowing)
        digitalWrite(swCfg.ledPin, LOW);
    else
        digitalWrite(swCfg.ledPin, HIGH);
}

void alertViaLed() {
    //blink the led for 2.4 sec
    for(int i=0; i<24; i++) {
        digitalWrite(swCfg.ledPin, LOW);
        delay(50);
        digitalWrite(swCfg.ledPin, HIGH);
        delay(50);
    }
}

void resetModule() {
  Serial.println("Someone requested a complete reset...");
  SPIFFS.begin();
  SPIFFS.remove(CONFIG_FILE_NAME);
  ESP.restart();
}

void rebootModule() {
    Serial.println("Someone requested just a reboot...");
    ESP.restart();
}




