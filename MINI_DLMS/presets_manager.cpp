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

    // 2-Way Global Settings
    cfg.xover_linked = true;
    cfg.xover_freq = 100.0f;       // Default Crossover 100 Hz
    cfg.xover_slope = SLOPE_24DB;  // 24 dB/oct Linkwitz-Riley

    // SUBWOOFER (OUT 1 / LEFT):
    cfg.sub.gain_db = 0.0f;
    cfg.sub.mute = false;
    cfg.sub.polarity_inverted = false;
    cfg.sub.delay_ms = 0.0f;
    // Subsonic HPF (protect sub cone): 25 Hz 24dB
    cfg.sub.hpf.enabled = true;
    cfg.sub.hpf.freq = 25.0f;
    cfg.sub.hpf.slope = SLOPE_24DB;
    // Crossover LPF: 100 Hz 24dB (silences vocals)
    cfg.sub.lpf.enabled = true;
    cfg.sub.lpf.freq = 100.0f;
    cfg.sub.lpf.slope = SLOPE_24DB;
    cfg.sub.limiter.enabled = true;
    cfg.sub.limiter.threshold_db = -1.0f;
    cfg.sub.limiter.attack_ms = 10.0f;
    cfg.sub.limiter.release_ms = 100.0f;

    // MID / HIGH (OUT 2 / RIGHT):
    cfg.mid.gain_db = 0.0f;
    cfg.mid.mute = false;
    cfg.mid.polarity_inverted = false;
    cfg.mid.delay_ms = 0.0f;
    // Crossover HPF: 100 Hz 24dB (blocks sub-bass)
    cfg.mid.hpf.enabled = true;
    cfg.mid.hpf.freq = 100.0f;
    cfg.mid.hpf.slope = SLOPE_24DB;
    // Protection LPF: 18000 Hz 12dB
    cfg.mid.lpf.enabled = true;
    cfg.mid.lpf.freq = 18000.0f;
    cfg.mid.lpf.slope = SLOPE_12DB;
    cfg.mid.limiter.enabled = true;
    cfg.mid.limiter.threshold_db = -1.0f;
    cfg.mid.limiter.attack_ms = 2.0f;
    cfg.mid.limiter.release_ms = 50.0f;

    // 5-band PEQ
    const float freqs[5] = { 50.0f, 80.0f, 1000.0f, 4000.0f, 12000.0f };
    for (int i = 0; i < 5; i++) {
        cfg.peq[i].enabled = true;
        cfg.peq[i].type = (i == 0) ? PEQ_LOW_SHELF : ((i == 4) ? PEQ_HIGH_SHELF : PEQ_PEAK);
        cfg.peq[i].freq = freqs[i];
        cfg.peq[i].gain_db = 0.0f;
        cfg.peq[i].q = 1.0f;
    }

    // Backward-compatibility mirrors
    cfg.hpf = cfg.sub.hpf;
    cfg.lpf = cfg.sub.lpf;
    cfg.delay.delay_ms = 0.0f;
    cfg.limiter = cfg.sub.limiter;

    return cfg;
}

bool PresetsManager::begin() {
    if (!prefs.begin("snet_dsp", false)) {
        Serial.println("[NVS] Failed to initialize Preferences namespace");
        return false;
    }

    // Check if initial format has been done
    uint32_t magic = prefs.getUInt("magic", 0);
    if (magic != 0x534E4554) { // 'SNET'
        Serial.println("[NVS] Formatting default presets...");
        resetAllToDefault();
        prefs.putUInt("magic", 0x534E4554);
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
