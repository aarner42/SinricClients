#include <SinricSwitch.h>
#include <DeviceConfigurator.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <FirmwareUpdater.h>

#define LAN_HOSTNAME  "ESP_NoConfig"
#define CONFIG_FILE_1NAME "/sinricSwitch1.txt"
#define CONFIG_FILE_2NAME "/sinricSwitch2.txt"
#define SKETCH_VERSION "v20190907-1700"
#define SKETCH_NAME "dualGangSwitch"

void alertViaLed();
void closeRelayOne();
void openRelayOne();
void toggleRelayOne();
void closeRelayTwo();
void openRelayTwo();
void toggleRelayTwo();

void resetModule();
void rebootModule();
void updateButtonStateOne()  ICACHE_RAM_ATTR;
void updateButtonStateTwo()  ICACHE_RAM_ATTR;

DeviceConfigurator* dev1 = nullptr;
DeviceConfigurator* dev2 = nullptr;
SinricSwitch *sinricSwitchOne = nullptr;
SinricSwitch *sinricSwitchTwo = nullptr;
FirmwareUpdater* fwFetcher = nullptr;
IOTConfig swCfg1;
IOTConfig swCfg2;
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
    swCfg1 = dev1->getConfig();
    dev2 = new DeviceConfigurator(CONFIG_FILE_2NAME);
    swCfg2 = dev2->getConfig();
    

    attachInterrupt(digitalPinToInterrupt(swCfg1.inputPin), updateButtonStateOne, FALLING);
    attachInterrupt(digitalPinToInterrupt(swCfg2.inputPin), updateButtonStateTwo, FALLING);

    sinricSwitchOne = new SinricSwitch(swCfg1.apiKey, swCfg1.deviceID, 80, closeRelayOne, openRelayOne, alertViaLed, rebootModule, resetModule);
    sinricSwitchTwo = new SinricSwitch(swCfg2.apiKey, swCfg2.deviceID, 88, closeRelayTwo, openRelayTwo, alertViaLed, rebootModule, resetModule);
    Serial.printf("SW1 Pin config is: LED:%d INPUT:%d RELAY:%d ON-level:%d OFF-level:%d\n", swCfg1.ledPin, swCfg1.inputPin, swCfg1.triggerPin, swCfg1.onLevel, swCfg1.offLevel);
    Serial.printf("SW2 Pin config is: LED:%d INPUT:%d RELAY:%d ON-level:%d OFF-level:%d\n", swCfg2.ledPin, swCfg2.inputPin, swCfg2.triggerPin, swCfg2.onLevel, swCfg2.offLevel);
    
    pinMode(swCfg1.ledPin, OUTPUT);
    pinMode(swCfg1.triggerPin, OUTPUT);
    pinMode(swCfg1.inputPin, INPUT_PULLUP);
    digitalWrite(swCfg1.ledPin, HIGH);    //switch illumination to on
    digitalWrite(swCfg1.triggerPin, swCfg1.offLevel);   //high-voltage side off

    pinMode(swCfg2.ledPin, OUTPUT);
    pinMode(swCfg2.triggerPin, OUTPUT);
    pinMode(swCfg2.inputPin, INPUT_PULLUP);
    digitalWrite(swCfg2.ledPin, HIGH);    //switch illumination to on
    digitalWrite(swCfg2.triggerPin, swCfg2.offLevel);   //high-voltage side off

}


void loop() {
  sinricSwitchOne -> loop();
  sinricSwitchTwo -> loop();

  if (buttonPressedOne > 0) {
      Serial.print("manual button press - switch 1: " );
      Serial.println(buttonPressedOne);
      buttonPressedOne--;
      toggleRelayOne();
  }
  if (buttonPressedTwo > 0) {
      Serial.print("manual button press - switch 2: " );
      Serial.println(buttonPressedTwo);
      buttonPressedTwo--;
      toggleRelayTwo();
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

void toggleRelayOne() {
    Serial.println("Toggling switch 1...");
    int currentState = digitalRead(swCfg1.triggerPin);
    digitalWrite(swCfg1.triggerPin, !currentState);
    digitalWrite(swCfg1.ledPin, currentState);
}
void toggleRelayTwo() {
    Serial.println("Toggling switch 2...");
    int currentState = digitalRead(swCfg2.triggerPin);
    digitalWrite(swCfg2.triggerPin, !currentState);
    digitalWrite(swCfg2.ledPin, currentState);
}

void openRelayOne() {
    Serial.println("Opening relay 1...");
    digitalWrite(swCfg1.triggerPin, swCfg1.offLevel);
    digitalWrite(swCfg1.ledPin, HIGH);
}
void openRelayTwo() {
    Serial.println("Opening relay 2...");
    digitalWrite(swCfg2.triggerPin, swCfg2.offLevel);
    digitalWrite(swCfg2.ledPin, HIGH);
}

void closeRelayOne() {
    Serial.println("Closing relay 1...");
    digitalWrite(swCfg1.triggerPin, swCfg1.onLevel);
    digitalWrite(swCfg1.ledPin, LOW);
}
void closeRelayTwo() {
    Serial.println("Closing relay 2...");
    digitalWrite(swCfg2.triggerPin, swCfg2.onLevel);
    digitalWrite(swCfg2.ledPin, LOW);
}

void alertViaLed() {
    //blink the leds for 2.4 sec
    for(int i=0; i<24; i++) {
        digitalWrite(swCfg1.ledPin, LOW);
        digitalWrite(swCfg2.ledPin, LOW);
        delay(50);
        digitalWrite(swCfg1.ledPin, HIGH);
        digitalWrite(swCfg2.ledPin, HIGH);
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





