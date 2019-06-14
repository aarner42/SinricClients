#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <CurrentSense.h>
#include <SinricSwitch3Way.h>

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4" 
#define MySSID "Dutenhoefer"        
#define MyWifiPassword "CepiCire99" 

#define DEVICE_ID "5cbbca6037636e7b39dd2a98"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard.
#define LAN_HOSTNAME  "TangoWhiskeyFive"
/************ define pins *********/
#define RELAY   D1   // D1 drives 120v relay
#define CONTACT D3   // D3 connects to momentary switch
#define LED D4       // D4 powers switch illumination

void alertViaLed();
void toggleRelay();
void resetModule();
void setLedState(bool currentFlow);

SinricSwitch3Way *sinricSwitch = nullptr;

void setup() {

    pinMode(LED, OUTPUT);
    pinMode(RELAY, OUTPUT);
    pinMode(CONTACT, INPUT_PULLUP);
    pinMode(A0, INPUT);
  
    digitalWrite(LED, HIGH);   // default switch illumination to on
  
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
        sinricSwitch = new SinricSwitch3Way(MyApiKey, DEVICE_ID, 80, toggleRelay, alertViaLed, resetModule);

  }
}

void loop() {
  sinricSwitch -> loop();

  //check whether the light is on.  (samples for 250 msec so effectively debounces the momentary switch)
  double currentFlow = calcCurrentFlow(false);
  bool newState = (currentFlow > CURRENT_FLOW_NONZERO_THRESHOLD);
  sinricSwitch -> setPowerState(newState);

  int buttonState = digitalRead(CONTACT);
  if (buttonState == LOW) {
      toggleRelay(); 
  }
  setLedState(newState);
  ArduinoOTA.handle();
}

void toggleRelay() {
    Serial.println("Toggling switch...");
    int currentState = digitalRead(RELAY);
    if (currentState)
        digitalWrite(RELAY, LOW);
    else
        digitalWrite(RELAY, HIGH);
}

void setLedState(bool currentFlowing) {
    if (currentFlowing)
        digitalWrite(LED, LOW);
    else
        digitalWrite(LED, HIGH);
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
  




