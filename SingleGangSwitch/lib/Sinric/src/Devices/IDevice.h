//
// Created by aarne on 10/4/2019.
//

#ifndef SINRIC_IDEVICE_H
#define SINRIC_IDEVICE_H

#include <Arduino.h>
#include "Communication/SinricEvent.h"

class IDevice
{
public:
    virtual ~IDevice() = default;
    virtual bool processRequest(SinricEvent t) = 0;
    virtual String getDeviceID() = 0;
    virtual bool canProcess(const SinricEvent& t) = 0;
    virtual unsigned long getLastEvent() = 0;
    virtual unsigned long getEventsFrequency() = 0;
    virtual void setLastEvent(unsigned long) = 0;
    virtual void setEventsFrequency(unsigned int) = 0;
};

class Device : public IDevice
{
public:
    ~Device() override = default;;
    String getDeviceID()                        override { return deviceID; };
    bool canProcess(const SinricEvent& t)       override { return deviceID == t.getDeviceID(); }
    unsigned long getLastEvent()                override { return lastEvent; }
    unsigned long getEventsFrequency()          override { return eventsFrequency; }
    void setLastEvent(unsigned long newEvent)   override {lastEvent = newEvent; }
    void setEventsFrequency(unsigned int eventFrequency) final { eventFrequency >= 100 ? eventsFrequency = eventFrequency : eventsFrequency = 100; }
protected:
    String deviceID;
    unsigned long lastEvent;
    unsigned long eventsFrequency = 100;
};


#endif //SINGLEGANGSWITCH_IDEVICE_H
