//
// Created by aarne on 10/4/2019.
//

#ifndef SINRIC_JSONFACTORY_H
#define SINRIC_JSONFACTORY_H

#include <Arduino.h>
#include <Crypto.h>
#include "Base64.h"
#include <ArduinoJson.h>
//#include <AESLib.h>

#include "JsonResponseBuilder.h"
#include "SinricProDebug.h"

#include <Communication/SinricEvent.h>
#include <Communication/SinricMessage.h>

//TODO - Extend this when needed to handle multiple protocols/versions/formats
class JsonFactory {
private:
    const int VERSION = 1;
    String apiKey;
    String appSecret;
    String calculateSignature(JsonDocument &jsonMessage);
    JsonResponseBuilder responseBuilder;
    bool validateMessageSignature(JsonDocument &jsonMessage);
    static JsonDocument getDocument(SinricMessage* msg);
    String signMessage(JsonDocument &jsonMessage);
public:
    JsonFactory(String, String);
    JsonResponseBuilder getResponseBuilder();
    SinricEvent getTransaction(SinricMessage* msg);
    String getSignedResponse(SinricEvent t);

};


#endif //SINGLEGANGSWITCH_JSONFACTORY_H
