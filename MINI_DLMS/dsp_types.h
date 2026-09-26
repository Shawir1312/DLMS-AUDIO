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

// Single Channel Configuration (Independent DSP for each PCM5102 DAC Output)
struct ChannelConfig {
    float gain_db;           // -60.0 to +12.0 dB (channel volume)
    bool mute;               // true/false
    bool polarity_inverted;  // false = Normal, true = Inverted (180 deg)
    float delay_ms;          // 0.0 to 50.0 ms
    HpfConfig hpf;           // High Pass Filter (20Hz - 20kHz, 12/24/48dB)
    LpfConfig lpf;           // Low Pass Filter (20Hz - 20kHz, 12/24/48dB)
    LimiterConfig limiter;   // Output Limiter
    PeqBandConfig peq[3];    // 3-Band Parametric EQ dedicated to this channel
};

// Full DSP Engine Parameters for 2 Independent DAC Channels
struct DspConfig {
    float master_gain_db;    // -60.0 to +12.0 dB (default 0 dB)
    bool mute;               // true/false
    bool polarity_inverted;  // false = Normal, true = Inverted

    // Optional 2-Way Crossover Linking (convenience)
    bool xover_linked;       // true = ch1 LPF and ch2 HPF track xover_freq
    float xover_freq;        // 20.0 to 20000.0 Hz (default 100 Hz)
    uint8_t xover_slope;     // 12, 24, or 48 dB/oct

    // Channel 1: DAC 1 (LEFT Output) - Independent DSP Pipeline
    union {
        ChannelConfig ch1;
        ChannelConfig sub;   // Legacy alias
    };

    // Channel 2: DAC 2 (RIGHT Output) - Independent DSP Pipeline
    union {
        ChannelConfig ch2;
        ChannelConfig mid;   // Legacy alias
    };

    // 6-band PEQ mirror (bands 0-2 for CH1, bands 3-5 for CH2)
    PeqBandConfig peq[6];

    // Compatibility fields with legacy code
    HpfConfig hpf;
    LpfConfig lpf;
    DelayConfig delay;
    LimiterConfig limiter;
};

// Real-time VU Meter and Telemetry Data (Independent 2-Channel DAC)
struct VuMeterData {
    float in_peak_db;     // Input Peak in dBFS (-60 to 0 dB)
    float in_rms_db;      // Input RMS in dBFS (-60 to 0 dB)
    
    // Channel 1 (DAC 1 / Left)
    float ch1_peak_db;    // CH1 Peak in dBFS (-60 to 0 dB)
    float ch1_rms_db;     // CH1 RMS in dBFS (-60 to 0 dB)
    bool  ch1_clip;       // CH1 clip flag

    // Channel 2 (DAC 2 / Right)
    float ch2_peak_db;    // CH2 Peak in dBFS (-60 to 0 dB)
    float ch2_rms_db;     // CH2 RMS in dBFS (-60 to 0 dB)
    bool  ch2_clip;       // CH2 clip flag

    float out_peak_db;    // Max output peak
    float out_rms_db;     // Max output rms
    float limiter_gr_db;  // Limiter Gain Reduction in dB (0 to -30 dB)
    bool  clip;           // General clip flag

    // Legacy aliases
    float sub_peak_db;
    float sub_rms_db;
    float mid_peak_db;
    float mid_rms_db;
    bool  sub_clip;
    bool  mid_clip;
};
