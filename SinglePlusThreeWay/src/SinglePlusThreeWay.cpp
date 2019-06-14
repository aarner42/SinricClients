#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <CurrentSense.h>
#include <SinricSwitch3Way.h>
#include <SinricSwitch.h>

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4"
#define MySSID "Dutenhoefer"
#define MyWifiPassword "CepiCire99"

#define DEVICE_ID_3WAY "5c3944433a54d866e07086c7"    // TangoWhiskeyTwo - deviceId from sinric.com dashboard.
#define DEVICE_ID_1POL "5c5906ed7c472c33aaff46a1"   // AlphaBravoFour

#define LAN_HOSTNAME  "TW2-AB4"

/************ define pins *********/
#define RELAY_3WAY   D1   // D1 drives 120v relay for three-way setup
#define CONTACT_3WAY D3   // D3 connects to three-way NO switch contact (low volt side)
#define LED_3WAY     D4   // D4 powers three-way switch illumination
#define RELAY_1POL   D2   // D2 drives NPN transistor base -> optoCoupler -> triac
#define CONTACT_1POL D5   // D5 connects to single pole NO switch contact (low volt side)
#define LED_1POL     D6   // D6 powers single-pole switch illumination

void alertViaLed();

void toggleThreeWay();
void toggleOnePole();
void onePoleOn();
void onePoleOff();

void setLedState(bool currentFlow, uint8_t ledPin);

void resetModule();
void initializeOTA();
void toggleRelay(uint8_t pin);

SinricSwitch3Way *sinricSwitch3Way = nullptr;
SinricSwitch *sinricSwitch1Pol = nullptr;

void setup() {

    pinMode(LED_1POL, OUTPUT);
    pinMode(LED_3WAY, OUTPUT);
    pinMode(RELAY_1POL, OUTPUT);
    pinMode(RELAY_3WAY, OUTPUT);
    pinMode(CONTACT_1POL, INPUT_PULLUP);
    pinMode(CONTACT_3WAY, INPUT_PULLUP);
    pinMode(A0, INPUT);

    digitalWrite(LED_1POL, HIGH);   // default switch illumination to on
    digitalWrite(LED_3WAY, HIGH);   // default switch illumination to on

    Serial.begin(115200);

    WiFi.begin(MySSID, MyWifiPassword);
    Serial.println();
    Serial.print("Connecting to Wifi: ");
    Serial.println(MySSID);

    // Waiting for Wifi connect
    while (WiFi.status() != WL_CONNECTED) {
        delay(150);
        Serial.print(".");
    }

    //process setup requiring WiFi
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("WiFi connected. ");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("OTA Host: ");
        Serial.println(LAN_HOSTNAME);

        initializeOTA();

        sinricSwitch3Way = new SinricSwitch3Way(MyApiKey, DEVICE_ID_3WAY, 80, toggleThreeWay, alertViaLed, resetModule);
        sinricSwitch1Pol = new SinricSwitch    (MyApiKey, DEVICE_ID_1POL, 88, onePoleOn, onePoleOff, alertViaLed, resetModule);
    }
}

void loop() {
    //process the switch loops
    sinricSwitch3Way->loop();
    sinricSwitch1Pol->loop();

    //check whether the lights are drawing current (3-Way) or the relay is closed (1-Pole)
    //(calcCurrentFlow samples for 250 msec synchronously)
    bool powerState3Way = (calcCurrentFlow(false) > CURRENT_FLOW_NONZERO_THRESHOLD);
    bool powerState1Pole = digitalRead(RELAY_1POL) != LOW;

    //publish the current powerState
    sinricSwitch3Way->setPowerState(powerState3Way);
    sinricSwitch1Pol->setPowerState(powerState1Pole);

    //set the LED state as needed
    setLedState(powerState1Pole, LED_1POL);
    setLedState(powerState3Way, LED_3WAY);

    //check for and handle manual presses of the switches
    if (digitalRead(CONTACT_3WAY) == LOW) {
        toggleThreeWay();
    }
    if (digitalRead(CONTACT_1POL) == LOW) {
        toggleOnePole();
    }

    ArduinoOTA.handle();
}

void toggleThreeWay() {
    Serial.println("Toggling relay (3Way)...");
    toggleRelay(RELAY_3WAY);
}

void toggleOnePole() {
    Serial.println("Toggling relay (1Pole)...");
    toggleRelay(RELAY_1POL);
}

void toggleRelay(const uint8_t pin) {
    int currentState = digitalRead(pin);
    if (currentState)
        digitalWrite(pin, LOW);
    else
        digitalWrite(pin, HIGH);
}

void onePoleOn() {
    Serial.println("Closing Single Pole relay...");
    digitalWrite(RELAY_1POL, HIGH);
}
void onePoleOff() {
    Serial.println("Opening Single Pole relay...");
    digitalWrite(RELAY_1POL, LOW);
}

void setLedState(bool currentFlowing, uint8_t ledPin) {
    if (currentFlowing)
        digitalWrite(ledPin, LOW);
    else
        digitalWrite(ledPin, HIGH);
}

void alertViaLed() {
    //blink the led for 2.4 sec
    for (int i = 0; i < 24; i++) {
        digitalWrite(LED_3WAY, LOW);
        digitalWrite(LED_1POL, LOW);
        delay(50);
        digitalWrite(LED_3WAY, HIGH);
        digitalWrite(LED_1POL, HIGH);
        delay(50);
    }
}

void resetModule() {
    Serial.println("Someone requested a reset...");
    ESP.restart();
}

void initializeOTA() {
    Serial.println("Setting up ArduinoOTA handlers...");
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(LAN_HOSTNAME);
    ArduinoOTA.onStart([]() { Serial.println("Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}





