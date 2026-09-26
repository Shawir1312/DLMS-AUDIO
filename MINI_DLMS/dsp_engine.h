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

    // Channel 1 (DAC 1 / Left) Dedicated Setters
    void setCh1Gain(float gain_db);
    void setCh1Mute(bool mute);
    void setCh1Invert(bool invert);
    void setCh1Hpf(bool enabled, float freq, uint8_t slope);
    void setCh1Lpf(bool enabled, float freq, uint8_t slope);
    void setCh1Delay(float delay_ms);
    void setCh1Limiter(bool enabled, float threshold_db, float attack_ms, float release_ms);
    void setCh1Peq(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q);

    // Channel 2 (DAC 2 / Right) Dedicated Setters
    void setCh2Gain(float gain_db);
    void setCh2Mute(bool mute);
    void setCh2Invert(bool invert);
    void setCh2Hpf(bool enabled, float freq, uint8_t slope);
    void setCh2Lpf(bool enabled, float freq, uint8_t slope);
    void setCh2Delay(float delay_ms);
    void setCh2Limiter(bool enabled, float threshold_db, float attack_ms, float release_ms);
    void setCh2Peq(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q);

    // 2-Way Crossover Setters (maps to ch1 and ch2)
    void setCrossover(float freq, uint8_t slope);
    void setSubGain(float gain_db) { setCh1Gain(gain_db); }
    void setMidGain(float gain_db) { setCh2Gain(gain_db); }
    void setSubMute(bool mute) { setCh1Mute(mute); }
    void setMidMute(bool mute) { setCh2Mute(mute); }
    void setSubInvert(bool invert) { setCh1Invert(invert); }
    void setMidInvert(bool invert) { setCh2Invert(invert); }

    // Legacy setters (maintain full compatibility)
    void setHpf(bool enabled, float freq, uint8_t slope);
    void setLpf(bool enabled, float freq, uint8_t slope);
    void setPeqBand(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q);
    void setDelay(float delay_ms);
    void setLimiter(bool enabled, float threshold_db, float attack_ms, float release_ms);

    // Main Audio Processing function (runs on Core 1)
    // in_pcm: 16-bit signed stereo interleaved (L, R, L, R)
    // out_pcm: 16-bit signed stereo interleaved (L = DAC 1 / CH1, R = DAC 2 / CH2)
    // frame_count: number of stereo sample frames (len_bytes / 4)
    void processAudio(const int16_t* in_pcm, int16_t* out_pcm, size_t frame_count);

    // VU Meter telemetry
    VuMeterData getVuMeterData();

private:
    uint32_t _sampleRate;
    DspConfig _config;

    // Linear gains
    float _masterGainLinear;
    float _ch1GainLinear;
    float _ch2GainLinear;

    // Channel 1 path biquads (DAC 1 / OUT LEFT)
    Biquad _ch1Hpf[4];       // HPF (up to 48dB)
    Biquad _ch1Lpf[4];       // LPF (up to 48dB)
    Biquad _ch1Peq[3];       // 3 Dedicated Parametric EQ Bands

    // Channel 2 path biquads (DAC 2 / OUT RIGHT)
    Biquad _ch2Hpf[4];       // HPF (up to 48dB)
    Biquad _ch2Lpf[4];       // LPF (up to 48dB)
    Biquad _ch2Peq[3];       // 3 Dedicated Parametric EQ Bands

    // Delay lines (Independent Ring Buffers for CH1 and CH2 alignment)
    float* _ch1DelayBuffer;
    float* _ch2DelayBuffer;
    size_t _delayBufferSize;
    size_t _ch1DelayWriteIdx;
    size_t _ch2DelayWriteIdx;
    float _ch1DelaySamples;
    float _ch2DelaySamples;

    // Channel 1 Limiter state
    float _ch1LimiterEnvelope;
    float _ch1LimiterThresholdLinear;
    float _ch1LimiterAttackCoeff;
    float _ch1LimiterReleaseCoeff;

    // Channel 2 Limiter state
    float _ch2LimiterEnvelope;
    float _ch2LimiterThresholdLinear;
    float _ch2LimiterAttackCoeff;
    float _ch2LimiterReleaseCoeff;
    float _currentGainReductionDb;

    // VU Meter Accumulators (computed per block with smooth ballistics)
    float _inPeakDb;
    float _inRmsDb;
    float _ch1PeakDb;
    float _ch1RmsDb;
    float _ch2PeakDb;
    float _ch2RmsDb;
    bool  _ch1ClipFlag;
    bool  _ch2ClipFlag;

    // Spinlock for parameter safety across tasks
    portMUX_TYPE _paramMux;

    void updateFilterCoefficients();
    void updateLimiterCoefficients();
    void updateDelaySamples();
};

extern DspEngine dspEngine;
