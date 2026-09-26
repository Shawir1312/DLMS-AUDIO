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
#include "tft_display.h"

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
        doc["ch1_peak"] = round(vu.ch1_peak_db * 10.0f) / 10.0f;
        doc["ch1_rms"]  = round(vu.ch1_rms_db * 10.0f) / 10.0f;
        doc["ch2_peak"] = round(vu.ch2_peak_db * 10.0f) / 10.0f;
        doc["ch2_rms"]  = round(vu.ch2_rms_db * 10.0f) / 10.0f;
        // Legacy mirrors
        doc["sub_peak"] = doc["ch1_peak"];
        doc["sub_rms"]  = doc["ch1_rms"];
        doc["mid_peak"] = doc["ch2_peak"];
        doc["mid_rms"]  = doc["ch2_rms"];
        doc["out_peak"] = round(vu.out_peak_db * 10.0f) / 10.0f;
        doc["out_rms"]  = round(vu.out_rms_db * 10.0f) / 10.0f;
        doc["limiter_gr"] = round(vu.limiter_gr_db * 10.0f) / 10.0f;
        doc["ch1_clip"] = vu.ch1_clip;
        doc["ch2_clip"] = vu.ch2_clip;
        doc["sub_clip"] = vu.ch1_clip;
        doc["mid_clip"] = vu.ch2_clip;
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

        doc["xover_freq"] = cfg.xover_freq;
        doc["xover_slope"] = cfg.xover_slope;
        doc["xover_linked"] = cfg.xover_linked;

        // Channel 1 (DAC 1 / Left)
        JsonObject ch1 = doc["ch1"].to<JsonObject>();
        ch1["gain_db"] = cfg.ch1.gain_db;
        ch1["mute"] = cfg.ch1.mute;
        ch1["polarity_inverted"] = cfg.ch1.polarity_inverted;
        ch1["delay_ms"] = cfg.ch1.delay_ms;
        JsonObject ch1_hpf = ch1["hpf"].to<JsonObject>();
        ch1_hpf["enabled"] = cfg.ch1.hpf.enabled;
        ch1_hpf["freq"] = cfg.ch1.hpf.freq;
        ch1_hpf["slope"] = cfg.ch1.hpf.slope;
        JsonObject ch1_lpf = ch1["lpf"].to<JsonObject>();
        ch1_lpf["enabled"] = cfg.ch1.lpf.enabled;
        ch1_lpf["freq"] = cfg.ch1.lpf.freq;
        ch1_lpf["slope"] = cfg.ch1.lpf.slope;
        JsonObject ch1_lim = ch1["limiter"].to<JsonObject>();
        ch1_lim["enabled"] = cfg.ch1.limiter.enabled;
        ch1_lim["threshold_db"] = cfg.ch1.limiter.threshold_db;
        ch1_lim["attack_ms"] = cfg.ch1.limiter.attack_ms;
        ch1_lim["release_ms"] = cfg.ch1.limiter.release_ms;
        JsonArray ch1_peq = ch1["peq"].to<JsonArray>();
        for (int i = 0; i < 3; i++) {
            JsonObject b = ch1_peq.add<JsonObject>();
            b["enabled"] = cfg.ch1.peq[i].enabled;
            b["type"] = cfg.ch1.peq[i].type;
            b["freq"] = cfg.ch1.peq[i].freq;
            b["gain_db"] = cfg.ch1.peq[i].gain_db;
            b["q"] = cfg.ch1.peq[i].q;
        }

        // Channel 2 (DAC 2 / Right)
        JsonObject ch2 = doc["ch2"].to<JsonObject>();
        ch2["gain_db"] = cfg.ch2.gain_db;
        ch2["mute"] = cfg.ch2.mute;
        ch2["polarity_inverted"] = cfg.ch2.polarity_inverted;
        ch2["delay_ms"] = cfg.ch2.delay_ms;
        JsonObject ch2_hpf = ch2["hpf"].to<JsonObject>();
        ch2_hpf["enabled"] = cfg.ch2.hpf.enabled;
        ch2_hpf["freq"] = cfg.ch2.hpf.freq;
        ch2_hpf["slope"] = cfg.ch2.hpf.slope;
        JsonObject ch2_lpf = ch2["lpf"].to<JsonObject>();
        ch2_lpf["enabled"] = cfg.ch2.lpf.enabled;
        ch2_lpf["freq"] = cfg.ch2.lpf.freq;
        ch2_lpf["slope"] = cfg.ch2.lpf.slope;
        JsonObject ch2_lim = ch2["limiter"].to<JsonObject>();
        ch2_lim["enabled"] = cfg.ch2.limiter.enabled;
        ch2_lim["threshold_db"] = cfg.ch2.limiter.threshold_db;
        ch2_lim["attack_ms"] = cfg.ch2.limiter.attack_ms;
        ch2_lim["release_ms"] = cfg.ch2.limiter.release_ms;
        JsonArray ch2_peq = ch2["peq"].to<JsonArray>();
        for (int i = 0; i < 3; i++) {
            JsonObject b = ch2_peq.add<JsonObject>();
            b["enabled"] = cfg.ch2.peq[i].enabled;
            b["type"] = cfg.ch2.peq[i].type;
            b["freq"] = cfg.ch2.peq[i].freq;
            b["gain_db"] = cfg.ch2.peq[i].gain_db;
            b["q"] = cfg.ch2.peq[i].q;
        }

        // Backward-compatibility aliases
        doc["sub"] = ch1;
        doc["mid"] = ch2;
        doc["hpf"] = ch1_hpf;
        doc["lpf"] = ch1_lpf;

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

        if (doc["xover_freq"].is<float>()) {
            cfg.xover_freq = doc["xover_freq"];
        }
        if (doc["xover_slope"].is<uint8_t>()) {
            cfg.xover_slope = doc["xover_slope"];
        }
        if (doc["xover_linked"].is<bool>()) {
            cfg.xover_linked = doc["xover_linked"];
        }

        // Helper lambda to parse a ChannelConfig from JSON
        auto parseChannel = [](JsonObject chObj, ChannelConfig& ch) {
            if (chObj["gain_db"].is<float>()) ch.gain_db = chObj["gain_db"];
            if (chObj["mute"].is<bool>()) ch.mute = chObj["mute"];
            if (chObj["polarity_inverted"].is<bool>()) ch.polarity_inverted = chObj["polarity_inverted"];
            if (chObj["delay_ms"].is<float>()) ch.delay_ms = chObj["delay_ms"];

            if (chObj["hpf"].is<JsonObject>()) {
                JsonObject h = chObj["hpf"];
                if (h["enabled"].is<bool>()) ch.hpf.enabled = h["enabled"];
                if (h["freq"].is<float>()) ch.hpf.freq = h["freq"];
                if (h["slope"].is<uint8_t>()) ch.hpf.slope = h["slope"];
            }
            if (chObj["lpf"].is<JsonObject>()) {
                JsonObject l = chObj["lpf"];
                if (l["enabled"].is<bool>()) ch.lpf.enabled = l["enabled"];
                if (l["freq"].is<float>()) ch.lpf.freq = l["freq"];
                if (l["slope"].is<uint8_t>()) ch.lpf.slope = l["slope"];
            }
            if (chObj["limiter"].is<JsonObject>()) {
                JsonObject lim = chObj["limiter"];
                if (lim["enabled"].is<bool>()) ch.limiter.enabled = lim["enabled"];
                if (lim["threshold_db"].is<float>()) ch.limiter.threshold_db = lim["threshold_db"];
                if (lim["attack_ms"].is<float>()) ch.limiter.attack_ms = lim["attack_ms"];
                if (lim["release_ms"].is<float>()) ch.limiter.release_ms = lim["release_ms"];
            }
            if (chObj["peq"].is<JsonArray>()) {
                JsonArray peq = chObj["peq"];
                for (size_t i = 0; i < peq.size() && i < 3; i++) {
                    JsonObject band = peq[i];
                    if (band["enabled"].is<bool>()) ch.peq[i].enabled = band["enabled"];
                    if (band["type"].is<uint8_t>()) ch.peq[i].type = band["type"];
                    if (band["freq"].is<float>()) ch.peq[i].freq = band["freq"];
                    if (band["gain_db"].is<float>()) ch.peq[i].gain_db = band["gain_db"];
                    if (band["q"].is<float>()) ch.peq[i].q = band["q"];
                }
            }
        };

        // Parse CH1 (or legacy 'sub')
        if (doc["ch1"].is<JsonObject>()) {
            parseChannel(doc["ch1"].as<JsonObject>(), cfg.ch1);
        } else if (doc["sub"].is<JsonObject>()) {
            parseChannel(doc["sub"].as<JsonObject>(), cfg.ch1);
        }

        // Parse CH2 (or legacy 'mid')
        if (doc["ch2"].is<JsonObject>()) {
            parseChannel(doc["ch2"].as<JsonObject>(), cfg.ch2);
        } else if (doc["mid"].is<JsonObject>()) {
            parseChannel(doc["mid"].as<JsonObject>(), cfg.ch2);
        }

        // Handle Crossover link if enabled
        if (cfg.xover_linked) {
            cfg.ch1.lpf.enabled = true;
            cfg.ch1.lpf.freq = cfg.xover_freq;
            cfg.ch1.lpf.slope = cfg.xover_slope;
            cfg.ch2.hpf.enabled = true;
            cfg.ch2.hpf.freq = cfg.xover_freq;
            cfg.ch2.hpf.slope = cfg.xover_slope;
        }

        // Legacy HPF / LPF fallbacks
        if (doc["hpf"].is<JsonObject>() && !doc["ch1"].is<JsonObject>()) {
            JsonObject hpf = doc["hpf"];
            if (hpf["enabled"].is<bool>()) cfg.ch1.hpf.enabled = hpf["enabled"];
            if (hpf["freq"].is<float>()) cfg.ch1.hpf.freq = hpf["freq"];
            if (hpf["slope"].is<uint8_t>()) cfg.ch1.hpf.slope = hpf["slope"];
        }
        if (doc["lpf"].is<JsonObject>() && !doc["ch1"].is<JsonObject>()) {
            JsonObject lpf = doc["lpf"];
            if (lpf["enabled"].is<bool>()) cfg.ch1.lpf.enabled = lpf["enabled"];
            if (lpf["freq"].is<float>()) cfg.ch1.lpf.freq = lpf["freq"];
            if (lpf["slope"].is<uint8_t>()) cfg.ch1.lpf.slope = lpf["slope"];
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

    // 12. POST Virtual Rotary Encoder Control
    server.on("/api/encoder", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, server.arg("plain"))) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        const char* act = doc["action"] | "";
        int delta = doc["delta"] | 0;
        if (strcmp(act, "left") == 0) {
            tftDisplay.handleEncoder(-1, false, false);
        } else if (strcmp(act, "right") == 0) {
            tftDisplay.handleEncoder(1, false, false);
        } else if (strcmp(act, "fast_left") == 0) {
            tftDisplay.handleEncoder(-5, false, false);
        } else if (strcmp(act, "fast_right") == 0) {
            tftDisplay.handleEncoder(5, false, false);
        } else if (strcmp(act, "click") == 0) {
            tftDisplay.handleEncoder(0, true, false);
        } else if (strcmp(act, "long") == 0) {
            tftDisplay.handleEncoder(0, false, true);
        } else if (delta != 0) {
            tftDisplay.handleEncoder(delta, false, false);
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
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
