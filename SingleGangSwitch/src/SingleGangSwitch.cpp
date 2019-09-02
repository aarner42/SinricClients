#include <SinricSwitch.h>
#include <DeviceConfigurator.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>


#define LAN_HOSTNAME  "ESP_NoConfig"
/************ define pins *********/
#define RELAY   D1   // D1 drives 120v relay
#define CONTACT D3   // D3 connects to momentary switch
#define LED D4       // D4 powers switch illumination

void alertViaLed();
void closeRelay();
void openRelay();
void toggleRelay();
void resetModule();
void rebootModule();
void updateButtonState()  ICACHE_RAM_ATTR;

SinricSwitch *sinricSwitch = nullptr;

volatile byte buttonPressed = 0;

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(RELAY, OUTPUT);
    pinMode(CONTACT, INPUT_PULLUP);
   
    digitalWrite(LED, HIGH);    //switch illumination to on
    digitalWrite(RELAY, LOW);   //high-voltage side off
    Serial.begin(115200);

    WiFiManager wiFiManager;
    wiFiManager.autoConnect(LAN_HOSTNAME);

    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("HotspotName: ");
    Serial.println(LAN_HOSTNAME);



    Serial.println("Starting FS");
    SPIFFS.begin();

    Serial.println("Checking for config file /sinric-config.txt");
    if (!SPIFFS.exists("/sinric-config.txt")) {
        initWebPortalForConfigCapture();
    } else {
        String deviceID = readConfigValueFromFile(DEVICE_ID_PARAM);
        String apiKey = readConfigValueFromFile(SINRIC_KEY_PARAM);
        validateConfig(SINRIC_KEY_PARAM, apiKey, 37);
        validateConfig(DEVICE_ID_PARAM, deviceID, 25);
        attachInterrupt(digitalPinToInterrupt(CONTACT), updateButtonState, FALLING);
        sinricSwitch = new SinricSwitch(apiKey, deviceID, 80, closeRelay, openRelay, alertViaLed, rebootModule, resetModule);
    }
}



void loop() {
  sinricSwitch -> loop();

  if (buttonPressed > 0) {
      Serial.print("manual press: " );
      Serial.println(buttonPressed);
      buttonPressed--;
      toggleRelay();
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
    int currentState = digitalRead(RELAY);
    if (currentState) {
        digitalWrite(RELAY, LOW);
        digitalWrite(LED, HIGH);
    }
    else {
        digitalWrite(RELAY, HIGH);
        digitalWrite(LED, LOW);
    }
    sinricSwitch->setPowerState(!currentState);
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





