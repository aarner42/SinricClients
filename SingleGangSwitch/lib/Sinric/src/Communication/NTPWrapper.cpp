//
// Created by aarne on 10/6/2019.
//

#include "NTPWrapper.h"

NTPWrapper::NTPWrapper() : _timeClient(_udpClient) {}

void NTPWrapper::begin() { _timeClient.begin(); _timeClient.update(); }

void NTPWrapper::update() { _timeClient.update(); }

unsigned long NTPWrapper::getTimestamp() { return _timeClient.getEpochTime(); }

