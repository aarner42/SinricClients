//
// Created by aarne on 10/4/2019.
//

#ifndef SINRICPRO_BINARYSWITCH_H
#define SINRICPRO_BINARYSWITCH_H

#include <utility>
#include <JSON/JsonFactory.h>

#include "IDevice.h"

typedef std::function<bool(String, bool)> PowerStateCallback;

class BinarySwitch :  public Device
{
public:

    BinarySwitch(String deviceId, PowerStateCallback cb, JsonFactory jsonFactory) {
        factory = jsonFactory;
        stateCallback = cb;
        deviceID = std::move(deviceId);
        state = false;
    }

    bool processRequest(SinricEvent t) override {
        JsonDocument request = t.getRequest();
        JsonObject requestPayloadValue = request["payload"]["value"];

        bool powerState = false;
        if (requestPayloadValue["state"] == "On")
            powerState = true;
        if (requestPayloadValue["state"] == "Off")
            powerState = false;
        bool cbResult = stateCallback(t.getDeviceID(), powerState);
        return cbResult;
    }



    ~BinarySwitch() override = default;

    void setState(bool newState) {
        if (newState != state) {
            state = newState;

            DynamicJsonDocument eventMessage = jsonFactory.prepareEvent(deviceId, "setPowerState", cause);
            JsonObject event_value = eventMessage["payload"]["value"];
            event_value["state"] = state?"On":"Off";
            sendEvent(eventMessage);

            //send update to sinric
            //response = new SinricMessage();

        }
    }

private:
    PowerStateCallback stateCallback;
    bool state;
};


#endif //SINGLEGANGSWITCH_BINARYSWITCH_H
