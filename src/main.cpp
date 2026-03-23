
const char* mDnsHostname = "gaptuner"; // Keep hostname for mDNS

/*
 * ESP32 GAP Antenna Tuner Web Interface - v3 (K&R Indent)
 *
 * Controls relays via a web interface served using ESPAsyncWebServer.
 * Object-oriented design with classes for managing relays, tuner state (GAPTuner),
 * network (NetworkMgr), and web server. Action arrays are encapsulated and
 * inline-initialized within GAPTuner (requires C++17).
 */

// --- Includes ---
#include <Arduino.h> // For Serial, setup, loop, PROGMEM etc.
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#include "DebugUtils.h"
#include "RelayController.h"
#include "GAPTuner.h"
#include "NetworkMgr.h"
#include "WebServerManager.h"
#include "RelayTestController.h"

// --- Global Object Instances ---
RelayController      g_relayController;
GAPTuner             g_gaptuner(g_relayController);
NetworkMgr           g_networkMgr(mDnsHostname);
AsyncWebServer       g_asyncServer(80);
RelayTestController  g_relayTestController;
WebServerManager     g_webServerManager(g_asyncServer, g_gaptuner, g_networkMgr, g_relayTestController);


// ==========================================================================
// Arduino Setup and Loop
// ==========================================================================
void setup()
{
    #if DEBUG > 0
      Serial.begin(115200);
    #endif
    DEBUG_PRINTLN("\nStarting GAP Antenna Tuner Controller (v3)...");

    // Check and handle WiFi reset button press
    g_networkMgr.checkAndHandleWiFiResetButton();

    // Initialize relay pins and set default state for the tuner
    g_relayController.initializePins();
    g_relayTestController.initializePins();
    g_gaptuner.applyDefaultState();

    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        DEBUG_PRINTLN("main: NVS partition was truncated and needs to be erased.");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    DEBUG_PRINTLN("main: NVS flash initialized.");

    bool connected = g_networkMgr.connect();
    if (connected) {
        g_networkMgr.setupMDNS();
    }
    g_webServerManager.setupRoutes();
    g_webServerManager.begin();
}

void loop()
{
    // Do nothing here. Application is driven through http requests to AsyncWebserver
    // See WebServerManager::handle* which processes incoming http requests and acts on them.
}

// Gap Tuner UI HTML and Javascript
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>GAP Antenna Tuner</title>
    <style>
        :root{--icom-black:#1a1a1a;--icom-dark-grey:#2c2c2c;--icom-light-grey:#e0e0e0;--icom-blue-accent:#00aaff;--icom-shadow-dark:rgba(0,0,0,0.6);--icom-shadow-light:rgba(255,255,255,0.05);--border-radius:5px;--group-spacing:25px;--button-v-spacing:10px;--button-h-spacing:8px;--button-bg:var(--icom-button-grey);--button-text:var(--icom-light-grey);--icom-button-grey:#424242;}
        body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;display:flex;justify-content:center;align-items:flex-start;min-height:100vh;background-color:var(--icom-black);margin:0;padding:20px 15px;box-sizing:border-box;-webkit-tap-highlight-color:transparent;}
        .container{background:var(--icom-dark-grey);padding:25px 30px;border-radius:var(--border-radius);box-shadow:0 0 15px var(--icom-shadow-dark), inset 0 0 5px var(--icom-shadow-light);text-align:center;width:100%;max-width:380px;box-sizing:border-box;}
        
        .header{display:flex;justify-content:space-between;align-items:center;margin-bottom:0;}
        h1{color:var(--icom-light-grey);margin-top:0;margin-bottom:0;font-weight:600;font-size:1.6em;}
        .settings-link{color:var(--icom-light-grey);font-size:1.3em;text-decoration:none;opacity:0.6;padding:4px;line-height:1;}
        .settings-link:hover{opacity:1;}

        .wifi-status-line { text-align: center; margin-bottom: 30px; height: 1.2em; }
        #wifiStatusIndicator { display: inline-block; width: 15px; height: 15px; border-radius: 50%; margin-right: 6px; vertical-align: middle; background-color: #ffc107; transition: background-color 0.5s ease; }
        .wifi-online { background-color: #28a745 !important; }
        .wifi-offline { background-color: #dc3545 !important; }
        #wifiStatusText { font-size: 1.0em; vertical-align: middle; color: var(--icom-light-grey); }
        
        .button-group{margin-bottom:var(--group-spacing);text-align:left;}
        .button-group:last-child{margin-bottom:15px;}
        .group-title{font-size:0.9em;font-weight:600;color:var(--icom-light-grey);text-transform:uppercase;letter-spacing:0.5px;margin-bottom:15px;padding-left:5px;}
        
        button { padding:12px 10px; font-size:0.95rem; font-weight:500; text-align:center; cursor:pointer; border: 1px solid var(--icom-black); border-radius:var(--border-radius); background-color: var(--button-bg); color:var(--button-text); transition: background-color 0.15s ease, transform 0.05s ease, box-shadow 0.15s ease; -webkit-tap-highlight-color: transparent; box-sizing:border-box; outline: none; box-shadow: inset 0 2px 5px var(--icom-shadow-light), inset 0 -2px 5px var(--icom-shadow-dark), 0 2px 4px var(--icom-shadow-dark); text-shadow: 0 1px 2px rgba(0,0,0,0.5); }
        button:focus, button:focus-visible { background-color: var(--icom-blue-accent); color: var(--icom-black); box-shadow: inset 0 1px 3px var(--icom-shadow-dark), 0 1px 2px var(--icom-shadow-dark); }
        button:active { background-color: var(--icom-blue-accent); transform: translateY(1px) scale(0.98); box-shadow: inset 0 1px 3px var(--icom-shadow-dark); }
        .highlighted { background-color: var(--icom-blue-accent); color: var(--icom-black); box-shadow: inset 0 1px 3px var(--icom-shadow-dark), 0 1px 2px var(--icom-shadow-dark); }
        
        .button-group .button-stack button{display:block;width:100%;margin-bottom:var(--button-v-spacing);}
        .button-group .button-stack button:last-child{margin-bottom:0;}
        .button-group .button-row{display:flex;gap:var(--button-h-spacing);justify-content:space-between;}
        .button-group .button-row button{flex:1;}
        .status{margin-top:25px; font-size:0.85em; color:var(--icom-light-grey); min-height:3em; line-height:1.4; text-align:left; background-color: var(--icom-black); padding: 8px 12px; border-radius: 5px; white-space: pre-wrap;}
    </style>
</head>
<body>
    <div class="container" id="controlContainer">
        <div class="header">
            <h1>GAP Antenna Tuner</h1>
            <a href="/settings" class="settings-link" aria-label="Settings">&#9881;</a>
        </div>
        <div class="wifi-status-line"> <span id="wifiStatusIndicator"></span><span id="wifiStatusText">Checking...</span> </div>
        <div class="button-group"> <h3 class="group-title">Antenna Length</h3> <div class="button-row"> <button data-id="1">Shorter</button> <button data-id="2">Longer</button> </div> </div>
        <div class="button-group"> <h3 class="group-title">Tuning Network</h3> <div class="button-row"> <button data-id="3">None</button> <button data-id="4">1</button> <button data-id="5">2</button> </div> </div>
        <div class="button-group"> <h3 class="group-title">Calibration</h3> <div class="button-row"> <button data-id="6">Open</button> <button data-id="7">Short</button> <button data-id="8">Load</button> </div> </div>
        <div class="button-group"> <div class="button-stack"> <button onclick="window.location='/test'">Test Mode</button> </div> </div>
        <div class="status" id="statusMessage">Select an option above.</div>
    </div>
    <script>
        document.addEventListener('DOMContentLoaded', () => {
            const controlContainer = document.getElementById('controlContainer');
            const statusMessage = document.getElementById('statusMessage');
            let lastHighlightedAntennaButton = null; // To keep track of the last highlighted button in the antenna group
            let lastHighlightedOtherButton = null;    // To keep track of the last highlighted button in the other group

            // Helper function to determine button group
            function getButtonGroup(buttonId) {
                const id = parseInt(buttonId, 10);
                if (id === 1 || id === 2) {
                    return 'antenna'; // ANTENNA_SHORT, ANTENNA_LONG
                } else if (id >= 3 && id <= 8) {
                    return 'other'; // TUNING_NONE, TUNING_1, TUNING_2, CAL_OPEN, CAL_SHORT, CAL_LOAD
                }
                return 'unknown'; // Should not happen with valid button IDs
            }

            function checkWifiStatus() {
                const wifiIndicator = document.getElementById('wifiStatusIndicator');
                const wifiStatusText = document.getElementById('wifiStatusText');
                if (!wifiIndicator || !wifiStatusText) { console.warn("WiFi status elements not found yet."); return; }
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 5000); // Increased timeout to 5 seconds
                fetch('/wifi-status', { signal: controller.signal })
                    .then(response => {
                        clearTimeout(timeoutId);
                        if (!response.ok) { console.warn(`WiFi status endpoint error: ${response.status}`); throw new Error(`Status request server error: ${response.status}`); }
                        return response.text();
                    })
                    .then(status => {
                        console.log("WiFi Status from ESP32:", status);
                        if (status === "online") { wifiIndicator.className = 'wifi-online'; wifiStatusText.textContent = "Online"; }
                        else { wifiIndicator.className = 'wifi-offline'; wifiStatusText.textContent = "Offline (Reported)"; }
                    })
                    .catch(error => {
                        clearTimeout(timeoutId);
                        console.error("Error checking WiFi status:", error.name, error.message);
                        if (wifiIndicator) wifiIndicator.className = 'wifi-offline';
                        if (wifiStatusText) {
                            if (error.name === 'AbortError') { wifiStatusText.textContent = "Offline (Timeout)"; }
                            else { wifiStatusText.textContent = "Offline (Unreachable)"; }
                        }
                    });
            }
            checkWifiStatus();
            setInterval(checkWifiStatus, 5000);

            if (controlContainer) {
                controlContainer.addEventListener('click', event => {
                    if (event.target.tagName === 'BUTTON' && event.target.dataset.id) {
                        event.preventDefault();
                        const button_id_str = event.target.dataset.id;
                        const button_text_content = event.target.textContent;
                        const currentButtonGroup = getButtonGroup(button_id_str);

                        if (currentButtonGroup === 'antenna') {
                            if (lastHighlightedAntennaButton && lastHighlightedAntennaButton !== event.target) {
                                lastHighlightedAntennaButton.classList.remove('highlighted');
                            }
                            event.target.classList.add('highlighted');
                            lastHighlightedAntennaButton = event.target;
                        } else if (currentButtonGroup === 'other') {
                            if (lastHighlightedOtherButton && lastHighlightedOtherButton !== event.target) {
                                lastHighlightedOtherButton.classList.remove('highlighted');
                            }
                            event.target.classList.add('highlighted');
                            lastHighlightedOtherButton = event.target;
                        }
                        // If 'unknown' group, do nothing with highlighting

                        if (statusMessage) { statusMessage.textContent = `Sending: ${button_text_content}...`; }
                        else { console.error("statusMessage element not found!"); }
                        fetch(`/button?id=${button_id_str}`)
                            .then(response => {
                                if (!response.ok) return response.text().then(text_content => { throw new Error(`Server error: ${response.status} - ${text_content || "No details"}`) });
                                return response.text();
                            })
                            .then(data_from_server => {
                                console.log(`Button Action Response (Raw): ${data_from_server}`);
                                if (statusMessage) { statusMessage.textContent = data_from_server; }
                            })
                            .catch(error_obj => {
                                console.error("Error sending button command:", error_obj);
                                if (statusMessage) { statusMessage.textContent = `Error: ${error_obj.message}`; }
                            });
                    }
                });
            } else { console.error("controlContainer element not found! Button clicks will not work."); }
        });
    </script>
</body>
</html>
)rawliteral";
