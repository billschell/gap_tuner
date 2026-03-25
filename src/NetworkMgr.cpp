#include "NetworkMgr.h"
#include "DebugUtils.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_system.h>

#define NVS_NAMESPACE "wifi_creds"
#define NVS_KEY_SSID  "ssid"
#define NVS_KEY_PASS  "password"
#define NVS_KEY_MODE  "mode"

// Initialises NVS flash, opens the NVS namespace, and computes the AP SSID from the MAC address.
NetworkMgr::NetworkMgr(const char* confHostname) :
    _hostname(confHostname), _wifiResetButtonPin(WIFI_RESET_BUTTON_PIN)
{
    // Compute AP SSID once so it's always available (even in home/STA mode)
    String macAddr = String(ESP.getEfuseMac(), HEX);
    _apSSID = "GAP Tuner - " + macAddr.substring(macAddr.length() - 4);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        DEBUG_PRINTLN("NetworkMgr: NVS partition truncated, erasing.");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &_nvsHandle);
    if (ret != ESP_OK) {
        DEBUG_PRINTF("NetworkMgr: Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    } else {
        DEBUG_PRINTLN("NetworkMgr: NVS opened successfully.");
    }
}

// Writes SSID and password to NVS flash and commits; returns false on any NVS error.
bool NetworkMgr::saveCredentials(const char* ssid, const char* password) {
    if (_nvsHandle == 0) {
        DEBUG_PRINTLN("NetworkMgr: NVS handle not open.");
        return false;
    }
    esp_err_t ret = nvs_set_str(_nvsHandle, NVS_KEY_SSID, ssid);
    if (ret != ESP_OK) { DEBUG_PRINTF("NetworkMgr: Failed to write SSID (%s)\n", esp_err_to_name(ret)); return false; }
    ret = nvs_set_str(_nvsHandle, NVS_KEY_PASS, password);
    if (ret != ESP_OK) { DEBUG_PRINTF("NetworkMgr: Failed to write password (%s)\n", esp_err_to_name(ret)); return false; }
    ret = nvs_commit(_nvsHandle);
    if (ret != ESP_OK) { DEBUG_PRINTF("NetworkMgr: Failed to commit NVS (%s)\n", esp_err_to_name(ret)); return false; }
    DEBUG_PRINTLN("NetworkMgr: Credentials saved to NVS.");
    return true;
}

// Reads stored SSID and password from NVS into member variables; returns false if not found.
bool NetworkMgr::loadCredentials() {
    if (_nvsHandle == 0) { return false; }
    size_t required_size;
    esp_err_t ret;

    ret = nvs_get_str(_nvsHandle, NVS_KEY_SSID, NULL, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) { DEBUG_PRINTLN("NetworkMgr: SSID not in NVS."); return false; }
    if (ret != ESP_OK) { DEBUG_PRINTF("NetworkMgr: Error reading SSID (%s)\n", esp_err_to_name(ret)); return false; }
    char* temp_ssid = (char*)malloc(required_size);
    nvs_get_str(_nvsHandle, NVS_KEY_SSID, temp_ssid, &required_size);
    _ssid = temp_ssid;
    free(temp_ssid);

    ret = nvs_get_str(_nvsHandle, NVS_KEY_PASS, NULL, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) { DEBUG_PRINTLN("NetworkMgr: Password not in NVS."); return false; }
    if (ret != ESP_OK) { DEBUG_PRINTF("NetworkMgr: Error reading password (%s)\n", esp_err_to_name(ret)); return false; }
    char* temp_password = (char*)malloc(required_size);
    nvs_get_str(_nvsHandle, NVS_KEY_PASS, temp_password, &required_size);
    _password = temp_password;
    free(temp_password);

    DEBUG_PRINTF("NetworkMgr: Loaded SSID: %s\n", _ssid.c_str());
    return true;
}

// Erases SSID and password keys from NVS and clears the in-memory copies.
void NetworkMgr::clearCredentials() {
    if (_nvsHandle == 0) { return; }
    nvs_erase_key(_nvsHandle, NVS_KEY_SSID);
    nvs_erase_key(_nvsHandle, NVS_KEY_PASS);
    esp_err_t ret = nvs_commit(_nvsHandle);
    if (ret == ESP_OK) { DEBUG_PRINTLN("NetworkMgr: Credentials cleared."); }
    _ssid = "";
    _password = "";
}

// Returns the stored operating mode ("field" or "home"), defaulting to "field" if absent.
String NetworkMgr::getMode() {
    if (_nvsHandle == 0) { return "field"; }
    size_t required_size;
    esp_err_t ret = nvs_get_str(_nvsHandle, NVS_KEY_MODE, NULL, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND || ret != ESP_OK) { return "field"; }
    char* buf = (char*)malloc(required_size);
    nvs_get_str(_nvsHandle, NVS_KEY_MODE, buf, &required_size);
    String mode = buf;
    free(buf);
    return mode;
}

// Persists the operating mode string to NVS flash.
void NetworkMgr::setMode(const char* mode) {
    if (_nvsHandle == 0) { return; }
    esp_err_t ret = nvs_set_str(_nvsHandle, NVS_KEY_MODE, mode);
    if (ret == ESP_OK) { nvs_commit(_nvsHandle); }
    DEBUG_PRINTF("NetworkMgr: Mode set to '%s'\n", mode);
}

// Connects as STA in home mode or starts a soft-AP in field mode; falls back to AP if STA fails.
bool NetworkMgr::connect() {
    String mode = getMode();
    DEBUG_PRINTF("NetworkMgr: Operating mode: '%s'\n", mode.c_str());

    if (mode == "home") {
        if (loadCredentials()) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(_ssid.c_str(), _password.c_str());
            DEBUG_PRINT("NetworkMgr: Connecting to '"); DEBUG_PRINT(_ssid); DEBUG_PRINT("' ...");
            int retries = 0;
            while (WiFi.status() != WL_CONNECTED && retries < 30) { // Wait up to ~15 seconds
                delay(500); DEBUG_PRINT("."); retries++;
            }
            if (WiFi.status() == WL_CONNECTED) {
                DEBUG_PRINTLN("\nNetworkMgr: WiFi Connected!");
                DEBUG_PRINT("  IP: http://"); DEBUG_PRINTLN(WiFi.localIP());
                return true;
            }
            DEBUG_PRINTLN("\nNetworkMgr: WiFi connection failed with saved credentials.");
            _wifiFailedFlag = true;
        } else {
            DEBUG_PRINTLN("NetworkMgr: No saved credentials for home mode.");
            _wifiFailedFlag = true;
        }
    }

    // Field mode, or home mode fallback
    startAP();
    return false;
}

// Configures the ESP32 as a soft-AP on 192.168.4.1 using the computed SSID and default password.
void NetworkMgr::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apSSID.c_str(), _apPassword);

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    DEBUG_PRINTF("NetworkMgr: AP started: '%s' / '%s'\n", _apSSID.c_str(), _apPassword);
    DEBUG_PRINTF("NetworkMgr: Access at http://192.168.4.1\n");
}

// Starts mDNS and advertises the HTTP service so the device is reachable at <hostname>.local.
void NetworkMgr::setupMDNS() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("NetworkMgr: Cannot setup mDNS, WiFi not connected."); return;
    }
    if (!MDNS.begin(_hostname)) {
        DEBUG_PRINTLN("NetworkMgr: Error setting up mDNS!");
    } else {
        MDNS.addService("http", "tcp", 80);
        DEBUG_PRINTF("NetworkMgr: mDNS started: http://%s.local\n", _hostname);
    }
}

// Returns true when the WiFi station interface reports WL_CONNECTED.
bool NetworkMgr::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Samples the reset button on startup; clears credentials and switches to field mode if held LOW.
void NetworkMgr::checkAndHandleWiFiResetButton() {
    pinMode(_wifiResetButtonPin, INPUT_PULLUP);
    delay(500);
    if (digitalRead(_wifiResetButtonPin) == LOW) {
        DEBUG_PRINTLN("NetworkMgr: Reset button held. Clearing credentials, setting field mode.");
        clearCredentials();
        setMode("field");
        delay(500);
    } else {
        DEBUG_PRINTLN("NetworkMgr: Reset button not pressed.");
    }
}
