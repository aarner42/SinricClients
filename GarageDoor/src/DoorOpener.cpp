#include <SinricPro.h>
#include <FirmwareUpdater.h>
#include <DoorConfigurator.h>
#include <SinricProDebug.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>


#define LAN_HOSTNAME  "ESP_NoConfig"
#define CONFIG_FILE_NAME "/sinricDoor1.txt"

void alertViaLed();
void toggleRelay();
void resetModule();
void rebootModule();
void doorIsOpened()  ICACHE_RAM_ATTR;
void doorIsClosed()  ICACHE_RAM_ATTR;
bool onContactState(const char* deviceId, ContactState_t& state);
bool onPowerState(const char* deviceID, PowerState_t& state);


#define SKETCH_VERSION "v20190915-0800"
#define SKETCH_NAME "garageDoorOpener"

DoorConfigurator* dev1 = nullptr;
FirmwareUpdater* fwFetcher = nullptr;
IOTConfig_door* doorCfg = nullptr;

volatile byte doorClosed = 0;
volatile byte doorOpened = 0;

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
    
    dev1 = new DoorConfigurator(CONFIG_FILE_NAME);
    doorCfg = dev1->getConfig();

//    attachInterrupt(digitalPinToInterrupt(doorCfg->closedPin), doorIsClosed, FALLING);
//    attachInterrupt(digitalPinToInterrupt(doorCfg->openedPin), doorIsOpened, FALLING);
//    attachInterrupt(digitalPinToInterrupt(doorCfg->closedPin), doorIsOpened, RISING);
//    attachInterrupt(digitalPinToInterrupt(doorCfg->openedPin), doorIsClosed, RISING);
//    sinricSwitch = new SinricSwitch(swCfg->apiKey, swCfg->deviceID, 80, toggleRelay, toggleRelay, alertViaLed, rebootModule, resetModule);
    Serial.println("interrupt handlers registered");

    SinricPro[doorCfg->deviceID].pollContactSensor(onContactState);
    SinricPro[doorCfg->deviceID].onPowerState(onPowerState);
    SinricPro.begin(doorCfg->apiKey);               // start SinricPro
    Serial.println("SinricPro devices initialized");

    Serial.printf("Pin config is: LED:%d openSense:%d closedSense:%d relayPin:%d\n", doorCfg->ledPin, doorCfg->openedPin, doorCfg->closedPin, doorCfg->relayPin);
    pinMode(doorCfg->ledPin, OUTPUT);
    pinMode(doorCfg->relayPin, OUTPUT);
    pinMode(doorCfg->closedPin, INPUT_PULLUP);
    pinMode(doorCfg->openedPin, INPUT_PULLUP);
    Serial.println("Setup complete");
}



void loop() {

    doorOpened = digitalRead(doorCfg->openedPin) == LOW;
    doorClosed = digitalRead(doorCfg->closedPin) == LOW;

  ContactState_t doorState = SinricPro[doorCfg->deviceID].getContactSensor();
  if (doorState == contact_Closed) {
      if (doorOpened) {
          Serial.println("Door states are out of sync on server - updating to CLOSED (lock_Locked)");
          SinricPro[doorCfg->deviceID].setContactSensor(contact_Closed, true);
      }
  }
  if (doorState == contact_Open) {
      if (doorClosed) {
          Serial.println("Door states are out of sync on server - updating to OPEN (lock_Unlocked)");
          SinricPro[doorCfg->deviceID].setContactSensor(contact_Open, true);
      }
  }

  if ((millis() % 5000) == 0) {
      Serial.printf("Door / Contact Sensor is: opened->%d closed:%d\n", doorOpened, doorClosed);
      delay(1);
  }
  if (fwFetcher->isUpdateCheckDue()) {
      fwFetcher->checkServerForUpdate();
  }

}
void doorIsClosed() {
    doorClosed = true;
    doorOpened = false;
}
void doorIsOpened() {
    doorOpened = true;
    doorClosed = false;
}

void updateDoorState(boolean isClosed) {
        static unsigned long last_interrupt_time = 0;
        unsigned long interrupt_time = millis();
        // If interrupts come faster than 200ms, assume it's a bounce and ignore
        if (interrupt_time - last_interrupt_time > 200)
        {
            doorClosed = isClosed;
            doorOpened = !isClosed;
        }
}

void toggleRelay() {
    Serial.print("Toggling switch..");
    digitalWrite(doorCfg->relayPin, HIGH);
    delay(125);
    digitalWrite(doorCfg->relayPin, LOW);
}

void alertViaLed() {
    //blink the led for 2.4 sec
    for(int i=0; i < 24; i++) {
        digitalWrite(doorCfg->ledPin, LOW);
        delay(50);
        digitalWrite(doorCfg->ledPin, HIGH);
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

// Callback-Routine
bool onContactState(const char* deviceId, ContactState_t& state) {
    Serial.printf("Device %s --> ", deviceId);
    switch (state) {
        case contact_Open   : Serial.printf("door opened\r\n"); updateDoorState(false); break;
        case contact_Closed : Serial.printf("door closed\r\n"); updateDoorState(true); break;
    }
    return true;
}

bool onPowerState(const char* deviceID, PowerState_t& state) {
    Serial.printf("Device %s --> ", deviceID);
    switch (state) {
        case power_OFF  : Serial.printf("door opened request\r\n"); toggleRelay(); break;
        case power_ON   : Serial.printf("door closed request\r\n"); toggleRelay(); break;
    }
    return true;

}





