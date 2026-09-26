#include "dsp_engine.h"
#include <math.h>
#include <string.h>

DspEngine dspEngine;

DspEngine::DspEngine() 
    : _sampleRate(AUDIO_SAMPLE_RATE_DEFAULT),
      _masterGainLinear(1.0f),
      _ch1GainLinear(1.0f),
      _ch2GainLinear(1.0f),
      _ch1DelayBuffer(nullptr),
      _ch2DelayBuffer(nullptr),
      _delayBufferSize(4800),
      _ch1DelayWriteIdx(0),
      _ch2DelayWriteIdx(0),
      _ch1DelaySamples(0.0f),
      _ch2DelaySamples(0.0f),
      _ch1LimiterEnvelope(0.0f),
      _ch1LimiterThresholdLinear(0.89125f), // -1 dB
      _ch1LimiterAttackCoeff(0.0f),
      _ch1LimiterReleaseCoeff(0.0f),
      _ch2LimiterEnvelope(0.0f),
      _ch2LimiterThresholdLinear(0.89125f), // -1 dB
      _ch2LimiterAttackCoeff(0.0f),
      _ch2LimiterReleaseCoeff(0.0f),
      _currentGainReductionDb(0.0f),
      _inPeakDb(-60.0f),
      _inRmsDb(-60.0f),
      _ch1PeakDb(-60.0f),
      _ch1RmsDb(-60.0f),
      _ch2PeakDb(-60.0f),
      _ch2RmsDb(-60.0f),
      _ch1ClipFlag(false),
      _ch2ClipFlag(false)
{
    _paramMux = portMUX_INITIALIZER_UNLOCKED;

    // Master configuration
    _config.master_gain_db = 0.0f;
    _config.mute = false;
    _config.polarity_inverted = false;

    // Crossover linking helper (optional)
    _config.xover_linked = false; // Independent by default!
    _config.xover_freq = 100.0f;
    _config.xover_slope = SLOPE_24DB;

    // ========================================================================
    // CHANNEL 1 (DAC 1 / LEFT OUTPUT) - Independent DSP Defaults
    // ========================================================================
    _config.ch1.gain_db = 0.0f;
    _config.ch1.mute = false;
    _config.ch1.polarity_inverted = false;
    _config.ch1.delay_ms = 0.0f;
    _config.ch1.hpf.enabled = true;
    _config.ch1.hpf.freq = 25.0f;
    _config.ch1.hpf.slope = SLOPE_24DB;
    _config.ch1.lpf.enabled = false;
    _config.ch1.lpf.freq = 20000.0f;
    _config.ch1.lpf.slope = SLOPE_24DB;
    _config.ch1.limiter.enabled = true;
    _config.ch1.limiter.threshold_db = -1.0f;
    _config.ch1.limiter.attack_ms = 10.0f;
    _config.ch1.limiter.release_ms = 100.0f;

    const float default_ch1_freqs[3] = { 50.0f, 100.0f, 250.0f };
    for (int i = 0; i < 3; i++) {
        _config.ch1.peq[i].enabled = true;
        _config.ch1.peq[i].type = (i == 0) ? PEQ_LOW_SHELF : PEQ_PEAK;
        _config.ch1.peq[i].freq = default_ch1_freqs[i];
        _config.ch1.peq[i].gain_db = 0.0f;
        _config.ch1.peq[i].q = 1.0f;
    }

    // ========================================================================
    // CHANNEL 2 (DAC 2 / RIGHT OUTPUT) - Independent DSP Defaults
    // ========================================================================
    _config.ch2.gain_db = 0.0f;
    _config.ch2.mute = false;
    _config.ch2.polarity_inverted = false;
    _config.ch2.delay_ms = 0.0f;
    _config.ch2.hpf.enabled = false;
    _config.ch2.hpf.freq = 20.0f;
    _config.ch2.hpf.slope = SLOPE_24DB;
    _config.ch2.lpf.enabled = false;
    _config.ch2.lpf.freq = 20000.0f;
    _config.ch2.lpf.slope = SLOPE_12DB;
    _config.ch2.limiter.enabled = true;
    _config.ch2.limiter.threshold_db = -1.0f;
    _config.ch2.limiter.attack_ms = 2.0f;
    _config.ch2.limiter.release_ms = 50.0f;

    const float default_ch2_freqs[3] = { 1000.0f, 4000.0f, 12000.0f };
    for (int i = 0; i < 3; i++) {
        _config.ch2.peq[i].enabled = true;
        _config.ch2.peq[i].type = (i == 2) ? PEQ_HIGH_SHELF : PEQ_PEAK;
        _config.ch2.peq[i].freq = default_ch2_freqs[i];
        _config.ch2.peq[i].gain_db = 0.0f;
        _config.ch2.peq[i].q = 1.0f;
    }

    // Sync global peq array
    for (int i = 0; i < 3; i++) _config.peq[i] = _config.ch1.peq[i];
    for (int i = 0; i < 3; i++) _config.peq[i + 3] = _config.ch2.peq[i];

    // Compatibility mirrors
    _config.hpf = _config.ch1.hpf;
    _config.lpf = _config.ch1.lpf;
    _config.delay.delay_ms = 0.0f;
    _config.limiter = _config.ch1.limiter;
}

DspEngine::~DspEngine() {
    if (_ch1DelayBuffer) { free(_ch1DelayBuffer); _ch1DelayBuffer = nullptr; }
    if (_ch2DelayBuffer) { free(_ch2DelayBuffer); _ch2DelayBuffer = nullptr; }
}

void DspEngine::init(uint32_t sample_rate) {
    _sampleRate = (sample_rate > 0) ? sample_rate : AUDIO_SAMPLE_RATE_DEFAULT;

    // Allocate delay buffers for both channels
    _delayBufferSize = (size_t)((MAX_DELAY_MS * 0.001f * (float)_sampleRate) + 64.0f);
    if (!_ch1DelayBuffer) {
        _ch1DelayBuffer = (float*)calloc(_delayBufferSize, sizeof(float));
    }
    if (!_ch2DelayBuffer) {
        _ch2DelayBuffer = (float*)calloc(_delayBufferSize, sizeof(float));
    }
    _ch1DelayWriteIdx = 0;
    _ch2DelayWriteIdx = 0;

    // Reset all biquads for CH1 and CH2
    for (int i = 0; i < 4; i++) {
        _ch1Hpf[i].reset();
        _ch1Lpf[i].reset();
        _ch2Hpf[i].reset();
        _ch2Lpf[i].reset();
    }
    for (int i = 0; i < 3; i++) {
        _ch1Peq[i].reset();
        _ch2Peq[i].reset();
    }

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
    _masterGainLinear = _config.mute ? 0.0f : powf(10.0f, _config.master_gain_db / 20.0f);
    _ch1GainLinear    = _config.ch1.mute ? 0.0f : powf(10.0f, _config.ch1.gain_db / 20.0f);
    _ch2GainLinear    = _config.ch2.mute ? 0.0f : powf(10.0f, _config.ch2.gain_db / 20.0f);
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

// ============================================================================
// CHANNEL 1 (DAC 1 / LEFT) METHODS
// ============================================================================
void DspEngine::setCh1Gain(float gain_db) {
    if (gain_db < -60.0f) gain_db = -60.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;

    portENTER_CRITICAL(&_paramMux);
    _config.ch1.gain_db = gain_db;
    _ch1GainLinear = _config.ch1.mute ? 0.0f : powf(10.0f, gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setCh1Mute(bool mute) {
    portENTER_CRITICAL(&_paramMux);
    _config.ch1.mute = mute;
    _ch1GainLinear = mute ? 0.0f : powf(10.0f, _config.ch1.gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setCh1Invert(bool invert) {
    portENTER_CRITICAL(&_paramMux);
    _config.ch1.polarity_inverted = invert;
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setCh1Hpf(bool enabled, float freq, uint8_t slope) {
    if (freq < 20.0f) freq = 20.0f;
    if (freq > 20000.0f) freq = 20000.0f;
    if (slope != 12 && slope != 24 && slope != 48) slope = 24;

    portENTER_CRITICAL(&_paramMux);
    _config.ch1.hpf.enabled = enabled;
    _config.ch1.hpf.freq = freq;
    _config.ch1.hpf.slope = slope;
    _config.hpf = _config.ch1.hpf;
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

void DspEngine::setCh1Lpf(bool enabled, float freq, uint8_t slope) {
    if (freq < 20.0f) freq = 20.0f;
    if (freq > 20000.0f) freq = 20000.0f;
    if (slope != 12 && slope != 24 && slope != 48) slope = 24;

    portENTER_CRITICAL(&_paramMux);
    _config.ch1.lpf.enabled = enabled;
    _config.ch1.lpf.freq = freq;
    _config.ch1.lpf.slope = slope;
    _config.lpf = _config.ch1.lpf;
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

void DspEngine::setCh1Delay(float delay_ms) {
    if (delay_ms < 0.0f) delay_ms = 0.0f;
    if (delay_ms > MAX_DELAY_MS) delay_ms = MAX_DELAY_MS;

    portENTER_CRITICAL(&_paramMux);
    _config.ch1.delay_ms = delay_ms;
    portEXIT_CRITICAL(&_paramMux);

    updateDelaySamples();
}

void DspEngine::setCh1Limiter(bool enabled, float threshold_db, float attack_ms, float release_ms) {
    portENTER_CRITICAL(&_paramMux);
    _config.ch1.limiter.enabled = enabled;
    _config.ch1.limiter.threshold_db = threshold_db;
    _config.ch1.limiter.attack_ms = attack_ms;
    _config.ch1.limiter.release_ms = release_ms;
    _config.limiter = _config.ch1.limiter;
    portEXIT_CRITICAL(&_paramMux);

    updateLimiterCoefficients();
}

void DspEngine::setCh1Peq(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q) {
    if (band_idx >= 3) return;
    portENTER_CRITICAL(&_paramMux);
    _config.ch1.peq[band_idx].enabled = enabled;
    _config.ch1.peq[band_idx].type = type;
    _config.ch1.peq[band_idx].freq = freq;
    _config.ch1.peq[band_idx].gain_db = gain_db;
    _config.ch1.peq[band_idx].q = q;
    _config.peq[band_idx] = _config.ch1.peq[band_idx];
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

// ============================================================================
// CHANNEL 2 (DAC 2 / RIGHT) METHODS
// ============================================================================
void DspEngine::setCh2Gain(float gain_db) {
    if (gain_db < -60.0f) gain_db = -60.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;

    portENTER_CRITICAL(&_paramMux);
    _config.ch2.gain_db = gain_db;
    _ch2GainLinear = _config.ch2.mute ? 0.0f : powf(10.0f, gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setCh2Mute(bool mute) {
    portENTER_CRITICAL(&_paramMux);
    _config.ch2.mute = mute;
    _ch2GainLinear = mute ? 0.0f : powf(10.0f, _config.ch2.gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setCh2Invert(bool invert) {
    portENTER_CRITICAL(&_paramMux);
    _config.ch2.polarity_inverted = invert;
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::setCh2Hpf(bool enabled, float freq, uint8_t slope) {
    if (freq < 20.0f) freq = 20.0f;
    if (freq > 20000.0f) freq = 20000.0f;
    if (slope != 12 && slope != 24 && slope != 48) slope = 24;

    portENTER_CRITICAL(&_paramMux);
    _config.ch2.hpf.enabled = enabled;
    _config.ch2.hpf.freq = freq;
    _config.ch2.hpf.slope = slope;
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

void DspEngine::setCh2Lpf(bool enabled, float freq, uint8_t slope) {
    if (freq < 20.0f) freq = 20.0f;
    if (freq > 20000.0f) freq = 20000.0f;
    if (slope != 12 && slope != 24 && slope != 48) slope = 24;

    portENTER_CRITICAL(&_paramMux);
    _config.ch2.lpf.enabled = enabled;
    _config.ch2.lpf.freq = freq;
    _config.ch2.lpf.slope = slope;
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

void DspEngine::setCh2Delay(float delay_ms) {
    if (delay_ms < 0.0f) delay_ms = 0.0f;
    if (delay_ms > MAX_DELAY_MS) delay_ms = MAX_DELAY_MS;

    portENTER_CRITICAL(&_paramMux);
    _config.ch2.delay_ms = delay_ms;
    portEXIT_CRITICAL(&_paramMux);

    updateDelaySamples();
}

void DspEngine::setCh2Limiter(bool enabled, float threshold_db, float attack_ms, float release_ms) {
    portENTER_CRITICAL(&_paramMux);
    _config.ch2.limiter.enabled = enabled;
    _config.ch2.limiter.threshold_db = threshold_db;
    _config.ch2.limiter.attack_ms = attack_ms;
    _config.ch2.limiter.release_ms = release_ms;
    portEXIT_CRITICAL(&_paramMux);

    updateLimiterCoefficients();
}

void DspEngine::setCh2Peq(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q) {
    if (band_idx >= 3) return;
    portENTER_CRITICAL(&_paramMux);
    _config.ch2.peq[band_idx].enabled = enabled;
    _config.ch2.peq[band_idx].type = type;
    _config.ch2.peq[band_idx].freq = freq;
    _config.ch2.peq[band_idx].gain_db = gain_db;
    _config.ch2.peq[band_idx].q = q;
    _config.peq[band_idx + 3] = _config.ch2.peq[band_idx];
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

// Optional 2-Way Crossover Helper
void DspEngine::setCrossover(float freq, uint8_t slope) {
    if (freq < 20.0f) freq = 20.0f;
    if (freq > 20000.0f) freq = 20000.0f;
    if (slope != 12 && slope != 24 && slope != 48) slope = 24;

    portENTER_CRITICAL(&_paramMux);
    _config.xover_freq = freq;
    _config.xover_slope = slope;
    _config.ch1.lpf.enabled = true;
    _config.ch1.lpf.freq = freq;
    _config.ch1.lpf.slope = slope;

    _config.ch2.hpf.enabled = true;
    _config.ch2.hpf.freq = freq;
    _config.ch2.hpf.slope = slope;

    _config.lpf = _config.ch1.lpf;
    portEXIT_CRITICAL(&_paramMux);

    updateFilterCoefficients();
}

void DspEngine::setHpf(bool enabled, float freq, uint8_t slope) {
    setCh1Hpf(enabled, freq, slope);
}

void DspEngine::setLpf(bool enabled, float freq, uint8_t slope) {
    setCrossover(freq, slope);
}

void DspEngine::setPeqBand(uint8_t band_idx, bool enabled, uint8_t type, float freq, float gain_db, float q) {
    if (band_idx < 3) {
        setCh1Peq(band_idx, enabled, type, freq, gain_db, q);
    } else if (band_idx < 6) {
        setCh2Peq(band_idx - 3, enabled, type, freq, gain_db, q);
    }
}

void DspEngine::setDelay(float delay_ms) {
    setCh1Delay(delay_ms);
    setCh2Delay(delay_ms);
}

void DspEngine::setLimiter(bool enabled, float threshold_db, float attack_ms, float release_ms) {
    setCh1Limiter(enabled, threshold_db, attack_ms, release_ms);
    setCh2Limiter(enabled, threshold_db, attack_ms, release_ms);
}

static void configureFilterBiquads(Biquad b[4], bool isHpf, bool enabled, float freq, float fs, uint8_t slope) {
    for (int i = 0; i < 4; i++) b[i].reset();
    if (!enabled) return;

    if (slope == SLOPE_12DB) {
        if (isHpf) b[0].setHighPass(freq, fs, 0.7071f);
        else       b[0].setLowPass(freq, fs, 0.7071f);
    } else if (slope == SLOPE_24DB) {
        if (isHpf) {
            b[0].setHighPass(freq, fs, 0.7071f);
            b[1].setHighPass(freq, fs, 0.7071f);
        } else {
            b[0].setLowPass(freq, fs, 0.7071f);
            b[1].setLowPass(freq, fs, 0.7071f);
        }
    } else { // 48dB / 8th order Linkwitz-Riley
        if (isHpf) {
            b[0].setHighPass(freq, fs, 0.5412f);
            b[1].setHighPass(freq, fs, 1.3065f);
            b[2].setHighPass(freq, fs, 0.5412f);
            b[3].setHighPass(freq, fs, 1.3065f);
        } else {
            b[0].setLowPass(freq, fs, 0.5412f);
            b[1].setLowPass(freq, fs, 1.3065f);
            b[2].setLowPass(freq, fs, 0.5412f);
            b[3].setLowPass(freq, fs, 1.3065f);
        }
    }
}

void DspEngine::updateFilterCoefficients() {
    float fs = (float)_sampleRate;

    // Temporary biquad staging objects
    Biquad ch1_h[4], ch1_l[4], ch1_p[3];
    Biquad ch2_h[4], ch2_l[4], ch2_p[3];

    for (int i = 0; i < 4; i++) {
        ch1_h[i].reset(); ch1_l[i].reset();
        ch2_h[i].reset(); ch2_l[i].reset();
    }
    for (int i = 0; i < 3; i++) {
        ch1_p[i].reset();
        ch2_p[i].reset();
    }

    // 1. Channel 1 HPF & LPF
    configureFilterBiquads(ch1_h, true,  _config.ch1.hpf.enabled, _config.ch1.hpf.freq, fs, _config.ch1.hpf.slope);
    configureFilterBiquads(ch1_l, false, _config.ch1.lpf.enabled, _config.ch1.lpf.freq, fs, _config.ch1.lpf.slope);

    // 2. Channel 2 HPF & LPF
    configureFilterBiquads(ch2_h, true,  _config.ch2.hpf.enabled, _config.ch2.hpf.freq, fs, _config.ch2.hpf.slope);
    configureFilterBiquads(ch2_l, false, _config.ch2.lpf.enabled, _config.ch2.lpf.freq, fs, _config.ch2.lpf.slope);

    // 3. Channel 1 PEQ (3 bands)
    for (int i = 0; i < 3; i++) {
        if (_config.ch1.peq[i].enabled && fabsf(_config.ch1.peq[i].gain_db) > 0.05f) {
            if (_config.ch1.peq[i].type == PEQ_LOW_SHELF)
                ch1_p[i].setLowShelf(_config.ch1.peq[i].freq, fs, _config.ch1.peq[i].gain_db, _config.ch1.peq[i].q);
            else if (_config.ch1.peq[i].type == PEQ_HIGH_SHELF)
                ch1_p[i].setHighShelf(_config.ch1.peq[i].freq, fs, _config.ch1.peq[i].gain_db, _config.ch1.peq[i].q);
            else
                ch1_p[i].setPeakingEQ(_config.ch1.peq[i].freq, fs, _config.ch1.peq[i].gain_db, _config.ch1.peq[i].q);
        }
    }

    // 4. Channel 2 PEQ (3 bands)
    for (int i = 0; i < 3; i++) {
        if (_config.ch2.peq[i].enabled && fabsf(_config.ch2.peq[i].gain_db) > 0.05f) {
            if (_config.ch2.peq[i].type == PEQ_LOW_SHELF)
                ch2_p[i].setLowShelf(_config.ch2.peq[i].freq, fs, _config.ch2.peq[i].gain_db, _config.ch2.peq[i].q);
            else if (_config.ch2.peq[i].type == PEQ_HIGH_SHELF)
                ch2_p[i].setHighShelf(_config.ch2.peq[i].freq, fs, _config.ch2.peq[i].gain_db, _config.ch2.peq[i].q);
            else
                ch2_p[i].setPeakingEQ(_config.ch2.peq[i].freq, fs, _config.ch2.peq[i].gain_db, _config.ch2.peq[i].q);
        }
    }

    // Atomic update into active processing filters
    portENTER_CRITICAL(&_paramMux);
    for (int i = 0; i < 4; i++) {
        _ch1Hpf[i].b0 = ch1_h[i].b0; _ch1Hpf[i].b1 = ch1_h[i].b1; _ch1Hpf[i].b2 = ch1_h[i].b2;
        _ch1Hpf[i].a1 = ch1_h[i].a1; _ch1Hpf[i].a2 = ch1_h[i].a2;

        _ch1Lpf[i].b0 = ch1_l[i].b0; _ch1Lpf[i].b1 = ch1_l[i].b1; _ch1Lpf[i].b2 = ch1_l[i].b2;
        _ch1Lpf[i].a1 = ch1_l[i].a1; _ch1Lpf[i].a2 = ch1_l[i].a2;

        _ch2Hpf[i].b0 = ch2_h[i].b0; _ch2Hpf[i].b1 = ch2_h[i].b1; _ch2Hpf[i].b2 = ch2_h[i].b2;
        _ch2Hpf[i].a1 = ch2_h[i].a1; _ch2Hpf[i].a2 = ch2_h[i].a2;

        _ch2Lpf[i].b0 = ch2_l[i].b0; _ch2Lpf[i].b1 = ch2_l[i].b1; _ch2Lpf[i].b2 = ch2_l[i].b2;
        _ch2Lpf[i].a1 = ch2_l[i].a1; _ch2Lpf[i].a2 = ch2_l[i].a2;
    }

    for (int i = 0; i < 3; i++) {
        _ch1Peq[i].b0 = ch1_p[i].b0; _ch1Peq[i].b1 = ch1_p[i].b1; _ch1Peq[i].b2 = ch1_p[i].b2;
        _ch1Peq[i].a1 = ch1_p[i].a1; _ch1Peq[i].a2 = ch1_p[i].a2;

        _ch2Peq[i].b0 = ch2_p[i].b0; _ch2Peq[i].b1 = ch2_p[i].b1; _ch2Peq[i].b2 = ch2_p[i].b2;
        _ch2Peq[i].a1 = ch2_p[i].a1; _ch2Peq[i].a2 = ch2_p[i].a2;
    }

    _masterGainLinear = _config.mute ? 0.0f : powf(10.0f, _config.master_gain_db / 20.0f);
    _ch1GainLinear    = _config.ch1.mute ? 0.0f : powf(10.0f, _config.ch1.gain_db / 20.0f);
    _ch2GainLinear    = _config.ch2.mute ? 0.0f : powf(10.0f, _config.ch2.gain_db / 20.0f);
    portEXIT_CRITICAL(&_paramMux);
}

void DspEngine::updateLimiterCoefficients() {
    float fs = (float)_sampleRate;

    // Channel 1 Limiter
    _ch1LimiterThresholdLinear = powf(10.0f, _config.ch1.limiter.threshold_db / 20.0f);
    float ch1_att = _config.ch1.limiter.attack_ms * 0.001f;
    float ch1_rel = _config.ch1.limiter.release_ms * 0.001f;
    _ch1LimiterAttackCoeff = expf(-1.0f / (ch1_att * fs));
    _ch1LimiterReleaseCoeff = expf(-1.0f / (ch1_rel * fs));

    // Channel 2 Limiter
    _ch2LimiterThresholdLinear = powf(10.0f, _config.ch2.limiter.threshold_db / 20.0f);
    float ch2_att = _config.ch2.limiter.attack_ms * 0.001f;
    float ch2_rel = _config.ch2.limiter.release_ms * 0.001f;
    _ch2LimiterAttackCoeff = expf(-1.0f / (ch2_att * fs));
    _ch2LimiterReleaseCoeff = expf(-1.0f / (ch2_rel * fs));
}

void DspEngine::updateDelaySamples() {
    _ch1DelaySamples = (_config.ch1.delay_ms * 0.001f) * (float)_sampleRate;
    _ch2DelaySamples = (_config.ch2.delay_ms * 0.001f) * (float)_sampleRate;

    if (_ch1DelaySamples < 0.0f) _ch1DelaySamples = 0.0f;
    if (_ch2DelaySamples < 0.0f) _ch2DelaySamples = 0.0f;
    if (_ch1DelaySamples > (float)(_delayBufferSize - 2)) _ch1DelaySamples = (float)(_delayBufferSize - 2);
    if (_ch2DelaySamples > (float)(_delayBufferSize - 2)) _ch2DelaySamples = (float)(_delayBufferSize - 2);
}

void DspEngine::processAudio(const int16_t* in_pcm, int16_t* out_pcm, size_t frame_count) {
    if (!in_pcm || !out_pcm || frame_count == 0) return;

    // Cache gain and filter parameters for this block
    float master_gain = _masterGainLinear;
    float ch1_gain    = _ch1GainLinear;
    float ch2_gain    = _ch2GainLinear;

    float master_pol  = _config.polarity_inverted ? -1.0f : 1.0f;
    float ch1_pol     = _config.ch1.polarity_inverted ? -1.0f : 1.0f;
    float ch2_pol     = _config.ch2.polarity_inverted ? -1.0f : 1.0f;

    bool ch1_hpf_on   = _config.ch1.hpf.enabled;
    uint8_t ch1_hpf_s = _config.ch1.hpf.slope;
    bool ch1_lpf_on   = _config.ch1.lpf.enabled;
    uint8_t ch1_lpf_s = _config.ch1.lpf.slope;

    bool ch2_hpf_on   = _config.ch2.hpf.enabled;
    uint8_t ch2_hpf_s = _config.ch2.hpf.slope;
    bool ch2_lpf_on   = _config.ch2.lpf.enabled;
    uint8_t ch2_lpf_s = _config.ch2.lpf.slope;

    bool ch1_lim_on   = _config.ch1.limiter.enabled;
    float ch1_thresh  = _ch1LimiterThresholdLinear;
    float ch1_att_c   = _ch1LimiterAttackCoeff;
    float ch1_rel_c   = _ch1LimiterReleaseCoeff;

    bool ch2_lim_on   = _config.ch2.limiter.enabled;
    float ch2_thresh  = _ch2LimiterThresholdLinear;
    float ch2_att_c   = _ch2LimiterAttackCoeff;
    float ch2_rel_c   = _ch2LimiterReleaseCoeff;

    float block_in_peak = 0.0f;
    float block_in_sum_sq = 0.0f;
    float block_ch1_peak = 0.0f;
    float block_ch1_sum_sq = 0.0f;
    float block_ch2_peak = 0.0f;
    float block_ch2_sum_sq = 0.0f;
    bool  block_ch1_clip = false;
    bool  block_ch2_clip = false;

    const float norm_factor = 1.0f / 32768.0f;

    for (size_t i = 0; i < frame_count; i++) {
        // Digital Audio Input: average L+R
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
        // CHANNEL 1: DAC 1 OUTPUT (LEFT)
        // ====================================================================
        float s_ch1 = master_sig * ch1_pol;

        // 1. HPF (up to 48dB)
        if (ch1_hpf_on) {
            s_ch1 = _ch1Hpf[0].process(s_ch1);
            if (ch1_hpf_s >= 24) s_ch1 = _ch1Hpf[1].process(s_ch1);
            if (ch1_hpf_s >= 48) {
                s_ch1 = _ch1Hpf[2].process(s_ch1);
                s_ch1 = _ch1Hpf[3].process(s_ch1);
            }
        }

        // 2. LPF (up to 48dB)
        if (ch1_lpf_on) {
            s_ch1 = _ch1Lpf[0].process(s_ch1);
            if (ch1_lpf_s >= 24) s_ch1 = _ch1Lpf[1].process(s_ch1);
            if (ch1_lpf_s >= 48) {
                s_ch1 = _ch1Lpf[2].process(s_ch1);
                s_ch1 = _ch1Lpf[3].process(s_ch1);
            }
        }

        // 3. PEQ (3 bands)
        if (_config.ch1.peq[0].enabled) s_ch1 = _ch1Peq[0].process(s_ch1);
        if (_config.ch1.peq[1].enabled) s_ch1 = _ch1Peq[1].process(s_ch1);
        if (_config.ch1.peq[2].enabled) s_ch1 = _ch1Peq[2].process(s_ch1);

        // 4. Delay Line
        if (_ch1DelaySamples > 0.5f && _ch1DelayBuffer) {
            _ch1DelayBuffer[_ch1DelayWriteIdx] = s_ch1;
            float r_idx = (float)_ch1DelayWriteIdx - _ch1DelaySamples;
            if (r_idx < 0.0f) r_idx += (float)_delayBufferSize;
            size_t idx0 = (size_t)r_idx;
            size_t idx1 = (idx0 + 1) % _delayBufferSize;
            float frac = r_idx - (float)idx0;
            s_ch1 = _ch1DelayBuffer[idx0] * (1.0f - frac) + _ch1DelayBuffer[idx1] * frac;
            _ch1DelayWriteIdx = (_ch1DelayWriteIdx + 1) % _delayBufferSize;
        }

        // 5. Gain & Mute
        s_ch1 = s_ch1 * ch1_gain;
        if (_config.ch1.mute || _config.mute) s_ch1 = 0.0f;

        // 6. Limiter
        if (ch1_lim_on) {
            float env = fabsf(s_ch1);
            if (env > _ch1LimiterEnvelope)
                _ch1LimiterEnvelope = ch1_att_c * _ch1LimiterEnvelope + (1.0f - ch1_att_c) * env;
            else
                _ch1LimiterEnvelope = ch1_rel_c * _ch1LimiterEnvelope + (1.0f - ch1_rel_c) * env;

            if (_ch1LimiterEnvelope > ch1_thresh) {
                float g = ch1_thresh / (_ch1LimiterEnvelope + 1e-9f);
                s_ch1 *= g;
            }
        }

        // CH1 Safety Hard Clip
        if (s_ch1 > 0.9999f) { s_ch1 = 0.9999f; block_ch1_clip = true; }
        else if (s_ch1 < -0.9999f) { s_ch1 = -0.9999f; block_ch1_clip = true; }

        float ch1_abs = fabsf(s_ch1);
        if (ch1_abs > block_ch1_peak) block_ch1_peak = ch1_abs;
        block_ch1_sum_sq += s_ch1 * s_ch1;

        // ====================================================================
        // CHANNEL 2: DAC 2 OUTPUT (RIGHT)
        // ====================================================================
        float s_ch2 = master_sig * ch2_pol;

        // 1. HPF (up to 48dB)
        if (ch2_hpf_on) {
            s_ch2 = _ch2Hpf[0].process(s_ch2);
            if (ch2_hpf_s >= 24) s_ch2 = _ch2Hpf[1].process(s_ch2);
            if (ch2_hpf_s >= 48) {
                s_ch2 = _ch2Hpf[2].process(s_ch2);
                s_ch2 = _ch2Hpf[3].process(s_ch2);
            }
        }

        // 2. LPF (up to 48dB)
        if (ch2_lpf_on) {
            s_ch2 = _ch2Lpf[0].process(s_ch2);
            if (ch2_lpf_s >= 24) s_ch2 = _ch2Lpf[1].process(s_ch2);
            if (ch2_lpf_s >= 48) {
                s_ch2 = _ch2Lpf[2].process(s_ch2);
                s_ch2 = _ch2Lpf[3].process(s_ch2);
            }
        }

        // 3. PEQ (3 bands)
        if (_config.ch2.peq[0].enabled) s_ch2 = _ch2Peq[0].process(s_ch2);
        if (_config.ch2.peq[1].enabled) s_ch2 = _ch2Peq[1].process(s_ch2);
        if (_config.ch2.peq[2].enabled) s_ch2 = _ch2Peq[2].process(s_ch2);

        // 4. Delay Line
        if (_ch2DelaySamples > 0.5f && _ch2DelayBuffer) {
            _ch2DelayBuffer[_ch2DelayWriteIdx] = s_ch2;
            float r_idx = (float)_ch2DelayWriteIdx - _ch2DelaySamples;
            if (r_idx < 0.0f) r_idx += (float)_delayBufferSize;
            size_t idx0 = (size_t)r_idx;
            size_t idx1 = (idx0 + 1) % _delayBufferSize;
            float frac = r_idx - (float)idx0;
            s_ch2 = _ch2DelayBuffer[idx0] * (1.0f - frac) + _ch2DelayBuffer[idx1] * frac;
            _ch2DelayWriteIdx = (_ch2DelayWriteIdx + 1) % _delayBufferSize;
        }

        // 5. Gain & Mute
        s_ch2 = s_ch2 * ch2_gain;
        if (_config.ch2.mute || _config.mute) s_ch2 = 0.0f;

        // 6. Limiter
        if (ch2_lim_on) {
            float env = fabsf(s_ch2);
            if (env > _ch2LimiterEnvelope)
                _ch2LimiterEnvelope = ch2_att_c * _ch2LimiterEnvelope + (1.0f - ch2_att_c) * env;
            else
                _ch2LimiterEnvelope = ch2_rel_c * _ch2LimiterEnvelope + (1.0f - ch2_rel_c) * env;

            if (_ch2LimiterEnvelope > ch2_thresh) {
                float g = ch2_thresh / (_ch2LimiterEnvelope + 1e-9f);
                s_ch2 *= g;
            }
        }

        // CH2 Safety Hard Clip
        if (s_ch2 > 0.9999f) { s_ch2 = 0.9999f; block_ch2_clip = true; }
        else if (s_ch2 < -0.9999f) { s_ch2 = -0.9999f; block_ch2_clip = true; }

        float ch2_abs = fabsf(s_ch2);
        if (ch2_abs > block_ch2_peak) block_ch2_peak = ch2_abs;
        block_ch2_sum_sq += s_ch2 * s_ch2;

        // Output to I2S:
        // Left Channel  = DAC 1 (Channel 1)
        // Right Channel = DAC 2 (Channel 2)
        out_pcm[i * 2]     = (int16_t)(s_ch1 * 32767.0f);
        out_pcm[i * 2 + 1] = (int16_t)(s_ch2 * 32767.0f);
    }

    // Compute block statistics in dBFS
    float in_rms  = sqrtf(block_in_sum_sq  / (float)frame_count);
    float ch1_rms = sqrtf(block_ch1_sum_sq / (float)frame_count);
    float ch2_rms = sqrtf(block_ch2_sum_sq / (float)frame_count);

    float in_peak_db  = (block_in_peak > 0.0001f)  ? (20.0f * log10f(block_in_peak))  : -60.0f;
    float in_rms_db   = (in_rms > 0.0001f)         ? (20.0f * log10f(in_rms))         : -60.0f;
    float ch1_peak_db = (block_ch1_peak > 0.0001f) ? (20.0f * log10f(block_ch1_peak)) : -60.0f;
    float ch1_rms_db  = (ch1_rms > 0.0001f)        ? (20.0f * log10f(ch1_rms))        : -60.0f;
    float ch2_peak_db = (block_ch2_peak > 0.0001f) ? (20.0f * log10f(block_ch2_peak)) : -60.0f;
    float ch2_rms_db  = (ch2_rms > 0.0001f)        ? (20.0f * log10f(ch2_rms))        : -60.0f;

    if (in_peak_db  < -60.0f) in_peak_db  = -60.0f;
    if (in_rms_db   < -60.0f) in_rms_db   = -60.0f;
    if (ch1_peak_db < -60.0f) ch1_peak_db = -60.0f;
    if (ch1_rms_db  < -60.0f) ch1_rms_db  = -60.0f;
    if (ch2_peak_db < -60.0f) ch2_peak_db = -60.0f;
    if (ch2_rms_db  < -60.0f) ch2_rms_db  = -60.0f;

    // Smooth ballistics
    if (in_peak_db > _inPeakDb) _inPeakDb = in_peak_db;
    else _inPeakDb = 0.92f * _inPeakDb + 0.08f * in_peak_db;
    _inRmsDb = 0.85f * _inRmsDb + 0.15f * in_rms_db;

    if (ch1_peak_db > _ch1PeakDb) _ch1PeakDb = ch1_peak_db;
    else _ch1PeakDb = 0.92f * _ch1PeakDb + 0.08f * ch1_peak_db;
    _ch1RmsDb = 0.85f * _ch1RmsDb + 0.15f * ch1_rms_db;

    if (ch2_peak_db > _ch2PeakDb) _ch2PeakDb = ch2_peak_db;
    else _ch2PeakDb = 0.92f * _ch2PeakDb + 0.08f * ch2_peak_db;
    _ch2RmsDb = 0.85f * _ch2RmsDb + 0.15f * ch2_rms_db;

    if (block_ch1_clip) _ch1ClipFlag = true;
    if (block_ch2_clip) _ch2ClipFlag = true;
}

VuMeterData DspEngine::getVuMeterData() {
    VuMeterData data;
    data.in_peak_db   = _inPeakDb;
    data.in_rms_db    = _inRmsDb;
    data.ch1_peak_db  = _ch1PeakDb;
    data.ch1_rms_db   = _ch1RmsDb;
    data.ch2_peak_db  = _ch2PeakDb;
    data.ch2_rms_db   = _ch2RmsDb;
    data.out_peak_db  = (_ch1PeakDb > _ch2PeakDb) ? _ch1PeakDb : _ch2PeakDb;
    data.out_rms_db   = (_ch1RmsDb > _ch2RmsDb) ? _ch1RmsDb : _ch2RmsDb;
    data.limiter_gr_db = _currentGainReductionDb;
    data.ch1_clip     = _ch1ClipFlag;
    data.ch2_clip     = _ch2ClipFlag;
    data.clip         = _ch1ClipFlag || _ch2ClipFlag;

    // Legacy aliases
    data.sub_peak_db  = _ch1PeakDb;
    data.sub_rms_db   = _ch1RmsDb;
    data.mid_peak_db  = _ch2PeakDb;
    data.mid_rms_db   = _ch2RmsDb;
    data.sub_clip     = _ch1ClipFlag;
    data.mid_clip     = _ch2ClipFlag;

    _ch1ClipFlag = false;
    _ch2ClipFlag = false;

    return data;
}
