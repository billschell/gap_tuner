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
    // Initialises NVS flash and derives the AP SSID from the last 4 hex digits of the MAC address.
    NetworkMgr(const char* confHostname);
    // Connects in the stored mode: STA in home mode, soft-AP in field mode; returns true on STA success.
    bool connect();
    // Registers the mDNS hostname so the device is reachable at <hostname>.local (STA mode only).
    void setupMDNS();
    // Returns true when the WiFi station interface reports WL_CONNECTED.
    bool isConnected();

    // Credential management
    // Writes SSID and password to NVS flash and commits; returns false on any NVS error.
    bool saveCredentials(const char* ssid, const char* password);
    // Reads stored SSID and password from NVS into member variables; returns false if not found.
    bool loadCredentials();
    // Erases SSID and password keys from NVS and clears the in-memory copies.
    void clearCredentials();
    // Samples the reset button on startup; if held LOW, clears credentials and switches to field mode.
    void checkAndHandleWiFiResetButton();

    // Mode management ("field" or "home")
    // Returns the stored operating mode string ("field" or "home"), defaulting to "field" if absent.
    String getMode();
    // Persists the operating mode string to NVS flash.
    void setMode(const char* mode);

    // Accessors for settings UI
    // Returns the soft-AP SSID (e.g. "GAP Tuner - XXXX").
    String getAPSSID()          const { return _apSSID; }
    // Returns the soft-AP password.
    const char* getAPPassword() const { return _apPassword; }
    // Returns the mDNS hostname (e.g. "gaptuner").
    const char* getHostname()   const { return _hostname; }
    // Returns the stored home WiFi SSID.
    String getStoredSSID()      const { return _ssid; }
    // Returns true if the last home WiFi connection attempt failed.
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

    // Configures the ESP32 as a soft-AP using the computed SSID and default password.
    void startAP();
};

#endif // NETWORK_MGR_H
