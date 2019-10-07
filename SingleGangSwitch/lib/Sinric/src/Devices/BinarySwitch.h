//
// Created by aarne on 10/4/2019.
//

#ifndef SINRICPRO_BINARYSWITCH_H
#define SINRICPRO_BINARYSWITCH_H

#include <utility>

#include "IDevice.h"

typedef std::function<bool(String, bool)> PowerStateCallback;

class BinarySwitch :  public Device
{
public:

    BinarySwitch(String deviceId, PowerStateCallback cb) {
        stateCallback = cb;
        deviceID = std::move(deviceId);
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

private:
    PowerStateCallback stateCallback;
};


#endif //SINGLEGANGSWITCH_BINARYSWITCH_H
