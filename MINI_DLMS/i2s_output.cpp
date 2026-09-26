#include "i2s_output.h"

I2sOutput i2sOutput;

I2sOutput::I2sOutput()
    : _sampleRate(AUDIO_SAMPLE_RATE_DEFAULT),
      _running(false)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    _txChanHandle = nullptr;
#else
    _i2sPort = 1; // Use I2S_NUM_1 for PCM5102 DAC output
#endif
}

I2sOutput::~I2sOutput() {
    end();
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <driver/i2s_std.h>

bool I2sOutput::begin(uint32_t sample_rate) {
    if (_running) end();
    _sampleRate = (sample_rate > 0) ? sample_rate : AUDIO_SAMPLE_RATE_DEFAULT;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 256;
    chan_cfg.auto_clear = true;

    i2s_chan_handle_t tx_chan = NULL;
    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_chan, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Error creating channel: %d\n", err);
        return false;
    }
    _txChanHandle = (void*)tx_chan;

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sampleRate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCK_PIN,
            .ws = (gpio_num_t)I2S_LRCK_PIN,
            .dout = (gpio_num_t)I2S_DATA_PIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(tx_chan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Error init std mode: %d\n", err);
        return false;
    }

    err = i2s_channel_enable(tx_chan);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Error enable channel: %d\n", err);
        return false;
    }

    _running = true;
    Serial.printf("[I2S] PCM5102 Ready: %u Hz, BCK=%d, DIN=%d, LRCK=%d\n",
                  _sampleRate, I2S_BCK_PIN, I2S_DATA_PIN, I2S_LRCK_PIN);
    return true;
}

void I2sOutput::end() {
    if (!_running) return;
    i2s_chan_handle_t tx_chan = (i2s_chan_handle_t)_txChanHandle;
    if (tx_chan) {
        i2s_channel_disable(tx_chan);
        i2s_del_channel(tx_chan);
        _txChanHandle = nullptr;
    }
    _running = false;
}

bool I2sOutput::setSampleRate(uint32_t sample_rate) {
    if (sample_rate == 0 || sample_rate == _sampleRate) return true;
    _sampleRate = sample_rate;
    if (!_running) return true;

    i2s_chan_handle_t tx_chan = (i2s_chan_handle_t)_txChanHandle;
    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
    esp_err_t err = i2s_channel_reconfig_std_clock(tx_chan, &clk_cfg);
    if (err != ESP_OK) {
        return begin(sample_rate);
    }
    Serial.printf("[I2S] Sample rate changed to %u Hz\n", sample_rate);
    return true;
}

size_t I2sOutput::write(const int16_t* pcm_data, size_t byte_count) {
    if (!_running || !pcm_data || byte_count == 0) return 0;
    i2s_chan_handle_t tx_chan = (i2s_chan_handle_t)_txChanHandle;
    size_t bytes_written = 0;
    esp_err_t err = i2s_channel_write(tx_chan, pcm_data, byte_count, &bytes_written, portMAX_DELAY);
    return (err == ESP_OK) ? bytes_written : 0;
}

#else
#include <driver/i2s.h>

bool I2sOutput::begin(uint32_t sample_rate) {
    if (_running) end();
    _sampleRate = (sample_rate > 0) ? sample_rate : AUDIO_SAMPLE_RATE_DEFAULT;

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = (int)_sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_LRCK_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install((i2s_port_t)_i2sPort, &i2s_config, 0, NULL);
    if (err != ESP_OK) return false;

    err = i2s_set_pin((i2s_port_t)_i2sPort, &pin_config);
    if (err != ESP_OK) return false;

    i2s_zero_dma_buffer((i2s_port_t)_i2sPort);
    _running = true;
    Serial.printf("[I2S] PCM5102 Ready (Legacy): %u Hz, BCK=%d, DIN=%d, LRCK=%d\n",
                  _sampleRate, I2S_BCK_PIN, I2S_DATA_PIN, I2S_LRCK_PIN);
    return true;
}

void I2sOutput::end() {
    if (!_running) return;
    i2s_driver_uninstall((i2s_port_t)_i2sPort);
    _running = false;
}

bool I2sOutput::setSampleRate(uint32_t sample_rate) {
    if (sample_rate == 0 || sample_rate == _sampleRate) return true;
    _sampleRate = sample_rate;
    if (!_running) return true;
    return (i2s_set_sample_rates((i2s_port_t)_i2sPort, sample_rate) == ESP_OK);
}

size_t I2sOutput::write(const int16_t* pcm_data, size_t byte_count) {
    if (!_running || !pcm_data || byte_count == 0) return 0;
    size_t bytes_written = 0;
    esp_err_t err = i2s_write((i2s_port_t)_i2sPort, pcm_data, byte_count, &bytes_written, portMAX_DELAY);
    return (err == ESP_OK) ? bytes_written : 0;
}
#endif
