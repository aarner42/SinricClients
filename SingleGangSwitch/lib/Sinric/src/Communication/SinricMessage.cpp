//
// Created by aarne on 10/6/2019.
//

#include "SinricMessage.h"

SinricMessage::SinricMessage(NETWORK_INTERFACE interface, const char* message) :
    _interface(interface)
    { _message = strdup(message); };

SinricMessage::~SinricMessage() { if (_message) free(_message); };
const char* SinricMessage::getMessage() { return _message; };
NETWORK_INTERFACE SinricMessage::getInterface() { return _interface; };
