#include <DeviceConfigurator.h>
#include <FirmwareUpdater.h>
#include <Devices/BinarySwitch.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <SinricProController.h>
#include <LocalWebController.h>
#include "DeviceConfigurator.h"


/************ define pins - Now in Config File *********/
//#define RELAY   D1   // D1 drives 120v relay
//#define CONTACT D3   // D3 connects to momentary switch
//#define LED D4       // D4 powers switch illumination
#define LAN_HOSTNAME  "ESP_NoConfig"
#define CONFIG_FILE_NAME "/sinricSwitch1.txt"

void closeRelay();
void openRelay();
void toggleRelay();
void updateButtonState()  ICACHE_RAM_ATTR;
bool stateCallback(String, bool);

void connectToWifi();

#define SKETCH_VERSION "v20191006-1605"
#define SKETCH_NAME "singleGangSwitch"

volatile byte buttonPressed = 0;
DeviceConfigurator* dev1 = nullptr;
FirmwareUpdater* fwFetcher = nullptr;
IOTConfig* swCfg = nullptr;
SinricProController* controller = nullptr;
LocalWebController* webController = nullptr;
BinarySwitch* binarySwitch = nullptr;

void setup() {
    connectToWifi();

    fwFetcher = new FirmwareUpdater(SKETCH_NAME, SKETCH_VERSION, 3600000);

    dev1 = new DeviceConfigurator(CONFIG_FILE_NAME);
    swCfg = dev1->getConfig();

    webController = new LocalWebController(swCfg->deviceID, 80, openRelay, closeRelay, stateCallback);

    controller = new SinricProController(swCfg->socketAuth, swCfg->signingKey, "ws.sinric.pro");
    binarySwitch = controller->buildBinarySwitch(swCfg->deviceID, stateCallback);

    controller->add(binarySwitch, 1000);
    controller->connect();

    Serial.printf("Pin config is: LED:%d INPUT:%d RELAY:%d ON-level:%d OFF-level:%d\n", swCfg->ledPin, swCfg->inputPin, swCfg->triggerPin, swCfg->onLevel, swCfg->offLevel);
    pinMode(swCfg->ledPin, OUTPUT);
    pinMode(swCfg->triggerPin, OUTPUT);
    pinMode(swCfg->inputPin, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(swCfg->inputPin), updateButtonState, FALLING);
    digitalWrite(swCfg->ledPin, HIGH);    //switch illumination to on
    digitalWrite(swCfg->triggerPin, swCfg->offLevel);   //high-voltage side off


}

void connectToWifi() {
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
}


void loop() {
    controller->handle();

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
    int currentState = digitalRead(swCfg->triggerPin);
    if (currentState) {
        digitalWrite(swCfg->triggerPin, LOW);
        digitalWrite(swCfg->ledPin, HIGH);
    } else {
        digitalWrite(swCfg->triggerPin, HIGH);
        digitalWrite(swCfg->ledPin, LOW);
    }
    binarySwitch->setState(digitalRead(swCfg->triggerPin) == swCfg->onLevel);
}

bool stateCallback(String deviceId, bool state) {
    if (state) {
        closeRelay();
        return digitalRead(swCfg->triggerPin) == swCfg->onLevel;
    } else {
        openRelay();
        return digitalRead(swCfg->triggerPin) == swCfg->offLevel;
    }
}

void openRelay() {
    Serial.println("Opening relay...");
    digitalWrite(swCfg->triggerPin, swCfg->offLevel);
    digitalWrite(swCfg->ledPin, HIGH);
}

void closeRelay() {
    Serial.println("Closing relay...");
    digitalWrite(swCfg->triggerPin, swCfg->onLevel);
    digitalWrite(swCfg->ledPin, LOW);
}







