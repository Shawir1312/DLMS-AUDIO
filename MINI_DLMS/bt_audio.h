#pragma once
#include <Arduino.h>
#include "dsp_types.h"
#include "config.h"

// ============================================================================
// S.NET AUDIO MANAGEMENT - AUDIO INPUT RECEIVER (ESP32-S3)
// Menerima audio digital lossless I2S dari board ESP-32S Bluetooth Receiver
// ============================================================================

class BtAudio {
public:
    BtAudio();
    ~BtAudio();

    bool begin(const char* device_name = "ESP32-S BT RECEIVER");
    void end();

    BtState getState() const { return _state; }
    const char* getStateString() const;
    bool isConnected() const;
    bool isStreaming() const;
    uint32_t getSampleRate() const { return _sampleRate; }

    // Membaca audio PCM dari I2S Input (I2S_NUM_0)
    size_t read(int16_t* buffer, size_t byte_count, TickType_t timeout);

    void setStreaming(bool active);

private:
    BtState _state;
    uint32_t _sampleRate;
    bool _streaming;
    char _deviceName[32];
    unsigned long _lastAudioTime;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    void* _rxChanHandle;
#else
    int _i2sPort;
#endif
};

extern BtAudio btAudio;
