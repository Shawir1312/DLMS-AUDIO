#include "bt_audio.h"
#include "dsp_engine.h"
#include "i2s_output.h"
#include "config.h"
#include <esp_idf_version.h>

BtAudio btAudio;
static TaskHandle_t s_dspTaskHandle = nullptr;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <driver/i2s_std.h>
#else
#include <driver/i2s.h>
#endif

// FreeRTOS Task: Real-time Audio DSP Processing & DAC Output on Core 1
static void audioDspTask(void* param) {
    const size_t CHUNK_FRAMES = 128; // 128 stereo frames = ~2.9ms pada 44.1kHz
    const size_t CHUNK_BYTES = CHUNK_FRAMES * 4;
    int16_t in_pcm[CHUNK_FRAMES * 2];
    int16_t out_pcm[CHUNK_FRAMES * 2];

    while (1) {
        // Baca data audio lossless digital dari I2S Input (ESP32-S BT Receiver)
        size_t bytes_read = btAudio.read(in_pcm, CHUNK_BYTES, pdMS_TO_TICKS(40));

        if (bytes_read >= 4) {
            size_t frame_count = bytes_read / 4;

            // Periksa apakah ada sinyal audio aktif
            bool has_sound = false;
            for (size_t i = 0; i < frame_count * 2; i += 4) {
                if (abs(in_pcm[i]) > 10) {
                    has_sound = true;
                    break;
                }
            }
            btAudio.setStreaming(has_sound);

            // Eksekusi Pipeline DSP (Gain, Polarity, HPF, LPF, 5-PEQ, Delay, Limiter, VU)
            dspEngine.processAudio(in_pcm, out_pcm, frame_count);

            // Kirim ke DAC PCM5102 via I2S_NUM_1
            i2sOutput.write(out_pcm, frame_count * 4);
        } else {
            // Jika ESP32-S belum mengirim sinyal / idle
            btAudio.setStreaming(false);
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

BtAudio::BtAudio()
    : _state(BT_DISCONNECTED),
      _sampleRate(AUDIO_SAMPLE_RATE_DEFAULT),
      _streaming(false),
      _lastAudioTime(0)
{
    strncpy(_deviceName, "ESP32-S BT RECEIVER", sizeof(_deviceName) - 1);
    _deviceName[sizeof(_deviceName) - 1] = '\0';

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    _rxChanHandle = nullptr;
#else
    _i2sPort = 0; // I2S_NUM_0 untuk input dari BT board
#endif
}

BtAudio::~BtAudio() {
    end();
}

bool BtAudio::begin(const char* device_name) {
    if (device_name && strlen(device_name) > 0) {
        strncpy(_deviceName, device_name, sizeof(_deviceName) - 1);
        _deviceName[sizeof(_deviceName) - 1] = '\0';
    }

    _sampleRate = AUDIO_SAMPLE_RATE_DEFAULT;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    // 1. Konfigurasi I2S_NUM_0 Slave Receiver (ESP-IDF 5.x)
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_SLAVE);
    rx_chan_cfg.dma_desc_num = 8;
    rx_chan_cfg.dma_frame_num = 256;
    rx_chan_cfg.auto_clear = false;

    i2s_chan_handle_t rx_chan = NULL;
    esp_err_t err = i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan);
    if (err != ESP_OK) {
        Serial.printf("[I2S-IN] Gagal membuat channel RX: %d\n", err);
        return false;
    }
    _rxChanHandle = (void*)rx_chan;

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sampleRate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_RX_BCK_PIN,
            .ws   = (gpio_num_t)I2S_RX_LRCK_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)I2S_RX_DATA_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(rx_chan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[I2S-IN] Gagal init mode STD RX: %d\n", err);
        return false;
    }

    err = i2s_channel_enable(rx_chan);
    if (err != ESP_OK) {
        Serial.printf("[I2S-IN] Gagal enable RX: %d\n", err);
        return false;
    }

#else
    // 1. Konfigurasi I2S_NUM_0 Slave Receiver (ESP-IDF 4.x / Legacy)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX),
        .sample_rate = (int)_sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_RX_BCK_PIN,
        .ws_io_num = I2S_RX_LRCK_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_RX_DATA_PIN
    };

    esp_err_t err = i2s_driver_install((i2s_port_t)_i2sPort, &i2s_config, 0, NULL);
    if (err != ESP_OK) return false;

    err = i2s_set_pin((i2s_port_t)_i2sPort, &pin_config);
    if (err != ESP_OK) return false;
#endif

    Serial.printf("[I2S-IN] Input Receiver Siap: BCK=Pin %d, LRCK=Pin %d, DATA=Pin %d\n",
                  I2S_RX_BCK_PIN, I2S_RX_LRCK_PIN, I2S_RX_DATA_PIN);

    // 2. Jalankan Dedicated DSP Audio Processing Task di Core 1
    if (!s_dspTaskHandle) {
        BaseType_t res = xTaskCreatePinnedToCore(
            audioDspTask,
            "AudioDspTask",
            4096,
            nullptr,
            configMAX_PRIORITIES - 2, // Prioritas tinggi untuk audio real-time
            &s_dspTaskHandle,
            1                         // Core 1 (Core 0 menangani WiFi/Web Server)
        );
        if (res != pdPASS) {
            Serial.println("[DSP] Gagal membuat task audio!");
            return false;
        }
    }

    _state = BT_CONNECTED;
    return true;
}

void BtAudio::end() {
    if (s_dspTaskHandle) {
        vTaskDelete(s_dspTaskHandle);
        s_dspTaskHandle = nullptr;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_handle_t rx_chan = (i2s_chan_handle_t)_rxChanHandle;
    if (rx_chan) {
        i2s_channel_disable(rx_chan);
        i2s_del_channel(rx_chan);
        _rxChanHandle = nullptr;
    }
#else
    i2s_driver_uninstall((i2s_port_t)_i2sPort);
#endif

    _state = BT_DISCONNECTED;
    _streaming = false;
}

size_t BtAudio::read(int16_t* buffer, size_t byte_count, TickType_t timeout) {
    if (!buffer || byte_count == 0) return 0;
    size_t bytes_read = 0;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_handle_t rx_chan = (i2s_chan_handle_t)_rxChanHandle;
    if (!rx_chan) return 0;
    esp_err_t err = i2s_channel_read(rx_chan, buffer, byte_count, &bytes_read, timeout);
    return (err == ESP_OK) ? bytes_read : 0;
#else
    esp_err_t err = i2s_read((i2s_port_t)_i2sPort, buffer, byte_count, &bytes_read, timeout);
    return (err == ESP_OK) ? bytes_read : 0;
#endif
}

void BtAudio::setStreaming(bool active) {
    if (active) {
        _lastAudioTime = millis();
        _streaming = true;
        _state = BT_STREAMING;
    } else {
        if (_streaming && millis() - _lastAudioTime > 500) {
            _streaming = false;
            _state = BT_CONNECTED;
        }
    }
}

const char* BtAudio::getStateString() const {
    switch (_state) {
        case BT_STREAMING:    return "STREAMING (AUDIO AKTIF)";
        case BT_CONNECTED:    return "STANDBY (RECEIVER READY)";
        case BT_CONNECTING:   return "CONNECTING...";
        case BT_DISCONNECTED: 
        default:              return "DISCONNECTED";
    }
}

bool BtAudio::isConnected() const {
    return (_state == BT_CONNECTED || _state == BT_STREAMING);
}

bool BtAudio::isStreaming() const {
    return _streaming;
}
