//
// Created by aarne on 10/6/2019.
//

#include "MessageID.h"

char* MessageID::getID() { return msg_id; };

MessageID::MessageID() {
    newMsgId();
}

void MessageID::newMsgId() {
    byte new_uuid[16];
    ESP8266TrueRandom.uuid(new_uuid);
    strlcpy(msg_id, ESP8266TrueRandom.uuidToString(new_uuid).c_str(), 37);
}
