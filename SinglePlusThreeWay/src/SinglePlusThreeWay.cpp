#include <SinricSwitch.h>
#include <DeviceConfigurator.h>
#include <SinricSwitch3Way.h>
#include <CurrentSense.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <FirmwareUpdater.h>

#define LAN_HOSTNAME  "ESP_NoConfig"
#define CONFIG_FILE_1NAME "/sinricSwitch1.txt"
#define CONFIG_FILE_2NAME "/sinricSwitch2.txt"
#define SKETCH_VERSION "v20190907-1700"
#define SKETCH_NAME "singlePlusThreeWay"
/************ define pins ********
#define RELAY_3WAY   D1   // D1 drives 120v relay for three-way setup
#define CONTACT_3WAY D3   // D3 connects to three-way NO switch contact (low volt side)
#define LED_3WAY     D4   // D4 powers three-way switch illumination
#define swCfg1Pole.triggerPin   D2   // D2 drives NPN transistor base -> optoCoupler -> triac
#define CONTACT_1POL D5   // D5 connects to single pole NO switch contact (low volt side)
#define LED_1POL     D6   // D6 powers single-pole switch illumination
*/

void alertViaLed();

void toggleThreeWay();
void toggleOnePole();
void onePoleOn();
void onePoleOff();

void setLedState(bool currentFlow, uint8_t ledPin);

void resetModule();
void rebootModule();
void updateButtonStateOne()  ICACHE_RAM_ATTR;
void updateButtonStateTwo()  ICACHE_RAM_ATTR;

void toggleRelay(uint8_t pin);

DeviceConfigurator* dev1 = nullptr;
DeviceConfigurator* dev2 = nullptr;
SinricSwitch3Way *sinricSwitch3Way = nullptr;
SinricSwitch *sinricSwitch1Pol = nullptr;
FirmwareUpdater* fwFetcher = nullptr;
IOTConfig swCfg3Way;
IOTConfig swCfg1Pole;
volatile byte buttonPressedOne = 0;
volatile byte buttonPressedTwo = 0;

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
    
    dev1 = new DeviceConfigurator(CONFIG_FILE_1NAME);
    swCfg3Way = dev1->getConfig();
    dev2 = new DeviceConfigurator(CONFIG_FILE_2NAME);
    swCfg1Pole = dev2->getConfig();
    

    attachInterrupt(digitalPinToInterrupt(swCfg3Way.inputPin),  updateButtonStateOne, FALLING);
    attachInterrupt(digitalPinToInterrupt(swCfg1Pole.inputPin), updateButtonStateTwo, FALLING);

    sinricSwitch3Way = new SinricSwitch3Way(swCfg3Way.apiKey, swCfg3Way.deviceID, 80, toggleThreeWay, alertViaLed, rebootModule, resetModule);
    sinricSwitch1Pol = new SinricSwitch    (swCfg1Pole.apiKey, swCfg1Pole.deviceID, 88, onePoleOn, onePoleOff, alertViaLed, rebootModule, resetModule);
    Serial.printf("3-Way  Pin config is: LED:%d INPUT:%d RELAY:%d ON-level:%d OFF-level:%d\n", swCfg3Way.ledPin, swCfg3Way.inputPin, swCfg3Way.triggerPin, swCfg3Way.onLevel, swCfg3Way.offLevel);
    Serial.printf("1-Pole Pin config is: LED:%d INPUT:%d RELAY:%d ON-level:%d OFF-level:%d\n", swCfg1Pole.ledPin, swCfg1Pole.inputPin, swCfg1Pole.triggerPin, swCfg1Pole.onLevel, swCfg1Pole.offLevel);

    pinMode(A0, INPUT);
    pinMode(swCfg3Way.ledPin, OUTPUT);
    pinMode(swCfg3Way.triggerPin, OUTPUT);
    pinMode(swCfg3Way.inputPin, INPUT_PULLUP);
    digitalWrite(swCfg3Way.ledPin, HIGH);    //switch illumination to on
    digitalWrite(swCfg3Way.triggerPin, swCfg3Way.offLevel);   //high-voltage side off

    pinMode(swCfg1Pole.ledPin, OUTPUT);
    pinMode(swCfg1Pole.triggerPin, OUTPUT);
    pinMode(swCfg1Pole.inputPin, INPUT_PULLUP);
    digitalWrite(swCfg1Pole.ledPin, HIGH);    //switch illumination to on
    digitalWrite(swCfg1Pole.triggerPin, swCfg1Pole.offLevel);   //high-voltage side off
}

void loop() {
    //process the switch loops
    sinricSwitch3Way->loop();
    sinricSwitch1Pol->loop();

    //check whether the lights are drawing current (3-Way) or the relay is closed (1-Pole)
    //(calcCurrentFlow samples for 250 msec synchronously)
    bool powerState3Way = (calcCurrentFlow(false) > CURRENT_FLOW_NONZERO_THRESHOLD);
    sinricSwitch3Way->setPowerState(powerState3Way);
    setLedState(powerState3Way, swCfg3Way.ledPin);

    bool powerState1Pole = digitalRead(swCfg1Pole.triggerPin) == swCfg1Pole.onLevel;
    sinricSwitch1Pol->setPowerState(powerState1Pole);
    setLedState(powerState1Pole, swCfg1Pole.ledPin);

  if (buttonPressedOne > 0) {
      Serial.print("manual button press - switch 1: " );
      Serial.println(buttonPressedOne);
      buttonPressedOne--;
      toggleThreeWay();
  }

  if (buttonPressedTwo > 0) {
      Serial.print("manual button press - switch 2: " );
      Serial.println(buttonPressedTwo);
      buttonPressedTwo--;
      toggleOnePole();
  }

  if (fwFetcher->isUpdateCheckDue()) {
      fwFetcher->checkServerForUpdate();
  }

}

void updateButtonStateOne() {
        static unsigned long last_interrupt_time = 0;
        unsigned long interrupt_time = millis();
        // If interrupts come faster than 200ms, assume it's a bounce and ignore
        if (interrupt_time - last_interrupt_time > 200)
        {
            buttonPressedOne++;
        }
        last_interrupt_time = interrupt_time;
}
void updateButtonStateTwo() {
        static unsigned long last_interrupt_time = 0;
        unsigned long interrupt_time = millis();
        // If interrupts come faster than 200ms, assume it's a bounce and ignore
        if (interrupt_time - last_interrupt_time > 200)
        {
            buttonPressedTwo++;
        }
        last_interrupt_time = interrupt_time;

}

void toggleThreeWay() {
    Serial.println("Toggling relay (3Way)...");
    toggleRelay(swCfg3Way.triggerPin);
}

void toggleOnePole() {
    Serial.println("Toggling relay (1Pole)...");
    toggleRelay(swCfg1Pole.triggerPin);
}

void toggleRelay(uint8_t pin) {
    int currentState = digitalRead(pin);
    if (currentState)
        digitalWrite(pin, LOW);
    else
        digitalWrite(pin, HIGH);
}

void onePoleOn() {
    Serial.println("Closing Single Pole relay...");
    digitalWrite(swCfg1Pole.triggerPin, swCfg1Pole.onLevel);
}
void onePoleOff() {
    Serial.println("Opening Single Pole relay...");
    digitalWrite(swCfg1Pole.triggerPin, swCfg1Pole.offLevel);
}

void setLedState(bool currentFlowing, uint8_t ledPin) {
    if (currentFlowing)
        digitalWrite(ledPin, LOW);
    else
        digitalWrite(ledPin, HIGH);
}

void alertViaLed() {
    //blink the leds for 2.4 sec
    for(int i=0; i<24; i++) {
        digitalWrite(swCfg3Way.ledPin, LOW);
        digitalWrite(swCfg1Pole.ledPin, LOW);
        delay(50);
        digitalWrite(swCfg3Way.ledPin, HIGH);
        digitalWrite(swCfg1Pole.ledPin, HIGH);
        delay(50);
    }
}

void resetModule() {
  Serial.println("Someone requested a complete reset...");
  SPIFFS.begin();
  SPIFFS.remove(CONFIG_FILE_1NAME);
  SPIFFS.remove(CONFIG_FILE_2NAME);
  ESP.restart();
}

void rebootModule() {
    Serial.println("Someone requested a reboot...");
    ESP.restart();
}





