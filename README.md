<div align="center">

# 🎛️ SOFGAM SS by Shawir
### *Mini DLMS Audio Management System (Dual-Core ESP32-S3 Pro Audio DSP)*
**"Better Sound, Better Exp."**

[![ESP32-S3](https://img.shields.io/badge/Hardware-ESP32--S3%20Dual--Core%20240MHz-red.svg?style=for-the-badge&logo=espressif)](https://www.espressif.com/)
[![DSP](https://img.shields.io/badge/DSP%20Engine-32--bit%20FPU%20Hardware%2048kHz-blue.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![DAC](https://img.shields.io/badge/Output-4--Channel%20Dual%20PCM5102-purple.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Display](https://img.shields.io/badge/TFT%20Display-1.8%22%20ST7735%20(160x128)-orange.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Interface](https://img.shields.io/badge/Control-EC11%20Knob%20%2B%20Web%20Dashboard-green.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![UI Sync](https://img.shields.io/badge/Visual%20Design-100%25%20Web%20%26%20LCD%201:1%20Sync-cyan.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)

<p align="center">
  <b>Sistem Manajemen Audio Digital (DLMS) 4-Channel Profesional Berbasis Dual-Core ESP32-S3</b><br>
  Tampilan Layar 1.8" TFT ST7735 Didesain <b>Sama Persis 1-to-1</b> dengan Web UI Dashboard Cyber Rack Modern.<br>
  Dilengkapi Dual Modul DAC PCM5102, Crossover Linkwitz-Riley hingga 48dB/oct, Parametric EQ Kurva Real-Time, Delay, Peak Limiter, dan Kontrol Rotary Encoder EC11 Fisik + Virtual Web Remote.
</p>

---

[Fitur Utama](#-fitur-utama) • [Tampilan UI 1-to-1](#-tampilan-layar-lcd--web-ui-1-to-1) • [Diagram Arsitektur](#-diagram-arsitektur-sistem) • [Skema Kabel (Pinout)](#-skema-pengkabelan-hardware) • [Panduan Navigasi](#-panduan-navigasi-rotary-encoder--layar) • [Simulator Web](#-simulator-interaktif-web--lcd) • [Instalasi Firmware](#-instalasi--upload-firmware)

---

</div>

## 🌟 Fitur Utama

| Komponen & Fitur | Spesifikasi & Kemampuan Teknis |
| :--- | :--- |
| **⚡ Dual Microcontroller Architecture** | **ESP32-S** didedikasikan 100% untuk Bluetooth A2DP audio receiver (bebas gangguan) + **ESP32-S3** khusus Real-Time Audio DSP, TFT Graphic Engine, Rotary Encoder, & Web Server SoftAP. |
| **🔊 32-bit Floating-Point DSP Engine** | Audio diproses pada sample rate **48 kHz** dengan pipeline audio 32-bit hardware FPU berlatensi sangat rendah (< 1.2 ms). |
| **🎛️ Dual DAC PCM5102 (4 Channel Output)** | 2 modul fisik DAC PCM5102 stereo independen: **DAC 1 = Channel 1 (Out 1 & Out 2)**, **DAC 2 = Channel 2 (Out 3 & Out 4)**. Bebas dikonfigurasi untuk 2-Way Crossover (Sub & Mid/High), Bi-Amp, ataupun 4-Channel Full-Range. |
| **🖥️ Layar 1.8" SPI TFT (160x128 Landscape)** | Tampilan grafis cyber rack pro audio bebas kedip (*flicker-free* 25+ FPS via 27MHz Hardware SPI). Tampilan dibuat **sama persis 1-to-1 dengan Web UI**. |
| **📊 Split-Card Real-Time Monitoring** | Layar utama membagi monitoring input & output secara rapi: kartu **IN ADC** (L & R), kartu **OUTPUT (4CH)** (1..4), kartu **Preset & Quick Params**, 5 pill **Hardware Diagnostics** (`ADC:OK`, `DSP:RUN`, `DAC1:OK`, `DAC2:OK`, `42'C`), dan **Bottom Cyber Dock** 7 tombol. |
| **🎚️ 2x5 Cyber Tile Menu Matrix** | Menu pengaturan menggunakan matriks kartu cyber 2 baris × 5 kolom (`HOME`, `GAIN`, `HPF`, `LPF`, `DELAY`, `PEQ`, `LIMIT`, `PRESET`, `STATUS`, `ABOUT`) dengan highlight menyala. |
| **📈 Live Visual Graphic PEQ Curve** | Sub-menu Parametric EQ dilengkapi kanvas grafik respons frekuensi kurva lonceng (*bell curve*) *real-time* yang bergerak dinamis saat gain/frekuensi diubah. |
| **🛡️ Audio Protection Suite** | Filter HPF & LPF Linkwitz-Riley **12 dB, 24 dB, hingga 48 dB/oct** (20 Hz – 20 kHz), True Peak Limiter per channel (Threshold, Attack, Release), Phase Invert 180°, dan Time Alignment Delay (0 – 50 ms / 0 – 17.15 meter). |
| **🔘 Dual Navigation: EC11 + Virtual Remote** | Navigasi menu instan dengan **Knob Rotary Encoder fisik EC11** atau **Virtual Remote Controller** di Web Dashboard tanpa perlu menunggu modul fisik tiba. Dilengkapi fitur *long press* (>0.6 detik) untuk langsung kembali ke Home. |
| **💾 NVS Flash Memory Presets** | 5 slot memori penyimpanan internal non-volatile flash untuk menyimpan setup preset suara lapangan secara permanen. |

---

## 🎨 Tampilan Layar LCD & Web UI (1-to-1)

Tampilan fisik pada LCD TFT 1.8" (160x128 piksel) didesain identik dengan Web UI bertema Cyber Dark Navy (`#060e1f`), Electric Cyan (`#00d2ff`), Neon Green (`#10b981`), Amber (`#f59e0b`), dan Red Peak (`#ef4444`):

### 1. Layar Utama (Home Screen - Split Cards & Cyber Dock)
```text
┌─────────────────────────────────────────────────────────────┐
│ SOFGAM SS                               48k  ● RUN          │
├──────────────────────────┬──────────────────────────────────┤
│ IN ADC                   │ OUTPUT (4CH)                     │
│ L [████░░░░] -18dB       │ 1 [██████░░░░░░] -20dB           │
│ R [████░░░░] -19dB       │ 2 [██████░░░░░░] -20dB           │
│                          │ 3 [█████░░░░░░░] -22dB           │
│                          │ 4 [████░░░░░░░░] -26dB           │
├──────────────────────────┴──────────────────────────────────┤
│ PRESET <01 Default>                                         │
│ G:+0dB  H:80  L:8.0k  EQ:ON  LIM:ON                         │
├─────────────────────────────────────────────────────────────┤
│ [ADC:OK]   [DSP:RUN]   [DAC1:OK]   [DAC2:OK]   [ 42'C ]     │
├─────────────────────────────────────────────────────────────┤
│ [HOM]   [GAI]   [HPF]   [LPF]   [DEL]   [PEQ]   [MEN]       │
└─────────────────────────────────────────────────────────────┘
```

### 2. Layar Menu (2x5 Cyber Tile Grid Matrix)
```text
┌─────────────────────────────────────────────────────────────┐
│ SOFGAM SS                           > MENU                  │
├─────────────────────────────────────────────────────────────┤
│  ┌──────┐    ┌──────┐    ┌──────┐    ┌──────┐    ┌──────┐   │
│  │ HOME │    │ GAIN │    │ HPF  │    │ LPF  │    │ DELA │   │
│  └──────┘    └──────┘    └──────┘    └──────┘    └──────┘   │
│  ┌──────┐    ┌──────┐    ┌──────┐    ┌──────┐    ┌──────┐   │
│  │ PEQ  │    │ LIMI │    │ PRES │    │ STAT │    │ ABOU │   │
│  └──────┘    └──────┘    └──────┘    └──────┘    └──────┘   │
├─────────────────────────────────────────────────────────────┤
│ PUTAR: PILIH  |  TEKAN: BUKA                                │
└─────────────────────────────────────────────────────────────┘
```

### 3. Layar Grafik Parametric EQ (PEQ Real-Time Curve)
```text
┌─────────────────────────────────────────────────────────────┐
│ SOFGAM SS                         SETTING PEQ               │
├─────────────────────────────────────────────────────────────┤
│ ┌─────────────────────────────────────────────────────────┐ │
│ │                  .---*---.                              │ │
│ │ ────────────────/─────────\───────────────────── (0 dB) │ │
│ └─────────────────────────────────────────────────────────┘ │
│ [B1] [B2] [B3] [B4] [B5]     F: 100Hz  G: +2.0dB  Q: 1.0    │
├─────────────────────────────────────────────────────────────┤
│ [ < Kembali ]                             [ Simpan ]        │
└─────────────────────────────────────────────────────────────┘
```

---

## 📐 Diagram Arsitektur Sistem

```text
       ┌────────────────────────┐         ┌────────────────────────┐
       │    Smartphone / HP     │         │   Line-In / Mixer      │
       │  (Bluetooth Classic)   │         │ (Analog Line Source)   │
       └───────────┬────────────┘         └───────────┬────────────┘
                   │                                  │
                   ▼ (Lossless Digital Stream)        ▼
     ┌──────────────────────────────┐   ┌──────────────────────────────┐
     │ ESP32-S (BLUETOOTH RECEIVER) │   │     PCM1808 / I2S ADC        │
     │   - Sketch: ESP32_BT_RCV     │   │  (24-bit 96kHz Analog ADC)   │
     │   - 100% CPU untuk BT A2DP   │   │  (Input Line-In Alternatif)  │
     └─────────────┬────────────────┘   └─────────────┬────────────────┘
                   │                                  │
                   └────────────┬─────────────────────┘
                                │ Kabel I2S Digital (Lossless PCM)
                                ▼
  ┌────────────────────────────────────────────────────────────────────────┐
  │                 ESP32-S3 DUAL-CORE AUDIO PROCESSOR                     │
  │                                                                        │
  │  [INPUT I2S_NUM_0 (RX)] : Pin 4 (BCK), Pin 5 (WS), Pin 6 (DIN)         │
  │                                                                        │
  │  [CORE 1: DUAL INDEPENDENT DSP AUDIO PIPELINES (48kHz 32-bit FPU)]     │
  │   ├── CHANNEL 1 (DAC 1): HPF 12/24/48dB -> LPF 12/24/48dB              │
  │   │                      -> 3-Band PEQ -> Gain -> Delay -> Limiter     │
  │   │                      -> Diteruskan ke OUT 1 & OUT 2 (DAC 1)        │
  │   │                                                                    │
  │   └── CHANNEL 2 (DAC 2): HPF 12/24/48dB -> LPF 12/24/48dB              │
  │                          -> 3-Band PEQ -> Gain -> Delay -> Limiter     │
  │                          -> Diteruskan ke OUT 3 & OUT 4 (DAC 2)        │
  │                                                                        │
  │  [CORE 0: KONTROL, GRAFIS & WEB SERVER]                                │
  │   ├── Web Server SoftAP ("SNET-AUDIO" -> http://192.168.4.1)           │
  │   ├── ST7735 TFT Graphic Engine (27 MHz SPI Hardware)                  │
  │   ├── Rotary Encoder EC11 Service (CLK, DT, SW)                        │
  │   └── Virtual Remote Engine (WebSocket & HTTP REST API)                │
  │                                                                        │
  │  [OUTPUT I2S_NUM_1 (TX)]: Pin 15 (BCK), Pin 16 (WS), Pin 17 (DIN)      │
  └───────────────────┬───────────────────────────────┬────────────────────┘
                      │                               │
                      ▼                               ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │     MODUL DAC 1 (PCM5102)    │ │     MODUL DAC 2 (PCM5102)    │
       │          CHANNEL 1           │ │          CHANNEL 2           │
       │  OUT 1 (Left) & OUT 2 (Right)│ │  OUT 3 (Left) & OUT 4 (Right)│
       └──────────────┬───────────────┘ └──────────────┬───────────────┘
                      │                                │
                      ▼                                ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │     Power Amplifier CH 1     │ │     Power Amplifier CH 2     │
       │   (Subwoofer / Low / Full)   │ │   (Mid-High / Tweeter / Full)│
       └──────────────────────────────┘ └──────────────────────────────┘
```

---

## 🔌 Skema Pengkabelan Hardware

### 1. Layar 1.8" SPI TFT LCD (Header 8-Pin ST7735) ke ESP32-S3
| Pin LCD Modul | Pin ESP32-S3 | Keterangan & Catatan Penting |
| :---: | :--- | :--- |
| **1 - RST** | **GPIO 14** | Reset Hardware Layar |
| **2 - CS** | **GPIO 10** | Chip Select SPI |
| **3 - D/C** | **GPIO 9** | Data / Command Selector (RS) |
| **4 - DIN** | **GPIO 11** | SPI Data In (MOSI / SDA) |
| **5 - CLK** | **GPIO 12** | SPI Clock (SCK / SCL) |
| **6 - VCC** | **Pin 5V (VIN)** | **WAJIB 5V** *(Modul memiliki regulator 3.3V onboard `U21`. Jangan dicolok ke 3.3V karena layar akan redup/pudar)* |
| **7 - BL** | **Pin 3.3V** (atau **GPIO 8**) | **WAJIB disambung ke 3.3V** agar lampu latar stabil terang terus-menerus |
| **8 - GND** | **GND** | Ground Bersama |

> 💡 **TIPS:** Kelupas plastik stiker pelindung layar transparan pada modul LCD baru agar tulisan dan grafik terlihat jernih, tajam, dan kontras maksimal!

---

### 2. Rotary Encoder (EC11 + Push Button) ke ESP32-S3
| Pin Rotary Encoder | Pin ESP32-S3 | Fungsi / Deskripsi |
| :--- | :--- | :--- |
| **CLK (Fase A)** | **GPIO 1** | Pulsa Rotasi A (Input Pull-up) |
| **DT (Fase B)** | **GPIO 2** | Pulsa Rotasi B (Input Pull-up) |
| **SW (Push Switch)** | **GPIO 42** | Tombol Tekan Knob (Active LOW) |
| **+ (VCC)** | **3.3V** | Power VCC Enkoder |
| **GND** | **GND** | Ground bersama |

---

### 3. Dual DAC PCM5102 ke ESP32-S3 (2 Modul Terpisah)
Kedua modul fisik PCM5102 menerima sinyal clock yang sama dari ESP32-S3:

| Pin ESP32-S3 | Hubungkan Ke DAC 1 & DAC 2 | Fungsi |
| :--- | :--- | :--- |
| **GPIO 15** | **BCK** (DAC 1 & DAC 2) | I2S Bit Clock Out |
| **GPIO 16** | **LCK / LRCK** (DAC 1 & DAC 2) | I2S Word Select Out |
| **GPIO 17** | **DIN** (DAC 1 & DAC 2) | I2S Serial Data Out |
| **GND** | **GND** | Ground bersama |
| **5V** atau **3.3V** | **VCC** | Power Supply DAC |

> **Konfigurasi Jumper Solder Sisi Bawah PCB PCM5102:**
> * `SCK` ──► `GND` *(Wajib! Mengaktifkan Internal PLL generator)*
> * `FLT`, `DMP`, `FMT` ──► `GND`
> * `XMT` ──► `3.3V` *(Hardware Unmute agar audio langsung keluar)*

---

### 4. Input Audio (ESP32-S BT Receiver / ADC PCM1808) ke ESP32-S3
| Pin Sumber Input | Pin ESP32-S3 | Fungsi |
| :--- | :--- | :--- |
| **Pin 26 (BCK)** / **BCK ADC** | **GPIO 4** | I2S Bit Clock Input |
| **Pin 25 (LRCK)** / **LRCK ADC** | **GPIO 5** | I2S Word Select Input |
| **Pin 22 (DOUT)** / **DOUT ADC** | **GPIO 6** | I2S Data Input |
| **GND** | **GND** | **Ground Bersama (Wajib Terhubung)** |

---

## 🎮 Panduan Navigasi Rotary Encoder & Layar

### A. Operasi Knob Fisik EC11
1. **Di Layar Utama (Home Screen):**
   * **Putar Knob:** Menggeser tombol aktif pada **Bottom Cyber Dock** (`HOM`, `GAI`, `HPF`, `LPF`, `DEL`, `PEQ`, `MEN`).
   * **Tekan Knob (Klik):** Masuk langsung ke layar sub-menu yang dipilih pada dock:
     * `GAI` ➔ Langsung ke Layar Gain
     * `HPF` ➔ Langsung ke Layar HPF
     * `LPF` ➔ Langsung ke Layar LPF
     * `DEL` ➔ Langsung ke Layar Delay
     * `PEQ` ➔ Langsung ke Layar Parametric EQ (Kurva Grafik)
     * `MEN` ➔ Masuk ke 2x5 Menu Grid
2. **Di Layar Menu Grid:**
   * **Putar Knob:** Menggeser seleksi di antara 10 kartu cyber tile.
   * **Tekan Knob:** Membuka modul pengaturan yang dipilih.
3. **Di Sub-Menu Pengaturan (Gain, HPF, LPF, Delay, PEQ, Limiter):**
   * **Putar Knob:** Berpindah antar kartu parameter atau tombol `< Kembali` / `Simpan`.
   * **Tekan Knob:** Masuk ke **Mode Edit Nilai** (teks nilai berubah menjadi **Kuning Amber**).
   * **Putar Knob (Saat Mode Edit):** Mengubah nilai desibel / frekuensi secara halus & *real-time*.
   * **Tekan Knob Lagi:** Mengunci nilai baru dan keluar dari mode edit.
4. **Shortcut Lompat Cepat (*Fast Jump*):**
   * **Tahan tombol knob selama > 0.6 detik** dari menu atau kedalaman level mana pun untuk **langsung kembali ke Layar Utama (Home)**.
   * Dilengkapi fitur proteksi *auto-timeout* kembali ke Home setelah 20 detik tanpa aktivitas.

---

### B. Remote Virtual di Web Dashboard
Web Dashboard menyediakan panel virtual rotary encoder interaktif lengkap:
* `[ ⟲ PUTAR KIRI ]` / `[ ⟳ PUTAR KANAN ]`
* `[ 🔘 TEKAN KNOB ]`
* `[ ⏪ Cepat -5 ]` / `[ ⏩ Cepat +5 ]`
* `[ ↩ TEKAN LAMA (HOME) ]`
* Shortcut keyboard laptop: **Panah Kiri/Kanan**, **Enter**, dan **Escape**.

---

## 💻 Simulator Interaktif Web & LCD

Anda dapat mencoba dan memverifikasi seluruh antarmuka sebelum mengunggah ke hardware fisik:

1. **Simulator Layar LCD (160x128):**
   * Buka file: [MINI_DLMS/lcd_preview.html](MINI_DLMS/lcd_preview.html) di browser.
   * Dilengkapi simulator knob fisik EC11 putar/klik interaktif untuk menguji semua perpindahan layar dan animasi kurva PEQ.
2. **Full Web UI Dashboard:**
   * Buka file: [MINI_DLMS/preview.html](MINI_DLMS/preview.html) di browser untuk melihat dashboard kontrol penuh (slider, visualizer spectrum, crossover 48dB, dan slot preset).

---

## 🚀 Instalasi & Upload Firmware

### 1. Library Arduino yang Diperlukan
Buka **Arduino IDE** -> **Sketch** -> **Include Library** -> **Manage Libraries...**, lalu pasang:
* `Adafruit GFX Library`
* `Adafruit ST7735 and ST7789 Library`
* `ArduinoJson` (v6 atau v7)

---

### 2. Upload Board 1: Bluetooth Receiver (`ESP32-S`)
1. Buka file: `ESP32_BT_RECEIVER/ESP32_BT_RECEIVER.ino`
2. **Board:** `ESP32 Dev Module`
3. **Partition Scheme:** `Default 4MB with spiffs`
4. Klik **Upload**.

---

### 3. Upload Board 2: DSP Processor + Web + TFT (`ESP32-S3`)
1. Buka file: `MINI_DLMS/MINI_DLMS.ino`
2. **Board:** `ESP32S3 Dev Module`
3. **USB CDC On Boot:** `Enabled`
4. **Partition Scheme:** `Huge APP (3MB No OTA / 1MB SPIFFS)`
5. **Upload Speed:** `921600`
6. Klik **Upload**.

---

## 📱 Cara Menghubungkan Web Control di HP / Laptop

1. Nyalakan sistem SOFGAM SS.
2. Buka Wi-Fi di HP / Laptop, hubungkan ke access point: **`SNET-AUDIO`** *(Tanpa Password)*.
3. Buka browser (Chrome / Safari / Firefox) dan akses: **`http://192.168.4.1`**.
4. Kontrol penuh siap digunakan!

---

## 📄 Lisensi & Kredit

* **System Design & Firmware:** SOFGAM SS by Shawir
* **Audio Engineering:** Linkwitz-Riley Biquads, 32-bit Floating-point Dual Pipeline
* **TFT Graphics Engine:** Fast Hardware SPI Adafruit_GFX Engine
* Dibuat khusus untuk sound system profesional Indonesia. Bebas dikembangkan dan dimodifikasi untuk kebutuhan audio panggung, rental sound system, maupun audio mobil.

**Better Sound, Better Exp.** 🚀
