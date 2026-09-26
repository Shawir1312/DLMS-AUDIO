#include "web_server_dsp.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "web_ui.h"
#include "dsp_engine.h"
#include "bt_audio.h"
#include "presets_manager.h"

WebServerDsp webServerDsp;
static WebServer server(WEB_SERVER_PORT);
static DNSServer dnsServer;

WebServerDsp::WebServerDsp() : _isApMode(false) {}

bool WebServerDsp::begin() {
    bool sta_connected = false;

    // 1. Try Station mode if credentials provided
    if (strlen(WIFI_STA_SSID) > 0) {
        Serial.printf("[WiFi] Connecting to \"%s\"...\n", WIFI_STA_SSID);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);

        unsigned long start_attempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start_attempt < 8000) {
            delay(250);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            sta_connected = true;
            _isApMode = false;
            Serial.printf("[WiFi] Connected! IP Address: %s\n", WiFi.localIP().toString().c_str());
        }
    }

    // 2. Fallback to SoftAP Mode
    if (!sta_connected) {
        _isApMode = true;
        WiFi.mode(WIFI_AP);
        delay(50);

        // Explicitly configure Static IP & Subnet for reliable DHCP server pool
        IPAddress apIP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.softAPConfig(apIP, gateway, subnet);

        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
                Serial.println("[WiFi-AP] Phone / Device CONNECTED to SNET-AUDIO!");
            } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
                Serial.println("[WiFi-AP] Phone / Device DISCONNECTED!");
            } else if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) {
                Serial.println("[WiFi-AP] DHCP IP Assigned to phone!");
            }
        });

        bool ap_ok = false;
        if (strlen(WIFI_FALLBACK_AP_PASS) >= 8) {
            ap_ok = WiFi.softAP(WIFI_FALLBACK_AP_SSID, WIFI_FALLBACK_AP_PASS, 1, 0, 4);
        } else {
            // Open Access Point (No password, instant connection)
            ap_ok = WiFi.softAP(WIFI_FALLBACK_AP_SSID, NULL, 1, 0, 4);
        }
        delay(100);

        // Start Captive Portal DNS server (redirects all queries to 192.168.4.1)
        dnsServer.start(53, "*", apIP);

        Serial.printf("[WiFi] Access Point started: %s\n", ap_ok ? "OK" : "FAILED");
        Serial.printf("[WiFi] SSID: %s | PASS: %s\n", WIFI_FALLBACK_AP_SSID, 
                      strlen(WIFI_FALLBACK_AP_PASS) > 0 ? WIFI_FALLBACK_AP_PASS : "(OPEN / NO PASSWORD)");
        Serial.printf("[WiFi] Web Interface URL: http://%s\n", WiFi.softAPIP().toString().c_str());
    }

    // 3. Register HTTP Routes
    setupRoutes();

    // 4. Start Web Server
    server.begin();
    Serial.println("[Web] HTTP Server started on port 80");
    return true;
}

void WebServerDsp::loop() {
    if (_isApMode) {
        dnsServer.processNextRequest();
    }
    server.handleClient();
}

bool WebServerDsp::isWifiConnected() const {
    return _isApMode ? true : (WiFi.status() == WL_CONNECTED);
}

String WebServerDsp::getIpAddress() const {
    return _isApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

void WebServerDsp::setupRoutes() {
    // 1. Web UI Dashboard
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", INDEX_HTML);
    });

    // 2. System Status API
    server.on("/api/status", HTTP_GET, []() {
        JsonDocument doc;
        VuMeterData vu = dspEngine.getVuMeterData();

        doc["wifi"] = webServerDsp.isWifiConnected();
        doc["ip"] = webServerDsp.getIpAddress();
        doc["bluetooth"] = btAudio.isConnected();
        doc["streaming"] = btAudio.isStreaming();
        doc["bt_state"] = btAudio.getStateString();
        doc["sampleRate"] = btAudio.getSampleRate();
        doc["inputPeak"] = round(vu.in_peak_db * 10.0f) / 10.0f;
        doc["outputPeak"] = round(vu.out_peak_db * 10.0f) / 10.0f;
        doc["clip"] = vu.clip;

        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    // 3. High-frequency Meter Telemetry API (~50-100ms polling)
    server.on("/api/meter", HTTP_GET, []() {
        JsonDocument doc;
        VuMeterData vu = dspEngine.getVuMeterData();

        doc["in_peak"] = round(vu.in_peak_db * 10.0f) / 10.0f;
        doc["in_rms"]  = round(vu.in_rms_db * 10.0f) / 10.0f;
        doc["sub_peak"] = round(vu.sub_peak_db * 10.0f) / 10.0f;
        doc["sub_rms"]  = round(vu.sub_rms_db * 10.0f) / 10.0f;
        doc["mid_peak"] = round(vu.mid_peak_db * 10.0f) / 10.0f;
        doc["mid_rms"]  = round(vu.mid_rms_db * 10.0f) / 10.0f;
        doc["out_peak"] = round(vu.out_peak_db * 10.0f) / 10.0f;
        doc["out_rms"]  = round(vu.out_rms_db * 10.0f) / 10.0f;
        doc["limiter_gr"] = round(vu.limiter_gr_db * 10.0f) / 10.0f;
        doc["sub_clip"] = vu.sub_clip;
        doc["mid_clip"] = vu.mid_clip;
        doc["clip"] = vu.clip;
        doc["bt_state"] = btAudio.getStateString();
        doc["sampleRate"] = btAudio.getSampleRate();
        doc["ip"] = webServerDsp.getIpAddress();

        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    // 4. GET DSP Configuration
    server.on("/api/dsp", HTTP_GET, []() {
        DspConfig cfg = dspEngine.getConfig();
        JsonDocument doc;

        doc["master_gain_db"] = cfg.master_gain_db;
        doc["mute"] = cfg.mute;
        doc["polarity_inverted"] = cfg.polarity_inverted;

        // 2-Way Crossover
        doc["xover_freq"] = cfg.xover_freq;
        doc["xover_slope"] = cfg.xover_slope;
        doc["xover_linked"] = cfg.xover_linked;

        // Sub Channel
        JsonObject sub = doc["sub"].to<JsonObject>();
        sub["gain_db"] = cfg.sub.gain_db;
        sub["mute"] = cfg.sub.mute;
        sub["polarity_inverted"] = cfg.sub.polarity_inverted;
        sub["delay_ms"] = cfg.sub.delay_ms;

        // Mid Channel
        JsonObject mid = doc["mid"].to<JsonObject>();
        mid["gain_db"] = cfg.mid.gain_db;
        mid["mute"] = cfg.mid.mute;
        mid["polarity_inverted"] = cfg.mid.polarity_inverted;
        mid["delay_ms"] = cfg.mid.delay_ms;

        // HPF
        JsonObject hpf = doc["hpf"].to<JsonObject>();
        hpf["enabled"] = cfg.sub.hpf.enabled;
        hpf["freq"] = cfg.sub.hpf.freq;
        hpf["slope"] = cfg.sub.hpf.slope;

        // LPF
        JsonObject lpf = doc["lpf"].to<JsonObject>();
        lpf["enabled"] = cfg.sub.lpf.enabled;
        lpf["freq"] = cfg.xover_freq;
        lpf["slope"] = cfg.xover_slope;

        // 5-band PEQ
        JsonArray peq = doc["peq"].to<JsonArray>();
        for (int i = 0; i < PEQ_BAND_COUNT; i++) {
            JsonObject band = peq.add<JsonObject>();
            band["enabled"] = cfg.peq[i].enabled;
            band["type"] = cfg.peq[i].type;
            band["freq"] = cfg.peq[i].freq;
            band["gain_db"] = cfg.peq[i].gain_db;
            band["q"] = cfg.peq[i].q;
        }

        // Delay
        JsonObject delayObj = doc["delay"].to<JsonObject>();
        delayObj["delay_ms"] = cfg.sub.delay_ms;

        // Limiter
        JsonObject lim = doc["limiter"].to<JsonObject>();
        lim["enabled"] = cfg.sub.limiter.enabled;
        lim["threshold_db"] = cfg.sub.limiter.threshold_db;
        lim["attack_ms"] = cfg.sub.limiter.attack_ms;
        lim["release_ms"] = cfg.sub.limiter.release_ms;

        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    // 5. POST DSP Configuration (Real-time update without restart)
    server.on("/api/dsp", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, server.arg("plain"));
        if (err) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        DspConfig cfg = dspEngine.getConfig();

        if (doc["master_gain_db"].is<float>()) {
            cfg.master_gain_db = doc["master_gain_db"];
        }
        if (doc["mute"].is<bool>()) {
            cfg.mute = doc["mute"];
        }
        if (doc["polarity_inverted"].is<bool>()) {
            cfg.polarity_inverted = doc["polarity_inverted"];
        }

        // 2-Way Crossover updates
        if (doc["xover_freq"].is<float>()) {
            cfg.xover_freq = doc["xover_freq"];
            cfg.sub.lpf.freq = cfg.xover_freq;
            cfg.mid.hpf.freq = cfg.xover_freq;
            cfg.lpf.freq = cfg.xover_freq;
        }
        if (doc["xover_slope"].is<uint8_t>()) {
            cfg.xover_slope = doc["xover_slope"];
            cfg.sub.lpf.slope = cfg.xover_slope;
            cfg.mid.hpf.slope = cfg.xover_slope;
            cfg.lpf.slope = cfg.xover_slope;
        }

        // Sub channel
        if (doc["sub"].is<JsonObject>()) {
            JsonObject sub = doc["sub"];
            if (sub["gain_db"].is<float>()) cfg.sub.gain_db = sub["gain_db"];
            if (sub["mute"].is<bool>()) cfg.sub.mute = sub["mute"];
            if (sub["polarity_inverted"].is<bool>()) cfg.sub.polarity_inverted = sub["polarity_inverted"];
            if (sub["delay_ms"].is<float>()) cfg.sub.delay_ms = sub["delay_ms"];
        }

        // Mid channel
        if (doc["mid"].is<JsonObject>()) {
            JsonObject mid = doc["mid"];
            if (mid["gain_db"].is<float>()) cfg.mid.gain_db = mid["gain_db"];
            if (mid["mute"].is<bool>()) cfg.mid.mute = mid["mute"];
            if (mid["polarity_inverted"].is<bool>()) cfg.mid.polarity_inverted = mid["polarity_inverted"];
            if (mid["delay_ms"].is<float>()) cfg.mid.delay_ms = mid["delay_ms"];
        }

        // HPF (Subsonic)
        if (doc["hpf"].is<JsonObject>()) {
            JsonObject hpf = doc["hpf"];
            if (hpf["enabled"].is<bool>()) cfg.sub.hpf.enabled = hpf["enabled"];
            if (hpf["freq"].is<float>()) cfg.sub.hpf.freq = hpf["freq"];
            if (hpf["slope"].is<uint8_t>()) cfg.sub.hpf.slope = hpf["slope"];
            cfg.hpf = cfg.sub.hpf;
        }

        // LPF (Crossover)
        if (doc["lpf"].is<JsonObject>()) {
            JsonObject lpf = doc["lpf"];
            if (lpf["enabled"].is<bool>()) {
                cfg.sub.lpf.enabled = lpf["enabled"];
                cfg.mid.hpf.enabled = lpf["enabled"];
            }
            if (lpf["freq"].is<float>()) {
                cfg.xover_freq = lpf["freq"];
                cfg.sub.lpf.freq = lpf["freq"];
                cfg.mid.hpf.freq = lpf["freq"];
            }
            if (lpf["slope"].is<uint8_t>()) {
                cfg.xover_slope = lpf["slope"];
                cfg.sub.lpf.slope = lpf["slope"];
                cfg.mid.hpf.slope = lpf["slope"];
            }
            cfg.lpf = cfg.sub.lpf;
        }

        // PEQ
        if (doc["peq"].is<JsonArray>()) {
            JsonArray peq = doc["peq"];
            for (size_t i = 0; i < peq.size() && i < PEQ_BAND_COUNT; i++) {
                JsonObject band = peq[i];
                if (band["enabled"].is<bool>()) cfg.peq[i].enabled = band["enabled"];
                if (band["type"].is<uint8_t>()) cfg.peq[i].type = band["type"];
                if (band["freq"].is<float>()) cfg.peq[i].freq = band["freq"];
                if (band["gain_db"].is<float>()) cfg.peq[i].gain_db = band["gain_db"];
                if (band["q"].is<float>()) cfg.peq[i].q = band["q"];
            }
        }

        // Delay
        if (doc["delay"].is<JsonObject>()) {
            JsonObject del = doc["delay"];
            if (del["delay_ms"].is<float>()) cfg.delay.delay_ms = del["delay_ms"];
        }

        // Limiter
        if (doc["limiter"].is<JsonObject>()) {
            JsonObject lim = doc["limiter"];
            if (lim["enabled"].is<bool>()) cfg.limiter.enabled = lim["enabled"];
            if (lim["threshold_db"].is<float>()) cfg.limiter.threshold_db = lim["threshold_db"];
            if (lim["attack_ms"].is<float>()) cfg.limiter.attack_ms = lim["attack_ms"];
            if (lim["release_ms"].is<float>()) cfg.limiter.release_ms = lim["release_ms"];
        }

        // Apply safely to DSP Engine
        dspEngine.setConfig(cfg);
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // 6. GET Presets
    server.on("/api/presets", HTTP_GET, []() {
        JsonDocument doc;
        doc["active_slot"] = presetsManager.getCurrentSlot();
        JsonArray arr = doc["slots"].to<JsonArray>();
        for (int i = 1; i <= PRESET_COUNT; i++) {
            JsonObject slot = arr.add<JsonObject>();
            slot["slot"] = i;
            slot["name"] = "Preset " + String(i);
        }
        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    // 7. POST Save Preset
    server.on("/api/preset/save", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, server.arg("plain"))) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        uint8_t slot = doc["slot"] | 1;
        DspConfig cfg = dspEngine.getConfig();
        bool ok = presetsManager.savePreset(slot, cfg);
        server.send(ok ? 200 : 500, "application/json", ok ? "{\"status\":\"saved\"}" : "{\"error\":\"save_failed\"}");
    });

    // 8. POST Load Preset
    server.on("/api/preset/load", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, server.arg("plain"))) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        uint8_t slot = doc["slot"] | 1;
        DspConfig cfg;
        if (presetsManager.loadPreset(slot, cfg)) {
            dspEngine.setConfig(cfg);
            server.send(200, "application/json", "{\"status\":\"loaded\"}");
        } else {
            server.send(500, "application/json", "{\"error\":\"load_failed\"}");
        }
    });

    // 9. POST Reset Preset
    server.on("/api/preset/reset", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, server.arg("plain"))) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        uint8_t slot = doc["slot"] | 1;
        if (presetsManager.resetPreset(slot)) {
            DspConfig def = PresetsManager::getDefaultConfig();
            dspEngine.setConfig(def);
            server.send(200, "application/json", "{\"status\":\"reset_ok\"}");
        } else {
            server.send(500, "application/json", "{\"error\":\"reset_failed\"}");
        }
    });

    // 10. GET System Info
    server.on("/api/system", HTTP_GET, []() {
        JsonDocument doc;
        doc["chip_model"] = ESP.getChipModel();
        doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
        doc["free_heap"] = ESP.getFreeHeap();
        doc["min_free_heap"] = ESP.getMinFreeHeap();
        doc["uptime_sec"] = millis() / 1000;
        doc["sdk_version"] = ESP.getSdkVersion();

        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    // 11. POST System Reboot
    server.on("/api/system/reboot", HTTP_POST, []() {
        server.send(200, "application/json", "{\"status\":\"rebooting\"}");
        delay(500);
        ESP.restart();
    });

    // Captive portal probes for Android, iOS, Windows
    server.on("/generate_204", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
    server.on("/hotspot-detect.html", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
    server.on("/canonical.html", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
    server.on("/connecttest.txt", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
    server.on("/ncsi.txt", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });

    // 404 / Captive Portal Fallback Handler
    server.onNotFound([]() {
        if (webServerDsp.isApMode()) {
            server.sendHeader("Location", "http://192.168.4.1/", true);
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "Not Found");
        }
    });
}
