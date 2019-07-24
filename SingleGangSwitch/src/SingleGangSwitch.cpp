#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <SinricSwitch.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4"
//#define MySSID "Dutenhoefer"
//#define MyWifiPassword "CepiCire99"

#define DEVICE_ID "5c3826353d4dd357f795d360"  // deviceId is the ID assigned to your smart-home-device in sinric.com dashboard.
#define LAN_HOSTNAME  "AlphaBravoTwo"
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
void updateButtonState()  ICACHE_RAM_ATTR;

SinricSwitch *sinricSwitch = nullptr;
AsyncWebServer server(80);
DNSServer dns;

volatile byte buttonPressed = 0;
unsigned long lastDebounceTime = 0;
unsigned const long debounceDelay = 50;

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(RELAY, OUTPUT);
    pinMode(CONTACT, INPUT_PULLUP);
   
    digitalWrite(LED, HIGH);    //switch illumination to on
    digitalWrite(RELAY, LOW);   //high-voltage side off
    Serial.begin(115200);

    AsyncWiFiManager wiFiManager(&server, &dns);
    wiFiManager.autoConnect(LAN_HOSTNAME);

    attachInterrupt(digitalPinToInterrupt(CONTACT), updateButtonState, FALLING);
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("OTA Host: ");
    Serial.println(LAN_HOSTNAME);

    initializeOTA();

    sinricSwitch = new SinricSwitch(MyApiKey, DEVICE_ID, 80, closeRelay, openRelay, alertViaLed, resetModule);
}


void loop() {
  sinricSwitch -> loop();

  if (buttonPressed > 0) {
      Serial.print("manual press: " );
      Serial.println(buttonPressed);
      buttonPressed--;
      toggleRelay();
  }
  ArduinoOTA.handle();
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





