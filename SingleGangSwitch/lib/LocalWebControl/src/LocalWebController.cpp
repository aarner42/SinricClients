//
// Created by aarne on 10/3/2019.
//

#include "LocalWebController.h"


LocalWebController::LocalWebController(String deviceId, int port, const WebCallback &resetCallback, const WebCallback &rebootCallback, const PowerStateCallback& powerCallback) {
    deviceID = std::move(deviceId);
    this->port = port;
    this->resetCallback = resetCallback;
    this->rebootCallback = rebootCallback;
    this->powerCallback = powerCallback;
}

void LocalWebController::startServer() {
    server = new ESP8266WebServer(port);

    server->on("/", [&]() {
        handleRoot();
    });

    server->on("/on", [&]() {
       powerCallback(deviceID, true);
    });
    server->on("/off", [&]() {
       powerCallback(deviceID, false);
    });
    server->on("/reboot", [&]() {
        rebootCallback();
    });
    server->on("/resetAll", [&]() {
        resetCallback();
    });

    server->begin();
    Serial.printf("WebServer started on port: %d\n", port);
}

void LocalWebController::loop() {
    if (server != nullptr) {
        server->handleClient();
        delay(1);
    }
}

void LocalWebController::handleRoot() {
    uint64_t now = millis();
    char buff[21];
    sprintf(buff, "%" PRIu64, now / 1000);
    server->send(200, "text/plain",
                 "Uptime: " + String(buff) + " seconds.  Call /reset if you want to reset me (deviceID=" + deviceID +
                 ")...");
}

void LocalWebController::handleReset() {
    server->send(200, "text/plain", "OMG WERE GONNA DIE!!!!");
    delay(250);
    resetCallback();
}


