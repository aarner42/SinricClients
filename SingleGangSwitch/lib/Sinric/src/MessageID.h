#ifndef __SINRIC_MESSAGEID_H__
#define __SINRIC_MESSAGEID_H__

#include "ESP8266TrueRandom.h"

class MessageID {
public:
  MessageID();
  char* getID();
private:
  void newMsgId();
  char msg_id[37];
};


#endif // __MESSAGEID_H__
