#include "dsp_engine.h"
#include <math.h>
#include <string.h>

DspEngine dspEngine;

DspEngine::DspEngine() 
    : _sampleRate(AUDIO_SAMPLE_RATE_DEFAULT),
      _masterGainLinear(1.0f),
      _subGainLinear(1.0f),
      _midGainLinear(1.0f),
      _subDelayBuffer(nullptr),
      _midDelayBuffer(nullptr),
      _delayBufferSize(4800),
      _subDelayWriteIdx(0),
      _midDelayWriteIdx(0),
      _subDelaySamples(0.0f),
      _midDelaySamples(0.0f),
      _subLimiterEnvelope(0.0f),
      _subLimiterThresholdLinear(0.89125f), // -1 dB
      _subLimiterAttackCoeff(0.0f),
      _subLimiterReleaseCoeff(0.0f),
      _midLimiterEnvelope(0.0f),
      _midLimiterThresholdLinear(0.89125f), // -1 dB
      _midLimiterAttackCoeff(0.0f),
      _midLimiterReleaseCoeff(0.0f),
      _currentGainReductionDb(0.0f),
      _inPeakDb(-60.0f),
      _inRmsDb(-60.0f),
      _subPeakDb(-60.0f),
      _subRmsDb(-60.0f),
      _midPeakDb(-60.0f),
      _midRmsDb(-60.0f),
      _subClipFlag(false),
      _midClipFlag(false)
{
    _paramMux = portMUX_INITIALIZER_UNLOCKED;

    // Default configuration for 2-Way Crossover
    _config.master_gain_db = 0.0f;
    _config.mute = false;
    _config.polarity_inverted = false;

    _config.xover_linked = true;
    _config.xover_freq = 100.0f;
    _config.xover_slope = SLOPE_24DB;

    // Subwoofer path defaults
    _config.sub.gain_db = 0.0f;
    _config.sub.mute = false;
    _config.sub.polarity_inverted = false;
    _config.sub.delay_ms = 0.0f;
    _config.sub.hpf.enabled = true;
    _config.sub.hpf.freq = 25.0f;
    _config.sub.hpf.slope = SLOPE_24DB;
    _config.sub.lpf.enabled = true;
    _config.sub.lpf.freq = 100.0f;
    _config.sub.lpf.slope = SLOPE_24DB;
    _config.sub.limiter.enabled = true;
    _config.sub.limiter.threshold_db = -1.0f;
    _config.sub.limiter.attack_ms = 10.0f;
    _config.sub.limiter.release_ms = 100.0f;

    // Mid/High path defaults
    _config.mid.gain_db = 0.0f;
    _config.mid.mute = false;
    _config.mid.polarity_inverted = false;
    _config.mid.delay_ms = 0.0f;
    _config.mid.hpf.enabled = true;
    _config.mid.hpf.freq = 100.0f;
    _config.mid.hpf.slope = SLOPE_24DB;
    _config.mid.lpf.enabled = true;
    _config.mid.lpf.freq = 18000.0f;
    _config.mid.lpf.slope = SLOPE_12DB;
    _config.mid.limiter.enabled = true;
    _config.mid.limiter.threshold_db = -1.0f;
    _config.mid.limiter.attack_ms = 2.0f;
    _config.mid.limiter.release_ms = 50.0f;

    // 5-band PEQ Defaults
    const float default_freqs[PEQ_BAND_COUNT] = { 50.0f, 80.0f, 1000.0f, 4000.0f, 12000.0f };
    for (int i = 0; i < PEQ_BAND_COUNT; i++) {
        _config.peq[i].enabled = true;
        _config.peq[i].type = (i == 0) ? PEQ_LOW_SHELF : ((i == 4) ? PEQ_HIGH_SHELF : PEQ_PEAK);
        _config.peq[i].freq = default_freqs[i];
        _config.peq[i].gain_db = 0.0f;
        _config.peq[i].q = 1.0f;
    }

    _config.hpf = _config.sub.hpf;
    _config.lpf = _config.sub.lpf;
    _config.delay.delay_ms = 0.0f;
    _config.limiter = _config.sub.limiter;
}

DspEngine::~DspEngine() {
    if (_subDelayBuffer) { free(_subDelayBuffer); _subDelayBuffer = nullptr; }
    if (_midDelayBuffer) { free(_midDelayBuffer); _midDelayBuffer = nullptr; }
}

void DspEngine::init(uint32_t sample_rate) {
    _sampleRate = (sample_rate > 0) ? sample_rate : AUDIO_SAMPLE_RATE_DEFAULT;

    // Allocate delay buffers
    _delayBufferSize = (size_t)((MAX_DELAY_MS * 0.001f * (float)_sampleRate) + 64.0f);
    if (!_subDelayBuffer) {
        _subDelayBuffer = (float*)calloc(_delayBufferSize, sizeof(float));
    }
    if (!_midDelayBuffer) {
        _midDelayBuffer = (float*)calloc(_delayBufferSize, sizeof(float));
    }
    _subDelayWriteIdx = 0;
    _midDelayWriteIdx = 0;

    // Reset all biquads
    for (int i = 0; i < 4; i++) {
        _subHpf[i].reset();
        _subLpf[i].reset();
        _midHpf[i].reset();
    }
    _midLpf[0].reset();
    _midLpf[1].reset();

    for (int i = 0; i < 2; i++) _subPeq[i].reset();
    for (int i = 0; i < 3; i++) _midPeq[i].reset();

    updateFilterCoefficients();
    updateLimiterCoefficients();
    updateDelaySamples();
}

void DspEngine::setSampleRate(uint32_t sample_rate) {
    if (sample_rate == 0 || sample_rate == _sampleRate) return;
    init(sample_rate);
}

void DspEngine::setConfig(const DspConfig& config) {
    portENTER_CRITICAL(&_paramMux);
    _config = config;
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
    updateLimiterCoefficients();
    updateDelaySamples();
}

DspConfig DspEngine::getConfig() const {
    DspConfig copy;
    portENTER_CRITICAL((portMUX_TYPE*)&_paramMux);
    copy = _config;
    portEXIT_CRITICAL((portMUX_TYPE*)&_paramMux);
    return copy;
}

void DspEngine::setMasterGain(float gain_db) {
    if (gain_db < -60.0f) gain_db = -60.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;

    portENTER_CRITICAL(&_paramMux);
    _config.master_gain_db = gain_db;
    _masterGainLinear = _config.mute ? 0.0f : powf(10.0f, gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setMasterMute(bool mute) {
    portENTER_CRITICAL(&_paramMux);
    _config.mute = mute;
    _masterGainLinear = mute ? 0.0f : powf(10.0f, _config.master_gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setPolarity(bool inverted) {
    portENTER_CRITICAL(&_paramMux);
    _config.polarity_inverted = inverted;
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setCrossover(float freq, uint8_t slope) {
    if (freq < 20.0f) freq = 20.0f;
    if (freq > 10000.0f) freq = 10000.0f;
    if (slope != 12 && slope != 24 && slope != 48) slope = 24;

    portENTER_CRITICAL(&_paramMux);
    _config.xover_freq = freq;
    _config.xover_slope = slope;
    _config.sub.lpf.freq = freq;
    _config.sub.lpf.slope = slope;
    _config.mid.hpf.freq = freq;
    _config.mid.hpf.slope = slope;
    // mirror for legacy web
    _config.lpf.freq = freq;
    _config.lpf.slope = slope;
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

void DspEngine::setSubGain(float gain_db) {
    if (gain_db < -60.0f) gain_db = -60.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;

    portENTER_CRITICAL(&_paramMux);
    _config.sub.gain_db = gain_db;
    _subGainLinear = _config.sub.mute ? 0.0f : powf(10.0f, gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setMidGain(float gain_db) {
    if (gain_db < -60.0f) gain_db = -60.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;

    portENTER_CRITICAL(&_paramMux);
    _config.mid.gain_db = gain_db;
    _midGainLinear = _config.mid.mute ? 0.0f : powf(10.0f, gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setSubMute(bool mute) {
    portENTER_CRITICAL(&_paramMux);
    _config.sub.mute = mute;
    _subGainLinear = mute ? 0.0f : powf(10.0f, _config.sub.gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setMidMute(bool mute) {
    portENTER_CRITICAL(&_paramMux);
    _config.mid.mute = mute;
    _midGainLinear = mute ? 0.0f : powf(10.0f, _config.mid.gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setSubInvert(bool invert) {
    portENTER_CRITICAL(&_paramMux);
    _config.sub.polarity_inverted = invert;
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setMidInvert(bool invert) {
    portENTER_CRITICAL(&_paramMux);
    _config.mid.polarity_inverted = invert;
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setHpf(bool enabled, float freq, uint8_t slope) {
    portENTER_CRITICAL(&_paramMux);
    _config.sub.hpf.enabled = enabled;
    _config.sub.hpf.freq = freq;
    _config.sub.hpf.slope = slope;
    _config.hpf = _config.sub.hpf;
    portEXIT_CRITICAL(&_paramMux);
    updateFilterCoefficients();
}

void DspEngine::setLpf(bool enabled, float freq, uint8_t slope) {
    setCrossover(freq, slope);
}

void DspEngine::setPeqBand(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q) {
    if (band_idx >= PEQ_BAND_COUNT) return;
    portENTER_CRITICAL(&_paramMux);
    _config.peq[band_idx].enabled = enabled;
    _config.peq[band_idx].type = type;
    _config.peq[band_idx].freq = freq;
    _config.peq[band_idx].gain_db = gain_db;
    _config.peq[band_idx].q = q;
    portEXIT_CRITICAL(&_paramMux);
    updateFilterCoefficients();
}

void DspEngine::setDelay(float delay_ms) {
    if (delay_ms < 0.0f) delay_ms = 0.0f;
    if (delay_ms > MAX_DELAY_MS) delay_ms = MAX_DELAY_MS;
    portENTER_CRITICAL(&_paramMux);
    _config.sub.delay_ms = delay_ms;
    _config.mid.delay_ms = delay_ms;
    _config.delay.delay_ms = delay_ms;
    portEXIT_CRITICAL(&_paramMux);
    updateDelaySamples();
}

void DspEngine::setLimiter(bool enabled, float threshold_db, float attack_ms, float release_ms) {
    portENTER_CRITICAL(&_paramMux);
    _config.sub.limiter.enabled = enabled;
    _config.sub.limiter.threshold_db = threshold_db;
    _config.sub.limiter.attack_ms = attack_ms;
    _config.sub.limiter.release_ms = release_ms;
    _config.mid.limiter = _config.sub.limiter;
    _config.limiter = _config.sub.limiter;
    portEXIT_CRITICAL(&_paramMux);
    updateLimiterCoefficients();
}

void DspEngine::updateFilterCoefficients() {
    float fs = (float)_sampleRate;

    // Temporary biquad staging objects (computed outside critical section)
    Biquad sub_h[4], sub_l[4], sub_p[2];
    Biquad mid_h[4], mid_l[2], mid_p[3];

    for (int i = 0; i < 4; i++) {
        sub_h[i].reset();
        sub_l[i].reset();
        mid_h[i].reset();
    }
    mid_l[0].reset();
    mid_l[1].reset();
    for (int i = 0; i < 2; i++) sub_p[i].reset();
    for (int i = 0; i < 3; i++) mid_p[i].reset();

    // 1. SUB PATH: Subsonic HPF (default 25 Hz)
    if (_config.sub.hpf.enabled) {
        float f_sub_hpf = _config.sub.hpf.freq;
        uint8_t slope = _config.sub.hpf.slope;
        if (slope == SLOPE_12DB) {
            sub_h[0].setHighPass(f_sub_hpf, fs, 0.7071f);
        } else if (slope == SLOPE_24DB) {
            sub_h[0].setHighPass(f_sub_hpf, fs, 0.7071f);
            sub_h[1].setHighPass(f_sub_hpf, fs, 0.7071f);
        } else { // 48dB
            sub_h[0].setHighPass(f_sub_hpf, fs, 0.5412f);
            sub_h[1].setHighPass(f_sub_hpf, fs, 1.3065f);
            sub_h[2].setHighPass(f_sub_hpf, fs, 0.5412f);
            sub_h[3].setHighPass(f_sub_hpf, fs, 1.3065f);
        }
    }

    // 2. SUB PATH: Crossover LPF (cuts all vocals!)
    float f_xover = _config.xover_freq;
    uint8_t x_slope = _config.xover_slope;
    if (_config.sub.lpf.enabled) {
        if (x_slope == SLOPE_12DB) {
            sub_l[0].setLowPass(f_xover, fs, 0.7071f);
        } else if (x_slope == SLOPE_24DB) {
            sub_l[0].setLowPass(f_xover, fs, 0.7071f);
            sub_l[1].setLowPass(f_xover, fs, 0.7071f);
        } else { // 48dB
            sub_l[0].setLowPass(f_xover, fs, 0.5412f);
            sub_l[1].setLowPass(f_xover, fs, 1.3065f);
            sub_l[2].setLowPass(f_xover, fs, 0.5412f);
            sub_l[3].setLowPass(f_xover, fs, 1.3065f);
        }
    }

    // 3. MID PATH: Crossover HPF (blocks all deep bass!)
    if (_config.mid.hpf.enabled) {
        if (x_slope == SLOPE_12DB) {
            mid_h[0].setHighPass(f_xover, fs, 0.7071f);
        } else if (x_slope == SLOPE_24DB) {
            mid_h[0].setHighPass(f_xover, fs, 0.7071f);
            mid_h[1].setHighPass(f_xover, fs, 0.7071f);
        } else { // 48dB
            mid_h[0].setHighPass(f_xover, fs, 0.5412f);
            mid_h[1].setHighPass(f_xover, fs, 1.3065f);
            mid_h[2].setHighPass(f_xover, fs, 0.5412f);
            mid_h[3].setHighPass(f_xover, fs, 1.3065f);
        }
    }

    // 4. MID PATH: Protection LPF (tweeter high-cut e.g. 18kHz)
    if (_config.mid.lpf.enabled) {
        mid_l[0].setLowPass(_config.mid.lpf.freq, fs, 0.7071f);
        if (_config.mid.lpf.slope >= 24) {
            mid_l[1].setLowPass(_config.mid.lpf.freq, fs, 0.7071f);
        }
    }

    // 5. PEQ Bands: Bands 0,1 for Sub; Bands 2,3,4 for Mid
    for (int i = 0; i < 2; i++) {
        if (_config.peq[i].enabled && fabsf(_config.peq[i].gain_db) > 0.05f) {
            if (_config.peq[i].type == PEQ_LOW_SHELF)
                sub_p[i].setLowShelf(_config.peq[i].freq, fs, _config.peq[i].gain_db, _config.peq[i].q);
            else if (_config.peq[i].type == PEQ_HIGH_SHELF)
                sub_p[i].setHighShelf(_config.peq[i].freq, fs, _config.peq[i].gain_db, _config.peq[i].q);
            else
                sub_p[i].setPeakingEQ(_config.peq[i].freq, fs, _config.peq[i].gain_db, _config.peq[i].q);
        }
    }

    for (int i = 0; i < 3; i++) {
        int bi = i + 2;
        if (_config.peq[bi].enabled && fabsf(_config.peq[bi].gain_db) > 0.05f) {
            if (_config.peq[bi].type == PEQ_LOW_SHELF)
                mid_p[i].setLowShelf(_config.peq[bi].freq, fs, _config.peq[bi].gain_db, _config.peq[bi].q);
            else if (_config.peq[bi].type == PEQ_HIGH_SHELF)
                mid_p[i].setHighShelf(_config.peq[bi].freq, fs, _config.peq[bi].gain_db, _config.peq[bi].q);
            else
                mid_p[i].setPeakingEQ(_config.peq[bi].freq, fs, _config.peq[bi].gain_db, _config.peq[bi].q);
        }
    }

    // Atomic update into active processing filters
    portENTER_CRITICAL(&_paramMux);
    for (int i = 0; i < 4; i++) {
        _subHpf[i].b0 = sub_h[i].b0; _subHpf[i].b1 = sub_h[i].b1; _subHpf[i].b2 = sub_h[i].b2;
        _subHpf[i].a1 = sub_h[i].a1; _subHpf[i].a2 = sub_h[i].a2;

        _subLpf[i].b0 = sub_l[i].b0; _subLpf[i].b1 = sub_l[i].b1; _subLpf[i].b2 = sub_l[i].b2;
        _subLpf[i].a1 = sub_l[i].a1; _subLpf[i].a2 = sub_l[i].a2;

        _midHpf[i].b0 = mid_h[i].b0; _midHpf[i].b1 = mid_h[i].b1; _midHpf[i].b2 = mid_h[i].b2;
        _midHpf[i].a1 = mid_h[i].a1; _midHpf[i].a2 = mid_h[i].a2;
    }

    for (int i = 0; i < 2; i++) {
        _midLpf[i].b0 = mid_l[i].b0; _midLpf[i].b1 = mid_l[i].b1; _midLpf[i].b2 = mid_l[i].b2;
        _midLpf[i].a1 = mid_l[i].a1; _midLpf[i].a2 = mid_l[i].a2;
    }

    for (int i = 0; i < 2; i++) {
        _subPeq[i].b0 = sub_p[i].b0; _subPeq[i].b1 = sub_p[i].b1; _subPeq[i].b2 = sub_p[i].b2;
        _subPeq[i].a1 = sub_p[i].a1; _subPeq[i].a2 = sub_p[i].a2;
    }

    for (int i = 0; i < 3; i++) {
        _midPeq[i].b0 = mid_p[i].b0; _midPeq[i].b1 = mid_p[i].b1; _midPeq[i].b2 = mid_p[i].b2;
        _midPeq[i].a1 = mid_p[i].a1; _midPeq[i].a2 = mid_p[i].a2;
    }

    _masterGainLinear = _config.mute ? 0.0f : powf(10.0f, _config.master_gain_db / 20.0f);
    _subGainLinear    = _config.sub.mute ? 0.0f : powf(10.0f, _config.sub.gain_db / 20.0f);
    _midGainLinear    = _config.mid.mute ? 0.0f : powf(10.0f, _config.mid.gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::updateLimiterCoefficients() {
    float fs = (float)_sampleRate;

    // Sub Limiter
    _subLimiterThresholdLinear = powf(10.0f, _config.sub.limiter.threshold_db / 20.0f);
    float sub_att = _config.sub.limiter.attack_ms * 0.001f;
    float sub_rel = _config.sub.limiter.release_ms * 0.001f;
    _subLimiterAttackCoeff = expf(-1.0f / (sub_att * fs));
    _subLimiterReleaseCoeff = expf(-1.0f / (sub_rel * fs));

    // Mid Limiter
    _midLimiterThresholdLinear = powf(10.0f, _config.mid.limiter.threshold_db / 20.0f);
    float mid_att = _config.mid.limiter.attack_ms * 0.001f;
    float mid_rel = _config.mid.limiter.release_ms * 0.001f;
    _midLimiterAttackCoeff = expf(-1.0f / (mid_att * fs));
    _midLimiterReleaseCoeff = expf(-1.0f / (mid_rel * fs));
}

void DspEngine::updateDelaySamples() {
    _subDelaySamples = (_config.sub.delay_ms * 0.001f) * (float)_sampleRate;
    _midDelaySamples = (_config.mid.delay_ms * 0.001f) * (float)_sampleRate;

    if (_subDelaySamples < 0.0f) _subDelaySamples = 0.0f;
    if (_midDelaySamples < 0.0f) _midDelaySamples = 0.0f;
    if (_subDelaySamples > (float)(_delayBufferSize - 2)) _subDelaySamples = (float)(_delayBufferSize - 2);
    if (_midDelaySamples > (float)(_delayBufferSize - 2)) _midDelaySamples = (float)(_delayBufferSize - 2);
}

void DspEngine::processAudio(const int16_t* in_pcm, int16_t* out_pcm, size_t frame_count) {
    if (!in_pcm || !out_pcm || frame_count == 0) return;

    // Cache gain and filter parameters for this block
    float master_gain = _masterGainLinear;
    float sub_gain    = _subGainLinear;
    float mid_gain    = _midGainLinear;

    float master_pol  = _config.polarity_inverted ? -1.0f : 1.0f;
    float sub_pol     = _config.sub.polarity_inverted ? -1.0f : 1.0f;
    float mid_pol     = _config.mid.polarity_inverted ? -1.0f : 1.0f;

    bool sub_hpf_on   = _config.sub.hpf.enabled;
    uint8_t sub_hpf_s = _config.sub.hpf.slope;
    bool sub_lpf_on   = _config.sub.lpf.enabled;
    uint8_t xover_s   = _config.xover_slope;

    bool mid_hpf_on   = _config.mid.hpf.enabled;
    bool mid_lpf_on   = _config.mid.lpf.enabled;
    uint8_t mid_lpf_s = _config.mid.lpf.slope;

    bool sub_lim_on   = _config.sub.limiter.enabled;
    float sub_thresh  = _subLimiterThresholdLinear;
    float sub_att_c   = _subLimiterAttackCoeff;
    float sub_rel_c   = _subLimiterReleaseCoeff;

    bool mid_lim_on   = _config.mid.limiter.enabled;
    float mid_thresh  = _midLimiterThresholdLinear;
    float mid_att_c   = _midLimiterAttackCoeff;
    float mid_rel_c   = _midLimiterReleaseCoeff;

    float block_in_peak = 0.0f;
    float block_in_sum_sq = 0.0f;
    float block_sub_peak = 0.0f;
    float block_sub_sum_sq = 0.0f;
    float block_mid_peak = 0.0f;
    float block_mid_sum_sq = 0.0f;
    bool  block_sub_clip = false;
    bool  block_mid_clip = false;

    const float norm_factor = 1.0f / 32768.0f;

    for (size_t i = 0; i < frame_count; i++) {
        // Digital Audio Input: average L+R or Left channel
        int16_t raw_l = in_pcm[i * 2];
        int16_t raw_r = in_pcm[i * 2 + 1];
        float raw_in = ((float)raw_l + (float)raw_r) * 0.5f * norm_factor;

        // Input metering
        float in_abs = fabsf(raw_in);
        if (in_abs > block_in_peak) block_in_peak = in_abs;
        block_in_sum_sq += raw_in * raw_in;

        // Apply Master Gain and Master Polarity
        float master_sig = raw_in * master_gain * master_pol;

        // ====================================================================
        // WAY 1: SUBWOOFER / LOW PATH (Feeds DAC Output LEFT)
        // ====================================================================
        float s_sub = master_sig * sub_pol;

        // 1. Subsonic HPF
        if (sub_hpf_on) {
            s_sub = _subHpf[0].process(s_sub);
            if (sub_hpf_s >= 24) s_sub = _subHpf[1].process(s_sub);
            if (sub_hpf_s >= 48) {
                s_sub = _subHpf[2].process(s_sub);
                s_sub = _subHpf[3].process(s_sub);
            }
        }

        // 2. Crossover LPF (cuts all vocals at 80-120Hz)
        if (sub_lpf_on) {
            s_sub = _subLpf[0].process(s_sub);
            if (xover_s >= 24) s_sub = _subLpf[1].process(s_sub);
            if (xover_s >= 48) {
                s_sub = _subLpf[2].process(s_sub);
                s_sub = _subLpf[3].process(s_sub);
            }
        }

        // 3. Sub PEQ (Bands 0 & 1)
        if (_config.peq[0].enabled) s_sub = _subPeq[0].process(s_sub);
        if (_config.peq[1].enabled) s_sub = _subPeq[1].process(s_sub);

        // 4. Sub Gain
        s_sub = s_sub * sub_gain;

        // 5. Sub Limiter
        if (sub_lim_on) {
            float env = fabsf(s_sub);
            if (env > _subLimiterEnvelope)
                _subLimiterEnvelope = sub_att_c * _subLimiterEnvelope + (1.0f - sub_att_c) * env;
            else
                _subLimiterEnvelope = sub_rel_c * _subLimiterEnvelope + (1.0f - sub_rel_c) * env;

            if (_subLimiterEnvelope > sub_thresh) {
                float g = sub_thresh / (_subLimiterEnvelope + 1e-9f);
                s_sub *= g;
            }
        }

        // Sub Safety Hard Clip
        if (s_sub > 0.9999f) { s_sub = 0.9999f; block_sub_clip = true; }
        else if (s_sub < -0.9999f) { s_sub = -0.9999f; block_sub_clip = true; }

        float sub_abs = fabsf(s_sub);
        if (sub_abs > block_sub_peak) block_sub_peak = sub_abs;
        block_sub_sum_sq += s_sub * s_sub;

        // ====================================================================
        // WAY 2: MID / HIGH PATH (Feeds DAC Output RIGHT)
        // ====================================================================
        float s_mid = master_sig * mid_pol;

        // 1. Crossover HPF (blocks bass)
        if (mid_hpf_on) {
            s_mid = _midHpf[0].process(s_mid);
            if (xover_s >= 24) s_mid = _midHpf[1].process(s_mid);
            if (xover_s >= 48) {
                s_mid = _midHpf[2].process(s_mid);
                s_mid = _midHpf[3].process(s_mid);
            }
        }

        // 2. Protection LPF (tweeter high cut)
        if (mid_lpf_on) {
            s_mid = _midLpf[0].process(s_mid);
            if (mid_lpf_s >= 24) s_mid = _midLpf[1].process(s_mid);
        }

        // 3. Mid/High PEQ (Bands 2, 3, 4)
        if (_config.peq[2].enabled) s_mid = _midPeq[0].process(s_mid);
        if (_config.peq[3].enabled) s_mid = _midPeq[1].process(s_mid);
        if (_config.peq[4].enabled) s_mid = _midPeq[2].process(s_mid);

        // 4. Mid Gain
        s_mid = s_mid * mid_gain;

        // 5. Mid Limiter
        if (mid_lim_on) {
            float env = fabsf(s_mid);
            if (env > _midLimiterEnvelope)
                _midLimiterEnvelope = mid_att_c * _midLimiterEnvelope + (1.0f - mid_att_c) * env;
            else
                _midLimiterEnvelope = mid_rel_c * _midLimiterEnvelope + (1.0f - mid_rel_c) * env;

            if (_midLimiterEnvelope > mid_thresh) {
                float g = mid_thresh / (_midLimiterEnvelope + 1e-9f);
                s_mid *= g;
            }
        }

        // Mid Safety Hard Clip
        if (s_mid > 0.9999f) { s_mid = 0.9999f; block_mid_clip = true; }
        else if (s_mid < -0.9999f) { s_mid = -0.9999f; block_mid_clip = true; }

        float mid_abs = fabsf(s_mid);
        if (mid_abs > block_mid_peak) block_mid_peak = mid_abs;
        block_mid_sum_sq += s_mid * s_mid;

        // Convert back to 16-bit signed PCM
        // Channel LEFT  (out_pcm[i*2])     = WAY 1: SUBWOOFER / LOW
        // Channel RIGHT (out_pcm[i*2 + 1]) = WAY 2: MID / HIGH
        out_pcm[i * 2]     = (int16_t)(s_sub * 32767.0f);
        out_pcm[i * 2 + 1] = (int16_t)(s_mid * 32767.0f);
    }

    // Compute block statistics in dBFS
    float in_rms  = sqrtf(block_in_sum_sq  / (float)frame_count);
    float sub_rms = sqrtf(block_sub_sum_sq / (float)frame_count);
    float mid_rms = sqrtf(block_mid_sum_sq / (float)frame_count);

    float in_peak_db  = (block_in_peak > 0.0001f)  ? (20.0f * log10f(block_in_peak))  : -60.0f;
    float in_rms_db   = (in_rms > 0.0001f)         ? (20.0f * log10f(in_rms))         : -60.0f;
    float sub_peak_db = (block_sub_peak > 0.0001f) ? (20.0f * log10f(block_sub_peak)) : -60.0f;
    float sub_rms_db  = (sub_rms > 0.0001f)        ? (20.0f * log10f(sub_rms))        : -60.0f;
    float mid_peak_db = (block_mid_peak > 0.0001f) ? (20.0f * log10f(block_mid_peak)) : -60.0f;
    float mid_rms_db  = (mid_rms > 0.0001f)        ? (20.0f * log10f(mid_rms))        : -60.0f;

    if (in_peak_db  < -60.0f) in_peak_db  = -60.0f;
    if (in_rms_db   < -60.0f) in_rms_db   = -60.0f;
    if (sub_peak_db < -60.0f) sub_peak_db = -60.0f;
    if (sub_rms_db  < -60.0f) sub_rms_db  = -60.0f;
    if (mid_peak_db < -60.0f) mid_peak_db = -60.0f;
    if (mid_rms_db  < -60.0f) mid_rms_db  = -60.0f;

    // Smooth ballistics (instant attack, slow decay)
    if (in_peak_db > _inPeakDb) _inPeakDb = in_peak_db;
    else _inPeakDb = 0.92f * _inPeakDb + 0.08f * in_peak_db;
    _inRmsDb = 0.85f * _inRmsDb + 0.15f * in_rms_db;

    if (sub_peak_db > _subPeakDb) _subPeakDb = sub_peak_db;
    else _subPeakDb = 0.92f * _subPeakDb + 0.08f * sub_peak_db;
    _subRmsDb = 0.85f * _subRmsDb + 0.15f * sub_rms_db;

    if (mid_peak_db > _midPeakDb) _midPeakDb = mid_peak_db;
    else _midPeakDb = 0.92f * _midPeakDb + 0.08f * mid_peak_db;
    _midRmsDb = 0.85f * _midRmsDb + 0.15f * mid_rms_db;

    if (block_sub_clip) _subClipFlag = true;
    if (block_mid_clip) _midClipFlag = true;
}

VuMeterData DspEngine::getVuMeterData() {
    VuMeterData data;
    data.in_peak_db   = _inPeakDb;
    data.in_rms_db    = _inRmsDb;
    data.sub_peak_db  = _subPeakDb;
    data.sub_rms_db   = _subRmsDb;
    data.mid_peak_db  = _midPeakDb;
    data.mid_rms_db   = _midRmsDb;
    data.out_peak_db  = (_subPeakDb > _midPeakDb) ? _subPeakDb : _midPeakDb;
    data.out_rms_db   = (_subRmsDb > _midRmsDb) ? _subRmsDb : _midRmsDb;
    data.limiter_gr_db = _currentGainReductionDb;
    data.sub_clip     = _subClipFlag;
    data.mid_clip     = _midClipFlag;
    data.clip         = _subClipFlag || _midClipFlag;

    // Clear clip sticky flags on read
    _subClipFlag = false;
    _midClipFlag = false;

    return data;
}
