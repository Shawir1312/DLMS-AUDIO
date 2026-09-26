#pragma once
#include <Arduino.h>
#include "config.h"
#include <esp_idf_version.h>

class I2sOutput {
public:
    I2sOutput();
    ~I2sOutput();

    bool begin(uint32_t sample_rate = AUDIO_SAMPLE_RATE_DEFAULT);
    void end();
    bool setSampleRate(uint32_t sample_rate);
    size_t write(const int16_t* pcm_data, size_t byte_count);

    uint32_t getSampleRate() const { return _sampleRate; }
    bool isRunning() const { return _running; }

private:
    uint32_t _sampleRate;
    bool _running;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    void* _txChanHandle;
#else
    int _i2sPort;
#endif
};

extern I2sOutput i2sOutput;
