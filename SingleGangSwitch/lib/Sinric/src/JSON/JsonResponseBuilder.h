//
// Created by aarne on 10/4/2019.
//

#ifndef _JSONRESPONSEBUILDER_H
#define _JSONRESPONSEBUILDER_H

#include <ArduinoJson.h>

#include "Communication/NTPWrapper.h"
#include "MessageID.h"

class JsonResponseBuilder {
private:
    const int VERSION = 1;

    NTPWrapper ntpWrapper;
public:
    JsonDocument prepareResponse(JsonDocument&);

    JsonDocument prepareEvent(String& deviceID, String& action, const char* cause);
};


#endif //SINGLEGANGSWITCH_JSONRESPONSEBUILDER_H
