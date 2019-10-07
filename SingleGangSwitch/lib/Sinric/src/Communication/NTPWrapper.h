#ifndef __SINRICPRO_NTP_H__
#define __SINRICPRO_NTP_H__


#if defined ESP8266
  #include <ESP8266WiFi.h>
#endif
#if defined ESP32
  #include <WiFi.h>
#endif

#include <WiFiUdp.h>
#include <NTPClient.h>

class NTPWrapper {
public:
  NTPWrapper();
  void begin();
  void update();
  unsigned long getTimestamp();
private:
  WiFiUDP _udpClient;
  NTPClient _timeClient;
};

#endif // _TIMESTAMP_H_
