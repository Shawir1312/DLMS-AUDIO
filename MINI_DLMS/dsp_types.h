#pragma once
#include <Arduino.h>

// Filter Slopes
enum FilterSlope {
    SLOPE_12DB = 12,
    SLOPE_24DB = 24,
    SLOPE_48DB = 48
};

// Filter Types for PEQ
enum PeqFilterType {
    PEQ_PEAK = 0,
    PEQ_LOW_SHELF = 1,
    PEQ_HIGH_SHELF = 2
};

// Bluetooth Connection State
enum BtState {
    BT_DISCONNECTED = 0,
    BT_CONNECTING = 1,
    BT_CONNECTED = 2,
    BT_STREAMING = 3
};

// Single Band PEQ Configuration
struct PeqBandConfig {
    bool enabled;
    uint8_t type;       // PeqFilterType
    float freq;         // 20.0 to 20000.0 Hz
    float gain_db;      // -18.0 to +18.0 dB
    float q;            // 0.1 to 10.0
};

// High Pass Filter Configuration (Low Cut)
struct HpfConfig {
    bool enabled;
    float freq;         // 20.0 to 20000.0 Hz (default 20 Hz)
    uint8_t slope;      // 12, 24, or 48 dB/oct
};

// Low Pass Filter Configuration (High Cut / Subwoofer Crossover)
struct LpfConfig {
    bool enabled;
    float freq;         // 20.0 to 20000.0 Hz (allows cutting vocals for subwoofers at 60-150 Hz)
    uint8_t slope;      // 12, 24, or 48 dB/oct
};

// Audio Delay Configuration
struct DelayConfig {
    float delay_ms;     // 0.0 to 50.0 ms (step 0.1 ms)
};

// Limiter Configuration
struct LimiterConfig {
    bool enabled;
    float threshold_db; // -30.0 to 0.0 dB (default -1.0 dB)
    float attack_ms;    // 0.1 to 50.0 ms
    float release_ms;   // 10.0 to 1000.0 ms
};

// Single Channel Configuration (Way 1 Sub, Way 2 Mid/High)
struct ChannelConfig {
    float gain_db;           // -60.0 to +12.0 dB (channel volume)
    bool mute;               // true/false
    bool polarity_inverted;  // false = Normal, true = Inverted (180 deg)
    float delay_ms;          // 0.0 to 50.0 ms
    HpfConfig hpf;           // High Pass Filter
    LpfConfig lpf;           // Low Pass Filter
    LimiterConfig limiter;   // Output Limiter
};

// Full DSP Engine Parameters
struct DspConfig {
    float master_gain_db;    // -60.0 to +12.0 dB (default 0 dB)
    bool mute;               // true/false
    bool polarity_inverted;  // false = Normal, true = Inverted

    // 2-Way Crossover Global Settings
    bool xover_linked;       // true = sub LPF and mid HPF track xover_freq
    float xover_freq;        // 40.0 to 5000.0 Hz (default 100 Hz)
    uint8_t xover_slope;     // 12, 24, or 48 dB/oct

    // Channel 1 (LEFT): SUBWOOFER / LOW
    ChannelConfig sub;

    // Channel 2 (RIGHT): MID / HIGH
    ChannelConfig mid;

    // 5-Band Parametric EQ (Bands 0-1 for Sub, 2-4 for Mid)
    PeqBandConfig peq[5];

    // Compatibility fields with previous 1-ch version
    HpfConfig hpf;
    LpfConfig lpf;
    DelayConfig delay;
    LimiterConfig limiter;
};

// Real-time VU Meter and Telemetry Data (Multi-Channel 2-Way)
struct VuMeterData {
    float in_peak_db;     // Input Peak in dBFS (-60 to 0 dB)
    float in_rms_db;      // Input RMS in dBFS (-60 to 0 dB)
    float sub_peak_db;    // Sub Out Peak in dBFS (-60 to 0 dB)
    float sub_rms_db;     // Sub Out RMS in dBFS (-60 to 0 dB)
    float mid_peak_db;    // Mid Out Peak in dBFS (-60 to 0 dB)
    float mid_rms_db;     // Mid Out RMS in dBFS (-60 to 0 dB)
    float out_peak_db;    // Max output peak
    float out_rms_db;     // Max output rms
    float limiter_gr_db;  // Limiter Gain Reduction in dB (0 to -30 dB)
    bool clip;            // General clip flag
    bool sub_clip;        // Sub channel clip
    bool mid_clip;        // Mid channel clip
};
