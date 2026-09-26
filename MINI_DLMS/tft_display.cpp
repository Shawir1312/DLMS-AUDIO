#include "tft_display.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "dsp_engine.h"
#include "presets_manager.h"
#include "web_server_dsp.h"

TftDisplay tftDisplay;

// Direct 5-pin constructor (Zero SPI crashes, 100% stable across all ESP32-S3 boards)
static Adafruit_ST7735 tft(TFT_CS_PIN, TFT_DC_PIN, TFT_MOSI_PIN, TFT_SCLK_PIN, TFT_RST_PIN);

// Color definitions (RGB565)
#define COLOR_BG         0x0000 // Black
#define COLOR_HEADER_BG  0x08A5 // Deep Navy / Slate
#define COLOR_TEXT_DIM   0x7BEF // Light Grey
#define COLOR_TEXT_BRT   0xFFFF // White
#define COLOR_ACCENT     0x07FF // Cyan
#define COLOR_GREEN      0x07E0 // Bright Green
#define COLOR_YELLOW     0xFFE0 // Bright Yellow
#define COLOR_ORANGE     0xFD20 // Orange
#define COLOR_RED        0xF800 // Bright Red
#define COLOR_BOX_BORDER 0x2124 // Dark Border
#define COLOR_SEL_BG     0x02CD // Teal Selection

static const float FREQ_STEPS[] = {
    40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f, 110.0f, 120.0f, 
    130.0f, 140.0f, 150.0f, 160.0f, 180.0f, 200.0f, 250.0f, 300.0f, 
    400.0f, 500.0f, 800.0f, 1000.0f, 1500.0f, 2000.0f, 3000.0f, 5000.0f
};
static const size_t FREQ_STEPS_COUNT = sizeof(FREQ_STEPS) / sizeof(FREQ_STEPS[0]);

#define MENU_ITEM_COUNT 11

TftDisplay::TftDisplay()
    : _currentMode(SCREEN_HOME),
      _isInitialized(false),
      _lastRenderTime(0),
      _lastUserActivityTime(0),
      _menuIndex(0),
      _inEditMode(false),
      _menuScrollOffset(0),
      _prevInBarW(0),
      _prevSubBarW(0),
      _prevMidBarW(0),
      _prevSubClip(false),
      _prevMidClip(false),
      _prevXover(-1.0f),
      _prevSubGain(-999.0f),
      _prevMidGain(-999.0f),
      _prevPreset(255)
{
}

bool TftDisplay::begin() {
    Serial.println("[TFT] Initializing 1.8\" SPI TFT (ST7735)...");

    // 1. Hardware Reset Pulse
    if (TFT_RST_PIN >= 0) {
        pinMode(TFT_RST_PIN, OUTPUT);
        digitalWrite(TFT_RST_PIN, HIGH);
        delay(10);
        digitalWrite(TFT_RST_PIN, LOW);
        delay(20);
        digitalWrite(TFT_RST_PIN, HIGH);
        delay(50);
    }

    // 2. Backlight pin (jika terhubung ke GPIO)
    if (TFT_BL_PIN >= 0) {
        pinMode(TFT_BL_PIN, OUTPUT);
        digitalWrite(TFT_BL_PIN, HIGH);
    }

    // 3. Initialize ST7735 128x160 (BlackTab / RedTab compatible)
    tft.initR(INITR_BLACKTAB);
    delay(50);
    tft.setRotation(1); // Landscape 160 x 128
    tft.fillScreen(COLOR_BG);

    _isInitialized = true;
    _lastUserActivityTime = millis();

    drawHomeScreenLayout();
    Serial.println("[TFT] Display initialized successfully (Landscape 160x128)");
    return true;
}

void TftDisplay::setScreenMode(DisplayScreenMode mode) {
    if (_currentMode == mode) return;
    _currentMode = mode;
    _inEditMode = false;
    _lastUserActivityTime = millis();

    tft.fillScreen(COLOR_BG);

    if (_currentMode == SCREEN_HOME) {
        _prevInBarW = -1;
        _prevSubBarW = -1;
        _prevMidBarW = -1;
        _prevXover = -1.0f;
        _prevSubGain = -999.0f;
        _prevMidGain = -999.0f;
        _prevPreset = 255;
        drawHomeScreenLayout();
    } else {
        drawMenuScreen();
    }
}

void TftDisplay::drawHeader(const char* title, uint16_t bg_color, uint16_t text_color) {
    tft.fillRect(0, 0, 160, 14, bg_color);
    tft.setTextSize(1);
    tft.setTextColor(text_color);
    tft.setCursor(6, 3);
    tft.print(title);
}

void TftDisplay::drawHomeScreenLayout() {
    // 1. Top Header Banner
    drawHeader("S.NET MINI DLMS 2-WAY", COLOR_HEADER_BG, COLOR_ACCENT);

    // 2. VU Meter Section Labels (Y: 18 - 58)
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);

    tft.setCursor(4, 18);
    tft.print("IN ");

    tft.setCursor(4, 30);
    tft.print("SUB");

    tft.setCursor(4, 42);
    tft.print("MID");

    // Static VU background troughs (dark grey frames)
    tft.drawRect(26, 17, 86, 9, COLOR_BOX_BORDER);
    tft.drawRect(26, 29, 86, 9, COLOR_BOX_BORDER);
    tft.drawRect(26, 41, 86, 9, COLOR_BOX_BORDER);

    // VU dB scale ticks under the bars
    tft.setTextColor(0x52AA);
    tft.setCursor(26, 52);
    tft.print("-30  -18  -12  -6   0 dB");

    // 3. Status Info Box (Y: 63 to 110)
    tft.drawRoundRect(2, 63, 156, 48, 3, COLOR_BOX_BORDER);

    // 4. Bottom Prompt Line
    tft.fillRect(0, 114, 160, 14, 0x01A3);
    tft.setTextColor(COLOR_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(8, 117);
    tft.print("[ TEKAN KNOB : MENU ]");
}

void TftDisplay::drawVuBar(int16_t x, int16_t y, int16_t w, int16_t h, float db, int16_t& prev_w) {
    // Map -40 dB ... 0 dB to 0 ... w pixels
    if (db < -40.0f) db = -40.0f;
    if (db > 0.0f)   db = 0.0f;

    float norm = (db + 40.0f) / 40.0f; // 0.0 to 1.0
    int16_t bar_w = (int16_t)(norm * (float)w);
    if (bar_w < 0) bar_w = 0;
    if (bar_w > w) bar_w = w;

    if (bar_w == prev_w) return;

    if (bar_w > prev_w) {
        // Draw new segments from prev_w to bar_w
        for (int16_t px = prev_w; px < bar_w; px++) {
            uint16_t col;
            float seg_ratio = (float)px / (float)w;
            if (seg_ratio < 0.65f) col = COLOR_GREEN;       // -40 to -14 dB
            else if (seg_ratio < 0.88f) col = COLOR_YELLOW; // -14 to -5 dB
            else col = COLOR_RED;                           // -5 to 0 dB
            tft.drawFastVLine(x + px, y, h, col);
        }
    } else {
        // Clear falling segments
        tft.fillRect(x + bar_w, y, prev_w - bar_w, h, COLOR_BG);
    }

    prev_w = bar_w;
}

void TftDisplay::updateHomeDynamicData() {
    VuMeterData vu = dspEngine.getVuMeterData();
    DspConfig cfg  = dspEngine.getConfig();

    // 1. Update VU Bars (Inner dimensions 84 x 7)
    drawVuBar(27, 18, 84, 7, vu.in_peak_db,  _prevInBarW);
    drawVuBar(27, 30, 84, 7, vu.sub_peak_db, _prevSubBarW);
    drawVuBar(27, 42, 84, 7, vu.mid_peak_db, _prevMidBarW);

    // 2. VU Numerical Readout / Clip Indicator
    tft.setTextSize(1);

    // Sub Clip / Level
    tft.setCursor(116, 30);
    if (vu.sub_clip) {
        tft.setTextColor(COLOR_RED, COLOR_BG);
        tft.print("CLIP ");
    } else {
        tft.setTextColor(COLOR_TEXT_BRT, COLOR_BG);
        char subBuf[8];
        snprintf(subBuf, sizeof(subBuf), "%+3.0fdB", vu.sub_peak_db);
        tft.print(subBuf);
    }

    // Mid Clip / Level
    tft.setCursor(116, 42);
    if (vu.mid_clip) {
        tft.setTextColor(COLOR_RED, COLOR_BG);
        tft.print("CLIP ");
    } else {
        tft.setTextColor(COLOR_TEXT_BRT, COLOR_BG);
        char midBuf[8];
        snprintf(midBuf, sizeof(midBuf), "%+3.0fdB", vu.mid_peak_db);
        tft.print(midBuf);
    }

    // 3. System Information Box lines (only redraw when changed)
    if (fabsf(cfg.xover_freq - _prevXover) > 0.5f) {
        _prevXover = cfg.xover_freq;
        tft.fillRect(6, 66, 148, 9, COLOR_BG);
        tft.setCursor(6, 66);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.printf("X-OVER: %4.0fHz (%udB LR)", cfg.xover_freq, cfg.xover_slope);
    }

    if (fabsf(cfg.sub.gain_db - _prevSubGain) > 0.2f || fabsf(cfg.mid.gain_db - _prevMidGain) > 0.2f) {
        _prevSubGain = cfg.sub.gain_db;
        _prevMidGain = cfg.mid.gain_db;
        tft.fillRect(6, 77, 148, 9, COLOR_BG);
        tft.setCursor(6, 77);
        tft.setTextColor(COLOR_TEXT_BRT, COLOR_BG);
        tft.printf("S:%+4.1fdB | M:%+4.1fdB", cfg.sub.gain_db, cfg.mid.gain_db);
    }

    uint8_t curSlot = presetsManager.getCurrentSlot();
    if (curSlot != _prevPreset) {
        _prevPreset = curSlot;
        tft.fillRect(6, 88, 148, 9, COLOR_BG);
        tft.setCursor(6, 88);
        tft.setTextColor(COLOR_YELLOW, COLOR_BG);
        tft.printf("PRESET: Slot %u [ACTIVE]", curSlot);

        tft.fillRect(6, 99, 148, 9, COLOR_BG);
        tft.setCursor(6, 99);
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.printf("IP: %s", webServerDsp.getIpAddress().c_str());
    }
}

void TftDisplay::drawMenuScreen() {
    drawHeader("DSP PARAMETER SETTINGS", COLOR_HEADER_BG, COLOR_YELLOW);

    // Visible menu rows: 6 items (Y: 18, 33, 48, 63, 78, 93)
    const uint8_t visible_count = 6;
    if (_menuIndex < _menuScrollOffset) {
        _menuScrollOffset = _menuIndex;
    } else if (_menuIndex >= _menuScrollOffset + visible_count) {
        _menuScrollOffset = _menuIndex - visible_count + 1;
    }

    DspConfig cfg = dspEngine.getConfig();

    for (uint8_t row = 0; row < visible_count; row++) {
        uint8_t item_idx = _menuScrollOffset + row;
        if (item_idx >= MENU_ITEM_COUNT) break;

        int16_t y = 17 + (row * 15);
        bool isSelected = (item_idx == _menuIndex);

        if (isSelected) {
            tft.fillRect(0, y, 160, 14, _inEditMode ? COLOR_RED : COLOR_SEL_BG);
            tft.setTextColor(COLOR_TEXT_BRT);
        } else {
            tft.fillRect(0, y, 160, 14, COLOR_BG);
            tft.setTextColor(COLOR_TEXT_DIM);
        }

        tft.setTextSize(1);
        tft.setCursor(4, y + 3);

        // Indicator
        tft.print(isSelected ? (_inEditMode ? "* " : "> ") : "  ");

        // Label & Value
        switch (item_idx) {
            case 0: // X-OVER FREQ
                tft.printf("X-OVER FREQ: %4.0f Hz", cfg.xover_freq);
                break;
            case 1: // X-OVER SLOPE
                tft.printf("SLOPE      : %2u dB LR", cfg.xover_slope);
                break;
            case 2: // SUB GAIN
                tft.printf("SUB GAIN   : %+4.1f dB", cfg.sub.gain_db);
                break;
            case 3: // MID GAIN
                tft.printf("MID GAIN   : %+4.1f dB", cfg.mid.gain_db);
                break;
            case 4: // MASTER VOL
                tft.printf("MASTER VOL : %+4.0f dB", cfg.master_gain_db);
                break;
            case 5: // SUB PHASE
                tft.printf("SUB PHASE  : %s", cfg.sub.polarity_inverted ? "INVERT 180" : "NORMAL");
                break;
            case 6: // MID PHASE
                tft.printf("MID PHASE  : %s", cfg.mid.polarity_inverted ? "INVERT 180" : "NORMAL");
                break;
            case 7: // MUTE
                tft.printf("MUTE       : %s", cfg.mute ? "MUTED [ON]" : "UNMUTED");
                break;
            case 8: // LOAD PRESET
                tft.printf("LOAD PRESET: Slot %u", presetsManager.getCurrentSlot());
                break;
            case 9: // SAVE TO NVS
                tft.printf("SAVE PRESET: [KLIK]");
                break;
            case 10: // BACK
                tft.printf("< KEMBALI KE VU METER >");
                break;
        }
    }

    // Bottom Help Banner
    tft.fillRect(0, 114, 160, 14, 0x01A3);
    tft.setTextColor(_inEditMode ? COLOR_YELLOW : COLOR_ACCENT);
    tft.setCursor(6, 117);
    if (_inEditMode) {
        tft.print("PUTAR: UBAH | TEKAN: OK");
    } else {
        tft.print("PUTAR: PILIH | TEKAN: EDIT");
    }
}

void TftDisplay::applyMenuEdit(int32_t delta) {
    DspConfig cfg = dspEngine.getConfig();

    switch (_menuIndex) {
        case 0: { // X-OVER FREQ
            int current_idx = 6; // default 100Hz
            float min_diff = 99999.0f;
            for (size_t i = 0; i < FREQ_STEPS_COUNT; i++) {
                float diff = fabsf(FREQ_STEPS[i] - cfg.xover_freq);
                if (diff < min_diff) {
                    min_diff = diff;
                    current_idx = (int)i;
                }
            }
            current_idx += delta;
            if (current_idx < 0) current_idx = 0;
            if (current_idx >= (int)FREQ_STEPS_COUNT) current_idx = (int)FREQ_STEPS_COUNT - 1;
            dspEngine.setCrossover(FREQ_STEPS[current_idx], cfg.xover_slope);
            break;
        }
        case 1: { // X-OVER SLOPE
            uint8_t slope = cfg.xover_slope;
            if (delta > 0) {
                if (slope == 12) slope = 24;
                else if (slope == 24) slope = 48;
            } else if (delta < 0) {
                if (slope == 48) slope = 24;
                else if (slope == 24) slope = 12;
            }
            dspEngine.setCrossover(cfg.xover_freq, slope);
            break;
        }
        case 2: { // SUB GAIN
            float g = cfg.sub.gain_db + (float)delta * 0.5f;
            dspEngine.setSubGain(g);
            break;
        }
        case 3: { // MID GAIN
            float g = cfg.mid.gain_db + (float)delta * 0.5f;
            dspEngine.setMidGain(g);
            break;
        }
        case 4: { // MASTER VOL
            float g = cfg.master_gain_db + (float)delta * 1.0f;
            dspEngine.setMasterGain(g);
            break;
        }
        case 5: { // SUB PHASE
            dspEngine.setSubInvert(!cfg.sub.polarity_inverted);
            break;
        }
        case 6: { // MID PHASE
            dspEngine.setMidInvert(!cfg.mid.polarity_inverted);
            break;
        }
        case 7: { // MUTE
            dspEngine.setMasterMute(!cfg.mute);
            break;
        }
        case 8: { // LOAD PRESET
            uint8_t cur = presetsManager.getCurrentSlot();
            int new_slot = (int)cur + delta;
            if (new_slot < 1) new_slot = 1;
            if (new_slot > PRESET_COUNT) new_slot = PRESET_COUNT;
            if (new_slot != cur) {
                DspConfig loadedCfg;
                if (presetsManager.loadPreset((uint8_t)new_slot, loadedCfg)) {
                    dspEngine.setConfig(loadedCfg);
                }
            }
            break;
        }
        case 9:
        case 10:
            break;
    }

    drawMenuScreen();
}

void TftDisplay::executeMenuSelect() {
    if (_menuIndex == 10) { // BACK TO HOME
        setScreenMode(SCREEN_HOME);
        return;
    }

    if (_menuIndex == 9) { // SAVE TO NVS
        DspConfig currentCfg = dspEngine.getConfig();
        presetsManager.savePreset(presetsManager.getCurrentSlot(), currentCfg);

        // Flash Confirmation Banner
        tft.fillRect(0, 114, 160, 14, COLOR_GREEN);
        tft.setTextColor(COLOR_BG);
        tft.setCursor(20, 117);
        tft.print("[ TERSIMPAN KE NVS! ]");
        delay(600);
        drawMenuScreen();
        return;
    }

    // Toggle Edit Mode for current parameter
    _inEditMode = !_inEditMode;
    drawMenuScreen();
}

void TftDisplay::handleEncoder(int32_t delta, bool clicked, bool longPressed) {
    if (!_isInitialized) return;

    _lastUserActivityTime = millis();

    // Long press always returns to Home screen immediately
    if (longPressed) {
        setScreenMode(SCREEN_HOME);
        return;
    }

    if (_currentMode == SCREEN_HOME) {
        // "TEKAN ENCODER UNTUK AKTIFKAN ROTARY ENCODER"
        if (clicked) {
            setScreenMode(SCREEN_MENU);
        }
        return;
    }

    // In Menu Screen:
    if (_inEditMode) {
        if (delta != 0) {
            applyMenuEdit(delta);
        }
        if (clicked) {
            // Confirm and lock in
            _inEditMode = false;
            drawMenuScreen();
        }
    } else {
        if (delta != 0) {
            int new_idx = _menuIndex + delta;
            if (new_idx < 0) new_idx = 0;
            if (new_idx >= MENU_ITEM_COUNT) new_idx = MENU_ITEM_COUNT - 1;
            if (new_idx != _menuIndex) {
                _menuIndex = new_idx;
                drawMenuScreen();
            }
        }
        if (clicked) {
            executeMenuSelect();
        }
    }
}

void TftDisplay::update() {
    if (!_isInitialized) return;

    unsigned long now = millis();

    // 1. Auto-return to Home Screen after 15 seconds of inactivity in menu
    if (_currentMode == SCREEN_MENU && (now - _lastUserActivityTime > 15000)) {
        setScreenMode(SCREEN_HOME);
        return;
    }

    // 2. High-speed, smooth 25 FPS update for VU Meter on Home Screen
    if (_currentMode == SCREEN_HOME && (now - _lastRenderTime >= 40)) {
        _lastRenderTime = now;
        updateHomeDynamicData();
    }
}
