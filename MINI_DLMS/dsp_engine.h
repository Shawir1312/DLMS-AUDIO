#pragma once
#include <Arduino.h>
#include "config.h"
#include "dsp_types.h"
#include "biquad.h"

class DspEngine {
public:
    DspEngine();
    ~DspEngine();

    void init(uint32_t sample_rate = AUDIO_SAMPLE_RATE_DEFAULT);
    void setSampleRate(uint32_t sample_rate);
    uint32_t getSampleRate() const { return _sampleRate; }

    // Thread-safe configuration update
    void setConfig(const DspConfig& config);
    DspConfig getConfig() const;

    // Fast setters for realtime rotary encoder and web adjustments
    void setMasterGain(float gain_db);
    void setMasterMute(bool mute);
    void setMute(bool mute) { setMasterMute(mute); }
    void setPolarity(bool inverted);

    // 2-Way Crossover Setters
    void setCrossover(float freq, uint8_t slope);
    void setSubGain(float gain_db);
    void setMidGain(float gain_db);
    void setSubMute(bool mute);
    void setMidMute(bool mute);
    void setSubInvert(bool invert);
    void setMidInvert(bool invert);

    // Legacy setters (maintain full compatibility)
    void setHpf(bool enabled, float freq, uint8_t slope);
    void setLpf(bool enabled, float freq, uint8_t slope);
    void setPeqBand(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q);
    void setDelay(float delay_ms);
    void setLimiter(bool enabled, float threshold_db, float attack_ms, float release_ms);

    // Main Audio Processing function (runs on Core 1)
    // in_pcm: 16-bit signed stereo interleaved (L, R, L, R)
    // out_pcm: 16-bit signed stereo interleaved (L = Sub, R = Mid/High)
    // frame_count: number of stereo sample frames (len_bytes / 4)
    void processAudio(const int16_t* in_pcm, int16_t* out_pcm, size_t frame_count);

    // VU Meter telemetry
    VuMeterData getVuMeterData();

private:
    uint32_t _sampleRate;
    DspConfig _config;

    // Linear gains
    float _masterGainLinear;
    float _subGainLinear;
    float _midGainLinear;

    // Subwoofer path biquads (OUT 1 / LEFT)
    Biquad _subHpf[4];       // Subsonic HPF (up to 48dB)
    Biquad _subLpf[4];       // Crossover LPF (up to 48dB)
    Biquad _subPeq[2];       // Sub Bands (0, 1)

    // Mid/High path biquads (OUT 2 / RIGHT)
    Biquad _midHpf[4];       // Crossover HPF (up to 48dB)
    Biquad _midLpf[2];       // Tweeter protection LPF (up to 24dB)
    Biquad _midPeq[3];       // Mid/High Bands (2, 3, 4)

    // Delay lines (Ring Buffers for Sub and Mid alignment)
    float* _subDelayBuffer;
    float* _midDelayBuffer;
    size_t _delayBufferSize;
    size_t _subDelayWriteIdx;
    size_t _midDelayWriteIdx;
    float _subDelaySamples;
    float _midDelaySamples;

    // Sub Limiter state
    float _subLimiterEnvelope;
    float _subLimiterThresholdLinear;
    float _subLimiterAttackCoeff;
    float _subLimiterReleaseCoeff;

    // Mid Limiter state
    float _midLimiterEnvelope;
    float _midLimiterThresholdLinear;
    float _midLimiterAttackCoeff;
    float _midLimiterReleaseCoeff;
    float _currentGainReductionDb;

    // VU Meter Accumulators (computed per block with smooth ballistics)
    float _inPeakDb;
    float _inRmsDb;
    float _subPeakDb;
    float _subRmsDb;
    float _midPeakDb;
    float _midRmsDb;
    bool  _subClipFlag;
    bool  _midClipFlag;

    // Spinlock for parameter safety across tasks
    portMUX_TYPE _paramMux;

    void updateFilterCoefficients();
    void updateLimiterCoefficients();
    void updateDelaySamples();
};

extern DspEngine dspEngine;
