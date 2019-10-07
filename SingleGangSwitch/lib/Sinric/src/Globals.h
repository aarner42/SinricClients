#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "Communication/SinricMessage.h"
#include "QueueList.h"


extern QueueList<SinricMessage*> receiveQueue;
extern QueueList<SinricMessage*> sendQueue;

#endif // _GLOBALS_H_
