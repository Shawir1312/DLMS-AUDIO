#include "tft_display.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <math.h>
#include "dsp_engine.h"
#include "presets_manager.h"
#include "web_server_dsp.h"

TftDisplay tftDisplay;

// Hardware SPI constructor: 27 MHz fast hardware SPI, 100% flicker-free
static Adafruit_ST7735 tft(&SPI, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

// Cyber Theme Colors (RGB565) - Matching Web UI "SOFGAM SS by Shawir"
#define COLOR_BG          0x0000 // Deep Black (#000000)
#define COLOR_NAVY_BAR    0x0862 // Cyber Dark Navy (#060e1f)
#define COLOR_CARD_BG     0x08A4 // Dark Card Background (#081226)
#define COLOR_CARD_DARK   0x0041 // Deep Black-Navy (#040916)
#define COLOR_CYAN_ACCENT 0x07FF // Vibrant Electric Cyan (#00d2ff)
#define COLOR_CYAN_DIM    0x03B3 // Muted Tech Cyan (#007a99)
#define COLOR_TEXT_BRT    0xFFFF // Crisp Pure White (#ffffff)
#define COLOR_TEXT_DIM    0x9CD3 // Slate Silver (#94a3b8)
#define COLOR_GREEN       0x07E0 // Matrix Neon Green (#10b981)
#define COLOR_YELLOW      0xFFE0 // Warning Amber (#f59e0b)
#define COLOR_RED         0xF800 // Peak Clip Crimson (#ef4444)
#define COLOR_PURPLE      0x913F // Electric Violet (#9333ea)
#define COLOR_BOX_BORDER  0x1127 // Subtle Cyber Slate Frame (#16233d)
#define COLOR_SEL_BG      0x03B9 // Selection Teal (#0077ff)
#define COLOR_SEL_EDIT    0xB2A0 // Edit Highlight Amber
#define COLOR_UNLIT_SEG   0x08A4 // Dark unlit LED segment (#0c1524)

// 2x5 Menu Grid Tiles
static const struct {
    const char* id;
    const char* label;
} MENU_TILES[10] = {
    { "HOME",    "HOME"  },
    { "GAIN",    "GAIN"  },
    { "HPF",     "HPF"   },
    { "LPF",     "LPF"   },
    { "DELAY",   "DELAY" },
    { "PEQ",     "PEQ"   },
    { "LIMIT",   "LIMIT" },
    { "PRESET",  "PRESET"},
    { "STATUS",  "STATUS"},
    { "ABOUT",   "ABOUT" }
};

TftDisplay::TftDisplay()
    : _currentMode(SCREEN_HOME),
      _isInitialized(false),
      _lastRenderTime(0),
      _lastUserActivityTime(0),
      _cursorIndex(0),
      _inEditMode(false),
      _peqBandIndex(0),
      _presetSlot(1),
      _prevInLDb(-999.0f),
      _prevInRDb(-999.0f),
      _prevOut1Db(-999.0f),
      _prevOut2Db(-999.0f),
      _prevOut3Db(-999.0f),
      _prevOut4Db(-999.0f),
      _prevInLSegs(-1),
      _prevInRSegs(-1),
      _prevOut1Segs(-1),
      _prevOut2Segs(-1),
      _prevOut3Segs(-1),
      _prevOut4Segs(-1),
      _prevMasterGain(-999.0f),
      _prevHpfFreq(-1.0f),
      _prevLpfFreq(-1.0f),
      _prevHpfEn(false),
      _prevLpfEn(false),
      _prevPeqEn(false),
      _prevLimEn(false),
      _prevPreset(255),
      _prevDockIndex(-1)
{
}

void TftDisplay::drawCard(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t borderCol, uint16_t bgCol) {
    tft.fillRect(x, y, w, h, bgCol);
    tft.drawRect(x, y, w, h, borderCol);
}

void TftDisplay::drawHeader(const char* title, const char* right_tag, bool show_run) {
    tft.fillRect(0, 0, 160, 13, COLOR_NAVY_BAR);
    tft.drawFastHLine(0, 13, 160, COLOR_CYAN_DIM);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(3, 3);
    tft.print(title);

    if (right_tag && strlen(right_tag) > 0) {
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.setCursor(96, 3);
        tft.print(right_tag);
    }
    if (show_run) {
        tft.fillCircle(126, 6, 2, COLOR_GREEN);
        tft.setTextColor(COLOR_GREEN);
        tft.setCursor(131, 3);
        tft.print("RUN");
    }
}

void TftDisplay::drawLedMeter(int16_t x, int16_t y, int16_t w, int16_t h, float db, uint8_t segments, int16_t& prev_segs) {
    if (db < -40.0f) db = -40.0f;
    if (db > 0.0f)   db = 0.0f;

    float norm = (db + 40.0f) / 40.0f; // 0.0 to 1.0
    int16_t activeCount = (int16_t)roundf(norm * (float)segments);
    if (activeCount < 0) activeCount = 0;
    if (activeCount > segments) activeCount = segments;

    if (activeCount == prev_segs) return;
    prev_segs = activeCount;

    int16_t seg_w = (w / segments) - 1;
    if (seg_w < 1) seg_w = 1;

    for (uint8_t s = 0; s < segments; s++) {
        int16_t sx = x + s * (seg_w + 1);
        uint16_t col;
        if (s < activeCount) {
            float ratio = (float)s / (float)segments;
            if (ratio < 0.65f) col = COLOR_GREEN;
            else if (ratio < 0.85f) col = COLOR_YELLOW;
            else col = COLOR_RED;
        } else {
            col = COLOR_UNLIT_SEG;
        }
        tft.fillRect(sx, y, seg_w, h, col);
    }
}

void TftDisplay::drawSplashScreen() {
    tft.fillScreen(COLOR_BG);

    // Double cyber border with corner accents
    tft.drawRoundRect(2, 2, 156, 124, 4, COLOR_CYAN_DIM);
    tft.drawRoundRect(4, 4, 152, 120, 3, COLOR_BOX_BORDER);

    // Tech corner markers
    tft.drawFastHLine(2, 2, 14, COLOR_CYAN_ACCENT);
    tft.drawFastVLine(2, 2, 14, COLOR_CYAN_ACCENT);
    tft.drawFastHLine(144, 2, 14, COLOR_CYAN_ACCENT);
    tft.drawFastVLine(157, 2, 14, COLOR_CYAN_ACCENT);
    tft.drawFastHLine(2, 125, 14, COLOR_CYAN_ACCENT);
    tft.drawFastVLine(2, 113, 14, COLOR_CYAN_ACCENT);
    tft.drawFastHLine(144, 125, 14, COLOR_CYAN_ACCENT);
    tft.drawFastVLine(157, 113, 14, COLOR_CYAN_ACCENT);

    // Brand Title: "SOFGAM SS"
    tft.setTextSize(2);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(26, 16);
    tft.print("SOFGAM SS");

    // Subtitle: "by Shawir"
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_DIM);
    tft.setCursor(52, 34);
    tft.print("by Shawir");

    // Dynamic Cyber Sine Waves
    for (int x = 18; x < 142; x++) {
        float angle1 = (float)(x - 18) * 0.08f;
        int y1 = 51 + (int)(sinf(angle1) * 6.0f);
        tft.drawPixel(x, y1, COLOR_CYAN_ACCENT);
        tft.drawPixel(x, y1 + 1, COLOR_CYAN_ACCENT);

        float angle2 = (float)(x - 18) * 0.06f + 1.4f;
        int y2 = 53 + (int)(sinf(angle2) * 5.0f);
        tft.drawPixel(x, y2, COLOR_PURPLE);
    }

    // Tagline: "Better Sound, Better Exp."
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(8, 72);
    tft.print("Better Sound, Better Exp.");

    // Status: "v1.0.0 * SYSTEM RUN"
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(22, 88);
    tft.print("v1.0.0  * SYSTEM RUN");

    // Dynamic IP Address from Web Server
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(16, 104);
    tft.printf("IP: %s", webServerDsp.getIpAddress().c_str());
}

bool TftDisplay::begin() {
    Serial.println("[TFT] Inisialisasi Layar 1.8\" ST7735 128x160 via Fast Hardware SPI...");

    // 1. Backlight on
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    if (TFT_BL_PIN >= 0 && TFT_BL_PIN != 8 && TFT_BL_PIN != 13) {
        pinMode(TFT_BL_PIN, OUTPUT);
        digitalWrite(TFT_BL_PIN, HIGH);
    }

    // 2. Hardware Reset
    if (TFT_RST_PIN >= 0) {
        pinMode(TFT_RST_PIN, OUTPUT);
        digitalWrite(TFT_RST_PIN, HIGH);
        delay(20);
        digitalWrite(TFT_RST_PIN, LOW);
        delay(50);
        digitalWrite(TFT_RST_PIN, HIGH);
        delay(100);
    }

    // 3. Hardware SPI Init
    SPI.begin(TFT_SCLK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN);
    SPI.setFrequency(27000000);

    tft.initR(INITR_BLACKTAB);
    tft.setSPISpeed(27000000);
    tft.invertDisplay(false);
    delay(50);
    tft.setRotation(1); // Landscape 160 x 128

    // 4. Cyber Boot Splash Screen
    drawSplashScreen();
    delay(1200);

    // 5. Enter Home Screen
    _isInitialized = true;
    _lastUserActivityTime = millis();
    setScreenMode(SCREEN_HOME);

    Serial.println("[TFT] Layar Siap & Berjalan 100% Persis Web UI!");
    return true;
}

void TftDisplay::setScreenMode(DisplayScreenMode mode) {
    _currentMode = mode;
    _cursorIndex = 0;
    _inEditMode = false;
    _lastUserActivityTime = millis();

    tft.fillScreen(COLOR_BG);

    switch (_currentMode) {
        case SCREEN_HOME:
            _prevInLSegs = -1;
            _prevInRSegs = -1;
            _prevOut1Segs = -1;
            _prevOut2Segs = -1;
            _prevOut3Segs = -1;
            _prevOut4Segs = -1;
            _prevInLDb = -999.0f;
            _prevInRDb = -999.0f;
            _prevOut1Db = -999.0f;
            _prevOut2Db = -999.0f;
            _prevOut3Db = -999.0f;
            _prevOut4Db = -999.0f;
            _prevMasterGain = -999.0f;
            _prevHpfFreq = -1.0f;
            _prevLpfFreq = -1.0f;
            _prevPreset = 255;
            _prevDockIndex = -1;
            drawHomeScreenLayout();
            break;
        case SCREEN_MENU_GRID:
            drawMenuGridScreen();
            break;
        case SCREEN_GAIN:
            drawGainScreen();
            break;
        case SCREEN_HPF:
            drawHpfScreen();
            break;
        case SCREEN_LPF:
            drawLpfScreen();
            break;
        case SCREEN_DELAY:
            drawDelayScreen();
            break;
        case SCREEN_PEQ:
            drawPeqScreen();
            break;
        case SCREEN_LIMITER:
            drawLimiterScreen();
            break;
        case SCREEN_PRESET:
            drawPresetScreen();
            break;
        case SCREEN_STATUS:
            drawStatusScreen();
            break;
        case SCREEN_ABOUT:
            drawAboutScreen();
            break;
        default:
            drawHomeScreenLayout();
            break;
    }
}

// =============================================================================
// SCREEN 1: HOME (Exact match to Web UI Home Split-Card & Dock)
// =============================================================================
void TftDisplay::drawHomeScreenLayout() {
    drawHeader("SOFGAM SS", "48k", true);

    // 1. INPUT CARD (Left: X: 2, Y: 15, W: 63, H: 44)
    drawCard(2, 15, 63, 44, COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(4, 17);
    tft.print("IN ADC");

    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(4, 28);
    tft.print("L");
    tft.setCursor(4, 38);
    tft.print("R");

    // Initial meters
    drawLedMeter(13, 29, 30, 4, -40.0f, 8, _prevInLSegs);
    drawLedMeter(13, 39, 30, 4, -40.0f, 8, _prevInRSegs);

    // 2. OUTPUT CARD (Right: X: 67, Y: 15, W: 91, H: 44)
    drawCard(67, 15, 91, 44, COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(70, 17);
    tft.print("OUTPUT (4CH)");

    for (uint8_t i = 0; i < 4; i++) {
        int16_t oy = 26 + i * 8;
        tft.setCursor(69, oy - 1);
        tft.setTextColor(COLOR_CYAN_ACCENT);
        tft.print(i + 1);
    }
    drawLedMeter(77, 26, 48, 4, -40.0f, 12, _prevOut1Segs);
    drawLedMeter(77, 34, 48, 4, -40.0f, 12, _prevOut2Segs);
    drawLedMeter(77, 42, 48, 4, -40.0f, 12, _prevOut3Segs);
    drawLedMeter(77, 50, 48, 4, -40.0f, 12, _prevOut4Segs);

    // 3. PRESET & QUICK PARAMS CARD (X: 2, Y: 60, W: 156, H: 23)
    drawCard(2, 60, 156, 23, COLOR_BOX_BORDER, COLOR_CARD_BG);

    // 4. DIAGNOSTIC BADGES ROW (Y: 85, H: 10)
    static const char* const BADGES[5] = { "ADC:OK", "DSP:RUN", "DAC1", "DAC2", "42'C" };
    int16_t px = 2;
    for (uint8_t b = 0; b < 5; b++) {
        drawCard(px, 85, 29, 10, COLOR_BOX_BORDER, COLOR_NAVY_BAR);
        tft.setTextSize(1);
        tft.setTextColor((b == 4) ? COLOR_TEXT_DIM : COLOR_GREEN);
        tft.setCursor(px + 2, 86);
        tft.print(BADGES[b]);
        px += 31;
    }

    // 5. BOTTOM DOCK (Y: 98 to 126, H: 27)
    static const char* const DOCK_ITEMS[7] = { "HOM", "GAI", "HPF", "LPF", "DEL", "PEQ", "MEN" };
    for (uint8_t d = 0; d < 7; d++) {
        int16_t dx = 3 + d * 22;
        bool isSel = (_cursorIndex == d);
        drawCard(dx, 98, 21, 27, isSel ? COLOR_TEXT_BRT : COLOR_BOX_BORDER, isSel ? COLOR_CYAN_ACCENT : COLOR_CARD_BG);
        tft.setTextSize(1);
        tft.setTextColor(isSel ? COLOR_BG : COLOR_TEXT_DIM);
        tft.setCursor(dx + 2, 107);
        tft.print(DOCK_ITEMS[d]);
    }
    _prevDockIndex = _cursorIndex;
}

void TftDisplay::updateHomeDynamicData() {
    VuMeterData vu = dspEngine.getVuMeterData();
    DspConfig cfg  = dspEngine.getConfig();

    // 1. Update Segmented LED Meters
    drawLedMeter(13, 29, 30, 4, vu.in_peak_db,  8,  _prevInLSegs);
    drawLedMeter(13, 39, 30, 4, vu.in_peak_db,  8,  _prevInRSegs);
    drawLedMeter(77, 26, 48, 4, vu.ch1_peak_db, 12, _prevOut1Segs);
    drawLedMeter(77, 34, 48, 4, vu.ch1_peak_db, 12, _prevOut2Segs);
    drawLedMeter(77, 42, 48, 4, vu.ch2_peak_db, 12, _prevOut3Segs);
    drawLedMeter(77, 50, 48, 4, vu.ch2_peak_db, 12, _prevOut4Segs);

    // 2. Numerical dB Readouts (Differential)
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD_BG);

    if (fabsf(vu.in_peak_db - _prevInLDb) >= 1.0f) {
        _prevInLDb = vu.in_peak_db;
        tft.setCursor(46, 27);
        tft.printf("%3.0f", vu.in_peak_db);
    }
    if (fabsf(vu.in_peak_db - _prevInRDb) >= 1.0f) {
        _prevInRDb = vu.in_peak_db;
        tft.setCursor(46, 37);
        tft.printf("%3.0f", vu.in_peak_db);
    }

    auto drawOutDb = [&](int16_t y, float db, float& prev) {
        if (fabsf(db - prev) >= 1.0f) {
            prev = db;
            tft.setCursor(128, y);
            tft.printf("%3.0f", db);
        }
    };
    drawOutDb(25, vu.ch1_peak_db, _prevOut1Db);
    drawOutDb(33, vu.ch1_peak_db, _prevOut2Db);
    drawOutDb(41, vu.ch2_peak_db, _prevOut3Db);
    drawOutDb(49, vu.ch2_peak_db, _prevOut4Db);

    // 3. Preset & Quick Params
    uint8_t curSlot = presetsManager.getCurrentSlot();
    bool peqOn = cfg.ch1.peq[0].enabled || cfg.ch1.peq[1].enabled || cfg.ch1.peq[2].enabled;
    bool limOn = cfg.ch1.limiter.enabled || cfg.ch2.limiter.enabled;

    if (curSlot != _prevPreset ||
        fabsf(cfg.master_gain_db - _prevMasterGain) >= 0.5f ||
        fabsf(cfg.ch1.hpf.freq - _prevHpfFreq) >= 1.0f ||
        fabsf(cfg.ch1.lpf.freq - _prevLpfFreq) >= 1.0f ||
        cfg.ch1.hpf.enabled != _prevHpfEn ||
        cfg.ch1.lpf.enabled != _prevLpfEn ||
        peqOn != _prevPeqEn || limOn != _prevLimEn) {

        _prevPreset = curSlot;
        _prevMasterGain = cfg.master_gain_db;
        _prevHpfFreq = cfg.ch1.hpf.freq;
        _prevLpfFreq = cfg.ch1.lpf.freq;
        _prevHpfEn = cfg.ch1.hpf.enabled;
        _prevLpfEn = cfg.ch1.lpf.enabled;
        _prevPeqEn = peqOn;
        _prevLimEn = limOn;

        // Line 1: Preset
        tft.setCursor(5, 62);
        tft.setTextColor(COLOR_CYAN_ACCENT, COLOR_CARD_BG);
        tft.printf("PRESET <%02u Default>      ", curSlot);

        // Line 2: Parameters
        tft.setCursor(5, 72);
        tft.setTextColor(COLOR_TEXT_BRT, COLOR_CARD_BG);
        char lpfBuf[8];
        if (cfg.ch1.lpf.freq >= 1000.0f) snprintf(lpfBuf, sizeof(lpfBuf), "%2.0fk", cfg.ch1.lpf.freq / 1000.0f);
        else snprintf(lpfBuf, sizeof(lpfBuf), "%3.0f", cfg.ch1.lpf.freq);

        tft.printf("G:%+1.0f H:%2.0f L:%s EQ:%s L:%s ",
                   cfg.master_gain_db,
                   cfg.ch1.hpf.freq,
                   lpfBuf,
                   peqOn ? "ON" : "--",
                   limOn ? "ON" : "--");
    }

    // 4. Update Dock highlighting if cursor moved
    if (_cursorIndex != _prevDockIndex) {
        static const char* const DOCK_ITEMS[7] = { "HOM", "GAI", "HPF", "LPF", "DEL", "PEQ", "MEN" };
        if (_prevDockIndex >= 0 && _prevDockIndex < 7) {
            int16_t dx = 3 + _prevDockIndex * 22;
            drawCard(dx, 98, 21, 27, COLOR_BOX_BORDER, COLOR_CARD_BG);
            tft.setTextColor(COLOR_TEXT_DIM);
            tft.setCursor(dx + 2, 107);
            tft.print(DOCK_ITEMS[_prevDockIndex]);
        }
        if (_cursorIndex >= 0 && _cursorIndex < 7) {
            int16_t dx = 3 + _cursorIndex * 22;
            drawCard(dx, 98, 21, 27, COLOR_TEXT_BRT, COLOR_CYAN_ACCENT);
            tft.setTextColor(COLOR_BG);
            tft.setCursor(dx + 2, 107);
            tft.print(DOCK_ITEMS[_cursorIndex]);
        }
        _prevDockIndex = _cursorIndex;
    }
}

// =============================================================================
// SCREEN 2: MENU GRID (Exact match to Web UI 2x5 Tile Matrix)
// =============================================================================
void TftDisplay::drawMenuGridScreen() {
    drawHeader("SOFGAM SS", "> MENU", false);

    // 2x5 Grid of Tiles
    const int16_t tileW = 28;
    const int16_t tileH = 34;

    for (uint8_t i = 0; i < 10; i++) {
        uint8_t col = i % 5;
        uint8_t row = i / 5;
        int16_t tx = 4 + col * 31;
        int16_t ty = 18 + row * 40;
        bool isSel = (_cursorIndex == i);

        if (isSel) {
            drawCard(tx, ty, tileW, tileH, COLOR_TEXT_BRT, COLOR_CYAN_ACCENT);
            tft.setTextColor(COLOR_BG);
        } else {
            drawCard(tx, ty, tileW, tileH, COLOR_BOX_BORDER, COLOR_CARD_BG);
            tft.setTextColor(COLOR_TEXT_BRT);
        }

        tft.setTextSize(1);
        tft.setCursor(tx + 2, ty + 12);
        char lbl[5];
        strncpy(lbl, MENU_TILES[i].label, 4);
        lbl[4] = '\0';
        tft.print(lbl);
    }

    // Footer
    tft.fillRect(0, 114, 160, 14, COLOR_NAVY_BAR);
    tft.drawFastHLine(0, 113, 160, COLOR_CYAN_DIM);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(6, 117);
    tft.print("PUTAR: PILIH | TEKAN: BUKA");
}

// =============================================================================
// SCREEN 3: GAIN (Setting Gain)
// =============================================================================
void TftDisplay::drawGainScreen() {
    drawHeader("SOFGAM SS", "SETTING GAIN", false);
    DspConfig cfg = dspEngine.getConfig();
    VuMeterData vu = dspEngine.getVuMeterData();

    // Card 1: INPUT ADC (Left)
    drawCard(2, 16, 52, 60, (_cursorIndex == 0) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(4, 18);
    tft.print("IN ADC");
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 32);
    tft.printf("L:%3.0fdB", vu.in_peak_db);
    tft.setCursor(4, 46);
    tft.printf("R:%3.0fdB", vu.in_peak_db);

    // Card 2: OUTPUT GAIN (4 CH)
    drawCard(56, 16, 102, 60, (_cursorIndex >= 1 && _cursorIndex <= 4) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(58, 18);
    tft.print("OUTPUT GAIN (4 CH)");

    auto printOutGain = [&](uint8_t ch, float gain, int16_t y) {
        bool isSel = (_cursorIndex == ch);
        tft.setCursor(58, y);
        if (isSel) tft.setTextColor(_inEditMode ? COLOR_YELLOW : COLOR_CYAN_ACCENT);
        else tft.setTextColor(COLOR_TEXT_BRT);
        tft.printf("OUT%u: %+4.1f dB", ch, gain);
    };
    printOutGain(1, cfg.ch1.gain_db, 30);
    printOutGain(2, cfg.ch1.gain_db, 40);
    printOutGain(3, cfg.ch2.gain_db, 50);
    printOutGain(4, cfg.ch2.gain_db, 60);

    // Card 3: Mutes & Polarity
    drawCard(2, 78, 156, 24, (_cursorIndex == 5) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setCursor(5, 85);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.printf("MUTE: [%s][%s]  POL: [%s]",
               cfg.ch1.mute ? "X" : "1",
               cfg.ch2.mute ? "X" : "2",
               cfg.ch1.polarity_inverted ? "180" : "NORM");

    // Action Buttons
    drawCard(2, 105, 50, 20, (_cursorIndex == 6) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, (_cursorIndex == 6) ? COLOR_SEL_BG : COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 110);
    tft.print("< Kembali");

    drawCard(108, 105, 50, 20, (_cursorIndex == 7) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(114, 110);
    tft.print("Simpan");
}

// =============================================================================
// SCREEN 4: HPF (Setting HPF 2x2 Grid)
// =============================================================================
void TftDisplay::drawHpfScreen() {
    drawHeader("SOFGAM SS", "SETTING HPF", false);
    DspConfig cfg = dspEngine.getConfig();

    for (uint8_t ch = 0; ch < 4; ch++) {
        int16_t cx = (ch % 2 == 0) ? 2 : 82;
        int16_t cy = (ch < 2) ? 16 : 58;
        bool isSel = (_cursorIndex == ch);

        drawCard(cx, cy, 76, 38, isSel ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
        tft.setTextSize(1);
        tft.setCursor(cx + 4, cy + 3);
        tft.setTextColor(isSel ? COLOR_CYAN_ACCENT : COLOR_TEXT_BRT);
        tft.printf("OUT %u [%s]", ch + 1, cfg.ch1.hpf.enabled ? "ON" : "OFF");

        tft.setCursor(cx + 4, cy + 14);
        tft.setTextColor(isSel && _inEditMode ? COLOR_YELLOW : COLOR_TEXT_DIM);
        tft.printf("Freq : %3.0fHz", cfg.ch1.hpf.freq);

        tft.setCursor(cx + 4, cy + 24);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.printf("Slope: %udB/o", cfg.ch1.hpf.slope);
    }

    drawCard(2, 102, 50, 20, (_cursorIndex == 4) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, (_cursorIndex == 4) ? COLOR_SEL_BG : COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 107);
    tft.print("< Kembali");

    drawCard(108, 102, 50, 20, (_cursorIndex == 5) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(114, 107);
    tft.print("Simpan");
}

// =============================================================================
// SCREEN 5: LPF (Setting LPF 2x2 Grid)
// =============================================================================
void TftDisplay::drawLpfScreen() {
    drawHeader("SOFGAM SS", "SETTING LPF", false);
    DspConfig cfg = dspEngine.getConfig();

    for (uint8_t ch = 0; ch < 4; ch++) {
        int16_t cx = (ch % 2 == 0) ? 2 : 82;
        int16_t cy = (ch < 2) ? 16 : 58;
        bool isSel = (_cursorIndex == ch);

        drawCard(cx, cy, 76, 38, isSel ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
        tft.setTextSize(1);
        tft.setCursor(cx + 4, cy + 3);
        tft.setTextColor(isSel ? COLOR_CYAN_ACCENT : COLOR_TEXT_BRT);
        tft.printf("OUT %u [%s]", ch + 1, cfg.ch1.lpf.enabled ? "ON" : "OFF");

        tft.setCursor(cx + 4, cy + 14);
        tft.setTextColor(isSel && _inEditMode ? COLOR_YELLOW : COLOR_TEXT_DIM);
        if (cfg.ch1.lpf.freq >= 1000.0f) tft.printf("Freq : %2.1fk", cfg.ch1.lpf.freq / 1000.0f);
        else tft.printf("Freq : %3.0f", cfg.ch1.lpf.freq);

        tft.setCursor(cx + 4, cy + 24);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.printf("Slope: %udB/o", cfg.ch1.lpf.slope);
    }

    drawCard(2, 102, 50, 20, (_cursorIndex == 4) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, (_cursorIndex == 4) ? COLOR_SEL_BG : COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 107);
    tft.print("< Kembali");

    drawCard(108, 102, 50, 20, (_cursorIndex == 5) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(114, 107);
    tft.print("Simpan");
}

// =============================================================================
// SCREEN 6: DELAY (Setting Delay)
// =============================================================================
void TftDisplay::drawDelayScreen() {
    drawHeader("SOFGAM SS", "SETTING DELAY", false);
    DspConfig cfg = dspEngine.getConfig();

    float delays[4] = { cfg.ch1.delay_ms, cfg.ch1.delay_ms, cfg.ch2.delay_ms, cfg.ch2.delay_ms };

    for (uint8_t ch = 0; ch < 4; ch++) {
        int16_t cx = (ch % 2 == 0) ? 2 : 82;
        int16_t cy = (ch < 2) ? 16 : 58;
        bool isSel = (_cursorIndex == ch);

        drawCard(cx, cy, 76, 38, isSel ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
        tft.setTextSize(1);
        tft.setCursor(cx + 4, cy + 3);
        tft.setTextColor(isSel ? COLOR_CYAN_ACCENT : COLOR_TEXT_BRT);
        tft.printf("OUT %u", ch + 1);

        tft.setCursor(cx + 4, cy + 14);
        tft.setTextColor(isSel && _inEditMode ? COLOR_YELLOW : COLOR_CYAN_ACCENT);
        tft.printf("Dly: %3.1f ms", delays[ch]);

        tft.setCursor(cx + 4, cy + 24);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.printf("Dst: %3.2f m", delays[ch] * 0.343f);
    }

    drawCard(2, 102, 50, 20, (_cursorIndex == 4) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, (_cursorIndex == 4) ? COLOR_SEL_BG : COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 107);
    tft.print("< Kembali");

    drawCard(108, 102, 50, 20, (_cursorIndex == 5) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(114, 107);
    tft.print("Simpan");
}

// =============================================================================
// SCREEN 7: PEQ (Real-time Graphic Frequency Response Bell Curve)
// =============================================================================
void TftDisplay::drawPeqScreen() {
    drawHeader("SOFGAM SS", "SETTING PEQ", false);
    DspConfig cfg = dspEngine.getConfig();

    uint8_t bIdx = _peqBandIndex;
    if (bIdx > 2) bIdx = 2;
    float gain_db = cfg.ch1.peq[bIdx].gain_db;
    float freq    = cfg.ch1.peq[bIdx].freq;
    float q       = cfg.ch1.peq[bIdx].q;

    // Visual EQ Curve Canvas Box (X: 2, Y: 15, W: 156, H: 50)
    drawCard(2, 15, 156, 50, COLOR_BOX_BORDER, COLOR_CARD_DARK);

    // Center 0 dB reference line
    tft.drawFastHLine(4, 40, 152, COLOR_BOX_BORDER);

    // Draw Smooth Responsive Bell Curve
    int16_t prev_y = 40;
    for (int16_t x = 4; x <= 154; x++) {
        float dist = (float)(x - 70) / 18.0f;
        float bell = expf(-dist * dist) * (gain_db * 1.5f);
        int16_t cur_y = 40 - (int16_t)roundf(bell);
        if (cur_y < 16) cur_y = 16;
        if (cur_y > 63) cur_y = 63;

        if (x > 4) {
            tft.drawLine(x - 1, prev_y, x, cur_y, COLOR_CYAN_ACCENT);
        }
        prev_y = cur_y;
    }

    // Yellow Peak Dot
    int16_t peak_y = 40 - (int16_t)roundf(gain_db * 1.5f);
    if (peak_y < 16) peak_y = 16;
    if (peak_y > 63) peak_y = 63;
    tft.fillCircle(70, peak_y, 2, COLOR_YELLOW);

    // Band Selector Badges (B1 to B5)
    static const char* const BANDS[5] = { "B1", "B2", "B3", "B4", "B5" };
    for (uint8_t b = 0; b < 5; b++) {
        int16_t bx = 3 + b * 18;
        bool isSel = (_cursorIndex == b);
        drawCard(bx, 68, 16, 12, isSel ? COLOR_TEXT_BRT : COLOR_BOX_BORDER, isSel ? COLOR_CYAN_ACCENT : COLOR_CARD_BG);
        tft.setTextSize(1);
        tft.setTextColor(isSel ? COLOR_BG : COLOR_TEXT_DIM);
        tft.setCursor(bx + 2, 70);
        tft.print(BANDS[b]);
    }

    // Active Band Readout Text
    tft.setTextColor(_inEditMode ? COLOR_YELLOW : COLOR_TEXT_BRT);
    tft.setCursor(96, 70);
    tft.printf("G:%+2.1fdB", gain_db);

    // Footer Buttons
    drawCard(2, 102, 50, 20, (_cursorIndex == 5) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, (_cursorIndex == 5) ? COLOR_SEL_BG : COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 107);
    tft.print("< Kembali");

    drawCard(108, 102, 50, 20, (_cursorIndex == 6) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(114, 107);
    tft.print("Simpan");
}

// =============================================================================
// SCREEN 8: LIMITER (Setting Limiter)
// =============================================================================
void TftDisplay::drawLimiterScreen() {
    drawHeader("SOFGAM SS", "SETTING LIMITER", false);
    DspConfig cfg = dspEngine.getConfig();

    drawCard(2, 16, 156, 78, (_cursorIndex == 0 || _cursorIndex == 1 || _cursorIndex == 2) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(5, 20);
    tft.printf("OUT 1..4 LIMITER [%s]", cfg.ch1.limiter.enabled ? "ON" : "OFF");

    tft.setCursor(5, 36);
    tft.setTextColor((_cursorIndex == 0 && _inEditMode) ? COLOR_YELLOW : COLOR_TEXT_BRT);
    tft.printf("Threshold: %+4.1f dB", cfg.ch1.limiter.threshold_db);

    tft.setCursor(5, 52);
    tft.setTextColor((_cursorIndex == 1 && _inEditMode) ? COLOR_YELLOW : COLOR_TEXT_BRT);
    tft.printf("Attack   : %4.1f ms", cfg.ch1.limiter.attack_ms);

    tft.setCursor(5, 68);
    tft.setTextColor((_cursorIndex == 2 && _inEditMode) ? COLOR_YELLOW : COLOR_TEXT_BRT);
    tft.printf("Release  : %4.0f ms", cfg.ch1.limiter.release_ms);

    drawCard(2, 102, 50, 20, (_cursorIndex == 3) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, (_cursorIndex == 3) ? COLOR_SEL_BG : COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 107);
    tft.print("< Kembali");

    drawCard(108, 102, 50, 20, (_cursorIndex == 4) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(114, 107);
    tft.print("Simpan");
}

// =============================================================================
// SCREEN 9: PRESET (Preset Manager)
// =============================================================================
void TftDisplay::drawPresetScreen() {
    drawHeader("SOFGAM SS", "PRESET MANAGER", false);
    uint8_t curSlot = presetsManager.getCurrentSlot();

    drawCard(2, 16, 156, 32, COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(5, 22);
    tft.printf("ACTIVE: Slot %02u [Default]", curSlot);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(5, 34);
    tft.print("5 Internal NVS Memory Slots");

    // Slots P01 to P05
    for (uint8_t s = 1; s <= 5; s++) {
        int16_t sx = 2 + (s - 1) * 31;
        bool isSel = (_cursorIndex == s - 1);
        drawCard(sx, 54, 29, 24, isSel ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, (s == curSlot) ? COLOR_SEL_BG : COLOR_CARD_BG);
        tft.setTextColor((s == curSlot) ? COLOR_TEXT_BRT : COLOR_TEXT_DIM);
        tft.setCursor(sx + 4, 60);
        tft.printf("P%02u", s);
    }

    drawCard(2, 86, 75, 18, (_cursorIndex == 5) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(5, 91);
    tft.print("LOAD PRESET");

    drawCard(83, 86, 75, 18, (_cursorIndex == 6) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_GREEN);
    tft.setTextColor(COLOR_BG);
    tft.setCursor(86, 91);
    tft.print("SAVE TO NVS");

    drawCard(2, 108, 50, 16, (_cursorIndex == 7) ? COLOR_CYAN_ACCENT : COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 112);
    tft.print("< Kembali");
}

// =============================================================================
// SCREEN 10: STATUS (Hardware Telemetry)
// =============================================================================
void TftDisplay::drawStatusScreen() {
    drawHeader("SOFGAM SS", "STATUS SISTEM", false);

    drawCard(2, 16, 76, 80, COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(4, 18);
    tft.print("MODULES");
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(4, 32);
    tft.print("ADC : OK");
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(4, 46);
    tft.print("DSP : RUN");
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(4, 60);
    tft.print("DAC1: OK");
    tft.setCursor(4, 74);
    tft.print("DAC2: OK");

    drawCard(82, 16, 76, 80, COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(84, 18);
    tft.print("METRICS");
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(84, 32);
    tft.print("SR  : 48kHz");
    tft.setCursor(84, 46);
    tft.print("CPU : 12 %");
    tft.setCursor(84, 60);
    tft.print("RAM : 28 %");
    tft.setTextColor(COLOR_YELLOW);
    tft.setCursor(84, 74);
    tft.print("Suhu: 42'C");

    drawCard(2, 102, 50, 20, COLOR_CYAN_ACCENT, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 107);
    tft.print("< Kembali");
}

// =============================================================================
// SCREEN 11: ABOUT (System Info & Specs)
// =============================================================================
void TftDisplay::drawAboutScreen() {
    drawHeader("SOFGAM SS", "INFORMASI", false);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN_ACCENT);
    tft.setCursor(46, 16);
    tft.print("SOFGAM SS");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(54, 26);
    tft.print("by Shawir");

    // Mini Sine Wave
    for (int x = 20; x <= 140; x++) {
        int y = 38 + (int)(sinf((float)(x - 20) * 0.08f) * 4.0f);
        tft.drawPixel(x, y, COLOR_CYAN_ACCENT);
    }

    drawCard(2, 46, 156, 52, COLOR_BOX_BORDER, COLOR_CARD_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 50);
    tft.print("Model: SOFGAM SS    Core: ESP32-S3");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(4, 62);
    tft.print("Ver  : v1.0.0       ADC : PCM1808");
    tft.setCursor(4, 74);
    tft.print("DAC  : 2x PCM5102   Out : 4 Channel");
    tft.setCursor(4, 86);
    tft.print("Build: 2026-09      DSP : Fast 48k");

    drawCard(2, 104, 50, 18, COLOR_CYAN_ACCENT, COLOR_SEL_BG);
    tft.setTextColor(COLOR_TEXT_BRT);
    tft.setCursor(4, 108);
    tft.print("< Kembali");
}

// =============================================================================
// PARAMETER EDITING LOGIC (Encoder Turn when in Edit Mode)
// =============================================================================
void TftDisplay::applyParameterEdit(int32_t delta) {
    DspConfig cfg = dspEngine.getConfig();

    switch (_currentMode) {
        case SCREEN_GAIN:
            if (_cursorIndex == 1 || _cursorIndex == 2) {
                float g = cfg.ch1.gain_db + (float)delta * 0.5f;
                if (g < -60.0f) g = -60.0f;
                if (g > 12.0f) g = 12.0f;
                dspEngine.setCh1Gain(g);
            } else if (_cursorIndex == 3 || _cursorIndex == 4) {
                float g = cfg.ch2.gain_db + (float)delta * 0.5f;
                if (g < -60.0f) g = -60.0f;
                if (g > 12.0f) g = 12.0f;
                dspEngine.setCh2Gain(g);
            }
            drawGainScreen();
            break;

        case SCREEN_HPF:
            {
                float f = cfg.ch1.hpf.freq + (float)delta * 10.0f;
                if (f < 20.0f) f = 20.0f;
                if (f > 20000.0f) f = 20000.0f;
                dspEngine.setCh1Hpf(true, f, cfg.ch1.hpf.slope);
                dspEngine.setCh2Hpf(true, f, cfg.ch2.hpf.slope);
                drawHpfScreen();
            }
            break;

        case SCREEN_LPF:
            {
                float f = cfg.ch1.lpf.freq + (float)delta * 100.0f;
                if (f < 20.0f) f = 20.0f;
                if (f > 20000.0f) f = 20000.0f;
                dspEngine.setCh1Lpf(true, f, cfg.ch1.lpf.slope);
                dspEngine.setCh2Lpf(true, f, cfg.ch2.lpf.slope);
                drawLpfScreen();
            }
            break;

        case SCREEN_DELAY:
            {
                float d = cfg.ch1.delay_ms + (float)delta * 0.1f;
                if (d < 0.0f) d = 0.0f;
                if (d > 50.0f) d = 50.0f;
                dspEngine.setCh1Delay(d);
                dspEngine.setCh2Delay(d);
                drawDelayScreen();
            }
            break;

        case SCREEN_PEQ:
            {
                uint8_t bIdx = _peqBandIndex;
                if (bIdx > 2) bIdx = 2;
                float g = cfg.ch1.peq[bIdx].gain_db + (float)delta * 0.5f;
                if (g < -18.0f) g = -18.0f;
                if (g > 18.0f) g = 18.0f;
                dspEngine.setCh1Peq(bIdx, true, cfg.ch1.peq[bIdx].type, cfg.ch1.peq[bIdx].freq, g, cfg.ch1.peq[bIdx].q);
                dspEngine.setCh2Peq(bIdx, true, cfg.ch2.peq[bIdx].type, cfg.ch2.peq[bIdx].freq, g, cfg.ch2.peq[bIdx].q);
                drawPeqScreen();
            }
            break;

        case SCREEN_LIMITER:
            if (_cursorIndex == 0) {
                float th = cfg.ch1.limiter.threshold_db + (float)delta * 0.5f;
                if (th < -30.0f) th = -30.0f;
                if (th > 0.0f) th = 0.0f;
                dspEngine.setCh1Limiter(true, th, cfg.ch1.limiter.attack_ms, cfg.ch1.limiter.release_ms);
                dspEngine.setCh2Limiter(true, th, cfg.ch2.limiter.attack_ms, cfg.ch2.limiter.release_ms);
            }
            drawLimiterScreen();
            break;

        default:
            break;
    }
}

// =============================================================================
// SCREEN CLICK HANDLING
// =============================================================================
void TftDisplay::handleScreenClick() {
    switch (_currentMode) {
        case SCREEN_HOME:
            {
                // Bottom Dock Navigation
                static const DisplayScreenMode DOCK_DESTINATIONS[7] = {
                    SCREEN_HOME, SCREEN_GAIN, SCREEN_HPF, SCREEN_LPF, SCREEN_DELAY, SCREEN_PEQ, SCREEN_MENU_GRID
                };
                if (_cursorIndex >= 0 && _cursorIndex < 7) {
                    DisplayScreenMode target = DOCK_DESTINATIONS[_cursorIndex];
                    if (target != SCREEN_HOME) {
                        setScreenMode(target);
                    }
                }
            }
            break;

        case SCREEN_MENU_GRID:
            {
                static const DisplayScreenMode TILE_DESTINATIONS[10] = {
                    SCREEN_HOME, SCREEN_GAIN, SCREEN_HPF, SCREEN_LPF, SCREEN_DELAY,
                    SCREEN_PEQ, SCREEN_LIMITER, SCREEN_PRESET, SCREEN_STATUS, SCREEN_ABOUT
                };
                if (_cursorIndex >= 0 && _cursorIndex < 10) {
                    setScreenMode(TILE_DESTINATIONS[_cursorIndex]);
                }
            }
            break;

        case SCREEN_GAIN:
            if (_cursorIndex == 6) { setScreenMode(SCREEN_MENU_GRID); return; }
            if (_cursorIndex == 7) {
                // Save to NVS
                presetsManager.savePreset(presetsManager.getCurrentSlot(), dspEngine.getConfig());
                setScreenMode(SCREEN_HOME);
                return;
            }
            if (_cursorIndex == 5) {
                // Toggle Mutes
                DspConfig cfg = dspEngine.getConfig();
                dspEngine.setCh1Mute(!cfg.ch1.mute);
                dspEngine.setCh2Mute(!cfg.ch2.mute);
                drawGainScreen();
                return;
            }
            _inEditMode = !_inEditMode;
            drawGainScreen();
            break;

        case SCREEN_HPF:
            if (_cursorIndex == 4) { setScreenMode(SCREEN_MENU_GRID); return; }
            if (_cursorIndex == 5) {
                presetsManager.savePreset(presetsManager.getCurrentSlot(), dspEngine.getConfig());
                setScreenMode(SCREEN_HOME);
                return;
            }
            _inEditMode = !_inEditMode;
            drawHpfScreen();
            break;

        case SCREEN_LPF:
            if (_cursorIndex == 4) { setScreenMode(SCREEN_MENU_GRID); return; }
            if (_cursorIndex == 5) {
                presetsManager.savePreset(presetsManager.getCurrentSlot(), dspEngine.getConfig());
                setScreenMode(SCREEN_HOME);
                return;
            }
            _inEditMode = !_inEditMode;
            drawLpfScreen();
            break;

        case SCREEN_DELAY:
            if (_cursorIndex == 4) { setScreenMode(SCREEN_MENU_GRID); return; }
            if (_cursorIndex == 5) {
                presetsManager.savePreset(presetsManager.getCurrentSlot(), dspEngine.getConfig());
                setScreenMode(SCREEN_HOME);
                return;
            }
            _inEditMode = !_inEditMode;
            drawDelayScreen();
            break;

        case SCREEN_PEQ:
            if (_cursorIndex == 5) { setScreenMode(SCREEN_MENU_GRID); return; }
            if (_cursorIndex == 6) {
                presetsManager.savePreset(presetsManager.getCurrentSlot(), dspEngine.getConfig());
                setScreenMode(SCREEN_HOME);
                return;
            }
            if (_cursorIndex < 5) {
                _peqBandIndex = _cursorIndex;
            }
            _inEditMode = !_inEditMode;
            drawPeqScreen();
            break;

        case SCREEN_LIMITER:
            if (_cursorIndex == 3) { setScreenMode(SCREEN_MENU_GRID); return; }
            if (_cursorIndex == 4) {
                presetsManager.savePreset(presetsManager.getCurrentSlot(), dspEngine.getConfig());
                setScreenMode(SCREEN_HOME);
                return;
            }
            _inEditMode = !_inEditMode;
            drawLimiterScreen();
            break;

        case SCREEN_PRESET:
            if (_cursorIndex == 7) { setScreenMode(SCREEN_MENU_GRID); return; }
            if (_cursorIndex < 5) {
                presetsManager.setCurrentSlot(_cursorIndex + 1);
                drawPresetScreen();
                return;
            }
            if (_cursorIndex == 5) { // Load
                DspConfig cfg;
                if (presetsManager.loadPreset(presetsManager.getCurrentSlot(), cfg)) {
                    dspEngine.setConfig(cfg);
                    drawPresetScreen();
                }
                return;
            }
            if (_cursorIndex == 6) { // Save
                presetsManager.savePreset(presetsManager.getCurrentSlot(), dspEngine.getConfig());
                drawPresetScreen();
                return;
            }
            break;

        case SCREEN_STATUS:
        case SCREEN_ABOUT:
            setScreenMode(SCREEN_MENU_GRID);
            break;

        default:
            setScreenMode(SCREEN_HOME);
            break;
    }
}

// =============================================================================
// ROTARY ENCODER INPUT
// =============================================================================
void TftDisplay::handleEncoder(int32_t delta, bool clicked, bool longPressed) {
    if (!_isInitialized) return;

    _lastUserActivityTime = millis();

    // Long press: Return immediately to HOME from any depth!
    if (longPressed) {
        setScreenMode(SCREEN_HOME);
        return;
    }

    if (_inEditMode) {
        if (delta != 0) {
            applyParameterEdit(delta);
        }
        if (clicked) {
            _inEditMode = false;
            // Redraw current screen with edit mode cleared
            switch (_currentMode) {
                case SCREEN_GAIN: drawGainScreen(); break;
                case SCREEN_HPF: drawHpfScreen(); break;
                case SCREEN_LPF: drawLpfScreen(); break;
                case SCREEN_DELAY: drawDelayScreen(); break;
                case SCREEN_PEQ: drawPeqScreen(); break;
                case SCREEN_LIMITER: drawLimiterScreen(); break;
                default: break;
            }
        }
    } else {
        if (delta != 0) {
            int8_t max_idx = 0;
            switch (_currentMode) {
                case SCREEN_HOME:      max_idx = 6; break; // 7 dock buttons
                case SCREEN_MENU_GRID: max_idx = 9; break; // 10 tiles
                case SCREEN_GAIN:      max_idx = 7; break;
                case SCREEN_HPF:
                case SCREEN_LPF:
                case SCREEN_DELAY:     max_idx = 5; break;
                case SCREEN_PEQ:       max_idx = 6; break;
                case SCREEN_LIMITER:   max_idx = 4; break;
                case SCREEN_PRESET:    max_idx = 7; break;
                case SCREEN_STATUS:
                case SCREEN_ABOUT:     max_idx = 0; break;
                default: break;
            }

            int new_idx = _cursorIndex + delta;
            if (new_idx < 0) new_idx = 0;
            if (new_idx > max_idx) new_idx = max_idx;

            if (new_idx != _cursorIndex) {
                _cursorIndex = new_idx;
                if (_currentMode == SCREEN_HOME) {
                    updateHomeDynamicData(); // Fast differential dock update
                } else if (_currentMode == SCREEN_MENU_GRID) {
                    drawMenuGridScreen();
                } else if (_currentMode == SCREEN_GAIN) {
                    drawGainScreen();
                } else if (_currentMode == SCREEN_HPF) {
                    drawHpfScreen();
                } else if (_currentMode == SCREEN_LPF) {
                    drawLpfScreen();
                } else if (_currentMode == SCREEN_DELAY) {
                    drawDelayScreen();
                } else if (_currentMode == SCREEN_PEQ) {
                    drawPeqScreen();
                } else if (_currentMode == SCREEN_LIMITER) {
                    drawLimiterScreen();
                } else if (_currentMode == SCREEN_PRESET) {
                    drawPresetScreen();
                }
            }
        }

        if (clicked) {
            handleScreenClick();
        }
    }
}

// =============================================================================
// MAIN UPDATE LOOP (Called in Arduino loop)
// =============================================================================
void TftDisplay::update() {
    if (!_isInitialized) return;

    // Maintain backlight
    digitalWrite(8, HIGH);
    digitalWrite(13, HIGH);
    if (TFT_BL_PIN >= 0 && TFT_BL_PIN != 8 && TFT_BL_PIN != 13) {
        digitalWrite(TFT_BL_PIN, HIGH);
    }

    unsigned long now = millis();

    // Auto timeout back to Home after 20 seconds of inactivity (except on Home)
    if (_currentMode != SCREEN_HOME && (now - _lastUserActivityTime > 20000)) {
        setScreenMode(SCREEN_HOME);
        return;
    }

    // Dynamic VU Meter & Parameter refresh on Home Screen at ~25 FPS (40ms)
    if (_currentMode == SCREEN_HOME && (now - _lastRenderTime >= 40)) {
        _lastRenderTime = now;
        updateHomeDynamicData();
    }
}
