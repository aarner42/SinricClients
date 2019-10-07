#ifndef __SINRICPRO_CONTROLLER_H__
#define __SINRICPRO_CONTROLLER_H__
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include "JSON/JsonFactory.h"
#include "WebSocketListener.h"
#include "Globals.h"
#include "MessageID.h"
#include "SinricProDebug.h"
#include "SinricProConfig.h"
#include "Devices/IDevice.h"

class SinricProController {
private:
    JsonFactory jsonFactory;
    String server;
    String socketAuthorization;
//    String app_secret;

    void handleRequest();
    bool handleCallbacks(const SinricEvent&); //String deviceId, String action, JsonObject& request_value, JsonObject& value);
    void handleSendQueue();
    void connect();
    void disconnect();
    void reconnect();
    bool canSendEvent(String deviceId);

    WebSocketListener socketListener;

    typedef std::vector<IDevice*> SinricProDeviceList;
    SinricProDeviceList devices;

public:
    ~SinricProController();
    SinricProController(const String& socketAuth, String app_secret, String serverURL);

    void add(IDevice* device, unsigned long eventsEveryMS = 10000);
    void handle();
    void stop();
    bool isConnected();

    int size();

};

#endif