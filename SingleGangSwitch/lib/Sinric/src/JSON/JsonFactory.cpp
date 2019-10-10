//
// Created by aarne on 10/4/2019.
//

#include "JsonFactory.h"

JsonResponseBuilder JsonFactory::getResponseBuilder() {
    return responseBuilder;
}

JsonFactory::JsonFactory(const String appKey, const String secret) {
    this->apiKey = appKey;
    this->appSecret = secret;
}


String JsonFactory::calculateSignature(JsonDocument &jsonMessage) {
    if (!jsonMessage.containsKey("payload")) return String("");
    String jsonPayload; serializeJson(jsonMessage["payload"], jsonPayload);

    byte rawSigBuf[SHA256HMAC_SIZE];
    String test;
    const char* key = appSecret.c_str();
    SHA256HMAC hmac((byte*) key, strlen(key));
    hmac.doUpdate(jsonPayload.c_str(), jsonPayload.length());
    hmac.doFinal(rawSigBuf);

    int b64_len = base64_enc_len(SHA256HMAC_SIZE);
    char sigBuf[b64_len+1];
    base64_encode(sigBuf, (char*) rawSigBuf, SHA256HMAC_SIZE);
    sigBuf[b64_len] = 0;
    String result = sigBuf;

    return result;
}

bool JsonFactory::validateMessageSignature(JsonDocument &jsonMessage) {
    String jsonHash = jsonMessage["signature"]["HMAC"];
    String calculatedHash = calculateSignature(jsonMessage);
    return jsonHash == calculatedHash;
}

String JsonFactory::signMessage(JsonDocument &jsonMessage) {
    if (!jsonMessage.containsKey("signature")) jsonMessage.createNestedObject("signature");
    jsonMessage["signature"]["HMAC"] = calculateSignature(jsonMessage);
    String signedMessageString;
    serializeJson(jsonMessage, signedMessageString);
    return signedMessageString;
}

JsonDocument JsonFactory::getDocument(SinricMessage* msg) {
    DynamicJsonDocument document(1024);
    deserializeJson(document, msg->getMessage());
    return document;
}

SinricEvent JsonFactory::getTransaction(SinricMessage *msg) {
    JsonDocument jsonMessage = getDocument(msg);
    String deviceId = jsonMessage["payload"]["deviceId"];
    bool sigMatch = validateMessageSignature(jsonMessage);
    JsonDocument response = getResponseBuilder().prepareResponse(jsonMessage);
    SinricEvent transaction(jsonMessage, response, deviceId, sigMatch);

    DEBUG_SINRIC("[SinricProController.handleRequest(): Signature is %s\r\n", sigMatch ? "valid" : "invalid");
    if (sigMatch) { // signature is valid }
    #ifndef NODEBUG_SINRIC
            String debugOutput;
            serializeJsonPretty(jsonMessage, debugOutput);
            DEBUG_SINRIC("%s\r\n", debugOutput.c_str());
    #endif
    #ifndef NODEBUG_SINRIC
            String responseStringPretty;
            serializeJsonPretty(response, responseStringPretty);
            DEBUG_SINRIC("[SinricPro.handleRequest()]: response:\r\n%s\r\n", responseStringPretty.c_str());
    #endif
    } else { // signature is invalid!
        DEBUG_SINRIC("[SinricProController.handleRequest(): Signature should be: %s\r\n", calculateSignature(jsonMessage).c_str());
    }
    return transaction;
}

String JsonFactory::getSignedResponse(SinricEvent t) {
    String responseString = signMessage(t.getResponse());
    return responseString;
}



