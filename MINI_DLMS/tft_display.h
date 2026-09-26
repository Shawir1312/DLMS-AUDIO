#pragma once
#include <Arduino.h>
#include "config.h"
#include "dsp_types.h"

enum DisplayScreenMode {
    SCREEN_HOME = 0,   // VU Meter & System Info
    SCREEN_MENU,       // Settings Menu
    SCREEN_EDIT        // Parameter Adjustment
};

class TftDisplay {
public:
    TftDisplay();

    bool begin();
    void update();

    // Call when rotary encoder generates events
    void handleEncoder(int32_t delta, bool clicked, bool longPressed);

    // Switch screens
    void setScreenMode(DisplayScreenMode mode);
    DisplayScreenMode getScreenMode() const { return _currentMode; }

private:
    DisplayScreenMode _currentMode;
    bool _isInitialized;
    unsigned long _lastRenderTime;
    unsigned long _lastUserActivityTime;

    // Menu State
    int8_t _menuIndex;
    bool _inEditMode;
    int8_t _menuScrollOffset;

    // Previous VU meter drawing cache (for flicker-free partial updates)
    int16_t _prevInBarW;
    int16_t _prevSubBarW;
    int16_t _prevMidBarW;
    bool    _prevSubClip;
    bool    _prevMidClip;
    float   _prevXover;
    float   _prevSubGain;
    float   _prevMidGain;
    uint8_t _prevPreset;

    void drawHomeScreenLayout();
    void updateHomeDynamicData();

    void drawMenuScreen();
    void updateMenuItemValue(uint8_t idx);
    void applyMenuEdit(int32_t delta);
    void executeMenuSelect();
    void drawHeader(const char* title, uint16_t bg_color, uint16_t text_color);
    void drawVuBar(int16_t x, int16_t y, int16_t w, int16_t h, float db, int16_t& prev_w);
};

extern TftDisplay tftDisplay;
