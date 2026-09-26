// ============================================================================
// S.NET AUDIO - DEDICATED BLUETOOTH A2DP RECEIVER
// Board Target: ESP32 / ESP-32S / ESP32-WROOM-32 (Xtensa LX6)
//
// Tugas Board Ini:
//   Menerima audio Bluetooth Classic A2DP dari HP ("SNET AUDIO DSP")
//   dan mengirimkan audio digital lossless I2S langsung ke ESP32-S3.
//
// Sambungan Kabel ke ESP32-S3 (DSP Processor):
//   ESP32-S Pin 26 (BCK)  --> ESP32-S3 Pin 4  (I2S RX BCK)
//   ESP32-S Pin 25 (LRCK) --> ESP32-S3 Pin 5  (I2S RX WS/LRCK)
//   ESP32-S Pin 22 (DOUT) --> ESP32-S3 Pin 6  (I2S RX DATA)
//   ESP32-S GND           --> ESP32-S3 GND    (WAJIB Sambung Ground Bersama!)
// ============================================================================

#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include <esp_idf_version.h>

// Driver I2S Kompatibel untuk ESP-IDF 4.x dan ESP-IDF 5.x
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <driver/i2s_std.h>
static i2s_chan_handle_t s_tx_chan = NULL;
#else
#include <driver/i2s.h>
#endif

// Pin I2S Master Transmitter ke ESP32-S3
#define BT_I2S_BCK_PIN   26
#define BT_I2S_LRCK_PIN  25
#define BT_I2S_DATA_PIN  22

// Nama Bluetooth yang muncul di HP
#define BT_DEVICE_NAME   "SNET AUDIO DSP"

// Inisialisasi Bluetooth A2DP Sink
BluetoothA2DPSink a2dp_sink;

// Inisialisasi Driver Hardware I2S Master Transmitter
static void init_i2s_transmitter(uint32_t sample_rate = 44100) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 256;
    chan_cfg.auto_clear = true;

    esp_err_t err = i2s_new_channel(&chan_cfg, &s_tx_chan, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Error new channel: %d\n", err);
        return;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)BT_I2S_BCK_PIN,
            .ws = (gpio_num_t)BT_I2S_LRCK_PIN,
            .dout = (gpio_num_t)BT_I2S_DATA_PIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(s_tx_chan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Error init std: %d\n", err);
        return;
    }

    i2s_channel_enable(s_tx_chan);
#else
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = (int)sample_rate,
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
        .bck_io_num = BT_I2S_BCK_PIN,
        .ws_io_num = BT_I2S_LRCK_PIN,
        .data_out_num = BT_I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
#endif
    Serial.printf("[I2S] Master Transmitter Siap: %u Hz (BCK=%d, LRCK=%d, DATA=%d)\n",
                  sample_rate, BT_I2S_BCK_PIN, BT_I2S_LRCK_PIN, BT_I2S_DATA_PIN);
}

// Callback penerima data stream audio Bluetooth -> Tulis ke I2S
static void audio_data_stream_reader(const uint8_t *data, uint32_t length) {
    if (!data || length == 0) return;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    if (s_tx_chan) {
        size_t bytes_written = 0;
        i2s_channel_write(s_tx_chan, data, length, &bytes_written, portMAX_DELAY);
    }
#else
    size_t bytes_written = 0;
    i2s_write(I2S_NUM_0, data, length, &bytes_written, portMAX_DELAY);
#endif
}

// Callback negosiasi sample rate
static void sample_rate_changed(uint16_t rate) {
    if (rate > 0) {
        Serial.printf("[BT] Sample rate dinegosiasikan: %u Hz\n", rate);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        if (s_tx_chan) {
            i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate);
            i2s_channel_reconfig_std_clock(s_tx_chan, &clk_cfg);
        }
#else
        i2s_set_sample_rates(I2S_NUM_0, rate);
#endif
    }
}

// Callback status koneksi Bluetooth
static void connection_state_changed(esp_a2d_connection_state_t state, void* ptr) {
    switch (state) {
        case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
            Serial.println("[BT] Status: Terputus (Disconnected)");
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTING:
            Serial.println("[BT] Status: Menyambungkan (Connecting)...");
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
            Serial.println("[BT] Status: TERSAMBUNG (Connected) ke HP!");
            break;
        case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
            Serial.println("[BT] Status: Memutuskan sambungan...");
            break;
    }
}

// Callback status audio streaming
static void audio_state_changed(esp_a2d_audio_state_t state, void* ptr) {
    if (state == ESP_A2D_AUDIO_STATE_STARTED) {
        Serial.println("[BT] Audio: SEDANG MEMUTAR LAGU (Streaming Audio)");
    } else {
        Serial.println("[BT] Audio: Berhenti / Dijeda (Paused)");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==============================================");
    Serial.println("  S.NET AUDIO - DEDICATED BLUETOOTH RECEIVER");
    Serial.println("  Hardware: ESP32 / ESP-32S (Xtensa LX6)");
    Serial.println("==============================================");

    // 1. Inisialisasi Hardware I2S Master Transmitter ke ESP32-S3
    init_i2s_transmitter(44100);

    // 2. Konfigurasi Stream Reader Callback (false = kirim raw PCM ke callback)
    a2dp_sink.set_stream_reader(audio_data_stream_reader, false);
    a2dp_sink.set_on_connection_state_changed(connection_state_changed);
    a2dp_sink.set_on_audio_state_changed(audio_state_changed);
    a2dp_sink.set_sample_rate_callback(sample_rate_changed);

    // 3. Mulai Bluetooth Sink (auto_reconnect = false agar menerima pairing baru dengan instan)
    a2dp_sink.start(BT_DEVICE_NAME, false);

    Serial.println();
    Serial.printf("[BT] Nama Bluetooth : %s\n", BT_DEVICE_NAME);
    Serial.printf("[I2S OUT ke S3]     : BCK=Pin %d, LRCK=Pin %d, DATA=Pin %d\n", 
                  BT_I2S_BCK_PIN, BT_I2S_LRCK_PIN, BT_I2S_DATA_PIN);
    Serial.println("[BT] Siap! Buka Bluetooth di HP Anda dan sambungkan.");
    Serial.println("==============================================\n");
}

void loop() {
    // Bluetooth A2DP Sink berjalan otomatis di latar belakang FreeRTOS
    delay(1000);
}
