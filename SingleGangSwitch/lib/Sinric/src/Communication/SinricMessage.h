#ifndef __SINRICPRO_SINRIC_MESSAGE_H__
#define __SINRICPRO_SINRIC_MESSAGE_H__

#include <Arduino.h>

typedef enum {
  IF_UNKNOWN    = 0,
  IF_WEBSOCKET  = 1,
  IF_UDP        = 2
} NETWORK_INTERFACE;

class SinricMessage {
private:
    NETWORK_INTERFACE _interface;
    char* _message;
public:
  SinricMessage(NETWORK_INTERFACE interface, const char* message);
  ~SinricMessage();
  const char* getMessage();
  NETWORK_INTERFACE getInterface();
};

#endif
