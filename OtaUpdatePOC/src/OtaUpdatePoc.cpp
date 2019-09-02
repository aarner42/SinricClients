//
// Created by aarne on 8/30/2019.
//
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include <FirmwareUpdater.h>

#define LAN_HOSTNAME            "ESP_NoConfig"
#define SKETCH_FW_NAME          "OtaUpdatePOC"
#define SKETCH_FW_VERSION       "v20190901-1700"

FirmwareUpdater *firmwareUpdater = nullptr;

void setup() {
    Serial.begin(115200);

    WiFiManager wiFiManager;
    wiFiManager.autoConnect(LAN_HOSTNAME);

    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hot-Spot / AP Name: ");
    Serial.println(LAN_HOSTNAME);

    firmwareUpdater = new FirmwareUpdater(SKETCH_FW_NAME, SKETCH_FW_VERSION, 30000);

    Serial.println("Allocated OTA updater");
}

void loop() {
    if (firmwareUpdater->isUpdateCheckDue()) {
        firmwareUpdater->checkServerForUpdate();
    }
}







