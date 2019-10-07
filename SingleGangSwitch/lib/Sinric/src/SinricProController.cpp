//
// Created by aarne on 10/4/2019.
//
#include "SinricProController.h"
#include "JSON/JsonFactory.h"

#include <utility>

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
    handleSendQueue();
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
/*

    if (powerStateCallback && action == "setPowerState") {
        bool powerState;
        if (request_value["state"] == "On") powerState = true;
        if (request_value["state"] == "Off") powerState = false;
        success = powerStateCallback(deviceId, powerState);
        response_value["state"] = powerState?"On":"Off";
        return success;
    }

    // setPowerLevel
    if (powerLevelCallback && action == "setPowerLevel") {
        int powerLevel = request_value["powerLevel"];
        success = powerLevelCallback(deviceId, powerLevel);
        response_value["powerLevel"] = powerLevel;
        return success;
    }

    // adjustPowerLevel
    // input: relative powerlevel
    // output: absolute powerLevel!
    if (adjustPowerLevelCallback && action == "adjustPowerLevel") {
        int powerLevelDelta = request_value["powerLevelDelta"];
        success = adjustPowerLevelCallback(deviceId, powerLevelDelta);
        response_value["powerLevel"] = powerLevelDelta;
        return success;
    }

    // setBrightness
    if (brightnessCallback && action == "setBrightness") {
        int brightness = request_value["brightness"];
        success = brightnessCallback(deviceId, brightness);
        response_value["brightness"] = brightness;
        return success;
    }
    // adjustBrightness
    // get relative brightness (brightnessDelta) / return absolute brightness!
    if (adjustBrightnessCallback && action == "adjustBrightness") {
        int brightnessDelta = request_value["brightnessDelta"];
        success = adjustBrightnessCallback(deviceId, brightnessDelta);
        response_value["brightness"] = brightnessDelta;
        return success;
    }

    // setColor
    if (colorCallback && action == "setColor") {
        unsigned char r, g, b;
        r = request_value["color"]["r"];
        g = request_value["color"]["g"];
        b = request_value["color"]["b"];
        success = colorCallback(deviceId, r, g, b);
        response_value.createNestedObject("color");
        response_value["color"]["r"] = r;
        response_value["color"]["g"] = g;
        response_value["color"]["b"] = b;
        return success;
    }

    // setColorTemperature
    if (colorTemperatureCallback && action == "setColorTemperature") {
        int colorTemperature = request_value["ColorTemperature"];
        success = colorTemperatureCallback(deviceId, colorTemperature);
        response_value["ColorTemperature"] = colorTemperature;
        return success;
    }

    // increaseColorTemperature
    // input: none
    // output colorTemperature
    if (increaseColorTemperatureCallback && action == "increaseColorTemperature") {
        int colorTemperature;
        success = increaseColorTemperatureCallback(deviceId, colorTemperature);
        response_value["colorTemperature"] = colorTemperature;
        return success;
    }

    // decreaseColorTemperature
    // input: none
    // output: colorTemperature
    if (decreaseColorTemperatureCallback && action == "decreaseColorTemperature") {
        int colorTemperature;
        success = decreaseColorTemperatureCallback(deviceId, colorTemperature);
        response_value["colorTemperature"] = colorTemperature;
        return success;
    }

    // targetTemperature
    // input: temperature, optional: duration
    // return: temperature
    if (setTemperatureCallback && action == "targetTemperature") {
        float temperature = request_value["temperature"];
        const char* duration = request_value["schedule"]["duration"] | "";
        success = setTemperatureCallback(deviceId, temperature, duration);
        response_value["temperature"] = temperature;
        return success;
    }

    // adjustTemperature
    // input: relative temperature
    // output: absolute temperature
    if (adjustTemperatureCallback && action == "adjustTemperature") {
        float temperature = request_value["temperature"];
        success = adjustTemperatureCallback(deviceId, temperature);
        response_value["temperature"] = temperature;
        return success;
    }

    // setThermostatMode
    if (setThermostatModeCallback && action == "setThermostatMode") {
        ThermostatMode_t thermostatMode;

        if (request_value["thermostatMode"] == "OFF") thermostatMode = thermostat_OFF;
        if (request_value["thermostatMode"] == "COOL") thermostatMode = thermostat_COOL;
        if (request_value["thermostatMode"] == "HEAT") thermostatMode = thermostat_HEAT;
        if (request_value["thermostatMode"] == "AUTO") thermostatMode = thermostat_AUTO;

        success = setThermostatModeCallback(deviceId, thermostatMode);

        switch (thermostatMode) {
            case thermostat_OFF:  response_value["thermostatMode"] = "OFF"; break;
            case thermostat_COOL: response_value["thermostatMode"] = "COOL"; break;
            case thermostat_HEAT: response_value["thermostatMode"] = "HEAT"; break;
            case thermostat_AUTO: response_value["thermostatMode"] = "AUTO"; break;
        }
        return success;
    }

    // setLockState
    if (lockStateCallback && action == "setLockState") {
        bool lockState;
        if (request_value["state"] == "lock") lockState = true;
        if (request_value["state"] == "unlock") lockState = false;

        success = lockStateCallback(deviceId, lockState);
        response_value["state"] = lockState?"LOCKED":"UNLOCKED";
        return success;
    }

    // setVolume
    if (volumeCallback && action == "setVolume") {
        int volume = request_value["volume"];
        success = volumeCallback(deviceId, volume);
        response_value["volume"] = volume;
        return success;
    }

    // adjustVolume
    // input: relative volume
    // output: absolute volume
    if (adjustVolumeCallback && action == "adjustVolume") {
        int volume = request_value["volume"];
        success = adjustVolumeCallback(deviceId, volume);
        response_value["volume"] = volume;
        return success;
    }

    // setMute
    if (muteCallback && action == "setMute") {
        bool mute = request_value["mute"];
        success = muteCallback(deviceId, mute);
        response_value["mute"] = mute;
        return success;
    }

    // selectInput
    if (selectInputCallback && action == "selectInput") {
        String input = request_value["input"];

        success = selectInputCallback(deviceId, input);

        response_value["input"] = input;
        return success;
    }

    // mediaControl
    if (mediaControlCallback && action == "mediaControl") {
        MediaControl_t mediaControl;
        if (request_value["control"] == "FastForward") mediaControl = media_FastForward;
        if (request_value["control"] == "Next") mediaControl = media_Next;
        if (request_value["control"] == "Pause") mediaControl = media_Pause;
        if (request_value["control"] == "Previous") mediaControl = media_Previous;
        if (request_value["control"] == "Rewind") mediaControl = media_Rewind;
        if (request_value["control"] == "StartOver") mediaControl = media_StartOver;
        if (request_value["control"] == "Stop") mediaControl = media_Stop;

        success = mediaControlCallback(deviceId, mediaControl);

        switch (mediaControl) {
            case media_FastForward : response_value["control"] = "FastForward"; break;
            case media_Next : response_value["control"] = "Next"; break;
            case media_Pause : response_value["control"] = "Pause"; break;
            case media_Previous : response_value["control"] = "Previous"; break;
            case media_Rewind : response_value["control"] = "Rewind"; break;
            case media_StartOver : response_value["control"] = "StartOver"; break;
            case media_Stop : response_value["control"] = "Stop"; break;
        }
        return success;
    }

    if (rangeValueCallback && action == "setRangeValue") {
        int rangeValue = request_value["rangeValue"];
        success = rangeValueCallback(deviceId, rangeValue);
        response_value["rangeValue"] = rangeValue;
        return success;
    }

    if (adjustRangeValueCallback && action == "adjustRangeValue") {
        int rangeValueDelta = request_value["rangeValueDelta"];
        success = adjustRangeValueCallback(deviceId, rangeValueDelta);
        response_value["rangeValue"] = rangeValueDelta;
        return success;
    }

    if (channelCallback && action == "changeChannel") {
        String channelName = request_value["channel"]["name"];
        success = channelCallback(deviceId, channelName);
        response_value["channel"].createNestedObject("name");
        response_value["channel"]["name"] = channelName;
        return success;
    }

    if (skipChannelCallback && action == "skipChannels") {
        String channelName = "";
        int channelCount = request_value["channelCount"];
        success = skipChannelCallback(deviceId, channelCount, channelName);
        response_value["channel"].createNestedObject("name");
        response_value["channel"]["name"] = channelName;
        return success;
    }

    if (modeCallback && action == "setMode") {
        String mode = request_value["mode"];
        success = modeCallback(deviceId, mode);
        response_value["mode"] = mode;
        return success;
    }

    if (setBandsCallback && action == "setBands") {
        JsonArray bandsArray = request_value["bands"];
        response_value.createNestedArray("bands");

        for (size_t i=0; i < bandsArray.size(); i++) {
            Bands_t bands = bands_BASS;
            int level = bandsArray[i]["level"];
            String bandsStr = bandsArray[i]["name"];
            if (bandsStr == "BASS") bands = bands_BASS;
            if (bandsStr == "MIDRANGE") bands = bands_MIDRANGE;
            if (bandsStr == "TREBBLE") bands = bands_TREBBLE;
            success = setBandsCallback(deviceId, bands, level);
            JsonObject response_value_bands = response_value["bands"].createNestedObject();
            response_value_bands["level"] = level;
            response_value_bands["name"] = bandsStr;
        }
        return success;
    }

    if (adjustBandsCallback && action == "adjustBands") {
        JsonArray bandsArray = request_value["bands"];
        response_value.createNestedArray("bands");

        for (size_t i=0; i < bandsArray.size(); i++) {
            Bands_t bands = bands_BASS;
            int level = request_value["bands"][i]["levelDelta"] | 1;
            if (request_value["bands"][i]["levelDirection"] == "DOWN") level *= -1;

            String bandsStr = request_value["bands"][i]["name"];
            if (bandsStr == "BASS") bands = bands_BASS;
            if (bandsStr == "MIDRANGE") bands = bands_MIDRANGE;
            if (bandsStr == "TREBBLE") bands = bands_TREBBLE;
            success = adjustBandsCallback(deviceId, bands, level);
            JsonObject response_value_bands = response_value["bands"].createNestedObject();
            response_value_bands["level"] = level;
            response_value_bands["name"] = bandsStr;
        }
        return success;
    }

    if (resetBandsCallback && action == "resetBands") {
        JsonArray bandsArray = request_value["bands"];
        response_value.createNestedArray("bands");
        for (size_t i=0; i < bandsArray.size(); i++) {
            Bands_t bands = bands_BASS;

            String bandsStr = request_value["bands"][i]["name"];
            if (bandsStr == "BASS") bands = bands_BASS;
            if (bandsStr == "MIDRANGE") bands = bands_MIDRANGE;
            if (bandsStr == "TREBBLE") bands = bands_TREBBLE;
            success = resetBandsCallback(deviceId, bands);
            JsonObject response_value_bands = response_value["bands"].createNestedObject();
            response_value_bands["level"] = 0;
            response_value_bands["name"] = bandsStr;
        }
        return success;
    }
    return success;
    */
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
bool SinricProController::canSendEvent(String deviceId) {
    for (auto& device : devices) {
        if (String(device->getDeviceID()) == deviceId) {
            unsigned long actualMillis = millis();
            if (actualMillis - device->getLastEvent() >= device->getEventsEveryMS()) {
                device->setLastEvent(actualMillis);
                return true;
            }
        }
    }
    return false;
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
