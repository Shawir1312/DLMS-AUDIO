# S.NET AUDIO MANAGEMENT — 2-WAY DLMS DIGITAL AUDIO PROCESSOR

Arsitektur Profesional **Dual ESP32 + 2-Way Active Crossover + Layar 1.8" SPI TFT + Rotary Encoder**:
1. **ESP32 / ESP-32S**: Dedicated Receiver **Bluetooth Classic A2DP Audio** (Audio stream lossless I2S ke ESP32-S3).
2. **ESP32-S3**: Core **Audio DSP Engine (Core 1) + Web UI Dashboard (Core 0) + Layar TFT 1.8" + Rotary Encoder + Dual DAC PCM5102 (2-Way Out: Sub & Mid/High) + Input I2S (BT / ADC)**.

Pemisahan modul ini menjamin 100% audio jernih tanpa latensi, tanpa lag/stuck, bebas tabrakan frekuensi WiFi-BT, dan memberikan kontrol fisik instan melalui Rotary Encoder di panel depan serta kontrol wireless via Web Browser.

---

## 1. DIAGRAM SISTEM 2-WAY DLMS LENGKAP

```text
       ┌────────────────────────┐         ┌────────────────────────┐
       │   Smartphone / HP      │         │   Line-In / Mixer      │
       │  (Bluetooth Classic)   │         │ (Analog Line Source)   │
       └───────────┬────────────┘         └───────────┬────────────┘
                   │                                  │
                   ▼                                  ▼
     ┌──────────────────────────────┐   ┌──────────────────────────────┐
     │ ESP32-S (BLUETOOTH RECEIVER) │   │     PCM1808 / I2S ADC        │
     │   (Audio stream lossless)    │   │  (24-bit 96kHz Analog ADC)   │
     └─────────────┬────────────────┘   └─────────────┬────────────────┘
                   │                                  │
                   └────────────┬─────────────────────┘
                                │ (Pilih salah satu / Switch I2S IN)
                                ▼
  ┌────────────────────────────────────────────────────────────────────────┐
  │                 ESP32-S3 CORE PROCESSOR & 2-WAY DLMS                   │
  │                                                                        │
  │  [INPUT I2S_0 (RX)] : Pin 4 (BCK), Pin 5 (WS), Pin 6 (DIN)             │
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
  │  [OUTPUT I2S_1 (TX)]: Pin 15 (BCK), Pin 16 (WS), Pin 17 (DIN)          │
  └───────────────────┬───────────────────────────────┬────────────────────┘
                      │                               │
                      ▼                               ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │   DAC 1 (PCM5102) - SUB      │ │  DAC 2 (PCM5102) - MID/HIGH  │
       │   Ambil Output LEFT (L)      │ │  Ambil Output RIGHT (R)      │
       └──────────────┬───────────────┘ └──────────────┬───────────────┘
                      │                                │
                      ▼                                ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │     Power Amplifier SUB      │ │    Power Amplifier MID/HI    │
       │     (Khusus Subwoofer)       │ │     (Speaker Mid & Horn)     │
       └──────────────────────────────┘ └──────────────────────────────┘
```

---

## 2. TABEL PENGKABELAN (WIRING) HARDWARE

### A. Input Audio (Dari ESP32-S Bluetooth ATAU PCM1808 ADC) ke ESP32-S3
| Pin ESP32-S (BT) / ADC PCM1808 | Hubungkan Ke Pin ESP32-S3 | Fungsi |
| :--- | :--- | :--- |
| **Pin 26 (BCK)** / **BCK ADC** | **GPIO 4** | I2S Bit Clock Input |
| **Pin 25 (WS/LRCK)** / **LRCK ADC** | **GPIO 5** | I2S Word Select / LR Clock Input |
| **Pin 22 (DOUT)** / **DOUT ADC** | **GPIO 6** | I2S Data Input |
| **GND** | **GND** | **Ground Bersama (Wajib Konek!)** |

*(Catatan: Jika memakai PCM1808 ADC modul, pastikan pin MD0 & MD1 terpasang sesuai mode Master/Slave, dan SCK dihubungkan ke ground/kristal board).*

---

### B. Output Audio ke 2 DAC PCM5102 (Way 1: Subwoofer & Way 2: Mid/High)
Kedua DAC PCM5102 dihubungkan **paralel** ke jalur I2S TX yang sama:
| Pin ESP32-S3 | Hubungkan Ke DAC 1 & DAC 2 | Keterangan |
| :--- | :--- | :--- |
| **GPIO 15** | **BCK** (DAC 1 & DAC 2) | I2S Bit Clock Out |
| **GPIO 16** | **LCK / LRCK** (DAC 1 & DAC 2) | I2S LR Clock Out |
| **GPIO 17** | **DIN** (DAC 1 & DAC 2) | I2S Data Out |
| **GND** | **GND** | Ground Bersama |
| **5V** atau **3.3V** | **VCC** | Power Supply DAC |

**Pengambilan Output Suara:**
* **DAC 1 (Subwoofer):** Colokkan kabel RCA / Jack dari **Socket L (Left)** ke Amplifier Subwoofer. (Vokal 100% dipotong oleh LPF).
* **DAC 2 (Mid / High):** Colokkan kabel RCA / Jack dari **Socket R (Right)** ke Amplifier Mid/High. (Bass sub dipotong oleh HPF).
*(Catatan: Jika hanya menggunakan 1 board DAC PCM5102, Socket L langsung menjadi Subwoofer dan Socket R langsung menjadi Mid/High).*

**Konfigurasi Jumper Modul PCM5102 (Solder ke GND/3.3V):**
* `SCK` -> Hubungkan ke `GND` (mengaktifkan internal PLL)
* `FLT` -> Hubungkan ke `GND`
* `DMP` -> Hubungkan ke `GND`
* `FMT` -> Hubungkan ke `GND`
* `XMT` -> Hubungkan ke `3.3V` (Unmute hardware)

---

### C. Layar 1.8" SPI TFT LCD (128x160 - Driver ST7735) ke ESP32-S3
| Pin Layar TFT 1.8" | Hubungkan Ke Pin ESP32-S3 | Keterangan |
| :--- | :--- | :--- |
| **VCC** | **3.3V** atau **5V** | Power layar (sesuai modul) |
| **GND** | **GND** | Ground |
| **CS** | **GPIO 10** | Chip Select SPI |
| **RESET / RES** | **GPIO 14** | Reset TFT |
| **A0 / DC** | **GPIO 9** | Data / Command |
| **SDA / MOSI** | **GPIO 11** | SPI Data (MOSI) |
| **SCK / SCL** | **GPIO 12** | SPI Clock |
| **LED / BLK** | **3.3V** (atau **GPIO 13**) | Lampu Latar Backlight |

---

### D. Rotary Encoder (EC11 + Push Button) ke ESP32-S3
| Pin Rotary Encoder | Hubungkan Ke Pin ESP32-S3 | Keterangan |
| :--- | :--- | :--- |
| **CLK (A)** | **GPIO 1** | Putaran Enkoder Fase A (Internal Pull-Up) |
| **DT (B)** | **GPIO 2** | Putaran Enkoder Fase B (Internal Pull-Up) |
| **SW (Button)** | **GPIO 42** | Tombol Tekan Push Button (Active LOW) |
| **+ (VCC)** | **3.3V** | Power modul enkoder |
| **GND** | **GND** | Ground bersama |

---

## 3. CARA PENGGUNAAN ROTARY ENCODER & LAYAR DEPAN

Layar 1.8" TFT didesain dalam orientasi mendatar (**Landscape 160 x 128 px**):

### A. Layar Utama (Home Screen - VU Meter 3-Channel)
* Menampilkan **VU Meter Real-Time** responsif (25 FPS):
  1. `IN  :` Level audio input dari BT / ADC.
  2. `SUB :` Level audio output Subwoofer dengan deteksi CLIP.
  3. `MID :` Level audio output Mid/High dengan deteksi CLIP.
* Menampilkan status Crossover aktif (misal `X-OVER: 100Hz 24dB LR`).
* Menampilkan Gain Sub & Mid (`S: +0.0dB | M: +0.0dB`).
* Menampilkan IP WiFi dan Preset aktif.

### B. Masuk ke Menu Pengaturan
1. Di layar utama, **TEKAN TOMBOL ROTARY ENCODER SEKALI**.
2. Layar akan beralih ke **DSP PARAMETER SETTINGS MENU**.
3. **Putar knob** ke kiri/kanan untuk memilih parameter:
   * `X-OVER FREQ` (40Hz, 60Hz, 80Hz, 100Hz, 120Hz, 150Hz, dst)
   * `SLOPE` (12 dB, 24 dB, 48 dB Linkwitz-Riley)
   * `SUB GAIN` (-30 dB s/d +12 dB)
   * `MID GAIN` (-30 dB s/d +12 dB)
   * `MASTER VOL` (-60 dB s/d +12 dB)
   * `SUB PHASE` (NORMAL / INVERT 180°)
   * `MID PHASE` (NORMAL / INVERT 180°)
   * `MUTE` (UNMUTED / MUTED)
   * `LOAD PRESET` (Preset Slot 1 s/d 5)
   * `SAVE PRESET` (Simpan konfigurasi ke memori NVS)
   * `< KEMBALI KE VU METER >`
4. **TEKAN SEKALI** pada parameter yang dipilih untuk masuk **MODE EDIT** (kursor berubah menjadi warna merah/tanda bintang `*`).
5. **Putar knob** untuk langsung mengubah nilai secara **REAL-TIME** (suara audio langsung berubah seketika tanpa jeda!).
6. **TEKAN SEKALI LAGI** untuk mengunci nilai.
7. **Tekan & tahan knob selama 1 detik** (atau diamkan 15 detik) untuk kembali ke Layar Utama VU Meter.

---

## 4. LIBRARY ARDUINO YANG DIBUTUHKAN

Buka **Arduino IDE** -> Menu **Sketch** -> **Include Library** -> **Manage Libraries...**, lalu cari dan install:
1. **`Adafruit GFX Library`** (oleh Adafruit)
2. **`Adafruit ST7735 and ST7789 Library`** (oleh Adafruit)
3. **`ArduinoJson`** (oleh Benoit Blanchon - versi 6 atau 7)

---

## 5. CARA UPLOAD SKETCH KE ESP32-S3

1. Sambungkan kabel USB ke port ESP32-S3.
2. Buka sketch: `MINI DLMS/MINI_DLMS/MINI_DLMS.ino`.
3. Pengaturan Menu **Tools**:
   * **Board:** `ESP32S3 Dev Module`
   * **USB CDC On Boot:** `Enabled`
   * **Partition Scheme:** `Huge APP (3MB No OTA / 1MB SPIFFS)` atau `Default 4MB with spiffs`
   * **Upload Speed:** `921600`
4. Klik **Upload**.
5. Sistem siap digunakan secara mandiri dengan Rotary Encoder & Layar TFT, atau melalui browser di `http://192.168.4.1`.
