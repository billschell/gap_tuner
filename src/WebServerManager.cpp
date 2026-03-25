#include "WebServerManager.h"
#include "GAPTuner.h"              // Need full definition for _gaptuner usage
#include "NetworkMgr.h"            // Need full definition for _networkMgr usage
#include "RelayTestController.h"   // Need full definition for _relayTestController usage
#include "DebugUtils.h"            // For DEBUG_PRINTLN, DEBUG_PRINTF

// Forward declaration — definition is at the end of this file.
static const char* getTestHtml();

// Stores references to all collaborating objects.
WebServerManager::WebServerManager(AsyncWebServer& srv, GAPTuner& tuner, NetworkMgr& netMgr,
                                   RelayTestController& testCtrl) :
    _server(srv), _gaptuner(tuner), _networkMgr(netMgr), _relayTestController(testCtrl) {}

// Registers all HTTP routes with the async web server.
void WebServerManager::setupRoutes() {
    DEBUG_PRINTLN("WebServerManager: Setting up routes...");
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleRootRequest(request);
    });
    _server.on("/button", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleButtonRequest(request);
    });
    _server.on("/wifi-status", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleWiFiStatusRequest(request);
    });
    _server.on("/info", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleInfoRequest(request);
    });
    // Register specific /test/* routes BEFORE /test — ESPAsyncWebServer uses prefix
    // matching, so /test would otherwise intercept all /test/* requests if registered first.
    _server.on("/test/relay", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleTestRelayRequest(request);
    });
    _server.on("/test/reset", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleTestResetRequest(request);
    });
    _server.on("/test", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleTestRootRequest(request);
    });
    _server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleSettingsRequest(request);
    });
    _server.on("/settings/save", HTTP_POST, [this](AsyncWebServerRequest *request){
        this->handleSettingsSaveRequest(request);
    });
    _server.onNotFound([this](AsyncWebServerRequest *request){
        this->handleNotFoundRequest(request);
    });
}

// Starts the HTTP server; call after setupRoutes().
void WebServerManager::begin() {
    _server.begin();
    DEBUG_PRINTLN("WebServerManager: HTTP server started.");
}

// GET / — serves the main tuner UI.
void WebServerManager::handleRootRequest(AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
}

// GET /button?id=N — translates a button ID (1–8) into a GAPTuner action and applies it.
void WebServerManager::handleButtonRequest(AsyncWebServerRequest *request) {
    String message = "Action request processed."; String actionDetails = ""; String finalResponse = ""; bool error = false;
    if (request->hasParam("id")) {
        String idStr = request->getParam("id")->value(); int buttonId = idStr.toInt();
        // GAPTuner::NUM_ACTIONS is accessible via GAPTuner.h
        if (buttonId >= 1 && buttonId <= GAPTuner::NUM_ACTIONS) { 
            actionDetails = _gaptuner.processButtonAction(buttonId, message);
            if (message.startsWith("Internal error:")) {
                error = true;
            }
        } else {
            message = "Invalid Button ID received"; error = true;
            DEBUG_PRINTF("WebServerManager: Invalid Button ID %s\n", idStr.c_str());
        }
    } else {
        message = "Missing 'id' parameter"; error = true;
        DEBUG_PRINTLN("WebServerManager: Missing 'id' parameter in button request");
    }
    finalResponse = error ? message : (message + "\n" + actionDetails);
    request->send(error ? 400 : 200, "text/plain", finalResponse);
}

// GET /wifi-status — returns "online" or "offline"; polled by the UI every 5 seconds.
// Online when connected as STA (home mode) or running as AP (field mode).
void WebServerManager::handleWiFiStatusRequest(AsyncWebServerRequest *request) {
    if (_networkMgr.isConnected() || (WiFi.getMode() & WIFI_AP)) {
        request->send(200, "text/plain", WIFI_STATUS_ONLINE);
        DEBUG_PRINTLN("WebServerManager: Sent WiFi Status: online");
    } else {
        request->send(200, "text/plain", WIFI_STATUS_OFFLINE);
        DEBUG_PRINTLN("WebServerManager: Sent WiFi Status: offline");
    }
}

// GET /info — returns device info as JSON (AP SSID, etc.) for use by the UI.
void WebServerManager::handleInfoRequest(AsyncWebServerRequest *request) {
    String json = "{\"apSSID\":\"" + _networkMgr.getAPSSID() + "\"}";
    request->send(200, "application/json", json);
}

// Catch-all 404 handler for unregistered routes.
void WebServerManager::handleNotFoundRequest(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

// GET /test — serves the manual relay test UI.
void WebServerManager::handleTestRootRequest(AsyncWebServerRequest *request) {
    request->send(200, "text/html", getTestHtml());
}

// GET /test/relay?group=C|L|N&relay=N&action=set|reset|on|off — drives a single relay in test mode.
void WebServerManager::handleTestRelayRequest(AsyncWebServerRequest *request) {
    if (!request->hasParam("group") || !request->hasParam("relay")) {
        request->send(400, "text/plain", "Missing group or relay parameter");
        return;
    }
    String group = request->getParam("group")->value();
    int relayNum = request->getParam("relay")->value().toInt();

    if (group == "C") {
        if (relayNum < 1 || relayNum > 8) { request->send(400, "text/plain", "relay out of range"); return; }
        if (!request->hasParam("action")) { request->send(400, "text/plain", "Missing action"); return; }
        bool isSet = (request->getParam("action")->value() == "set");
        _relayTestController.setC(relayNum, isSet);
        request->send(200, "text/plain", isSet ? "set" : "reset");
    }
    else if (group == "L") {
        if (relayNum < 1 || relayNum > 8) { request->send(400, "text/plain", "relay out of range"); return; }
        if (!request->hasParam("action")) { request->send(400, "text/plain", "Missing action"); return; }
        bool isSet = (request->getParam("action")->value() == "set");
        _relayTestController.setL(relayNum, isSet);
        request->send(200, "text/plain", isSet ? "set" : "reset");
    }
    else if (group == "N") {
        if (relayNum < 1 || relayNum > 7) { request->send(400, "text/plain", "relay out of range"); return; }
        if (!request->hasParam("action")) { request->send(400, "text/plain", "Missing action parameter"); return; }
        bool turnOn = (request->getParam("action")->value() == "on");
        _relayTestController.setN(relayNum, turnOn);
        request->send(200, "text/plain", turnOn ? "on" : "off");
    }
    else {
        request->send(400, "text/plain", "Unknown group");
    }
}

// GET /test/reset — resets all relays to their off/reset state.
void WebServerManager::handleTestResetRequest(AsyncWebServerRequest *request) {
    _relayTestController.resetAll();
    request->send(200, "text/plain", "ok");
}

// GET /settings — serves the settings page with the current mode and stored SSID pre-populated.
void WebServerManager::handleSettingsRequest(AsyncWebServerRequest *request) {
    String currentMode = _networkMgr.getMode();
    String storedSSID  = _networkMgr.getStoredSSID();
    String apSSID      = _networkMgr.getAPSSID();
    String apPassword  = _networkMgr.getAPPassword();
    bool   wifiFailed  = _networkMgr.wifiConnectFailed();

    String warnBanner = "";
    if (wifiFailed && storedSSID.length() > 0) {
        warnBanner = "<div class='warning'>Could not connect to &ldquo;" + storedSSID +
                     "&rdquo;. Check your WiFi credentials below.</div>";
    }

    String html = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>Settings — GAP Tuner</title>
<style>
  :root{--icom-black:#1a1a1a;--icom-dark-grey:#2c2c2c;--icom-light-grey:#e0e0e0;--icom-blue-accent:#00aaff;--icom-shadow-dark:rgba(0,0,0,0.6);--icom-shadow-light:rgba(255,255,255,0.05);--border-radius:5px;--group-spacing:25px;--button-v-spacing:10px;--button-h-spacing:8px;--button-bg:var(--icom-button-grey);--button-text:var(--icom-light-grey);--icom-button-grey:#424242;}
  body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;display:flex;justify-content:center;align-items:flex-start;min-height:100vh;background-color:var(--icom-black);margin:0;padding:20px 15px;box-sizing:border-box;-webkit-tap-highlight-color:transparent;}
  .container{background:var(--icom-dark-grey);padding:25px 30px;border-radius:var(--border-radius);box-shadow:0 0 15px var(--icom-shadow-dark), inset 0 0 5px var(--icom-shadow-light);text-align:center;width:100%;max-width:380px;box-sizing:border-box;}
  .group-title{font-size:0.9em;font-weight:600;color:var(--icom-light-grey);text-transform:uppercase;letter-spacing:0.5px;margin-bottom:15px;padding-left:5px;text-align:left;}
  button{padding:12px 10px;font-size:0.95rem;font-weight:500;text-align:center;cursor:pointer;border:1px solid var(--icom-black);border-radius:var(--border-radius);background-color:var(--button-bg);color:var(--button-text);transition:background-color 0.15s ease,transform 0.05s ease,box-shadow 0.15s ease;-webkit-tap-highlight-color:transparent;box-sizing:border-box;outline:none;box-shadow:inset 0 2px 5px var(--icom-shadow-light),inset 0 -2px 5px var(--icom-shadow-dark),0 2px 4px var(--icom-shadow-dark);text-shadow:0 1px 2px rgba(0,0,0,0.5);}
  button:focus,button:focus-visible{background-color:var(--icom-blue-accent);color:var(--icom-black);box-shadow:inset 0 1px 3px var(--icom-shadow-dark),0 1px 2px var(--icom-shadow-dark);}
  button:active{background-color:var(--icom-blue-accent);transform:translateY(1px) scale(0.98);box-shadow:inset 0 1px 3px var(--icom-shadow-dark);}
  .highlighted{background-color:var(--icom-blue-accent);color:var(--icom-black);box-shadow:inset 0 1px 3px var(--icom-shadow-dark),0 1px 2px var(--icom-shadow-dark);}
  /* Settings-specific styles */
  .page-title{color:var(--icom-light-grey);font-weight:600;font-size:1.6em;margin-top:0;margin-bottom:25px;}
  .back-link{display:block;margin-top:20px;color:var(--icom-blue-accent);text-decoration:none;font-size:0.95em;text-align:left;}
  .segment-toggle{display:flex;border:1px solid var(--icom-black);border-radius:var(--border-radius);overflow:hidden;margin-bottom:20px;}
  .segment-toggle button{flex:1;border:none;border-radius:0;}
  .segment-toggle button.active{background-color:var(--icom-blue-accent);color:var(--icom-black);}
  .section{display:none;}
  .section.visible{display:block;}
  .info-box{background:var(--icom-black);border-radius:var(--border-radius);padding:12px 14px;margin-bottom:15px;font-size:0.9em;line-height:1.6;text-align:left;}
  .info-box p{margin:0 0 10px 0;color:#aaa;}
  .info-row{margin-bottom:2px;}
  .info-label{color:#666;margin-right:6px;}
  .info-value{color:var(--icom-light-grey);font-family:monospace;}
  .warning{background:#4a2e00;border:1px solid #7a5000;border-radius:var(--border-radius);padding:10px 12px;margin-bottom:15px;font-size:0.85em;line-height:1.4;text-align:left;}
  #fieldDesc,#homeDesc{color:var(--icom-light-grey);font-size:0.9em;margin-bottom:14px;text-align:left;}
  label{display:block;margin-bottom:6px;font-size:0.9em;color:var(--icom-light-grey);text-align:left;}
  input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid var(--icom-black);border-radius:var(--border-radius);background-color:var(--icom-black);color:var(--icom-light-grey);box-shadow:inset 0 1px 3px var(--icom-shadow-dark);box-sizing:border-box;outline:none;font-size:0.95em;margin-bottom:14px;}
  input[type=text]:focus,input[type=password]:focus{border-color:var(--icom-blue-accent);}
  .password-wrapper{position:relative;margin-bottom:14px;}
  .password-wrapper input{width:100%;padding:10px 40px 10px 10px;margin-bottom:0;}
  .password-hint{font-size:0.78em;color:#666;margin-top:-10px;margin-bottom:14px;text-align:left;}
  .toggle-password{position:absolute;right:10px;top:50%;transform:translateY(-50%);background:none;border:none;padding:0;cursor:pointer;color:var(--icom-light-grey);opacity:0.6;box-shadow:none;text-shadow:none;}
  .toggle-password:hover{opacity:1;}
  #applyBtn{display:block;width:100%;margin-top:8px;background-color:var(--icom-blue-accent);color:var(--icom-black);}
  #applyBtn:disabled{opacity:0.5;cursor:default;transform:none;}
  #statusMsg{margin-top:14px;font-size:0.85em;min-height:1.4em;color:#aaa;text-align:center;}
</style>
</head>
<body>
<div class="container">
  <h1 class="page-title">GAP Tuner Settings</h1>
)rawliteral";

    html += warnBanner;

    html += R"rawliteral(
  <div class="group-title">Operating Mode</div>
  <div class="segment-toggle">
    <button id="btnField" onclick="selectMode('field')">Field</button>
    <button id="btnHome"  onclick="selectMode('home')">Home WiFi</button>
  </div>

  <div id="sectionField" class="section">
    <div class="info-box">
      <p id="fieldDesc"></p>
      <div class="info-row"><span class="info-label">Network:</span><span class="info-value">)rawliteral";
    html += apSSID;
    html += R"rawliteral(</span></div>
      <div class="info-row"><span class="info-label">Password:</span><span class="info-value">)rawliteral";
    html += apPassword;
    html += R"rawliteral(</span></div>
    </div>
  </div>

  <div id="sectionHome" class="section">
    <p id="homeDesc"></p>
    <label for="ssid">SSID</label>
    <input type="text" id="ssid" name="ssid" placeholder="WiFi network name" oninput="updateApplyBtn()" value=")rawliteral";
    html += storedSSID;
    html += R"rawliteral(">
    <label for="password">Password</label>
    <div class="password-wrapper">
      <input type="password" id="password" name="password" placeholder="Enter password" oninput="updateApplyBtn()">
      <button type="button" class="toggle-password" onclick="togglePassword()" aria-label="Show/hide password">
        <svg id="eye-icon" xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>
        </svg>
      </button>
    </div>)rawliteral";

    if (storedSSID.length() > 0) {
        html += "<p class='password-hint'>Leave password blank to keep existing.</p>";
    }

    html += R"rawliteral(
  </div>

  <button id="applyBtn" onclick="applySettings()" disabled>Apply &amp; Restart</button>
  <div id="statusMsg"></div>
  <a href="/" class="back-link">&#8592; Back</a>
</div>
<script>
const INIT_MODE = ')rawliteral";
    html += currentMode;
    html += R"rawliteral(';
const HOSTNAME = ')rawliteral";
    html += _networkMgr.getHostname();
    html += R"rawliteral(';
const AP_SSID  = ')rawliteral";
    html += apSSID;
    html += R"rawliteral(';
const AP_PASS    = ')rawliteral";
    html += apPassword;
    html += R"rawliteral(';
const STORED_SSID = ')rawliteral";
    html += storedSSID;
    html += R"rawliteral(';
let selectedMode = INIT_MODE;

const FIELD_DESC_ACTIVE  = 'Field mode is active. Your phone is connected directly to the GAP Tuner\u2019s WiFi.';
const FIELD_DESC_SWITCH  = 'Switch to field mode. Your phone will connect directly to the GAP Tuner\u2019s WiFi. No home router needed.';
const HOME_DESC_ACTIVE   = 'Home WiFi mode is active. The GAP Tuner is connected to your home network.';
const HOME_DESC_SWITCH   = 'Switch to home WiFi mode. Enter your network credentials below.';

// Enables the Apply button only when the selected mode or credentials differ from the saved values.
function updateApplyBtn() {
  const modeChanged  = selectedMode !== INIT_MODE;
  const ssid = document.getElementById('ssid');
  const pass = document.getElementById('password');
  const credsChanged = selectedMode === 'home'
    && (ssid && ssid.value !== STORED_SSID || pass && pass.value !== '');
  document.getElementById('applyBtn').disabled = !modeChanged && !credsChanged;
}

// Updates the segment toggle, visible section, and description text for the selected mode.
function selectMode(mode) {
  selectedMode = mode;
  document.getElementById('sectionField').classList.toggle('visible', mode === 'field');
  document.getElementById('sectionHome').classList.toggle('visible',  mode === 'home');
  document.getElementById('btnField').classList.toggle('active', mode === 'field');
  document.getElementById('btnHome').classList.toggle('active',  mode === 'home');
  document.getElementById('fieldDesc').textContent = mode === 'field' && INIT_MODE === 'field'
    ? FIELD_DESC_ACTIVE : FIELD_DESC_SWITCH;
  document.getElementById('homeDesc').textContent  = mode === 'home'  && INIT_MODE === 'home'
    ? HOME_DESC_ACTIVE  : HOME_DESC_SWITCH;
  updateApplyBtn();
}
selectMode(INIT_MODE);

// POSTs the selected mode and credentials to /settings/save, then shows restart instructions.
function applySettings() {
  const params = new URLSearchParams();
  params.append('mode', selectedMode);
  if (selectedMode === 'home') {
    params.append('ssid',     document.getElementById('ssid').value);
    params.append('password', document.getElementById('password').value);
  }
  const btn = document.getElementById('applyBtn');
  const msg = document.getElementById('statusMsg');
  btn.disabled = true;
  msg.textContent = 'Saving\u2026';
  // Abort after 4 seconds — the device restarts and the AP disappears, so the
  // fetch will hang rather than reject. The timeout forces the catch() to fire.
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 4000);
  fetch('/settings/save', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: params.toString(),
    signal: controller.signal
  })
  .then(r => { clearTimeout(timeoutId); return r.text().then(t => ({ ok: r.ok, text: t })); })
  .then(({ ok, text }) => {
    if (!ok) { msg.textContent = text; btn.disabled = false; return; }
    if (selectedMode === 'home') {
      let secs = 20;
      msg.innerHTML = 'Restarting. Connect your phone to your home WiFi.'
        + '<br>Redirecting to http://' + HOSTNAME + '.local in <span id="countdown">' + secs + '</span>s\u2026';
      const iv = setInterval(() => {
        secs--;
        const el = document.getElementById('countdown');
        if (el) el.textContent = secs;
        if (secs <= 0) { clearInterval(iv); window.location = 'http://' + HOSTNAME + '.local'; }
      }, 1000);
    } else {
      msg.innerHTML = 'Restarting. Connect your phone to WiFi network &ldquo;' + AP_SSID
        + '&rdquo; (password: ' + AP_PASS + '), then visit '
        + '<a href="http://192.168.4.1" style="color:var(--icom-blue-accent)">192.168.4.1</a>';
    }
  })
  .catch(() => {
    clearTimeout(timeoutId);
    // A network error or timeout here means the device restarted — treat it as success.
    if (selectedMode === 'home') {
      let secs = 20;
      msg.innerHTML = 'Restarting. Connect your phone to your home WiFi.'
        + '<br>Redirecting to http://' + HOSTNAME + '.local in <span id="countdown">' + secs + '</span>s\u2026';
      const iv = setInterval(() => {
        secs--;
        const el = document.getElementById('countdown');
        if (el) el.textContent = secs;
        if (secs <= 0) { clearInterval(iv); window.location = 'http://' + HOSTNAME + '.local'; }
      }, 1000);
    } else {
      msg.innerHTML = 'Restarting. Connect your phone to WiFi network &ldquo;' + AP_SSID
        + '&rdquo; (password: ' + AP_PASS + '), then visit '
        + '<a href="http://192.168.4.1" style="color:var(--icom-blue-accent)">192.168.4.1</a>';
    }
  });
}

// Toggles the password input between visible text and masked password, updating the eye icon.
function togglePassword() {
  const inp = document.getElementById('password');
  const ico = document.getElementById('eye-icon');
  if (inp.type === 'password') {
    inp.type = 'text';
    ico.innerHTML = '<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line>';
  } else {
    inp.type = 'password';
    ico.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
  }
}
</script>
</body>
</html>
)rawliteral";

    request->send(200, "text/html", html);
}

// POST /settings/save — persists mode and (optionally) new WiFi credentials, then restarts.
void WebServerManager::handleSettingsSaveRequest(AsyncWebServerRequest *request) {
    String mode = "";
    if (request->hasParam("mode", true)) {
        mode = request->getParam("mode", true)->value();
    }
    if (mode != "field" && mode != "home") {
        request->send(400, "text/plain", "Invalid mode.");
        return;
    }

    if (mode == "home") {
        String ssid = "";
        String password = "";
        if (request->hasParam("ssid", true)) {
            ssid = request->getParam("ssid", true)->value();
        }
        if (request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        }

        bool hasStoredCreds = (_networkMgr.getStoredSSID().length() > 0);

        if (ssid.length() > 0) {
            // New SSID provided — save full credentials
            if (!_networkMgr.saveCredentials(ssid.c_str(), password.c_str())) {
                request->send(500, "text/plain", "Failed to save WiFi credentials.");
                return;
            }
        } else if (!hasStoredCreds) {
            request->send(400, "text/plain", "SSID is required.");
            return;
        }
        // else: SSID blank + creds already stored → keep existing, nothing to save
    }

    _networkMgr.setMode(mode.c_str());
    request->send(200, "text/plain", "Settings saved. Restarting\u2026");
    DEBUG_PRINTF("WebServerManager: Mode set to '%s', restarting.\n", mode.c_str());
    delay(1500);
    ESP.restart();
}

// =============================================================================
// Test Mode Page HTML
// =============================================================================

static const char test_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>GAP Tuner — Test Mode</title>
    <style>
        :root{--icom-black:#1a1a1a;--icom-dark-grey:#2c2c2c;--icom-light-grey:#e0e0e0;--icom-blue-accent:#00aaff;--icom-shadow-dark:rgba(0,0,0,0.6);--icom-shadow-light:rgba(255,255,255,0.05);--border-radius:5px;--group-spacing:25px;--button-v-spacing:10px;--button-h-spacing:8px;--button-bg:var(--icom-button-grey);--button-text:var(--icom-light-grey);--icom-button-grey:#424242;}
        body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;display:flex;justify-content:center;align-items:flex-start;min-height:100vh;background-color:var(--icom-black);margin:0;padding:20px 15px;box-sizing:border-box;-webkit-tap-highlight-color:transparent;}
        .container{background:var(--icom-dark-grey);padding:25px 30px;border-radius:var(--border-radius);box-shadow:0 0 15px var(--icom-shadow-dark), inset 0 0 5px var(--icom-shadow-light);text-align:center;width:100%;max-width:380px;box-sizing:border-box;}
        h1{color:var(--icom-light-grey);margin-top:0;margin-bottom:5px;font-weight:600;font-size:1.6em;}
        .page-subtitle{font-size:0.8em;color:#888;margin-bottom:25px;text-transform:uppercase;letter-spacing:1px;}
        .button-group{margin-bottom:var(--group-spacing);text-align:left;}
        .button-group:last-child{margin-bottom:15px;}
        .group-title{font-size:0.9em;font-weight:600;color:var(--icom-light-grey);text-transform:uppercase;letter-spacing:0.5px;margin-bottom:15px;padding-left:5px;}
        button{padding:12px 10px;font-size:0.95rem;font-weight:500;text-align:center;cursor:pointer;border:1px solid var(--icom-black);border-radius:var(--border-radius);background-color:var(--button-bg);color:var(--button-text);transition:background-color 0.15s ease,transform 0.05s ease,box-shadow 0.15s ease;-webkit-tap-highlight-color:transparent;box-sizing:border-box;outline:none;box-shadow:inset 0 2px 5px var(--icom-shadow-light),inset 0 -2px 5px var(--icom-shadow-dark),0 2px 4px var(--icom-shadow-dark);text-shadow:0 1px 2px rgba(0,0,0,0.5);}
        button:focus,button:focus-visible{outline:none;}
        button:active{transform:translateY(1px) scale(0.98);box-shadow:inset 0 1px 3px var(--icom-shadow-dark);}
        .highlighted{background-color:var(--icom-blue-accent);color:var(--icom-black);box-shadow:inset 0 1px 3px var(--icom-shadow-dark),0 1px 2px var(--icom-shadow-dark);}
        .pulsing{background-color:var(--icom-blue-accent);color:var(--icom-black);box-shadow:inset 0 1px 3px var(--icom-shadow-dark),0 1px 2px var(--icom-shadow-dark);pointer-events:none;}
        .button-row{display:flex;gap:var(--button-h-spacing);justify-content:space-between;margin-bottom:var(--button-v-spacing);}
        .button-row:last-child{margin-bottom:0;}
        .button-row button{flex:1;}
        .button-stack button{display:block;width:100%;margin-bottom:var(--button-v-spacing);}
        .button-stack button:last-child{margin-bottom:0;}
        .reset-all-btn{background-color:#5a1a1a;border-color:#3a0a0a;}
        .reset-all-btn:hover{background-color:#6e2020;}
        .reset-all-btn:active{background-color:#dc3545;transform:translateY(1px) scale(0.98);}
        .status{margin-top:25px;font-size:0.85em;color:var(--icom-light-grey);height:8em;overflow-y:auto;line-height:1.4;text-align:left;background-color:var(--icom-black);padding:8px 12px;border-radius:5px;}
    </style>
</head>
<body>
    <div class="container">
        <div class="button-group">
            <div class="button-stack">
                <button onclick="window.location='/'">&#8592; Back to Main</button>
            </div>
        </div>
        <h1>GAP Antenna Tuner</h1>
        <p class="page-subtitle">Test Mode</p>

        <div class="button-group">
            <h3 class="group-title">Capacitance Relays (C)</h3>
            <div class="button-row">
                <button data-group="C" data-relay="1">C1</button>
                <button data-group="C" data-relay="2">C2</button>
                <button data-group="C" data-relay="3">C3</button>
                <button data-group="C" data-relay="4">C4</button>
            </div>
            <div class="button-row">
                <button data-group="C" data-relay="5">C5</button>
                <button data-group="C" data-relay="6">C6</button>
                <button data-group="C" data-relay="7">C7</button>
                <button data-group="C" data-relay="8">C8</button>
            </div>
        </div>

        <div class="button-group">
            <h3 class="group-title">Inductance Relays (L)</h3>
            <div class="button-row">
                <button data-group="L" data-relay="1">L1</button>
                <button data-group="L" data-relay="2">L2</button>
                <button data-group="L" data-relay="3">L3</button>
                <button data-group="L" data-relay="4">L4</button>
            </div>
            <div class="button-row">
                <button data-group="L" data-relay="5">L5</button>
                <button data-group="L" data-relay="6">L6</button>
                <button data-group="L" data-relay="7">L7</button>
                <button data-group="L" data-relay="8">L8</button>
            </div>
        </div>

        <div class="button-group">
            <h3 class="group-title">Non-Latching Relays (N) — N4: Toggle | others: Pulse</h3>
            <div class="button-row">
                <button data-group="N" data-relay="1">N1</button>
                <button data-group="N" data-relay="2">N2</button>
                <button data-group="N" data-relay="3">N3</button>
                <button data-group="N" data-relay="4" data-mode="toggle">N4</button>
            </div>
            <div class="button-row">
                <button data-group="N" data-relay="5">N5</button>
                <button data-group="N" data-relay="6">N6</button>
                <button data-group="N" data-relay="7">N7</button>
            </div>
        </div>

        <div class="button-group">
            <div class="button-stack">
                <button id="resetAllBtn" class="reset-all-btn">Reset All Relays</button>
            </div>
        </div>

        <div class="status" id="statusMessage">Test mode active. All relays in RESET/OFF state.</div>
    </div>
    <script>
        const statusLog = document.getElementById('statusMessage');

        // Appends a new text line to the status log and scrolls to the bottom.
        function log(text) {
            const line = document.createElement('div');
            line.textContent = text;
            statusLog.appendChild(line);
            statusLog.scrollTop = statusLog.scrollHeight;
        }

        // Calls /test/reset and clears all button highlight and pulse states in the UI.
        function doReset() {
            return fetch('/test/reset')
                .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); })
                .then(() => {
                    document.querySelectorAll('button[data-group="C"], button[data-group="L"]')
                        .forEach(b => b.classList.remove('highlighted'));
                    document.querySelectorAll('button[data-group="N"][data-mode="toggle"]')
                        .forEach(b => b.classList.remove('highlighted'));
                    document.querySelectorAll('button[data-group="N"]:not([data-mode="toggle"])')
                        .forEach(b => b.classList.remove('pulsing'));
                });
        }

        // Reset all relays on page load so hardware and UI always start in sync.
        doReset()
            .then(() => log('Ready — all relays RESET.'))
            .catch(err => log('Reset on load failed: ' + err.message));

        // C and L relay buttons — client decides the action based on current highlight state.
        document.querySelectorAll('button[data-group="C"], button[data-group="L"]').forEach(btn => {
            btn.addEventListener('click', () => {
                if (btn.dataset.inflight) return;
                const group  = btn.dataset.group;
                const relay  = btn.dataset.relay;
                const action = btn.classList.contains('highlighted') ? 'reset' : 'set';
                btn.dataset.inflight = '1';
                fetch('/test/relay?group=' + group + '&relay=' + relay + '&action=' + action)
                    .then(r => {
                        if (!r.ok) throw new Error('HTTP ' + r.status);
                        return r.text();
                    })
                    .then(resp => {
                        const isSet = resp.trim() === 'set';
                        btn.classList.toggle('highlighted', isSet);
                        log(group + relay + ': ' + (isSet ? 'SET' : 'RESET'));
                    })
                    .catch(err => log(group + relay + ' error: ' + err.message))
                    .finally(() => delete btn.dataset.inflight);
            });
        });

        // N4 toggle button — behaves like C/L relays (persistent ON/OFF state).
        document.querySelectorAll('button[data-group="N"][data-mode="toggle"]').forEach(btn => {
            btn.addEventListener('click', () => {
                if (btn.dataset.inflight) return;
                const relay  = btn.dataset.relay;
                const action = btn.classList.contains('highlighted') ? 'off' : 'on';
                btn.dataset.inflight = '1';
                fetch('/test/relay?group=N&relay=' + relay + '&action=' + action)
                    .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); return r.text(); })
                    .then(resp => {
                        const isOn = resp.trim() === 'on';
                        btn.classList.toggle('highlighted', isOn);
                        log('N' + relay + ': ' + (isOn ? 'ON' : 'OFF'));
                    })
                    .catch(err => log('N' + relay + ' error: ' + err.message))
                    .finally(() => delete btn.dataset.inflight);
            });
        });

        // N relay buttons (non-toggle) — 1-second pulse.
        document.querySelectorAll('button[data-group="N"]:not([data-mode="toggle"])').forEach(btn => {
            btn.addEventListener('click', () => {
                if (btn.classList.contains('pulsing')) return;
                const relay = btn.dataset.relay;
                btn.classList.add('pulsing');
                log('N' + relay + ': ON');
                fetch('/test/relay?group=N&relay=' + relay + '&action=on')
                    .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); })
                    .then(() => new Promise(resolve => setTimeout(resolve, 500))) // Wait 500ms before sending OFF command to ensure minimum pulse duration
                    .then(() => fetch('/test/relay?group=N&relay=' + relay + '&action=off'))
                    .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); })
                    .then(() => { btn.classList.remove('pulsing'); log('N' + relay + ': OFF'); })
                    .catch(err => { btn.classList.remove('pulsing'); log('N' + relay + ' error: ' + err.message); });
            });
        });

        // Reset All button.
        document.getElementById('resetAllBtn').addEventListener('click', () => {
            log('Resetting all relays...');
            doReset()
                .then(() => log('All relays RESET.'))
                .catch(err => log('Reset error: ' + err.message));
        });
    </script>
</body>
</html>
)rawliteral";

static const char* getTestHtml() { return test_html; }
