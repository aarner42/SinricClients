//
// Created by aarne on 10/5/2019.
//

#include "SinricEvent.h"

SinricEvent::SinricEvent(JsonDocument& request, JsonDocument& response, String& device, bool sigOK) :
    requestMessage(request),
    responseMessage(response),
    deviceID(device),
    valid(sigOK)
{}

String SinricEvent::getDeviceID() const {
    return deviceID;
}

bool SinricEvent::isValid() {
    return valid;
}

JsonDocument& SinricEvent::getResponse() {
    return responseMessage;
}

JsonDocument& SinricEvent::getRequest() {
    return requestMessage;
}

void SinricEvent::setSuccess(bool success) {
    responseMessage["payload"]["success"] = success;
}





