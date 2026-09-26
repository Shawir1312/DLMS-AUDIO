#pragma once
#include <Arduino.h>

// ============================================================================
// S.NET AUDIO MANAGEMENT - 1 CHANNEL DSP PROCESSOR
// Target Hardware: ESP32-S3 (DSP Processor + Web Server + PCM5102 DAC)
// ============================================================================

// 1. Hardware Pins: Output I2S Master ke PCM5102 DAC (I2S_NUM_1)
// Channel LEFT  = DAC 1: CHANNEL 1 (Configurable: Sub / Mid / Low / Full-range)
// Channel RIGHT = DAC 2: CHANNEL 2 (Configurable: Sub / Mid / High / Full-range)
#define I2S_TX_BCK_PIN        15   // BCK  -> PCM5102 DAC 1 & 2 BCK
#define I2S_TX_LRCK_PIN       16   // LRCK -> PCM5102 DAC 1 & 2 LRCK
#define I2S_TX_DATA_PIN       17   // DIN  -> PCM5102 DAC 1 & 2 DIN

// Kompatibilitas alias untuk pin DAC
#define I2S_BCK_PIN           I2S_TX_BCK_PIN
#define I2S_LRCK_PIN          I2S_TX_LRCK_PIN
#define I2S_DATA_PIN          I2S_TX_DATA_PIN

// 2. Hardware Pins: Input I2S Slave dari ESP-32S Bluetooth atau PCM1808 ADC (I2S_NUM_0)
#define I2S_RX_BCK_PIN        4    // BCK IN  <- Pin 26 ESP32-S / PCM1808 BCK
#define I2S_RX_LRCK_PIN       5    // LRCK IN <- Pin 25 ESP32-S / PCM1808 LRCK
#define I2S_RX_DATA_PIN       6    // DATA IN <- Pin 22 ESP32-S / PCM1808 DOUT

// 3. Hardware Pins: 1.8" SPI TFT LCD Module 128x160 (Driver ST7735)
// Sesuai modul 8-pin di bagian bawah:
// Pin 1 (RST) -> GPIO 14
// Pin 2 (CS)  -> GPIO 10
// Pin 3 (D/C) -> GPIO 9
// Pin 4 (DIN) -> GPIO 11
// Pin 5 (CLK) -> GPIO 12
// Pin 6 (VCC) -> Wajib colok ke Pin 5V (VIN) ESP32 (karena jumper J1 open / modul punya LDO U21)
// Pin 7 (BL)  -> Colok ke Pin 3.3V ESP32 atau GPIO 8
// Pin 8 (GND) -> GND
#define TFT_CS_PIN            10   // Pin 2 - CS
#define TFT_DC_PIN            9    // Pin 3 - D/C (Command/Data)
#define TFT_RST_PIN           14   // Pin 1 - RST
#define TFT_MOSI_PIN          11   // Pin 4 - DIN (SDA/MOSI)
#define TFT_SCLK_PIN          12   // Pin 5 - CLK (SCK/SCL)
#define TFT_BL_PIN            8    // Pin 7 - BL (GPIO 8 atau colok langsung ke 3.3V)

// 4. Hardware Pins: Rotary Encoder (EC11 dengan Push Button)
// Set ke false jika modul rotary encoder fisik belum dipasang / belum datang
// agar pin floating (GPIO 1, 2, 42) tidak memicu pulsa hantu/acak ke menu LCD!
// Ubah ke true jika modul EC11 fisik sudah dicolok ke pin GPIO.
#define ENCODER_PHYSICAL_ATTACHED false

#define ENCODER_CLK_PIN       1    // Pin CLK / A
#define ENCODER_DT_PIN        2    // Pin DT / B
#define ENCODER_SW_PIN        42   // Pin Tombol Push / SW (Active LOW)

// 5. Wi-Fi Configuration (SoftAP Standalone)
#define WIFI_FALLBACK_AP_SSID "SNET-AUDIO"
#define WIFI_FALLBACK_AP_PASS ""   // Open Network (tanpa password, koneksi instan)

// Optional Station Wi-Fi
#define WIFI_STA_SSID         ""
#define WIFI_STA_PASS         ""

// Web Server Port
#define WEB_SERVER_PORT       80

// Audio Buffer & DSP Constants
#define AUDIO_SAMPLE_RATE_DEFAULT  44100
#define AUDIO_RING_BUFFER_SIZE     (16 * 1024)   // 16 KB ring buffer
#define MAX_DELAY_MS               50.0f         // Delay max 50ms
#define MAX_DELAY_SAMPLES          2400          // 50ms pada 48kHz
#define PEQ_BAND_COUNT             5             // 5-Band Parametric EQ

// Serial Debug Baud Rate
#define SERIAL_DEBUG_BAUD          115200
