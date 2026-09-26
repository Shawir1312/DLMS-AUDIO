#pragma once
#include <Arduino.h>
#include "dsp_types.h"

#define PRESET_COUNT 5

class PresetsManager {
public:
    PresetsManager();

    bool begin();
    bool savePreset(uint8_t slot, const DspConfig& config);
    bool loadPreset(uint8_t slot, DspConfig& out_config);
    bool resetPreset(uint8_t slot);
    void resetAllToDefault();

    uint8_t getCurrentSlot() const { return _currentSlot; }
    void setCurrentSlot(uint8_t slot) { _currentSlot = slot; }

    static DspConfig getDefaultConfig();

private:
    uint8_t _currentSlot;
};

extern PresetsManager presetsManager;
