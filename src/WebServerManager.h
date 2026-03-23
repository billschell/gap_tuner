#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <Arduino.h> // For String
#include <ESPAsyncWebServer.h>

// Forward declarations for classes used by reference/pointer
class GAPTuner;
class NetworkMgr;
class RelayTestController;

// Extern declaration for HTML string defined in main.cpp
extern const char index_html[];

class WebServerManager {
public:
    static constexpr const char* WIFI_STATUS_ONLINE = "online";
    static constexpr const char* WIFI_STATUS_OFFLINE = "offline";

    WebServerManager(AsyncWebServer& srv, GAPTuner& tuner, NetworkMgr& netMgr,
                     RelayTestController& testCtrl);
    void setupRoutes();
    void begin();

private:
    AsyncWebServer&      _server;
    GAPTuner&            _gaptuner;
    NetworkMgr&          _networkMgr;
    RelayTestController& _relayTestController;

    void handleRootRequest(AsyncWebServerRequest *request);
    void handleButtonRequest(AsyncWebServerRequest *request);
    void handleWiFiStatusRequest(AsyncWebServerRequest *request);
    void handleNotFoundRequest(AsyncWebServerRequest *request);

    void handleTestRootRequest(AsyncWebServerRequest *request);
    void handleTestRelayRequest(AsyncWebServerRequest *request);
    void handleTestResetRequest(AsyncWebServerRequest *request);

    void handleSettingsRequest(AsyncWebServerRequest *request);
    void handleSettingsSaveRequest(AsyncWebServerRequest *request);
    void handleInfoRequest(AsyncWebServerRequest *request);
};

#endif // WEBSERVER_MANAGER_H
