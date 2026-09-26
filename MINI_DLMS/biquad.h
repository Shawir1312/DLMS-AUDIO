#pragma once
#include <Arduino.h>
#include <math.h>

// Direct Form II Transposed Biquad Filter
// Uses single-precision float for real-time DSP to utilize ESP32-S3 Hardware FPU (1-cycle float)
// Setup math is computed in double precision to eliminate coefficient rounding error
class Biquad {
public:
    float b0, b1, b2;
    float a1, a2;
    float d1, d2;

    Biquad() {
        reset();
        setPassThrough();
    }

    inline void reset() {
        d1 = 0.0f;
        d2 = 0.0f;
    }

    inline void setPassThrough() {
        b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
        a1 = 0.0f; a2 = 0.0f;
    }

    inline float process(float in) {
        float out = b0 * in + d1;
        d1 = b1 * in - a1 * out + d2;
        d2 = b2 * in - a2 * out;

        // Fast NaN and infinity safety check
        if (isnan(out) || isinf(out)) {
            d1 = 0.0f;
            d2 = 0.0f;
            return 0.0f;
        }

        // State stabilization: prevent runaway / feedback accumulation
        if (fabsf(d1) > 8.0f) d1 = 0.0f;
        if (fabsf(d2) > 8.0f) d2 = 0.0f;

        return out;
    }

    // Robert Bristow-Johnson Audio EQ Cookbook implementations
    void setLowPass(float f0, float fs, float q = 0.70710678f) {
        if (fs <= 0.0f) fs = 44100.0f;
        if (f0 >= fs * 0.49f) f0 = fs * 0.49f;
        if (f0 < 10.0f) f0 = 10.0f;
        if (q < 0.05f) q = 0.05f;

        double omega = 2.0 * M_PI * (double)f0 / (double)fs;
        double alpha = sin(omega) / (2.0 * (double)q);
        double cos_w = cos(omega);

        double a0_inv = 1.0 / (1.0 + alpha);
        b0 = (float)(((1.0 - cos_w) * 0.5) * a0_inv);
        b1 = (float)((1.0 - cos_w) * a0_inv);
        b2 = (float)(((1.0 - cos_w) * 0.5) * a0_inv);
        a1 = (float)((-2.0 * cos_w) * a0_inv);
        a2 = (float)((1.0 - alpha) * a0_inv);
    }

    void setHighPass(float f0, float fs, float q = 0.70710678f) {
        if (fs <= 0.0f) fs = 44100.0f;
        if (f0 >= fs * 0.49f) f0 = fs * 0.49f;
        if (f0 < 10.0f) f0 = 10.0f;
        if (q < 0.05f) q = 0.05f;

        double omega = 2.0 * M_PI * (double)f0 / (double)fs;
        double alpha = sin(omega) / (2.0 * (double)q);
        double cos_w = cos(omega);

        double a0_inv = 1.0 / (1.0 + alpha);
        b0 = (float)(((1.0 + cos_w) * 0.5) * a0_inv);
        b1 = (float)(-(1.0 + cos_w) * a0_inv);
        b2 = (float)(((1.0 + cos_w) * 0.5) * a0_inv);
        a1 = (float)((-2.0 * cos_w) * a0_inv);
        a2 = (float)((1.0 - alpha) * a0_inv);
    }

    void setPeakingEQ(float f0, float fs, float gain_db, float q = 1.0f) {
        if (fs <= 0.0f) fs = 44100.0f;
        if (f0 >= fs * 0.49f) f0 = fs * 0.49f;
        if (f0 < 10.0f) f0 = 10.0f;
        if (q < 0.05f) q = 0.05f;

        double A = pow(10.0, (double)gain_db / 40.0);
        double omega = 2.0 * M_PI * (double)f0 / (double)fs;
        double alpha = sin(omega) / (2.0 * (double)q);
        double cos_w = cos(omega);

        double a0_inv = 1.0 / (1.0 + alpha / A);
        b0 = (float)((1.0 + alpha * A) * a0_inv);
        b1 = (float)((-2.0 * cos_w) * a0_inv);
        b2 = (float)((1.0 - alpha * A) * a0_inv);
        a1 = (float)((-2.0 * cos_w) * a0_inv);
        a2 = (float)((1.0 - alpha / A) * a0_inv);
    }

    void setLowShelf(float f0, float fs, float gain_db, float q = 0.70710678f) {
        if (fs <= 0.0f) fs = 44100.0f;
        if (f0 >= fs * 0.49f) f0 = fs * 0.49f;
        if (f0 < 10.0f) f0 = 10.0f;
        if (q < 0.05f) q = 0.05f;

        double A = pow(10.0, (double)gain_db / 40.0);
        double omega = 2.0 * M_PI * (double)f0 / (double)fs;
        double cos_w = cos(omega);
        double sin_w = sin(omega);
        double alpha = sin_w / (2.0 * (double)q);
        double beta = 2.0 * sqrt(A) * alpha;

        double a0 = (A + 1.0) + (A - 1.0) * cos_w + beta;
        double a0_inv = 1.0 / a0;

        b0 = (float)((A * ((A + 1.0) - (A - 1.0) * cos_w + beta)) * a0_inv);
        b1 = (float)((2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w)) * a0_inv);
        b2 = (float)((A * ((A + 1.0) - (A - 1.0) * cos_w - beta)) * a0_inv);
        a1 = (float)((-2.0 * ((A - 1.0) + (A + 1.0) * cos_w)) * a0_inv);
        a2 = (float)(((A + 1.0) + (A - 1.0) * cos_w - beta) * a0_inv);
    }

    void setHighShelf(float f0, float fs, float gain_db, float q = 0.70710678f) {
        if (fs <= 0.0f) fs = 44100.0f;
        if (f0 >= fs * 0.49f) f0 = fs * 0.49f;
        if (f0 < 10.0f) f0 = 10.0f;
        if (q < 0.05f) q = 0.05f;

        double A = pow(10.0, (double)gain_db / 40.0);
        double omega = 2.0 * M_PI * (double)f0 / (double)fs;
        double cos_w = cos(omega);
        double sin_w = sin(omega);
        double alpha = sin_w / (2.0 * (double)q);
        double beta = 2.0 * sqrt(A) * alpha;

        double a0 = (A + 1.0) - (A - 1.0) * cos_w + beta;
        double a0_inv = 1.0 / a0;

        b0 = (float)((A * ((A + 1.0) + (A - 1.0) * cos_w + beta)) * a0_inv);
        b1 = (float)((-2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w)) * a0_inv);
        b2 = (float)((A * ((A + 1.0) + (A - 1.0) * cos_w - beta)) * a0_inv);
        a1 = (float)((2.0 * ((A - 1.0) - (A + 1.0) * cos_w)) * a0_inv);
        a2 = (float)(((A + 1.0) - (A - 1.0) * cos_w - beta) * a0_inv);
    }
};
