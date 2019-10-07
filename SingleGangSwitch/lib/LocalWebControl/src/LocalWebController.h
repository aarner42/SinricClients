//
// Created by aarne on 10/3/2019.
//

#ifndef LOCALWEBCONTROLLER_H
#define LOCALWEBCONTROLLER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "Devices/BinarySwitch.h"

typedef std::function<void()> WebCallback;

class LocalWebController {
private:

    ESP8266WebServer *server = nullptr;
    WebCallback resetCallback;
    WebCallback rebootCallback;
    PowerStateCallback powerCallback;
    String deviceID;
    int port;
    void handleRoot();
    void handleReset();
    void loop();
    void startServer();

public:
    LocalWebController(String, int, const WebCallback&, const WebCallback&, const PowerStateCallback&);

};


#endif //LOCALWEBCONTROLLER_H
