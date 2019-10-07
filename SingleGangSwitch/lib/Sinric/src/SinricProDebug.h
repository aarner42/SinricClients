#ifndef __SINRICPRODEBUG_H__
#define __SINRICPRODEBUG_H__

#ifndef NODEBUG_SINRIC
#ifdef DEBUG_ESP_PORT
#define DEBUG_SINRIC(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
//#define DEBUG_WEBSOCKETS(...) os_printf( __VA_ARGS__ )
#endif  //DEBUG_ESP_PORT
#endif //NODEBUG_SINRIC
#endif //__SINRICPRODEBUG_H__

