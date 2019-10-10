//
// Created by aarne on 10/4/2019.
//

#include <Arduino.h>
#include <ArduinoJson.h>

#include "JsonResponseBuilder.h"
#include "../MessageID.h"

JsonDocument JsonResponseBuilder::prepareResponse(JsonDocument& requestMessage) {
    DynamicJsonDocument responseMessage(1024);
    JsonObject header = responseMessage.createNestedObject("header");
    header["payloadVersion"] = 2;
    header["signatureVersion"] = 1;

    JsonObject payload = responseMessage.createNestedObject("payload");
    payload["action"] = requestMessage["payload"]["action"];
    payload["clientId"] = requestMessage["payload"]["clientId"];
    payload["createdAt"] = ntpWrapper.getTimestamp();
    payload["deviceId"] = requestMessage["payload"]["deviceId"];
    payload["message"] = "OK";
    payload["replyToken"] = requestMessage["payload"]["replyToken"];
    payload["success"] = false;
    payload["type"] = "response";
    payload.createNestedObject("value");
    return responseMessage;
}

JsonDocument JsonResponseBuilder::prepareEvent(String& deviceID, String& action, const char* cause) {
    DynamicJsonDocument eventMessage(1024);
    JsonObject header = eventMessage.createNestedObject("header");
    header["payloadVersion"] = 2;
    header["signatureVersion"] = 1;

    JsonObject payload = eventMessage.createNestedObject("payload");
    payload["action"] = action;
    payload["cause"] = cause;
    payload["createdAt"] = ntpWrapper.getTimestamp();
    payload["deviceId"] = deviceID;
    MessageID msgId;
    payload["replyToken"] = msgId.getID();
    payload["type"] = "event";
    payload.createNestedObject("value");
    return eventMessage;
}

