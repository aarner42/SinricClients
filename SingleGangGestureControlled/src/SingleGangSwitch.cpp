
#include <SinricSwitch.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <WiFiManager.h>
#include <FS.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <DNSServer.h>

#define LAN_HOSTNAME  "AlphaBravoEight"
//************ define pins *********

#define RELAY       D8   // D1 drives 120v relay
#define GESTURE_PIN D3   // D3 connects to momentary switch
#define LED         D4       // D4 powers switch illumination

void alertViaLed();
void closeRelay();
void openRelay();
void resetModule();
void rebootModule();

void gestureInterrupt()  ICACHE_RAM_ATTR;
void handleGesture();

void initWebPortalForConfigCapture();
void validateConfig(const String &name, const String &value, uint8 expected);
String readConfigValueFromFile(const String &name);

SinricSwitch *sinricSwitch = nullptr;
AsyncWebServer server(80);
DNSServer dns;
SparkFun_APDS9960 apds = SparkFun_APDS9960();

volatile int gestureAvailable = 0;

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(RELAY, OUTPUT);
    pinMode(GESTURE_PIN, INPUT);
   
    digitalWrite(LED, HIGH);    //switch illumination to on
    digitalWrite(RELAY, LOW);   //high-voltage side off
    Serial.begin(115200);

    WiFiManager wiFiManager;
    wiFiManager.autoConnect(LAN_HOSTNAME);

    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("OTA Host: ");
    Serial.println(LAN_HOSTNAME);

    Serial.println("Starting FS");
    SPIFFS.begin();

    Serial.println("Checking for config file /sinric-config.txt");
    if (!SPIFFS.exists("/sinric-config.txt")) {
        initWebPortalForConfigCapture();
    } else {
        String apiKey = readConfigValueFromFile("apiKey");
        String deviceID = readConfigValueFromFile("deviceID");
        validateConfig("apiKey", apiKey, 37);
        validateConfig("deviceID", deviceID, 25);
        attachInterrupt(digitalPinToInterrupt(GESTURE_PIN), gestureInterrupt, FALLING);

        if ( apds.init() ) {
            Serial.println(F("APDS-9960 initialization complete"));
        } else {
            Serial.println(F("Something went wrong during APDS-9960 init!"));
        }
        // Start running the APDS-9960 gesture sensor engine
        if ( apds.enableGestureSensor(true) ) {
            Serial.println(F("Gesture sensor is now running"));
        } else {
            Serial.println(F("Something went wrong during gesture sensor init!"));
        }

        sinricSwitch = new SinricSwitch(apiKey.c_str(), deviceID.c_str(), 80, closeRelay, openRelay, alertViaLed, rebootModule, resetModule);
    }
}

void validateConfig(const String &name, const String &value, const uint8 expected) {
    Serial.print(name);
    Serial.print("=");
    Serial.print(value);
    if (value.length() != expected) {
        Serial.print(" - is");
        Serial.print(value.length());
        Serial.print(" bytes. Should be");
        Serial.print(expected);
        Serial.println(" bytes.");
        Serial.println("Erasing bad config file ...");
        SPIFFS.remove("/sinric-config.txt");
        Serial.println("...And reset.");
        ESP.reset();
    } else {
        Serial.println(" - is OK");
    }


}

String readConfigValueFromFile(const String &name) {
    char lineBuffer[128];
    File configFile = SPIFFS.open("/sinric-config.txt", "r");
    String returnVal = "";
    //read the apiKey
    while (configFile.available()) {
        int l = configFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer));
        lineBuffer[l] = 0;
        String line = lineBuffer;
        if (line.indexOf(name) > -1) {
            returnVal = line.substring(line.lastIndexOf("=")+1);
            break;
        }
    }
    configFile.close();
    return returnVal;

}


void initWebPortalForConfigCapture() {
    Serial.println("config file not present - starting web server for configuration purposes.");
    server.reset();
    Serial.println("Server reset complete...");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "You'll want to submit /config?apiKey=[yourKey]&deviceID=[yourDeviceId]");
    });
    Serial.println("Setup / handler");

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("apiKey") && request->hasParam("deviceID")) {
            String apiKey = request->getParam("apiKey")->value();
            String deviceID = request->getParam("deviceID")->value();

            File configFile = SPIFFS.open("/sinric-config.txt", "w");
            if (!configFile) {
                Serial.println("File won't open - Can't complete...");
                request->send(500, "text/plain", "The flash FS is fucked...Try again.");
            } else {
                int written = configFile.print("apiKey=");
                written += configFile.println(apiKey);
                written += configFile.print("deviceID=");
                written += configFile.println(deviceID);
                configFile.flush();
                configFile.close();
                if (written < 50) {
                    Serial.println("Didn't write enough data.  Can't complete");
                } else {
                    Serial.println("Wrote config values to flash");
                    Serial.print("apiKey=");
                    Serial.println(apiKey);
                    Serial.print("deviceID=");
                    Serial.println(deviceID);
                    Serial.println("Resetting...");
                    ESP.reset();
                }
            }
        } else {
            request->send(400, "text/plain", "You fucked that up.  Try again, but this time with the required params.");
        }
    });
    Serial.println("Setup /config handler");

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "That'll be a 404.  Try again.");
    });
    Serial.println("Setup 404 handler");

    server.begin();
    Serial.println("Server initialized");
    while (true) {
        yield();
    }
}


void loop() {
  sinricSwitch -> loop();

  if (gestureAvailable > 0) {
      Serial.print("gesture sensor requests interrupt: " );
      gestureAvailable=0;
      handleGesture();
  }

}

void gestureInterrupt()  {
    Serial.println("Interrupt handler hits");
    gestureAvailable++;
}

void handleGesture() {
    if ( apds.isGestureAvailable() ) {
        switch ( apds.readGesture() ) {
            case DIR_UP:
                Serial.println("UP");
                closeRelay();
                sinricSwitch -> setPowerState(digitalRead(RELAY));
                break;
            case DIR_DOWN:
                Serial.println("DOWN");
                openRelay();
                sinricSwitch -> setPowerState(digitalRead(RELAY));
                break;
            case DIR_LEFT:
                Serial.println("LEFT");
                break;
            case DIR_RIGHT:
                Serial.println("RIGHT");
                break;
            case DIR_NEAR:
                Serial.println("NEAR");
                break;
            case DIR_FAR:
                Serial.println("FAR");
                break;
            default:
                Serial.println("NONE");
        }
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
  Serial.println("Someone requested a full reset...");
  SPIFFS.remove("/sinric-config.txt");
  ESP.restart();
}

void rebootModule() {
    Serial.println("Someone requested a reboot...");
    ESP.restart();

}
