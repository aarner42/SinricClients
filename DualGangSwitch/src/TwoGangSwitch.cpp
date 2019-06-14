#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <SinricSwitch.h>

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4"
#define MySSID "Dutenhoefer"
#define MyWifiPassword "CepiCire99"

#define DEVICE_ID_1 "5cba5d061061436d83613048"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard.
#define DEVICE_ID_2 "5cba5d181061436d8361304b"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard.
#define LAN_HOSTNAME  "TangoGolf56"
/************ define pins *********/
#define CONTACT_1 D3   // D3 connects to momentary switch 1
#define LED_1     D5   // D5 powers switch illumination 1
#define RELAY_1   D7   // D7 drives Triac 1

#define CONTACT_2 D2   // D2 connects to momentary switch 2
#define LED_2     D6   // D6 powers switch illumination 2
#define RELAY_2   D0   // D0 drives Triac 2

void alertViaLed();
void closeRelayOne();
void openRelayOne();
void toggleRelayOne();
void closeRelayTwo();
void openRelayTwo();
void toggleRelayTwo();

void resetModule();
void initializeOTA();

SinricSwitch *sinricSwitchOne = nullptr;
SinricSwitch *sinricSwitchTwo = nullptr;

void setup() {

    pinMode(LED_1, OUTPUT);
    pinMode(RELAY_1, OUTPUT);
    pinMode(CONTACT_1, INPUT_PULLUP);
    pinMode(LED_2, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(CONTACT_2, INPUT_PULLUP);

    digitalWrite(LED_1, HIGH);    //switch illumination to on
    digitalWrite(LED_2, HIGH);    //switch illumination to on
    digitalWrite(RELAY_1, LOW);   //high-voltage side off
    digitalWrite(RELAY_2, LOW);   //high-voltage side off
    Serial.begin(115200);
  
    WiFi.begin(MySSID, MyWifiPassword);
    Serial.println();
    Serial.print("Connecting to Wifi: ");
    Serial.println(MySSID);  

    // Waiting for Wifi connect
    while(WiFi.status() != WL_CONNECTED) {
        delay(150);
        Serial.print(".");
    }
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("WiFi connected. ");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("OTA Host: ");
        Serial.println(LAN_HOSTNAME);

        initializeOTA();

        sinricSwitchOne = new SinricSwitch(MyApiKey, DEVICE_ID_1, 80, closeRelayOne, openRelayOne, alertViaLed, resetModule);
        sinricSwitchTwo = new SinricSwitch(MyApiKey, DEVICE_ID_2, 88, closeRelayTwo, openRelayTwo, alertViaLed, resetModule);
  }
}


void loop() {
  sinricSwitchOne -> loop();
  sinricSwitchTwo -> loop();

  int buttonStateOne = digitalRead(CONTACT_1);
  if (buttonStateOne == LOW) {
      toggleRelayOne();
      delay(500); //in case someone holds the switch down - don't send the relay into a fit.
      bool powerState = digitalRead(RELAY_1) != LOW;
      sinricSwitchOne -> setPowerState(powerState);
  }

  int buttonStateTwo = digitalRead(CONTACT_2);
  if (buttonStateTwo == LOW) {
      toggleRelayTwo();
      delay(500); //in case someone holds the switch down - don't send the relay into a fit.
      bool powerState = digitalRead(RELAY_2) != LOW;
      sinricSwitchTwo -> setPowerState(powerState);
  }
  ArduinoOTA.handle();
}

void toggleRelayOne() {
    Serial.println("Toggling switch 1...");
    int currentState = digitalRead(RELAY_1);
    if (currentState) {
        digitalWrite(RELAY_1, LOW);
        digitalWrite(LED_1, HIGH);
    }
    else {
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(LED_1, LOW);
    }
}
void toggleRelayTwo() {
    Serial.println("Toggling switch 2...");
    int currentState = digitalRead(RELAY_2);
    if (currentState) {
        digitalWrite(RELAY_2, LOW);
        digitalWrite(LED_2, HIGH);
    }
    else {
        digitalWrite(RELAY_2, HIGH);
        digitalWrite(LED_2, LOW);
    }
}

void openRelayOne() {
    Serial.println("Opening relay 1...");
    digitalWrite(RELAY_1, LOW);
    digitalWrite(LED_1, HIGH);
}
void openRelayTwo() {
    Serial.println("Opening relay 2...");
    digitalWrite(RELAY_2, LOW);
    digitalWrite(LED_2, HIGH);
}

void closeRelayOne() {
    Serial.println("Closing relay 1...");
    digitalWrite(RELAY_1, HIGH);
    digitalWrite(LED_1, LOW);
}
void closeRelayTwo() {
    Serial.println("Closing relay 2...");
    digitalWrite(RELAY_2, HIGH);
    digitalWrite(LED_2, LOW);
}

void alertViaLed() {
    //blink the leds for 2.4 sec
    for(int i=0; i<24; i++) {
        digitalWrite(LED_1, LOW);
        digitalWrite(LED_2, LOW);
        delay(50);
        digitalWrite(LED_1, HIGH);
        digitalWrite(LED_2, HIGH);
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
    ArduinoOTA.onStart([]() {  Serial.println("Start");  });
    ArduinoOTA.onEnd([]()   {  Serial.println("\nEnd");  });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));   });
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





