#include <Communication/SinricMessage.h>
//
// Created by aarne on 10/4/2019.
//
#include "SinricProController.h"
#include "JSON/JsonFactory.h"

#include <utility>
#include <Devices/BinarySwitch.h>

SinricProController::SinricProController(const String& socketAuth, String signingKey, String wsURL) :
    jsonFactory(socketAuth, std::move(signingKey)),
    socketAuthorization(socketAuth),
    server(std::move(wsURL))
{}

SinricProController::~SinricProController() {
    stop();
    for (auto& device : devices) {
        delete device;
        device = nullptr;
    }
    devices.clear();
/*
    if (_app_key) delete _app_key;
    if (_app_secret) delete _app_secret;
*/
}

int SinricProController::size()  { return devices.size(); }

void SinricProController::connect() {
    String deviceList;
    int i = 0;
    for (auto& device : devices) {
        if (i>0) deviceList += ";";
        deviceList += String(device->getDeviceID());
        i++;
    }

    socketListener.begin(server, socketAuthorization, deviceList.c_str());
    while (!socketListener.isConnected()) { DEBUG_SINRIC("."); delay(250); }
    DEBUG_SINRIC("\r\n");
}

void SinricProController::disconnect() {
    socketListener.disconnect();
    socketListener.stop();
}

void SinricProController::add(IDevice* newDevice, unsigned long eventsEveryMS) {
    newDevice->setEventsFrequency(eventsEveryMS);
    devices.push_back(newDevice);
    if (isConnected()) reconnect();
}

void SinricProController::handle() {
    if (!isConnected()) connect();
    socketListener.handle();

    handleRequest();
    handleDeviceUpdates();
    handleSendQueue();
}

void SinricProController::handleDeviceUpdates() {
    for (auto device : devices) {
        if (device->hasUpdate()) {
            ServerUpdate* update = device->getUpdate();
            //push to sendQueue
        }
    }
}



/* handleRequest()
 * pop incoming request form receiveQueue
 * check signature of incoming request
 * callback handling
 * push response to sendQueue
 */
void SinricProController::handleRequest() {
    if (receiveQueue.count() > 0) {
        DEBUG_SINRIC("[SinricProController.handleRequest()]: %i requests in queue\r\n", receiveQueue.count());
        // POP requests and call device.handle() for each related device
        while (receiveQueue.count() > 0) {
            SinricMessage* rawMessage = receiveQueue.pop();
            SinricEvent transaction = jsonFactory.getTransaction(rawMessage);
            bool success = handleCallbacks(transaction);
            transaction.setSuccess(success);
            sendQueue.push(new SinricMessage(rawMessage->getInterface(), jsonFactory.getSignedResponse(transaction).c_str()));
            delete rawMessage;
        }
    }
}

bool SinricProController::handleCallbacks(const SinricEvent& transaction) {
    boolean allOK = false;
    for (auto device : devices) {
        if (device->canProcess(transaction))
            allOK = device->processRequest(transaction);
    }
    return allOK;

}

void SinricProController::handleSendQueue() {
    if (!isConnected()) return;
    if (sendQueue.count() > 0) {
        DEBUG_SINRIC("[SinricProController:handleSendQueue]: %i send items in queue\r\n", sendQueue.count());
        SinricMessage* rawMessage = sendQueue.pop();
        DEBUG_SINRIC("[SinricProController:handleSendQueue]:\r\n%s\r\n", rawMessage->getMessage());
        switch (rawMessage->getInterface()) {
            case IF_WEBSOCKET: socketListener.sendResponse(rawMessage->getMessage()); break;
//            case IF_UDP:       _udpListener.sendResponse(rawMessage->getMessage()); break;
            default:           break;
        }
        delete rawMessage;
    }
}

void SinricProController::stop() {
    DEBUG_SINRIC("[SinricProController:stop()\r\n");
    socketListener.disconnect();
    socketListener.stop();
}

bool SinricProController::isConnected() {
    return socketListener.isConnected();
};


void SinricProController::reconnect() {
    DEBUG_SINRIC("SinricProController.reconnect(): disconnecting\r\n");
    disconnect();
    DEBUG_SINRIC("SinricProController.reconnect(): wait 1second\r\n");
    delay(1000);
    DEBUG_SINRIC("SinricProController.reconnect(): connecting\r\n");
    connect();
}


// check if device can send an event
bool SinricProController::canSendEvent(const String& deviceId) {
    for (auto& device : devices) {
        if (String(device->getDeviceID()) == deviceId) {
            unsigned long actualMillis = millis();
            if (actualMillis - device->getLastEvent() >= device->getEventsFrequency()) {
                device->setLastEvent(actualMillis);
                return true;
            }
        }
    }
    return false;
}

BinarySwitch* SinricProController::buildBinarySwitch(String deviceID, PowerStateCallback cb) {
    return new BinarySwitch(std::move(deviceID), std::move(cb), jsonFactory);
}

// events
/*

void SinricProController::sendPowerStateEvent(String deviceId, bool state, String cause) {
    if (!canSendEvent(deviceId)) return;
    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setPowerState", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["state"] = state?"On":"Off";
    sendEvent(eventMessage);
}

void SinricProController::sendPowerLevelEvent(String deviceId, int powerLevel, String cause) {
    if (!canSendEvent(deviceId)) return;
    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setPowerLevel", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["powerLevel"] = powerLevel;
    sendEvent(eventMessage);
}

void SinricProController::sendBrightnessEvent(String deviceId, int brightness, String cause) {
    if (!canSendEvent(deviceId)) return;
    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setBrightness", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["brightness"] = brightness;
    sendEvent(eventMessage);
}

void SinricProController::sendColorEvent(String deviceId, unsigned char r, unsigned char g, unsigned char b, String cause) {
    if (!canSendEvent(deviceId)) return;
    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setColor", cause);
    JsonObject event_color = eventMessage["payload"]["value"].createNestedObject("color");
    event_color["r"] = r;
    event_color["g"] = g;
    event_color["b"] = b;
    sendEvent(eventMessage);
}

void SinricProController::sendColorTemperatureEvent(String deviceId, int colorTemperature, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setColorTemperature", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["colorTemperature"] = colorTemperature;
    sendEvent(eventMessage);
}

void SinricProController::sendDoorbellEvent(String deviceId, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "DoorbellPress", cause);

    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["state"] = "pressed";
    sendEvent(eventMessage);
}

void SinricProController::sendTemperatureEvent(String deviceId, float temperature, float humidity, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "currentTemperature", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["humidity"] = humidity;
    event_value["temperature"] = temperature;
    sendEvent(eventMessage);
}

void SinricProController::sendLockStateEvent(String deviceId, bool state, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setLockState", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    state ? event_value["state"] = "LOCKED" : event_value["state"] = "UNLOCKED";
    sendEvent(eventMessage);
}

void SinricProController::sendMotionEvent(String deviceId, bool state, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "motion", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["state"] = state?"detected":"notDetected";
    sendEvent(eventMessage);
}

void SinricProController::sendContactEvent(String deviceId, bool state, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setContactState", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["state"] = state?"closed":"open";
    sendEvent(eventMessage);
}

void SinricProController::sendRangeValueEvent(String deviceId, int rangeValue, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setRangeValue", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["rangeValue"] = rangeValue;
    sendEvent(eventMessage);
}

void SinricProController::sendVolumeEvent(String deviceId, int volume, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setVolume", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["volume"] = volume;
    sendEvent(eventMessage);
}

void SinricProController::sendInputEvent(String deviceId, String input, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "selectInput", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["input"] = input;
    sendEvent(eventMessage);
}

void SinricProController::sendMediaControlEvent(String deviceId, MediaControl_t control, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "mediaControl", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    switch (control) {
        case media_FastForward : event_value["control"] = "FastForward"; break;
        case media_Next : event_value["control"] = "Next"; break;
        case media_Pause : event_value["control"] = "Pause"; break;
        case media_Previous : event_value["control"] = "Previous"; break;
        case media_Rewind : event_value["control"] = "Rewind"; break;
        case media_StartOver : event_value["control"] = "StartOver"; break;
        case media_Stop : event_value["control"] = "Stop"; break;
        default: break;
    }
    sendEvent(eventMessage);
}

void SinricProController::sendThermostatModeEvent(String deviceId, ThermostatMode_t mode, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setThermostatMode", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    switch (mode) {
        case thermostat_OFF:  event_value["thermostatMode"] = "OFF"; break;
        case thermostat_COOL: event_value["thermostatMode"] = "COOL"; break;
        case thermostat_HEAT: event_value["thermostatMode"] = "HEAT"; break;
        case thermostat_AUTO: event_value["thermostatMode"] = "AUTO"; break;
        default: break;
    }
    sendEvent(eventMessage);
}

void SinricProController::sendChannelEvent(String deviceId, String channelName, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "changeChannel", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value.createNestedObject("channel");
    event_value["channel"]["name"] = channelName;
    sendEvent(eventMessage);
}

void SinricProController::sendModeEvent(String deviceId, String mode, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setMode", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value["mode"] = mode;
    sendEvent(eventMessage);
}

void SinricProController::sendBandsEvent(String deviceId, Bands_t bands, int level, String cause) {
    if (!canSendEvent(deviceId)) return;

    DynamicJsonDocument eventMessage = prepareEvent(deviceId, "setBands", cause);
    JsonObject event_value = eventMessage["payload"]["value"];
    event_value.createNestedArray("bands");
    event_value["bands"][0]["level"] = level;
    switch (bands) {
        case bands_BASS: event_value["bands"][0]["bands"] = "BASS"; break;
        case bands_MIDRANGE: event_value["bands"][0]["bands"] = "MIDRANGE"; break;
        case bands_TREBBLE: event_value["bands"][0]["bands"] = "TREBBLE"; break;
    }
    sendEvent(eventMessage);
}
*/
