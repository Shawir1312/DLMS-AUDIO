// ============================================================================
// S.NET AUDIO MANAGEMENT - 1 CHANNEL DSP PROCESSOR
// Target Hardware: ESP32-S3 (Dual-Core LX7) + PCM5102 I2S DAC
// Digital Input: Lossless I2S dari ESP-32S (Dedicated Bluetooth Receiver)
// ============================================================================

#include <Arduino.h>
#include "config.h"
#include "dsp_engine.h"
#include "i2s_output.h"
#include "bt_audio.h"
#include "presets_manager.h"
#include "web_server_dsp.h"
#include "rotary_encoder.h"
#include "tft_display.h"

void printBootBanner() {
    Serial.println("\n");
    Serial.println("================================================");
    Serial.println("  S.NET AUDIO MANAGEMENT - 2-WAY DLMS PROCESSOR");
    Serial.println("  Core Processor : ESP32-S3 (DSP + Web + TFT + DAC)");
    Serial.println("================================================");
    Serial.println();
    Serial.println("ESP32-S3 Audio DSP Starting...");
    Serial.println();
}

void setup() {
    // 1. Initialize Serial Monitor
    Serial.begin(SERIAL_DEBUG_BAUD);
    delay(500);
    printBootBanner();

    // 2. Initialize Presets / NVS Storage
    presetsManager.begin();
    DspConfig activeConfig;
    if (presetsManager.loadPreset(presetsManager.getCurrentSlot(), activeConfig)) {
        Serial.printf("Loaded Preset %u from NVS\n", presetsManager.getCurrentSlot());
    } else {
        activeConfig = PresetsManager::getDefaultConfig();
        Serial.println("Using Default DSP Profile");
    }

    // 3. Initialize DSP Engine with default sample rate (Core 1)
    dspEngine.init(AUDIO_SAMPLE_RATE_DEFAULT);
    dspEngine.setConfig(activeConfig);
    Serial.println("DSP Engine: READY (2-Way Active Crossover)");

    // 4. Initialize I2S_NUM_1 Output ke Dual PCM5102 DAC (Sub: L, Mid: R)
    if (i2sOutput.begin(AUDIO_SAMPLE_RATE_DEFAULT)) {
        Serial.println("I2S DAC (PCM5102 Dual Way): READY");
    } else {
        Serial.println("I2S DAC (PCM5102 Dual Way): ERROR INITIALIZING!");
    }

    // 5. Initialize I2S_NUM_0 Input Receiver dari ESP32-S / PCM1808 ADC
    if (btAudio.begin("ESP32-S BT / ADC RECEIVER")) {
        Serial.println("I2S Audio Input (from BT / ADC): READY");
    } else {
        Serial.println("I2S Audio Input: FAILED!");
    }

    // 6. Initialize Wi-Fi SoftAP & Web Server di Core 0
    Serial.println("WiFi: STARTING...");
    webServerDsp.begin();
    Serial.println("Web Server: READY");

    // 7. Initialize Rotary Encoder
    rotaryEncoder.begin(ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);
    Serial.printf("Rotary Encoder: READY (CLK=%d, DT=%d, SW=%d)\n", 
                  ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);

    // 8. Initialize 1.8" SPI TFT LCD (ST7735 128x160)
    tftDisplay.begin();
    Serial.printf("TFT Display 1.8\": READY (CS=%d, DC=%d, RST=%d, MOSI=%d, SCLK=%d)\n",
                  TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, TFT_MOSI_PIN, TFT_SCLK_PIN);

    Serial.println();
    Serial.println("==================================================");
    Serial.printf(" Web Interface   : http://%s\n", webServerDsp.getIpAddress().c_str());
    Serial.printf(" I2S RX (Input)  : BCK=Pin %d, WS=Pin %d, DIN=Pin %d\n", 
                  I2S_RX_BCK_PIN, I2S_RX_LRCK_PIN, I2S_RX_DATA_PIN);
    Serial.printf(" I2S TX (to DACs): BCK=Pin %d, WS=Pin %d, DIN=Pin %d\n", 
                  I2S_TX_BCK_PIN, I2S_TX_LRCK_PIN, I2S_TX_DATA_PIN);
    Serial.println(" DAC Channel L   : WAY 1 (SUBWOOFER / LOW)");
    Serial.println(" DAC Channel R   : WAY 2 (MID / HIGH)");
    Serial.printf(" Sample Rate     : %u Hz\n", dspEngine.getSampleRate());
    Serial.println("==================================================");
    Serial.println("System Running. Audio DSP on Core 1, Web/UI on Core 0.");
}

void loop() {
    // 1. Handle incoming Web Server client requests (Non-blocking pada Core 0)
    webServerDsp.loop();

    // 2. Service Rotary Encoder events
    rotaryEncoder.update();
    int32_t delta = rotaryEncoder.getDelta();
    EncoderButtonEvent btnEv = rotaryEncoder.getButtonEvent();
    bool clicked = (btnEv == BTN_CLICKED);
    bool longPressed = (btnEv == BTN_LONG_PRESSED);

    if (delta != 0 || clicked || longPressed) {
        tftDisplay.handleEncoder(delta, clicked, longPressed);
    }

    // 3. Update TFT Display (Smooth 25 FPS VU Meter & Screen State)
    tftDisplay.update();

    // Small yield to allow FreeRTOS background tasks (WiFi, TCP, IDLE) to run smoothly
    vTaskDelay(pdMS_TO_TICKS(1));
}
