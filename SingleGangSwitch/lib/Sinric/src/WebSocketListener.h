#ifndef _SINRICPRO_WEBSOCKET_H__
#define _SINRICPRO_WEBSOCKET_H__

#if defined ESP8266
#include <ESP8266WiFi.h>
#endif
#if defined ESP32
#include <WiFi.h>
#endif

#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#include "Globals.h"
#include "SinricProConfig.h"

class WebSocketListener
{
public:
    typedef std::function<void(void)> wsConnectedCallback;
    typedef std::function<void(void)> wsDisconnectedCallback;

    WebSocketListener();
    ~WebSocketListener();

    void begin(String server, String appkey, String deviceIds);
    void handle();
    void stop();
    bool isConnected() { return _isConnected; }

    void sendResponse(String response);
    void sendEvent(String& event) { sendResponse(event); }

    void onConnected(const wsConnectedCallback& callback) { _wsConnectedCb = callback; }
    void onDisconnected(const wsDisconnectedCallback& callback) { _wsDisconnectedCb = callback; }

    void disconnect() { webSocket.disconnect(); }
private:
    bool _isConnected = false;

    WebSocketsClient webSocket;

    wsConnectedCallback _wsConnectedCb;
    wsDisconnectedCallback _wsDisconnectedCb;

    void handleSocketEvent(WStype_t type, uint8_t * payload, size_t length);
};

WebSocketListener::WebSocketListener() : _isConnected(false) {}

WebSocketListener::~WebSocketListener() {
    stop();
}

void WebSocketListener::begin(String server, String appkey, String deviceIds) {

    DEBUG_SINRIC("[SinricPro:Websocket]: Conecting to WebSocket Server\r\n");

    if (_isConnected) {
        stop();
    }

    String headers = "appkey:" + appkey + "\r\n" + "deviceids:" + deviceIds;
    DEBUG_SINRIC("[SinricPro:Websocket]: headers: \"%s\"\r\n", headers.c_str());
    webSocket.setExtraHeaders(headers.c_str());
    webSocket.begin(server, SERVER_PORT, "/"); // server address, port and URL
    webSocket.onEvent([&](WStype_t type, uint8_t * payload, size_t length) { handleSocketEvent(type, payload, length); });
    webSocket.enableHeartbeat(WEBSOCKET_PING_INTERVAL, WEBSOCKET_PING_TIMEOUT, WEBSOCKET_RETRY_COUNT);
    DEBUG_SINRIC("[SinricPro:Websocket]: connecting");
    while (!_isConnected) {
        DEBUG_SINRIC(".");
        webSocket.loop();
        delay(100);
    }
}

void WebSocketListener::handle() {
    webSocket.loop();
}

void WebSocketListener::stop() {
    if (_isConnected) {
        webSocket.disconnect();
    }
}

void WebSocketListener::sendResponse(String response) {
    webSocket.sendTXT(response);
}


void WebSocketListener::handleSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
    switch (type) {
        case WStype_DISCONNECTED:
            _isConnected = false;
            DEBUG_SINRIC("[SinricPro:Websocket]: disconnected\r\n");
            if (_wsDisconnectedCb) _wsDisconnectedCb();
            break;
        case WStype_CONNECTED:
            _isConnected = true;
            DEBUG_SINRIC("[SinricPro:Websocket]: connected\r\n");
            if (_wsConnectedCb) _wsConnectedCb();
            break;
        case WStype_TEXT: {
            auto* request = new SinricMessage(IF_WEBSOCKET, (char*)payload);
            DEBUG_SINRIC("[SinricPro:Websocket]: receiving request\r\n");
            receiveQueue.push(request);
            break;
        }
        default: break;
    }
}

#endif