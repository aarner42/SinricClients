#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <SinricSwitch.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <DNSServer.h>

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4"
//#define MySSID "Dutenhoefer"
//#define MyWifiPassword "CepiCire99"

#define DEVICE_ID_1 "5cba5d061061436d83613048"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard.
#define DEVICE_ID_2 "5cba5d181061436d8361304b"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard.
#define LAN_HOSTNAME  "TangoGolf56"
/************ define pins *********/
#define CONTACT_1 D3   // D3 connects to momentary switch 1
#define LED_1     D7   // D7 powers switch illumination 1
#define RELAY_1   D0   // D0 drives Triac 1

#define CONTACT_2 D4   // D4 connects to momentary switch 2
#define LED_2     D8   // D8 powers switch illumination 2
#define RELAY_2   D1   // D1 drives Triac 2

void alertViaLed();
void closeRelayOne();
void openRelayOne();
void toggleRelayOne();
void closeRelayTwo();
void openRelayTwo();
void toggleRelayTwo();

void resetModule();
void initializeOTA();
void updateButtonStateOne()  ICACHE_RAM_ATTR;
void updateButtonStateTwo()  ICACHE_RAM_ATTR;

SinricSwitch *sinricSwitchOne = nullptr;
SinricSwitch *sinricSwitchTwo = nullptr;

AsyncWebServer server(80);
DNSServer dns;

volatile byte buttonPressedOne = 0;
volatile byte buttonPressedTwo = 0;
unsigned long lastDebounceTimeOne = 0;
unsigned long lastDebounceTimeTwo = 0;
unsigned const long debounceDelay = 50;

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
  
    AsyncWiFiManager wiFiManager(&server, &dns);
    wiFiManager.autoConnect(LAN_HOSTNAME);

    attachInterrupt(digitalPinToInterrupt(CONTACT_1), updateButtonStateOne, FALLING);
    attachInterrupt(digitalPinToInterrupt(CONTACT_2), updateButtonStateTwo, FALLING);

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


void loop() {
  sinricSwitchOne -> loop();
  sinricSwitchTwo -> loop();

  if (buttonPressedOne > 0) {
      Serial.print("manual press - switch 1: " );
      Serial.println(buttonPressedOne);
      buttonPressedOne--;
      toggleRelayOne();
  }
  if (buttonPressedTwo > 0) {
      Serial.print("manual press - switch 2: " );
      Serial.println(buttonPressedTwo);
      buttonPressedTwo--;
      toggleRelayTwo();
  }

  ArduinoOTA.handle();
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





