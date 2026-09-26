#include "presets_manager.h"
#include <Preferences.h>
#include <string.h>

PresetsManager presetsManager;
static Preferences prefs;

PresetsManager::PresetsManager() : _currentSlot(1) {}

DspConfig PresetsManager::getDefaultConfig() {
    DspConfig cfg;
    cfg.master_gain_db = 0.0f;
    cfg.mute = false;
    cfg.polarity_inverted = false;

    // Independent 2-Channel DAC default
    cfg.xover_linked = false;
    cfg.xover_freq = 100.0f;
    cfg.xover_slope = SLOPE_24DB;

    // CHANNEL 1 (DAC 1 / LEFT): Subwoofer Crossover Defaults
    cfg.ch1.gain_db = 0.0f;
    cfg.ch1.mute = false;
    cfg.ch1.polarity_inverted = false;
    cfg.ch1.delay_ms = 0.0f;
    cfg.ch1.hpf.enabled = true;
    cfg.ch1.hpf.freq = 25.0f;
    cfg.ch1.hpf.slope = SLOPE_24DB;
    cfg.ch1.lpf.enabled = true;
    cfg.ch1.lpf.freq = 100.0f;
    cfg.ch1.lpf.slope = SLOPE_24DB;
    cfg.ch1.limiter.enabled = true;
    cfg.ch1.limiter.threshold_db = -1.0f;
    cfg.ch1.limiter.attack_ms = 10.0f;
    cfg.ch1.limiter.release_ms = 100.0f;

    const float default_ch1_freqs[3] = { 45.0f, 80.0f, 120.0f };
    for (int i = 0; i < 3; i++) {
        cfg.ch1.peq[i].enabled = true;
        cfg.ch1.peq[i].type = (i == 0) ? PEQ_LOW_SHELF : PEQ_PEAK;
        cfg.ch1.peq[i].freq = default_ch1_freqs[i];
        cfg.ch1.peq[i].gain_db = 0.0f;
        cfg.ch1.peq[i].q = 1.0f;
    }

    // CHANNEL 2 (DAC 2 / RIGHT): Mid/High Crossover Defaults
    cfg.ch2.gain_db = 0.0f;
    cfg.ch2.mute = false;
    cfg.ch2.polarity_inverted = false;
    cfg.ch2.delay_ms = 0.0f;
    cfg.ch2.hpf.enabled = true;
    cfg.ch2.hpf.freq = 100.0f;
    cfg.ch2.hpf.slope = SLOPE_24DB;
    cfg.ch2.lpf.enabled = false;
    cfg.ch2.lpf.freq = 20000.0f;
    cfg.ch2.lpf.slope = SLOPE_12DB;
    cfg.ch2.limiter.enabled = true;
    cfg.ch2.limiter.threshold_db = -1.0f;
    cfg.ch2.limiter.attack_ms = 2.0f;
    cfg.ch2.limiter.release_ms = 50.0f;

    const float default_ch2_freqs[3] = { 1000.0f, 4000.0f, 12000.0f };
    for (int i = 0; i < 3; i++) {
        cfg.ch2.peq[i].enabled = true;
        cfg.ch2.peq[i].type = (i == 2) ? PEQ_HIGH_SHELF : PEQ_PEAK;
        cfg.ch2.peq[i].freq = default_ch2_freqs[i];
        cfg.ch2.peq[i].gain_db = 0.0f;
        cfg.ch2.peq[i].q = 1.0f;
    }

    for (int i = 0; i < 3; i++) cfg.peq[i] = cfg.ch1.peq[i];
    for (int i = 0; i < 3; i++) cfg.peq[i + 3] = cfg.ch2.peq[i];

    // Compatibility mirrors
    cfg.hpf = cfg.ch1.hpf;
    cfg.lpf = cfg.ch1.lpf;
    cfg.delay.delay_ms = 0.0f;
    cfg.limiter = cfg.ch1.limiter;

    return cfg;
}

bool PresetsManager::begin() {
    if (!prefs.begin("snet_dsp", false)) {
        Serial.println("[NVS] Failed to initialize Preferences namespace");
        return false;
    }

    // Check if initial format has been done (v3 for tuned subwoofer crossover defaults)
    uint32_t magic = prefs.getUInt("magic", 0);
    if (magic != 0x534E4533) { // 'SNE3'
        Serial.println("[NVS] Formatting tuned crossover default presets...");
        resetAllToDefault();
        prefs.putUInt("magic", 0x534E4533);
    }

    _currentSlot = (uint8_t)prefs.getUChar("active_slot", 1);
    if (_currentSlot < 1 || _currentSlot > PRESET_COUNT) _currentSlot = 1;
    Serial.printf("[NVS] Preferences loaded. Active Slot: %u\n", _currentSlot);
    return true;
}

bool PresetsManager::savePreset(uint8_t slot, const DspConfig& config) {
    if (slot < 1 || slot > PRESET_COUNT) return false;
    char key[16];
    snprintf(key, sizeof(key), "preset_%u", slot);

    size_t written = prefs.putBytes(key, &config, sizeof(DspConfig));
    if (written == sizeof(DspConfig)) {
        _currentSlot = slot;
        prefs.putUChar("active_slot", slot);
        Serial.printf("[NVS] Preset %u saved successfully\n", slot);
        return true;
    }
    Serial.printf("[NVS] Error saving preset %u\n", slot);
    return false;
}

bool PresetsManager::loadPreset(uint8_t slot, DspConfig& out_config) {
    if (slot < 1 || slot > PRESET_COUNT) return false;
    char key[16];
    snprintf(key, sizeof(key), "preset_%u", slot);

    size_t read = prefs.getBytes(key, &out_config, sizeof(DspConfig));
    if (read == sizeof(DspConfig)) {
        _currentSlot = slot;
        prefs.putUChar("active_slot", slot);
        Serial.printf("[NVS] Preset %u loaded successfully\n", slot);
        return true;
    }

    // Fallback default
    out_config = getDefaultConfig();
    return false;
}

bool PresetsManager::resetPreset(uint8_t slot) {
    if (slot < 1 || slot > PRESET_COUNT) return false;
    DspConfig def = getDefaultConfig();
    return savePreset(slot, def);
}

void PresetsManager::resetAllToDefault() {
    DspConfig def = getDefaultConfig();
    for (uint8_t s = 1; s <= PRESET_COUNT; s++) {
        char key[16];
        snprintf(key, sizeof(key), "preset_%u", s);
        prefs.putBytes(key, &def, sizeof(DspConfig));
    }
    _currentSlot = 1;
    prefs.putUChar("active_slot", 1);
}
