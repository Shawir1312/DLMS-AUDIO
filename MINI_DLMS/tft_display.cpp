#include "tft_display.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "dsp_engine.h"
#include "presets_manager.h"
#include "web_server_dsp.h"

TftDisplay tftDisplay;

// Hardware SPI constructor: 27 MHz fast hardware SPI, 100% flicker-free
static Adafruit_ST7735 tft(&SPI, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

// Bright, high-contrast Color definitions (RGB565)
#define COLOR_BG         0x0000 // Black
#define COLOR_HEADER_BG  0x0014 // Deep Blue Header
#define COLOR_TEXT_DIM   0xA514 // Silver / Light Grey
#define COLOR_TEXT_BRT   0xFFFF // Crisp White
#define COLOR_ACCENT     0x07FF // Bright Cyan
#define COLOR_GREEN      0x07E0 // Bright Green
#define COLOR_YELLOW     0xFFE0 // Bright Yellow
#define COLOR_ORANGE     0xFD20 // Orange
#define COLOR_RED        0xF800 // Bright Red
#define COLOR_BOX_BORDER 0x39E7 // Crisp Slate Grey
#define COLOR_SEL_BG     0x0419 // Vibrant Teal

static const float FREQ_STEPS[] = {
    20.0f, 25.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f, 
    120.0f, 150.0f, 180.0f, 200.0f, 250.0f, 300.0f, 400.0f, 500.0f, 800.0f, 
    1000.0f, 1200.0f, 1500.0f, 2000.0f, 2500.0f, 3000.0f, 4000.0f, 5000.0f, 
    8000.0f, 10000.0f, 12000.0f, 16000.0f, 20000.0f
};
static const size_t FREQ_STEPS_COUNT = sizeof(FREQ_STEPS) / sizeof(FREQ_STEPS[0]);

#define MENU_ITEM_COUNT 37

TftDisplay::TftDisplay()
    : _currentMode(SCREEN_HOME),
      _isInitialized(false),
      _lastRenderTime(0),
      _lastUserActivityTime(0),
      _menuIndex(0),
      _inEditMode(false),
      _menuScrollOffset(0),
      _prevInBarW(0),
      _prevCh1BarW(0),
      _prevCh2BarW(0),
      _prevCh1Clip(false),
      _prevCh2Clip(false),
      _prevCh1Gain(-999.0f),
      _prevCh2Gain(-999.0f),
      _prevCh1Hpf(-1.0f),
      _prevCh2Hpf(-1.0f),
      _prevPreset(255),
      _prevCh1Mute(false),
      _prevCh2Mute(false),
      _prevCh1HpfEn(false),
      _prevCh1HpfFreq(-1.0f),
      _prevCh1LpfEn(false),
      _prevCh1LpfFreq(-1.0f),
      _prevCh2HpfEn(false),
      _prevCh2HpfFreq(-1.0f),
      _prevCh2LpfEn(false),
      _prevCh2LpfFreq(-1.0f),
      _prevEqMask(255),
      _prevRenderedMenuIndex(-1),
      _prevRenderedScrollOffset(-1),
      _prevRenderedEditMode(false)
{
}

bool TftDisplay::begin() {
    Serial.println("[TFT] Memulai Inisialisasi Layar 1.8\" ST7735 via Hardware SPI...");

    // 1. Pastikan Backlight menyala (Aktifkan Pin 8 dan Pin 13 dan TFT_BL_PIN)
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    if (TFT_BL_PIN >= 0 && TFT_BL_PIN != 8 && TFT_BL_PIN != 13) {
        pinMode(TFT_BL_PIN, OUTPUT);
        digitalWrite(TFT_BL_PIN, HIGH);
    }

    // 2. Hardware Reset Pulse pada Pin 14
    if (TFT_RST_PIN >= 0) {
        pinMode(TFT_RST_PIN, OUTPUT);
        digitalWrite(TFT_RST_PIN, HIGH);
        delay(20);
        digitalWrite(TFT_RST_PIN, LOW);
        delay(50);
        digitalWrite(TFT_RST_PIN, HIGH);
        delay(100);
    }

    // 3. Inisialisasi Hardware SPI Bus & ST7735 controller
    SPI.begin(TFT_SCLK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN);
    SPI.setFrequency(27000000); // 27 MHz Fast Hardware SPI (Super Cepat & Halus)

    tft.initR(INITR_BLACKTAB);
    tft.setSPISpeed(27000000);
    tft.invertDisplay(false); // Pastikan warna jernih tidak terbalik / pudar
    delay(50);
    tft.setRotation(1); // Landscape 160 x 128

    // 4. TEST VISUAL: Tampilkan Layar Biru Terang agar pasti terlihat menyala!
    tft.fillScreen(ST77XX_BLUE);
    tft.drawRect(2, 2, 156, 124, ST77XX_YELLOW);
    tft.drawRect(4, 4, 152, 120, ST77XX_YELLOW);

    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, 22);
    tft.print("S.NET DLMS");

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(18, 50);
    tft.print("2-CH INDEPENDENT DAC");

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(34, 74);
    tft.print("[ SYSTEM READY ]");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(22, 98);
    tft.printf("IP: %s", webServerDsp.getIpAddress().c_str());

    Serial.println("[TFT] Splash Screen Ditampilkan. Menunggu 1.2 detik...");
    delay(1200);

    // 5. Masuk ke Layar Utama VU Meter
    _isInitialized = true;
    _lastUserActivityTime = millis();
    _prevInBarW = 0;
    _prevCh1BarW = 0;
    _prevCh2BarW = 0;

    tft.fillScreen(COLOR_BG);
    drawHomeScreenLayout();
    Serial.println("[TFT] Layar Siap & Berjalan 100%!");
    return true;
}

void TftDisplay::setScreenMode(DisplayScreenMode mode) {
    if (_currentMode == mode) return;
    _currentMode = mode;
    _inEditMode = false;
    _lastUserActivityTime = millis();

    tft.fillScreen(COLOR_BG);

    if (_currentMode == SCREEN_HOME) {
        _prevInBarW = 0;
        _prevCh1BarW = 0;
        _prevCh2BarW = 0;
        _prevCh1Gain = -999.0f;
        _prevCh2Gain = -999.0f;
        _prevCh1Hpf = -1.0f;
        _prevCh2Hpf = -1.0f;
        _prevCh1Mute = false;
        _prevCh2Mute = false;
        _prevCh1HpfEn = false;
        _prevCh1HpfFreq = -1.0f;
        _prevCh1LpfEn = false;
        _prevCh1LpfFreq = -1.0f;
        _prevCh2HpfEn = false;
        _prevCh2HpfFreq = -1.0f;
        _prevCh2LpfEn = false;
        _prevCh2LpfFreq = -1.0f;
        _prevEqMask = 255;
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
    tft.setCursor(4, 3);
    tft.print(title);
}

void TftDisplay::drawHomeScreenLayout() {
    // 1. Top Header Banner
    drawHeader("S.NET DLMS - DUAL DAC", COLOR_HEADER_BG, COLOR_ACCENT);

    // 2. VU Meter Section Labels (Y: 18 - 58)
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);

    tft.setCursor(4, 18);
    tft.print("IN ");

    tft.setCursor(4, 30);
    tft.print("CH1");

    tft.setCursor(4, 42);
    tft.print("CH2");

    // Static VU background troughs (dark grey frames)
    tft.drawRect(26, 17, 86, 9, COLOR_BOX_BORDER);
    tft.drawRect(26, 29, 86, 9, COLOR_BOX_BORDER);
    tft.drawRect(26, 41, 86, 9, COLOR_BOX_BORDER);

    // VU dB scale ticks under the bars
    tft.setTextColor(COLOR_TEXT_DIM);
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
    if (db < -40.0f) db = -40.0f;
    if (db > 0.0f)   db = 0.0f;

    float norm = (db + 40.0f) / 40.0f; // 0.0 to 1.0
    int16_t bar_w = (int16_t)(norm * (float)w);
    if (bar_w < 0) bar_w = 0;
    if (bar_w > w) bar_w = w;

    if (prev_w < 0) prev_w = 0;
    if (bar_w == prev_w) return;

    if (bar_w > prev_w) {
        for (int16_t px = prev_w; px < bar_w; px++) {
            uint16_t col;
            float seg_ratio = (float)px / (float)w;
            if (seg_ratio < 0.65f) col = COLOR_GREEN;       // -40 to -14 dB
            else if (seg_ratio < 0.88f) col = COLOR_YELLOW; // -14 to -5 dB
            else col = COLOR_RED;                           // -5 to 0 dB
            tft.drawFastVLine(x + px, y, h, col);
        }
    } else {
        tft.fillRect(x + bar_w, y, prev_w - bar_w, h, COLOR_BG);
    }

    prev_w = bar_w;
}

void TftDisplay::updateHomeDynamicData() {
    VuMeterData vu = dspEngine.getVuMeterData();
    DspConfig cfg  = dspEngine.getConfig();

    // 1. Update VU Bars (Inner dimensions 84 x 7)
    drawVuBar(27, 18, 84, 7, vu.in_peak_db,  _prevInBarW);
    drawVuBar(27, 30, 84, 7, vu.ch1_peak_db, _prevCh1BarW);
    drawVuBar(27, 42, 84, 7, vu.ch2_peak_db, _prevCh2BarW);

    // 2. VU Numerical Readout / Clip Indicator
    tft.setTextSize(1);

    // CH1 Clip / Level
    tft.setCursor(116, 30);
    if (vu.ch1_clip) {
        tft.setTextColor(COLOR_RED, COLOR_BG);
        tft.print("CLIP ");
    } else {
        tft.setTextColor(COLOR_TEXT_BRT, COLOR_BG);
        char b1[8];
        snprintf(b1, sizeof(b1), "%+3.0fdB", vu.ch1_peak_db);
        tft.print(b1);
    }

    // CH2 Clip / Level
    tft.setCursor(116, 42);
    if (vu.ch2_clip) {
        tft.setTextColor(COLOR_RED, COLOR_BG);
        tft.print("CLIP ");
    } else {
        tft.setTextColor(COLOR_TEXT_BRT, COLOR_BG);
        char b2[8];
        snprintf(b2, sizeof(b2), "%+3.0fdB", vu.ch2_peak_db);
        tft.print(b2);
    }

    // 3. System Information Box lines (Real-time Crossover & EQ Status Display)
    bool ch1M = cfg.ch1.mute || cfg.mute;
    bool ch2M = cfg.ch2.mute || cfg.mute;
    if (fabsf(cfg.ch1.gain_db - _prevCh1Gain) > 0.2f || 
        fabsf(cfg.ch2.gain_db - _prevCh2Gain) > 0.2f ||
        ch1M != _prevCh1Mute || ch2M != _prevCh2Mute) {
        _prevCh1Gain = cfg.ch1.gain_db;
        _prevCh2Gain = cfg.ch2.gain_db;
        _prevCh1Mute = ch1M;
        _prevCh2Mute = ch2M;
        tft.fillRect(4, 65, 152, 9, COLOR_BG);
        tft.setCursor(6, 65);
        tft.setTextColor(ch1M ? COLOR_RED : COLOR_TEXT_BRT, COLOR_BG);
        tft.printf("CH1:%+4.1fdB%s", cfg.ch1.gain_db, ch1M ? "[M]" : "  ");
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.print(" | ");
        tft.setTextColor(ch2M ? COLOR_RED : COLOR_TEXT_BRT, COLOR_BG);
        tft.printf("CH2:%+4.1fdB%s", cfg.ch2.gain_db, ch2M ? "[M]" : "  ");
    }

    // Line 2 (Y=76): Channel 1 Crossover (HPF & LPF)
    bool ch1HpfEn = cfg.ch1.hpf.enabled;
    float ch1HpfFreq = cfg.ch1.hpf.freq;
    bool ch1LpfEn = cfg.ch1.lpf.enabled;
    float ch1LpfFreq = cfg.ch1.lpf.freq;
    if (ch1HpfEn != _prevCh1HpfEn || fabsf(ch1HpfFreq - _prevCh1HpfFreq) > 0.5f ||
        ch1LpfEn != _prevCh1LpfEn || fabsf(ch1LpfFreq - _prevCh1LpfFreq) > 0.5f) {
        _prevCh1HpfEn = ch1HpfEn;
        _prevCh1HpfFreq = ch1HpfFreq;
        _prevCh1LpfEn = ch1LpfEn;
        _prevCh1LpfFreq = ch1LpfFreq;

        tft.fillRect(4, 76, 152, 9, COLOR_BG);
        tft.setCursor(6, 76);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.print("XO1: ");
        if (ch1HpfEn) tft.printf("H:%4.0f ", ch1HpfFreq);
        else tft.print("H:OFF  ");
        if (ch1LpfEn) tft.printf("L:%4.0f", ch1LpfFreq);
        else tft.print("L:OFF ");
    }

    // Line 3 (Y=87): Channel 2 Crossover (HPF & LPF)
    bool ch2HpfEn = cfg.ch2.hpf.enabled;
    float ch2HpfFreq = cfg.ch2.hpf.freq;
    bool ch2LpfEn = cfg.ch2.lpf.enabled;
    float ch2LpfFreq = cfg.ch2.lpf.freq;
    if (ch2HpfEn != _prevCh2HpfEn || fabsf(ch2HpfFreq - _prevCh2HpfFreq) > 0.5f ||
        ch2LpfEn != _prevCh2LpfEn || fabsf(ch2LpfFreq - _prevCh2LpfFreq) > 0.5f) {
        _prevCh2HpfEn = ch2HpfEn;
        _prevCh2HpfFreq = ch2HpfFreq;
        _prevCh2LpfEn = ch2LpfEn;
        _prevCh2LpfFreq = ch2LpfFreq;

        tft.fillRect(4, 87, 152, 9, COLOR_BG);
        tft.setCursor(6, 87);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.print("XO2: ");
        if (ch2HpfEn) tft.printf("H:%4.0f ", ch2HpfFreq);
        else tft.print("H:OFF  ");
        if (ch2LpfEn) tft.printf("L:%4.0f", ch2LpfFreq);
        else tft.print("L:OFF ");
    }

    // Line 4 (Y=98): EQ Active Bands & Active Preset
    uint8_t eqMask = (cfg.ch1.peq[0].enabled ? 1 : 0) |
                     (cfg.ch1.peq[1].enabled ? 2 : 0) |
                     (cfg.ch1.peq[2].enabled ? 4 : 0) |
                     (cfg.ch2.peq[0].enabled ? 8 : 0) |
                     (cfg.ch2.peq[1].enabled ? 16 : 0) |
                     (cfg.ch2.peq[2].enabled ? 32 : 0);
    uint8_t curSlot = presetsManager.getCurrentSlot();
    if (eqMask != _prevEqMask || curSlot != _prevPreset) {
        _prevEqMask = eqMask;
        _prevPreset = curSlot;

        tft.fillRect(4, 98, 152, 9, COLOR_BG);
        tft.setCursor(6, 98);
        tft.setTextColor(COLOR_YELLOW, COLOR_BG);
        tft.printf("EQ1:[%c%c%c] EQ2:[%c%c%c] P%u",
            cfg.ch1.peq[0].enabled ? '1' : '-',
            cfg.ch1.peq[1].enabled ? '2' : '-',
            cfg.ch1.peq[2].enabled ? '3' : '-',
            cfg.ch2.peq[0].enabled ? '1' : '-',
            cfg.ch2.peq[1].enabled ? '2' : '-',
            cfg.ch2.peq[2].enabled ? '3' : '-',
            curSlot
        );
    }
}

void TftDisplay::drawMenuFooter() {
    tft.fillRect(0, 114, 160, 14, 0x01A3);
    tft.setTextColor(_inEditMode ? COLOR_YELLOW : COLOR_ACCENT);
    tft.setCursor(6, 117);
    if (_inEditMode) {
        tft.print("PUTAR: UBAH | TEKAN: OK");
    } else {
        tft.print("PUTAR: PILIH | TEKAN: EDIT");
    }
}

void TftDisplay::drawMenuRow(uint8_t row, bool isSelected, bool isEditMode) {
    uint8_t item_idx = _menuScrollOffset + row;
    if (item_idx >= MENU_ITEM_COUNT) return;

    int16_t y = 17 + (row * 15);
    DspConfig cfg = dspEngine.getConfig();

    if (isSelected) {
        tft.fillRect(0, y, 160, 14, isEditMode ? COLOR_RED : COLOR_SEL_BG);
        tft.setTextColor(COLOR_TEXT_BRT);
    } else {
        tft.fillRect(0, y, 160, 14, COLOR_BG);
        tft.setTextColor(COLOR_TEXT_DIM);
    }

    tft.setTextSize(1);
    tft.setCursor(4, y + 3);
    tft.print(isSelected ? (isEditMode ? "* " : "> ") : "  ");

    switch (item_idx) {
        case 0:  tft.printf("CH1 GAIN   : %+4.1f dB", cfg.ch1.gain_db); break;
        case 1:  tft.printf("CH2 GAIN   : %+4.1f dB", cfg.ch2.gain_db); break;
        case 2:  tft.printf("CH1 HPF SW : %s", cfg.ch1.hpf.enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 3:  tft.printf("CH1 HPF FRQ: %4.0f Hz", cfg.ch1.hpf.freq); break;
        case 4:  tft.printf("CH1 LPF SW : %s", cfg.ch1.lpf.enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 5:  tft.printf("CH1 LPF FRQ: %4.0f Hz", cfg.ch1.lpf.freq); break;
        case 6:  tft.printf("CH2 HPF SW : %s", cfg.ch2.hpf.enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 7:  tft.printf("CH2 HPF FRQ: %4.0f Hz", cfg.ch2.hpf.freq); break;
        case 8:  tft.printf("CH2 LPF SW : %s", cfg.ch2.lpf.enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 9:  tft.printf("CH2 LPF FRQ: %4.0f Hz", cfg.ch2.lpf.freq); break;
        case 10: tft.printf("CH1 EQ1 SW : %s", cfg.ch1.peq[0].enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 11: tft.printf("CH1 EQ1 FRQ: %4.0f Hz", cfg.ch1.peq[0].freq); break;
        case 12: tft.printf("CH1 EQ1 GAIN:%+4.1f dB", cfg.ch1.peq[0].gain_db); break;
        case 13: tft.printf("CH1 EQ2 SW : %s", cfg.ch1.peq[1].enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 14: tft.printf("CH1 EQ2 FRQ: %4.0f Hz", cfg.ch1.peq[1].freq); break;
        case 15: tft.printf("CH1 EQ2 GAIN:%+4.1f dB", cfg.ch1.peq[1].gain_db); break;
        case 16: tft.printf("CH1 EQ3 SW : %s", cfg.ch1.peq[2].enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 17: tft.printf("CH1 EQ3 FRQ: %4.0f Hz", cfg.ch1.peq[2].freq); break;
        case 18: tft.printf("CH1 EQ3 GAIN:%+4.1f dB", cfg.ch1.peq[2].gain_db); break;
        case 19: tft.printf("CH2 EQ1 SW : %s", cfg.ch2.peq[0].enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 20: tft.printf("CH2 EQ1 FRQ: %4.0f Hz", cfg.ch2.peq[0].freq); break;
        case 21: tft.printf("CH2 EQ1 GAIN:%+4.1f dB", cfg.ch2.peq[0].gain_db); break;
        case 22: tft.printf("CH2 EQ2 SW : %s", cfg.ch2.peq[1].enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 23: tft.printf("CH2 EQ2 FRQ: %4.0f Hz", cfg.ch2.peq[1].freq); break;
        case 24: tft.printf("CH2 EQ2 GAIN:%+4.1f dB", cfg.ch2.peq[1].gain_db); break;
        case 25: tft.printf("CH2 EQ3 SW : %s", cfg.ch2.peq[2].enabled ? "AKTIF [ON]" : "BYPASS [OFF]"); break;
        case 26: tft.printf("CH2 EQ3 FRQ: %4.0f Hz", cfg.ch2.peq[2].freq); break;
        case 27: tft.printf("CH2 EQ3 GAIN:%+4.1f dB", cfg.ch2.peq[2].gain_db); break;
        case 28: tft.printf("CH1 PHASE  : %s", cfg.ch1.polarity_inverted ? "INVERT 180" : "NORMAL"); break;
        case 29: tft.printf("CH2 PHASE  : %s", cfg.ch2.polarity_inverted ? "INVERT 180" : "NORMAL"); break;
        case 30: tft.printf("CH1 MUTE   : %s", cfg.ch1.mute ? "MUTED [ON]" : "UNMUTED"); break;
        case 31: tft.printf("CH2 MUTE   : %s", cfg.ch2.mute ? "MUTED [ON]" : "UNMUTED"); break;
        case 32: tft.printf("MASTER VOL : %+4.0f dB", cfg.master_gain_db); break;
        case 33: tft.printf("ALL MUTE   : %s", cfg.mute ? "MUTED [ON]" : "UNMUTED"); break;
        case 34: tft.printf("LOAD PRESET: Slot %u", presetsManager.getCurrentSlot()); break;
        case 35: tft.printf("SAVE PRESET: [KLIK]"); break;
        case 36: tft.printf("< KEMBALI KE VU METER >"); break;
    }
}

void TftDisplay::drawMenuScreen(bool forceFullRedraw) {
    const uint8_t visible_count = 6;
    if (_menuIndex < _menuScrollOffset) {
        _menuScrollOffset = _menuIndex;
    } else if (_menuIndex >= _menuScrollOffset + visible_count) {
        _menuScrollOffset = _menuIndex - visible_count + 1;
    }

    bool needFullRedraw = forceFullRedraw || 
                         (_prevRenderedScrollOffset != _menuScrollOffset) ||
                         (_prevRenderedMenuIndex < 0);

    if (needFullRedraw) {
        drawHeader("DSP PARAMETER SETTINGS", COLOR_HEADER_BG, COLOR_YELLOW);
        for (uint8_t row = 0; row < visible_count; row++) {
            uint8_t item_idx = _menuScrollOffset + row;
            if (item_idx >= MENU_ITEM_COUNT) {
                int16_t y = 17 + (row * 15);
                tft.fillRect(0, y, 160, 14, COLOR_BG);
                continue;
            }
            bool isSelected = (item_idx == _menuIndex);
            drawMenuRow(row, isSelected, _inEditMode);
        }
        drawMenuFooter();
    } else {
        // Super-fast Differential Rendering: Only redraw the row that lost selection and the row that gained it!
        if (_prevRenderedMenuIndex != _menuIndex || _prevRenderedEditMode != _inEditMode) {
            if (_prevRenderedMenuIndex >= _menuScrollOffset && 
                _prevRenderedMenuIndex < _menuScrollOffset + visible_count) {
                uint8_t old_row = _prevRenderedMenuIndex - _menuScrollOffset;
                drawMenuRow(old_row, false, false);
            }
            uint8_t cur_row = _menuIndex - _menuScrollOffset;
            drawMenuRow(cur_row, true, _inEditMode);

            if (_prevRenderedEditMode != _inEditMode) {
                drawMenuFooter();
            }
        }
    }

    _prevRenderedMenuIndex = _menuIndex;
    _prevRenderedScrollOffset = _menuScrollOffset;
    _prevRenderedEditMode = _inEditMode;
}

static float stepFrequency(float current_freq, int32_t delta) {
    int current_idx = 0;
    float min_diff = 99999.0f;
    for (size_t i = 0; i < FREQ_STEPS_COUNT; i++) {
        float diff = fabsf(FREQ_STEPS[i] - current_freq);
        if (diff < min_diff) {
            min_diff = diff;
            current_idx = (int)i;
        }
    }
    current_idx += delta;
    if (current_idx < 0) current_idx = 0;
    if (current_idx >= (int)FREQ_STEPS_COUNT) current_idx = (int)FREQ_STEPS_COUNT - 1;
    return FREQ_STEPS[current_idx];
}

void TftDisplay::applyMenuEdit(int32_t delta) {
    DspConfig cfg = dspEngine.getConfig();

    switch (_menuIndex) {
        case 0: { // CH1 GAIN
            float g = cfg.ch1.gain_db + (float)delta * 0.5f;
            dspEngine.setCh1Gain(g);
            break;
        }
        case 1: { // CH2 GAIN
            float g = cfg.ch2.gain_db + (float)delta * 0.5f;
            dspEngine.setCh2Gain(g);
            break;
        }
        case 2: { // CH1 HPF SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch1.hpf.enabled);
            dspEngine.setCh1Hpf(en, cfg.ch1.hpf.freq, cfg.ch1.hpf.slope);
            break;
        }
        case 3: { // CH1 HPF FRQ
            float f = stepFrequency(cfg.ch1.hpf.freq, delta);
            dspEngine.setCh1Hpf(cfg.ch1.hpf.enabled, f, cfg.ch1.hpf.slope);
            break;
        }
        case 4: { // CH1 LPF SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch1.lpf.enabled);
            dspEngine.setCh1Lpf(en, cfg.ch1.lpf.freq, cfg.ch1.lpf.slope);
            break;
        }
        case 5: { // CH1 LPF FRQ
            float f = stepFrequency(cfg.ch1.lpf.freq, delta);
            dspEngine.setCh1Lpf(cfg.ch1.lpf.enabled, f, cfg.ch1.lpf.slope);
            break;
        }
        case 6: { // CH2 HPF SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch2.hpf.enabled);
            dspEngine.setCh2Hpf(en, cfg.ch2.hpf.freq, cfg.ch2.hpf.slope);
            break;
        }
        case 7: { // CH2 HPF FRQ
            float f = stepFrequency(cfg.ch2.hpf.freq, delta);
            dspEngine.setCh2Hpf(cfg.ch2.hpf.enabled, f, cfg.ch2.hpf.slope);
            break;
        }
        case 8: { // CH2 LPF SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch2.lpf.enabled);
            dspEngine.setCh2Lpf(en, cfg.ch2.lpf.freq, cfg.ch2.lpf.slope);
            break;
        }
        case 9: { // CH2 LPF FRQ
            float f = stepFrequency(cfg.ch2.lpf.freq, delta);
            dspEngine.setCh2Lpf(cfg.ch2.lpf.enabled, f, cfg.ch2.lpf.slope);
            break;
        }
        case 10: { // CH1 EQ1 SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch1.peq[0].enabled);
            dspEngine.setCh1Peq(0, en, cfg.ch1.peq[0].type, cfg.ch1.peq[0].freq, cfg.ch1.peq[0].gain_db, cfg.ch1.peq[0].q);
            break;
        }
        case 11: { // CH1 EQ1 FRQ
            float f = stepFrequency(cfg.ch1.peq[0].freq, delta);
            dspEngine.setCh1Peq(0, cfg.ch1.peq[0].enabled, cfg.ch1.peq[0].type, f, cfg.ch1.peq[0].gain_db, cfg.ch1.peq[0].q);
            break;
        }
        case 12: { // CH1 EQ1 GAIN
            float g = cfg.ch1.peq[0].gain_db + (float)delta * 0.5f;
            dspEngine.setCh1Peq(0, cfg.ch1.peq[0].enabled, cfg.ch1.peq[0].type, cfg.ch1.peq[0].freq, g, cfg.ch1.peq[0].q);
            break;
        }
        case 13: { // CH1 EQ2 SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch1.peq[1].enabled);
            dspEngine.setCh1Peq(1, en, cfg.ch1.peq[1].type, cfg.ch1.peq[1].freq, cfg.ch1.peq[1].gain_db, cfg.ch1.peq[1].q);
            break;
        }
        case 14: { // CH1 EQ2 FRQ
            float f = stepFrequency(cfg.ch1.peq[1].freq, delta);
            dspEngine.setCh1Peq(1, cfg.ch1.peq[1].enabled, cfg.ch1.peq[1].type, f, cfg.ch1.peq[1].gain_db, cfg.ch1.peq[1].q);
            break;
        }
        case 15: { // CH1 EQ2 GAIN
            float g = cfg.ch1.peq[1].gain_db + (float)delta * 0.5f;
            dspEngine.setCh1Peq(1, cfg.ch1.peq[1].enabled, cfg.ch1.peq[1].type, cfg.ch1.peq[1].freq, g, cfg.ch1.peq[1].q);
            break;
        }
        case 16: { // CH1 EQ3 SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch1.peq[2].enabled);
            dspEngine.setCh1Peq(2, en, cfg.ch1.peq[2].type, cfg.ch1.peq[2].freq, cfg.ch1.peq[2].gain_db, cfg.ch1.peq[2].q);
            break;
        }
        case 17: { // CH1 EQ3 FRQ
            float f = stepFrequency(cfg.ch1.peq[2].freq, delta);
            dspEngine.setCh1Peq(2, cfg.ch1.peq[2].enabled, cfg.ch1.peq[2].type, f, cfg.ch1.peq[2].gain_db, cfg.ch1.peq[2].q);
            break;
        }
        case 18: { // CH1 EQ3 GAIN
            float g = cfg.ch1.peq[2].gain_db + (float)delta * 0.5f;
            dspEngine.setCh1Peq(2, cfg.ch1.peq[2].enabled, cfg.ch1.peq[2].type, cfg.ch1.peq[2].freq, g, cfg.ch1.peq[2].q);
            break;
        }
        case 19: { // CH2 EQ1 SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch2.peq[0].enabled);
            dspEngine.setCh2Peq(0, en, cfg.ch2.peq[0].type, cfg.ch2.peq[0].freq, cfg.ch2.peq[0].gain_db, cfg.ch2.peq[0].q);
            break;
        }
        case 20: { // CH2 EQ1 FRQ
            float f = stepFrequency(cfg.ch2.peq[0].freq, delta);
            dspEngine.setCh2Peq(0, cfg.ch2.peq[0].enabled, cfg.ch2.peq[0].type, f, cfg.ch2.peq[0].gain_db, cfg.ch2.peq[0].q);
            break;
        }
        case 21: { // CH2 EQ1 GAIN
            float g = cfg.ch2.peq[0].gain_db + (float)delta * 0.5f;
            dspEngine.setCh2Peq(0, cfg.ch2.peq[0].enabled, cfg.ch2.peq[0].type, cfg.ch2.peq[0].freq, g, cfg.ch2.peq[0].q);
            break;
        }
        case 22: { // CH2 EQ2 SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch2.peq[1].enabled);
            dspEngine.setCh2Peq(1, en, cfg.ch2.peq[1].type, cfg.ch2.peq[1].freq, cfg.ch2.peq[1].gain_db, cfg.ch2.peq[1].q);
            break;
        }
        case 23: { // CH2 EQ2 FRQ
            float f = stepFrequency(cfg.ch2.peq[1].freq, delta);
            dspEngine.setCh2Peq(1, cfg.ch2.peq[1].enabled, cfg.ch2.peq[1].type, f, cfg.ch2.peq[1].gain_db, cfg.ch2.peq[1].q);
            break;
        }
        case 24: { // CH2 EQ2 GAIN
            float g = cfg.ch2.peq[1].gain_db + (float)delta * 0.5f;
            dspEngine.setCh2Peq(1, cfg.ch2.peq[1].enabled, cfg.ch2.peq[1].type, cfg.ch2.peq[1].freq, g, cfg.ch2.peq[1].q);
            break;
        }
        case 25: { // CH2 EQ3 SW
            bool en = (delta > 0) ? true : (delta < 0 ? false : !cfg.ch2.peq[2].enabled);
            dspEngine.setCh2Peq(2, en, cfg.ch2.peq[2].type, cfg.ch2.peq[2].freq, cfg.ch2.peq[2].gain_db, cfg.ch2.peq[2].q);
            break;
        }
        case 26: { // CH2 EQ3 FRQ
            float f = stepFrequency(cfg.ch2.peq[2].freq, delta);
            dspEngine.setCh2Peq(2, cfg.ch2.peq[2].enabled, cfg.ch2.peq[2].type, f, cfg.ch2.peq[2].gain_db, cfg.ch2.peq[2].q);
            break;
        }
        case 27: { // CH2 EQ3 GAIN
            float g = cfg.ch2.peq[2].gain_db + (float)delta * 0.5f;
            dspEngine.setCh2Peq(2, cfg.ch2.peq[2].enabled, cfg.ch2.peq[2].type, cfg.ch2.peq[2].freq, g, cfg.ch2.peq[2].q);
            break;
        }
        case 28: { // CH1 PHASE
            dspEngine.setCh1Invert(!cfg.ch1.polarity_inverted);
            break;
        }
        case 29: { // CH2 PHASE
            dspEngine.setCh2Invert(!cfg.ch2.polarity_inverted);
            break;
        }
        case 30: { // CH1 MUTE
            dspEngine.setCh1Mute(!cfg.ch1.mute);
            break;
        }
        case 31: { // CH2 MUTE
            dspEngine.setCh2Mute(!cfg.ch2.mute);
            break;
        }
        case 32: { // MASTER VOL
            float g = cfg.master_gain_db + (float)delta * 1.0f;
            dspEngine.setMasterGain(g);
            break;
        }
        case 33: { // ALL MUTE
            dspEngine.setMasterMute(!cfg.mute);
            break;
        }
        case 34: { // LOAD PRESET
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
        case 35: // SAVE PRESET (handled in executeMenuSelect)
        case 36: // BACK TO HOME (handled in executeMenuSelect)
            break;
    }

    uint8_t cur_row = _menuIndex - _menuScrollOffset;
    drawMenuRow(cur_row, true, true);
}

void TftDisplay::executeMenuSelect() {
    if (_menuIndex == 36) { // BACK TO HOME
        setScreenMode(SCREEN_HOME);
        return;
    }

    if (_menuIndex == 35) { // SAVE TO NVS
        DspConfig currentCfg = dspEngine.getConfig();
        presetsManager.savePreset(presetsManager.getCurrentSlot(), currentCfg);

        tft.fillRect(0, 114, 160, 14, COLOR_GREEN);
        tft.setTextColor(COLOR_BG);
        tft.setCursor(20, 117);
        tft.print("[ TERSIMPAN KE NVS! ]");
        delay(600);
        drawMenuScreen(true);
        return;
    }

    DspConfig cfg = dspEngine.getConfig();

    // Fast 1-click toggle switches!
    switch (_menuIndex) {
        case 2: // CH1 HPF SW
            dspEngine.setCh1Hpf(!cfg.ch1.hpf.enabled, cfg.ch1.hpf.freq, cfg.ch1.hpf.slope);
            break;
        case 4: // CH1 LPF SW
            dspEngine.setCh1Lpf(!cfg.ch1.lpf.enabled, cfg.ch1.lpf.freq, cfg.ch1.lpf.slope);
            break;
        case 6: // CH2 HPF SW
            dspEngine.setCh2Hpf(!cfg.ch2.hpf.enabled, cfg.ch2.hpf.freq, cfg.ch2.hpf.slope);
            break;
        case 8: // CH2 LPF SW
            dspEngine.setCh2Lpf(!cfg.ch2.lpf.enabled, cfg.ch2.lpf.freq, cfg.ch2.lpf.slope);
            break;
        case 10: // CH1 EQ1 SW
            dspEngine.setCh1Peq(0, !cfg.ch1.peq[0].enabled, cfg.ch1.peq[0].type, cfg.ch1.peq[0].freq, cfg.ch1.peq[0].gain_db, cfg.ch1.peq[0].q);
            break;
        case 13: // CH1 EQ2 SW
            dspEngine.setCh1Peq(1, !cfg.ch1.peq[1].enabled, cfg.ch1.peq[1].type, cfg.ch1.peq[1].freq, cfg.ch1.peq[1].gain_db, cfg.ch1.peq[1].q);
            break;
        case 16: // CH1 EQ3 SW
            dspEngine.setCh1Peq(2, !cfg.ch1.peq[2].enabled, cfg.ch1.peq[2].type, cfg.ch1.peq[2].freq, cfg.ch1.peq[2].gain_db, cfg.ch1.peq[2].q);
            break;
        case 19: // CH2 EQ1 SW
            dspEngine.setCh2Peq(0, !cfg.ch2.peq[0].enabled, cfg.ch2.peq[0].type, cfg.ch2.peq[0].freq, cfg.ch2.peq[0].gain_db, cfg.ch2.peq[0].q);
            break;
        case 22: // CH2 EQ2 SW
            dspEngine.setCh2Peq(1, !cfg.ch2.peq[1].enabled, cfg.ch2.peq[1].type, cfg.ch2.peq[1].freq, cfg.ch2.peq[1].gain_db, cfg.ch2.peq[1].q);
            break;
        case 25: // CH2 EQ3 SW
            dspEngine.setCh2Peq(2, !cfg.ch2.peq[2].enabled, cfg.ch2.peq[2].type, cfg.ch2.peq[2].freq, cfg.ch2.peq[2].gain_db, cfg.ch2.peq[2].q);
            break;
        case 28: // CH1 PHASE
            dspEngine.setCh1Invert(!cfg.ch1.polarity_inverted);
            break;
        case 29: // CH2 PHASE
            dspEngine.setCh2Invert(!cfg.ch2.polarity_inverted);
            break;
        case 30: // CH1 MUTE
            dspEngine.setCh1Mute(!cfg.ch1.mute);
            break;
        case 31: // CH2 MUTE
            dspEngine.setCh2Mute(!cfg.ch2.mute);
            break;
        case 33: // ALL MUTE
            dspEngine.setMasterMute(!cfg.mute);
            break;
        default:
            // Adjust values toggle edit mode
            _inEditMode = !_inEditMode;
            break;
    }

    uint8_t cur_row = _menuIndex - _menuScrollOffset;
    drawMenuRow(cur_row, true, _inEditMode);
    drawMenuFooter();
}

void TftDisplay::handleEncoder(int32_t delta, bool clicked, bool longPressed) {
    if (!_isInitialized) return;

    _lastUserActivityTime = millis();

    if (longPressed) {
        setScreenMode(SCREEN_HOME);
        return;
    }

    if (_currentMode == SCREEN_HOME) {
        if (clicked || delta != 0) {
            setScreenMode(SCREEN_MENU);
        }
        return;
    }

    if (_inEditMode) {
        if (delta != 0) {
            applyMenuEdit(delta);
        }
        if (clicked) {
            _inEditMode = false;
            drawMenuScreen(false);
        }
    } else {
        if (delta != 0) {
            int new_idx = _menuIndex + delta;
            if (new_idx < 0) new_idx = 0;
            if (new_idx >= MENU_ITEM_COUNT) new_idx = MENU_ITEM_COUNT - 1;
            if (new_idx != _menuIndex) {
                _menuIndex = new_idx;
                drawMenuScreen(false);
            }
        }
        if (clicked) {
            executeMenuSelect();
        }
    }
}

void TftDisplay::update() {
    if (!_isInitialized) return;

    // Pastikan backlight pin selalu HIGH
    digitalWrite(8, HIGH);
    digitalWrite(13, HIGH);
    if (TFT_BL_PIN >= 0 && TFT_BL_PIN != 8 && TFT_BL_PIN != 13) {
        digitalWrite(TFT_BL_PIN, HIGH);
    }

    unsigned long now = millis();

    if (_currentMode == SCREEN_MENU && (now - _lastUserActivityTime > 15000)) {
        setScreenMode(SCREEN_HOME);
        return;
    }

    if (_currentMode == SCREEN_HOME && (now - _lastRenderTime >= 40)) {
        _lastRenderTime = now;
        updateHomeDynamicData();
    }
}
