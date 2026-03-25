
const char* mDnsHostname = "gaptuner"; // Keep hostname for mDNS

/*
 * ESP32 GAP Antenna Tuner Web Interface - v3
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

        .wifi-status-line { text-align: center; margin-bottom: 30px; min-height: 1.2em; }
        #wifiStatusIndicator { display: inline-block; width: 15px; height: 15px; border-radius: 50%; margin-right: 6px; vertical-align: middle; background-color: #ffc107; transition: background-color 0.5s ease; }
        .wifi-online { background-color: #28a745 !important; }
        .wifi-offline { background-color: #dc3545 !important; }
        #wifiStatusText { font-size: 1.0em; vertical-align: middle; color: var(--icom-light-grey); }
        #wifiStatusHint { font-size: 0.8em; color: var(--icom-light-grey); opacity: 0.7; margin-top: 3px; }
        
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
            <a href="/settings" class="settings-link" aria-label="Settings">
              <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                <path d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.325.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.241-.438.613-.43.992a7.723 7.723 0 0 1 0 .255c-.008.378.137.75.43.991l1.004.827c.424.35.534.955.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.47 6.47 0 0 1-.22.128c-.331.183-.581.495-.644.869l-.213 1.281c-.09.543-.56.94-1.11.94h-2.594c-.55 0-1.019-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.991a6.932 6.932 0 0 1 0-.255c.007-.38-.138-.751-.43-.992l-1.004-.827a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.086.22-.128.332-.183.582-.495.644-.869l.214-1.28Z"/>
                <path d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z"/>
              </svg>
            </a>
        </div>
        <div class="wifi-status-line">
            <div><span id="wifiStatusIndicator"></span><span id="wifiStatusText">Checking...</span></div>
            <div id="wifiStatusHint"></div>
        </div>
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
            let apSSID = '';
            fetch('/info').then(r => r.json()).then(d => { apSSID = d.apSSID; }).catch(() => {});

            // Returns 'antenna' for buttons 1–2, 'other' for buttons 3–8, or 'unknown'.
            function getButtonGroup(buttonId) {
                const id = parseInt(buttonId, 10);
                if (id === 1 || id === 2) {
                    return 'antenna'; // ANTENNA_SHORT, ANTENNA_LONG
                } else if (id >= 3 && id <= 8) {
                    return 'other'; // TUNING_NONE, TUNING_1, TUNING_2, CAL_OPEN, CAL_SHORT, CAL_LOAD
                }
                return 'unknown'; // Should not happen with valid button IDs
            }

            // Fetches /wifi-status and updates the indicator dot and status text; aborts after 5 s.
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
                        const hintEl = document.getElementById('wifiStatusHint');
                        if (status === "online") {
                            wifiIndicator.className = 'wifi-online';
                            wifiStatusText.textContent = "Online";
                            if (hintEl) hintEl.textContent = '';
                        } else {
                            wifiIndicator.className = 'wifi-offline';
                            wifiStatusText.textContent = "Offline";
                            if (hintEl) hintEl.textContent = '';
                        }
                    })
                    .catch(error => {
                        clearTimeout(timeoutId);
                        console.error("Error checking WiFi status:", error.name, error.message);
                        const hintEl = document.getElementById('wifiStatusHint');
                        if (wifiIndicator) wifiIndicator.className = 'wifi-offline';
                        if (wifiStatusText) wifiStatusText.textContent = 'Offline';
                        if (hintEl) hintEl.textContent = apSSID ? `Connect to access point: ${apSSID}` : '';
                    });
            }
            checkWifiStatus();
            setInterval(checkWifiStatus, 5000);

            // Handles button clicks: highlights the active button and POSTs the action to /button.
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
