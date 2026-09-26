#pragma once
#include <Arduino.h>
#include "config.h"
#include "dsp_types.h"

// Screen Modes matching Web UI 1-to-1
enum DisplayScreenMode {
    SCREEN_HOME = 0,    // Split-card cyber layout (IN ADC, OUT 4CH, Preset, Badges, Bottom Cyber Dock)
    SCREEN_MENU_GRID,   // 2x5 Cyber Menu Tile Matrix
    SCREEN_GAIN,        // Setting Gain (Input ADC, Output 4CH, Mutes, Polarity)
    SCREEN_HPF,         // Setting HPF (2x2 channel cards)
    SCREEN_LPF,         // Setting LPF (2x2 channel cards)
    SCREEN_DELAY,       // Setting Delay (2x2 channel cards with ms & distance)
    SCREEN_PEQ,         // Setting PEQ (Live graphical response curve, B1..B5)
    SCREEN_LIMITER,     // Setting Limiter (Threshold, Attack, Release)
    SCREEN_PRESET,      // Preset Manager (Slots P01..P05, Load, Save)
    SCREEN_STATUS,      // Hardware Telemetry & Modules
    SCREEN_ABOUT,       // System Model & Info
    SCREEN_MENU = SCREEN_MENU_GRID // Legacy compatibility alias
};

class TftDisplay {
public:
    TftDisplay();

    bool begin();
    void update();

    // Call when rotary encoder generates events (EC11 hardware or Web virtual knob)
    void handleEncoder(int32_t delta, bool clicked, bool longPressed);

    // Switch screen modes
    void setScreenMode(DisplayScreenMode mode);
    DisplayScreenMode getScreenMode() const { return _currentMode; }

private:
    DisplayScreenMode _currentMode;
    bool _isInitialized;
    unsigned long _lastRenderTime;
    unsigned long _lastUserActivityTime;

    // Navigation and edit state
    int8_t _cursorIndex;    // Active selected index on current screen
    bool   _inEditMode;     // True when actively tweaking a numeric value with encoder
    uint8_t _peqBandIndex;  // 0..4 (B1..B5)
    uint8_t _presetSlot;    // 1..5

    // Dynamic VU & dynamic text cache for Home Screen
    float   _prevInLDb;
    float   _prevInRDb;
    float   _prevOut1Db;
    float   _prevOut2Db;
    float   _prevOut3Db;
    float   _prevOut4Db;
    int16_t _prevInLSegs;
    int16_t _prevInRSegs;
    int16_t _prevOut1Segs;
    int16_t _prevOut2Segs;
    int16_t _prevOut3Segs;
    int16_t _prevOut4Segs;

    // Home parameter cache
    float   _prevMasterGain;
    float   _prevHpfFreq;
    float   _prevLpfFreq;
    bool    _prevHpfEn;
    bool    _prevLpfEn;
    bool    _prevPeqEn;
    bool    _prevLimEn;
    uint8_t _prevPreset;
    int8_t  _prevDockIndex;

    // Drawing Primitives & Web-matching Renderers
    void drawHeader(const char* title, const char* right_tag = "48k", bool show_run = true);
    void drawCard(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t borderCol, uint16_t bgCol);
    void drawLedMeter(int16_t x, int16_t y, int16_t w, int16_t h, float db, uint8_t segments, int16_t& prev_segs);

    void drawSplashScreen();
    void drawHomeScreenLayout();
    void updateHomeDynamicData();

    void drawMenuGridScreen();
    void drawGainScreen();
    void drawHpfScreen();
    void drawLpfScreen();
    void drawDelayScreen();
    void drawPeqScreen();
    void drawLimiterScreen();
    void drawPresetScreen();
    void drawStatusScreen();
    void drawAboutScreen();

    void applyParameterEdit(int32_t delta);
    void handleScreenClick();
};

extern TftDisplay tftDisplay;
