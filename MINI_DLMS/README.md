<div align="center">

# 🎛️ S.NET AUDIO MANAGEMENT (MINI DLMS 2-WAY)
### *Professional Dual-Core ESP32-S3 Digital Loudspeaker Management System*

[![ESP32-S3](https://img.shields.io/badge/Hardware-ESP32--S3%20LX7%20240MHz-red.svg?style=for-the-badge&logo=espressif)](https://www.espressif.com/)
[![DSP](https://img.shields.io/badge/DSP%20Engine-32--bit%20FPU%20Hardware-blue.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Crossover](https://img.shields.io/badge/Crossover-2--Way%20Linkwitz--Riley%2048dB-purple.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Display](https://img.shields.io/badge/Display-1.8%22%20SPI%20TFT%20ST7735-orange.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Interface](https://img.shields.io/badge/Control-Rotary%20Encoder%20%2B%20Web%20UI-green.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)

<p align="center">
  <b>Sistem Manajemen Audio Digital (DLMS) 2-Way Aktif Berbasis Dual ESP32</b><br>
  Dilengkapi Layar Depan 1.8" TFT, Rotary Encoder Real-Time, Dual DAC PCM5102, dan Web Dashboard Wi-Fi Tanpa Aplikasi.
</p>

---

[Fitur Utama](#-fitur-utama) • [Diagram Arsitektur](#-diagram-arsitektur-sistem) • [Skema Kabel (Pinout)](#-skema-pengkabelan-hardware) • [Panduan Rotary Encoder](#-panduan-penggunaan-rotary-encoder--layar) • [Instalasi](#-instalasi--upload)

---

</div>

## 🌟 Fitur Utama

| Modul | Kemampuan & Spesifikasi |
| :--- | :--- |
| **⚡ Dual Microcontroller Architecture** | **ESP32-S** khusus audio Bluetooth receiver (Zero dropouts) + **ESP32-S3** khusus Real-Time DSP Audio, TFT Display, Rotary Encoder, & Web Server. |
| **🔊 2-Way Active Crossover** | Linkwitz-Riley **12 dB, 24 dB, hingga 48 dB/oct**. Vokal pada subwoofer terpotong 100% tuntas dan nada sub-bass diblokir sempurna dari speaker mid/high. |
| **🎛️ Dual DAC PCM5102 I2S** | Output stereo digital lossless terpisah: **Channel Kiri (L) = SUBWOOFER / LOW**, **Channel Kanan (R) = MID / HIGH**. |
| **🖥️ Layar 1.8" SPI TFT (128x160)** | Tampilan mendatar (*landscape*) bergaya rackmount audio profesional dengan **3-Channel VU Meter Real-Time (25 FPS)** dan indikator `CLIP`. |
| **🔘 Knob Rotary Encoder (EC11)** | Navigasi menu panel depan & perubahan frekuensi/gain secara instan (*real-time*). Dilengkapi mode kunci parameter dan proteksi auto-return. |
| **🌐 Standalone Web Controller** | SoftAP Wi-Fi mandiri (`SNET-AUDIO` -> `192.168.4.1`) dengan Captive Portal, respon kurva EQ visual, slider logaritmik, dan 5 slot memori NVS. |
| **🛡️ Audio Protection Suite** | Subsonic HPF (25 Hz), Tweeter Protection LPF (18 kHz), Dual Peak Limiter, Phase Invert 180°, dan Delay Alignment (0–50 ms). |

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
  │  [CORE 1: DSP AUDIO PIPELINE 2-WAY REAL-TIME]                          │
  │   ├── WAY 1 (SUBWOOFER): Subsonic HPF 25Hz -> Crossover LPF 80-120Hz   │
  │   │                      -> Sub PEQ -> Sub Gain -> Sub Delay -> Limiter│
  │   │                      -> Diteruskan ke Output KIRI (LEFT)           │
  │   └── WAY 2 (MID/HIGH) : Crossover HPF 80-120Hz -> Tweeter LPF 18kHz  │
  │                          -> Mid PEQ -> Mid Gain -> Mid Delay -> Limiter│
  │                          -> Diteruskan ke Output KANAN (RIGHT)         │
  │                                                                        │
  │  [CORE 0: KONTROL & TAMPILAN]                                          │
  │   ├── Web Server SoftAP ("SNET-AUDIO" -> http://192.168.4.1)           │
  │   ├── Layar 1.8" SPI TFT 128x160 (ST7735: VU Meter 3-Ch & Info DSP)    │
  │   └── Rotary Encoder (EC11: Navigasi Menu & Live Setting Parameter)    │
  │                                                                        │
  │  [OUTPUT I2S_NUM_1 (TX)]: Pin 15 (BCK), Pin 16 (WS), Pin 17 (DIN)      │
  └───────────────────┬───────────────────────────────┬────────────────────┘
                      │                               │
                      ▼                               ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │   DAC 1 (PCM5102) - SUB      │ │  DAC 2 (PCM5102) - MID/HIGH  │
       │   Ambil Output KIRI (L)      │ │  Ambil Output KANAN (R)      │
       └──────────────┬───────────────┘ └──────────────┬───────────────┘
                      │                                │
                      ▼                                ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │     Power Amplifier SUB      │ │    Power Amplifier MID/HI    │
       │     (Khusus Subwoofer)       │ │     (Speaker Mid & Horn)     │
       └──────────────────────────────┘ └──────────────────────────────┘
```

---

## 🔌 Skema Pengkabelan Hardware

### 1. Layar 1.8" SPI TFT LCD (Header 8-Pin Bawah) ke ESP32-S3
Sesuai modul pada foto (Header 8 pin di bagian bawah):

| No & Label Pin Modul | Hubungkan Ke Pin ESP32-S3 | Fungsi / Keterangan |
| :---: | :--- | :--- |
| **1 - RST** | **GPIO 14** | Reset Layar LCD |
| **2 - CS** | **GPIO 10** | Chip Select SPI |
| **3 - D/C** | **GPIO 9** | Data / Command Selector (RS) |
| **4 - DIN** | **GPIO 11** | SPI Data In (MOSI / SDA) |
| **5 - CLK** | **GPIO 12** | SPI Clock (SCK / SCL) |
| **6 - VCC** | **Pin 5V** (atau **3.3V**) | Daya Modul *(Disarankan 5V ESP32)* |
| **7 - BL** | **Pin 3.3V** (atau **GPIO 13**) | Backlight / Lampu Latar Layar |
| **8 - GND** | **GND** | Ground Bersama |

> 💡 **Tips Daya (VCC Modul):**
> * Sesuai tulisan di modul: `VCC=5V -> J1 OPEN` (default pabrik). Cukup sambungkan **Pin 6 (VCC)** ke **Pin 5V / VIN** ESP32-S3 karena modul sudah memiliki IC regulator penurun tegangan 3.3V onboard (`U21`).
> * **Pin 7 (BL)** dapat langsung disambungkan ke pin **3.3V** ESP32 agar lampu layar langsung menyala terang stabil.

---

### 2. Rotary Encoder (EC11 + Switch Button) ke ESP32-S3
| Pin Rotary Encoder | Pin ESP32-S3 | Fungsi / Deskripsi |
| :--- | :--- | :--- |
| **CLK (Fase A)** | **GPIO 1** | Pulsa Putar A (Interrupt Handler) |
| **DT (Fase B)** | **GPIO 2** | Pulsa Putar B (Interrupt Handler) |
| **SW (Push Switch)** | **GPIO 42** | Tombol Tekan Encoder (Active LOW) |
| **+ (VCC)** | **3.3V** | Power VCC Enkoder |
| **GND** | **GND** | Ground bersama |

---

### 3. Dual DAC PCM5102 ke ESP32-S3 (2-Way Out)
Kedua modul DAC PCM5102 dihubungkan secara **paralel** ke jalur I2S TX yang sama:

| Pin ESP32-S3 | Sambung Ke DAC 1 & DAC 2 | Fungsi |
| :--- | :--- | :--- |
| **GPIO 15** | **BCK** (DAC 1 & DAC 2) | I2S Bit Clock Out |
| **GPIO 16** | **LCK / LRCK** (DAC 1 & DAC 2) | I2S Word Select Out |
| **GPIO 17** | **DIN** (DAC 1 & DAC 2) | I2S Serial Data Out |
| **GND** | **GND** | Ground bersama |
| **5V** atau **3.3V** | **VCC** | Power Supply DAC |

> **Cara Ambil Output Suara:**
> * **DAC 1 (SUBWOOFER):** Colokkan kabel RCA / Jack dari **Socket KIRI (L)** ke Amplifier Subwoofer. (Vokal terpotong 100% oleh LPF).
> * **DAC 2 (MID / HIGH):** Colokkan kabel RCA / Jack dari **Socket KANAN (R)** ke Amplifier Mid/High. (Bass sub diblokir oleh HPF).
> *(Jika menggunakan 1 board DAC PCM5102 saja, Socket L langsung menjadi Subwoofer dan Socket R langsung menjadi Mid/High).*

> **Konfigurasi Jumper Solder PCB PCM5102:**
> * `SCK` ──► `GND` *(Wajib! Mengaktifkan Internal PLL generator)*
> * `FLT`, `DMP`, `FMT` ──► `GND`
> * `XMT` ──► `3.3V` *(Hardware Unmute)*

---

### 4. Input Audio (ESP32-S BT Receiver / ADC PCM1808) ke ESP32-S3
| Pin Sumber Input | Pin ESP32-S3 | Fungsi |
| :--- | :--- | :--- |
| **Pin 26 (BCK)** / **BCK ADC** | **GPIO 4** | I2S Bit Clock Input |
| **Pin 25 (LRCK)** / **LRCK ADC** | **GPIO 5** | I2S Word Select Input |
| **Pin 22 (DOUT)** / **DOUT ADC** | **GPIO 6** | I2S Data Input |
| **GND** | **GND** | **Ground Bersama (Wajib Konek!)** |

---

## 🎮 Panduan Penggunaan Rotary Encoder & Layar

```text
 ┌─────────────────────────────────────────────────────────┐
 │ S.NET MINI DLMS 2-WAY                                   │
 ├─────────────────────────────────────────────────────────┤
 │ IN  [████████████░░░░░░░░░░░░░░░░░░] -14dB              │
 │ SUB [██████████████████░░░░░░░░░░░░] -6dB               │
 │ MID [████████████░░░░░░░░░░░░░░░░░░] -12dB              │
 ├─────────────────────────────────────────────────────────┤
 │ X-OVER: 100Hz (24dB LR) | SUB:+0.0dB | MID:+0.0dB       │
 │ PRESET: Slot 1 [ACTIVE] | IP: 192.168.4.1               │
 ├─────────────────────────────────────────────────────────┤
 │ [ TEKAN KNOB : MENU ]                                   │
 └─────────────────────────────────────────────────────────┘
```

1. **Layar Utama (Home Monitoring)**:
   * Menampilkan VU meter 3-channel responsif 25 FPS dengan indikator `CLIP`.
   * Menampilkan parameter crossover aktif, gain level, IP address, dan status preset.
2. **Masuk ke Menu Pengaturan**:
   * **Tekan tombol encoder 1x** saat di layar utama untuk masuk ke Menu Pengaturan.
   * **Putar knob** untuk navigasi menu:
     * `X-OVER FREQ` : 40 Hz – 5000 Hz
     * `SLOPE` : 12 dB, 24 dB, 48 dB Linkwitz-Riley
     * `SUB GAIN` : -30 dB s/d +12 dB
     * `MID GAIN` : -30 dB s/d +12 dB
     * `MASTER VOL`: -60 dB s/d +12 dB
     * `SUB PHASE` : Normal / Invert 180°
     * `MID PHASE` : Normal / Invert 180°
     * `MUTE` : Unmuted / Muted
     * `LOAD PRESET`: Slot 1 – 5
     * `SAVE PRESET`: Simpan konfigurasi ke memori NVS
     * `< KEMBALI KE VU METER >`
3. **Mengubah Nilai Parameter**:
   * **Tekan knob 1x** pada menu yang dipilih -> kursor berubah menjadi tanda bintang `*` (Mode Edit Aktif).
   * **Putar knob** -> Audio DSP langsung berubah seketika (*real-time*, tanpa jeda).
   * **Tekan knob 1x lagi** -> Mengunci nilai yang diubah.
   * **Tekan & tahan knob selama 1 detik** (atau biarkan 15 detik) untuk kembali ke Layar Utama VU Meter.

---

## 🚀 Instalasi & Upload

### A. Library Arduino yang Dibutuhkan
Buka **Arduino IDE** -> **Sketch** -> **Include Library** -> **Manage Libraries...**, lalu instal:
1. **`Adafruit GFX Library`** (oleh Adafruit)
2. **`Adafruit ST7735 and ST7789 Library`** (oleh Adafruit)
3. **`ArduinoJson`** (oleh Benoit Blanchon - versi 6 atau 7)

---

### B. Upload Board 1: Bluetooth Receiver (`ESP32-S`)
1. Buka file: `ESP32_BT_RECEIVER/ESP32_BT_RECEIVER.ino`
2. Tools -> **Board:** `ESP32 Dev Module`
3. Tools -> **Partition Scheme:** `Default 4MB with spiffs`
4. Klik **Upload**.

---

### C. Upload Board 2: DSP Processor + Web + TFT (`ESP32-S3`)
1. Buka file: `MINI_DLMS/MINI_DLMS.ino`
2. Tools -> **Board:** `ESP32S3 Dev Module`
3. Tools -> **USB CDC On Boot:** `Enabled`
4. Tools -> **Partition Scheme:** `Huge APP (3MB No OTA / 1MB SPIFFS)` atau `Default 4MB with spiffs`
5. Tools -> **Upload Speed:** `921600`
6. Klik **Upload**.

---

## 📱 Kontrol Wireless Web Dashboard

1. Nyalakan sistem.
2. Sambungkan Wi-Fi di HP / Laptop ke SSID: **`SNET-AUDIO`** *(Open Network / Tanpa Password)*.
3. Buka browser dan kunjungi: **`http://192.168.4.1`**.
4. Dashboard web interaktif menampilkan:
   * Visual kurva respon frekuensi real-time.
   * Slider Crossover 20 Hz – 20.000 Hz dengan opsi kemiringan slope 12/24/48 dB.
   * Preset instan: `Sub 80Hz (Cut Vokal)`, `Sub 100Hz`, `Sub 80Hz Ekstrem 48dB`, `Mid`, `Bypass`.
   * VU Meter Level, Gain, Invert, Delay, dan 5 Slot Penyimpanan Preset.

---

## 📄 Lisensi & Pembuat

Dikembangkan untuk kebutuhan audio profesional Indonesia. Bebas dimodifikasi untuk keperluan pribadi maupun sound system rental.
Semoga bermanfaat dan sukses selalu! 🚀
