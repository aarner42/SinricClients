//
// Created by aarne on 10/5/2019.
//

#ifndef _SINRICEVENT_H
#define _SINRICEVENT_H

#include <ArduinoJson.h>
#include <Arduino.h>

class SinricEvent {
private:
    JsonDocument& requestMessage;
    JsonDocument& responseMessage;
    String deviceID;
    bool valid;
public:
    SinricEvent(JsonDocument& request, JsonDocument& response, String& device, bool sigOK);
    String getDeviceID() const;
    bool isValid();
    void setSuccess(bool b);
    JsonDocument& getResponse();
    JsonDocument& getRequest();
};


#endif //SINGLEGANGSWITCH_SINRICEVENT_H
