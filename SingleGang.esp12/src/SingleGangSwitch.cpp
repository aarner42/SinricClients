#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <SinricSwitch.h>

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4"
#define MySSID "Dutenhoefer"
#define MyWifiPassword "CepiCire99"

#define DEVICE_ID "5c56239496d7762177d7e959"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard.
#define LAN_HOSTNAME  "AlphaBravoThree"
/************ define pins *********/
#define RELAY   D1   // D1 drives 120v relay
#define CONTACT D3   // D3 connects to momentary switch
#define LED D4       // D4 powers switch illumination

void alertViaLed();
void closeRelay();
void openRelay();
void toggleRelay();
void resetModule();
void initializeOTA();

SinricSwitch *sinricSwitch = nullptr;

void setup() {

    pinMode(LED, OUTPUT);
    pinMode(RELAY, OUTPUT);
    pinMode(CONTACT, INPUT_PULLUP);
   
    digitalWrite(LED, HIGH);    //switch illumination to on
    digitalWrite(RELAY, LOW);   //high-voltage side off
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

        sinricSwitch = new SinricSwitch(MyApiKey, DEVICE_ID, 80, closeRelay, openRelay, alertViaLed, resetModule);
  }
}


void loop() {
  sinricSwitch -> loop();

  int buttonState = digitalRead(CONTACT);
  if (buttonState == LOW) {
      toggleRelay();
      delay(500); //in case someone holds the switch down - don't send the relay into a fit.
      bool powerState = digitalRead(RELAY) != LOW;
      sinricSwitch -> setPowerState(powerState);
  }
  ArduinoOTA.handle();
}

void toggleRelay() {
    Serial.println("Toggling switch...");
    int currentState = digitalRead(RELAY);
    if (currentState) {
        digitalWrite(RELAY, LOW);
        digitalWrite(LED, HIGH);
    }
    else {
        digitalWrite(RELAY, HIGH);
        digitalWrite(LED, LOW);
    }
}

void openRelay() {
    Serial.println("Opening relay...");
    digitalWrite(RELAY, LOW);
    digitalWrite(LED, HIGH);
}

void closeRelay() {
    Serial.println("Closing relay...");
    digitalWrite(RELAY, HIGH);
    digitalWrite(LED, LOW);
}

void alertViaLed() {
    //blink the led for 2.4 sec
    for(int i=0; i<24; i++) {
        digitalWrite(LED, LOW);
        delay(50);
        digitalWrite(LED, HIGH);
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





