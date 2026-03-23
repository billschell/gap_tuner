#ifndef NETWORK_MGR_H
#define NETWORK_MGR_H

#include <Arduino.h> // For String, delay (used in .cpp)
#include <WiFi.h>
#include <ESPmDNS.h>
#include <nvs_flash.h> // For NVS flash operations
#include <nvs.h>       // For NVS API

// Define the WiFi reset button pin
#define WIFI_RESET_BUTTON_PIN GPIO_NUM_1

class NetworkMgr {
public:
    NetworkMgr(const char* confHostname);
    bool connect();
    void setupMDNS();
    bool isConnected();

    // Credential management
    bool saveCredentials(const char* ssid, const char* password);
    bool loadCredentials();
    void clearCredentials();
    void checkAndHandleWiFiResetButton();

    // Mode management ("field" or "home")
    String getMode();
    void setMode(const char* mode);

    // Accessors for settings UI
    String getAPSSID()          const { return _apSSID; }
    const char* getAPPassword() const { return _apPassword; }
    const char* getHostname()   const { return _hostname; }
    String getStoredSSID()      const { return _ssid; }
    bool wifiConnectFailed()    const { return _wifiFailedFlag; }

private:
    String _ssid;
    String _password;
    String _apSSID;
    const char* _apPassword = "gaptuner";
    const char* _hostname;
    const int _wifiResetButtonPin;
    bool _wifiFailedFlag = false;

    nvs_handle_t _nvsHandle;

    void startAP();
};

#endif // NETWORK_MGR_H
