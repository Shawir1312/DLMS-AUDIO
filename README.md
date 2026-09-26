<div align="center">

# 🎛️ S.NET AUDIO MANAGEMENT (MINI DLMS 2-CHANNEL INDEPENDENT)
### *Professional Dual-Core ESP32-S3 Digital Loudspeaker Management System*

[![ESP32-S3](https://img.shields.io/badge/Hardware-ESP32--S3%20LX7%20240MHz-red.svg?style=for-the-badge&logo=espressif)](https://www.espressif.com/)
[![DSP](https://img.shields.io/badge/DSP%20Engine-32--bit%20FPU%20Hardware-blue.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Crossover](https://img.shields.io/badge/Channels-2--Ch%20Independent%20DSP-purple.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Display](https://img.shields.io/badge/Display-1.8%22%20SPI%20TFT%20ST7735-orange.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)
[![Interface](https://img.shields.io/badge/Control-Virtual%20Knob%20%2B%20EC11-green.svg?style=for-the-badge)](https://github.com/Shawir1312/DLMS-AUDIO)

<p align="center">
  <b>Sistem Manajemen Audio Digital (DLMS) Dual-Channel Independen Berbasis Dual ESP32</b><br>
  Dilengkapi Layar Depan 1.8" TFT, Dual Modul DAC PCM5102 Stereo, Rotary Encoder Real-Time + Tombol Virtual Web Dashboard.
</p>

---

[Fitur Utama](#-fitur-utama) • [Diagram Arsitektur](#-diagram-arsitektur-sistem) • [Skema Kabel (Pinout)](#-skema-pengkabelan-hardware) • [Panduan Rotary Encoder](#-panduan-penggunaan-rotary-encoder--layar) • [Instalasi](#-instalasi--upload)

---

</div>

## 🌟 Fitur Utama

| Modul | Kemampuan & Spesifikasi |
| :--- | :--- |
| **⚡ Dual Microcontroller Architecture** | **ESP32-S** khusus audio Bluetooth receiver (Zero dropouts) + **ESP32-S3** khusus Real-Time DSP Audio, TFT Display, Rotary Encoder, & Web Server. |
| **🔊 Dual-Channel Independent DSP Engine** | Dua pipeline audio mandiri (**Channel 1** & **Channel 2**): Filter HPF & LPF Linkwitz-Riley **12 dB, 24 dB, hingga 48 dB/oct** (20 Hz - 20 kHz), 3-Band Parametric EQ per channel, Independent Gain, Delay Alignment, Limiter, dan Mute. Bebas difungsikan untuk Crossover 2-Way, Active Bi-Amp, ataupun Stereo Full-Range! |
| **🎛️ Dual DAC PCM5102 (2 Modul Terpisah)** | Menggunakan 2 modul fisik PCM5102: **DAC 1 = CHANNEL 1 (Output Stereo L & R)**, **DAC 2 = CHANNEL 2 (Output Stereo L & R)**. Masing-masing modul DAC menghasilkan output audio stereo independen. |
| **🖥️ Layar 1.8" SPI TFT (128x160)** | Tampilan mendatar (*landscape*) bergaya rackmount audio profesional dengan **3-Channel VU Meter Real-Time: IN, CH1, CH2 (25 FPS)** dan indikator `CLIP`. |
| **🔘 Knob Rotary Encoder & Tombol Virtual** | Navigasi menu panel depan & perubahan parameter secara instan (*real-time*) lewat knob fisik EC11 **ATAU** tombol virtual di Web Dashboard tanpa menunggu knob fisik tiba. |
| **🌐 Standalone Web Controller** | SoftAP Wi-Fi mandiri (`SNET-AUDIO` -> `192.168.4.1`) dengan Captive Portal, respon kurva EQ visual, slider logaritmik, dan 5 slot memori NVS. |
| **🛡️ Audio Protection Suite** | Subsonic HPF, Tweeter Protection LPF, Dual Peak Limiter per channel, Phase Invert 180°, dan Delay Alignment (0–50 ms). |

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
  │  [CORE 1: DUAL INDEPENDENT DSP AUDIO PIPELINES REAL-TIME]              │
  │   ├── CHANNEL 1 (DAC 1): HPF 20Hz-20kHz (12/24/48dB) -> LPF (12/24/48dB)│
  │   │                      -> 3-Band PEQ -> Gain -> Delay -> Limiter     │
  │   │                      -> Diteruskan ke DAC 1 (Stereo Output L & R)  │
  │   │                                                                    │
  │   └── CHANNEL 2 (DAC 2): HPF 20Hz-20kHz (12/24/48dB) -> LPF (12/24/48dB)│
  │                          -> 3-Band PEQ -> Gain -> Delay -> Limiter     │
  │                          -> Diteruskan ke DAC 2 (Stereo Output L & R)  │
  │                                                                        │
  │  [CORE 0: KONTROL & TAMPILAN]                                          │
  │   ├── Web Server SoftAP ("SNET-AUDIO" -> http://192.168.4.1)           │
  │   ├── Layar 1.8" SPI TFT 128x160 (ST7735: VU Meter 3-Ch: IN, CH1, CH2)│
  │   ├── Tombol Virtual Rotary Controller (Web Live Dashboard)            │
  │   └── Rotary Encoder EC11 Fisik (CLK, DT, SW)                          │
  │                                                                        │
  │  [OUTPUT I2S_NUM_1 (TX)]: Pin 15 (BCK), Pin 16 (WS), Pin 17 (DIN)      │
  └───────────────────┬───────────────────────────────┬────────────────────┘
                      │                               │
                      ▼                               ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │     MODUL DAC 1 (PCM5102)    │ │     MODUL DAC 2 (PCM5102)    │
       │          CHANNEL 1           │ │          CHANNEL 2           │
       │   Output Stereo Jack L & R   │ │   Output Stereo Jack L & R   │
       └──────────────┬───────────────┘ └──────────────┬───────────────┘
                      │                                │
                      ▼                                ▼
       ┌──────────────────────────────┐ ┌──────────────────────────────┐
       │    Power Amplifier CH 1      │ │    Power Amplifier CH 2      │
       │  (Bebas: Sub / Low / Full)   │ │  (Bebas: Mid / High / Full)  │
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
| **6 - VCC** | **Pin 5V (VIN)** ESP32 | Daya Modul *(WAJIB 5V karena ada regulator U21 onboard)* |
| **7 - BL** | **Pin 3.3V** (atau **GPIO 8**) | Backlight / Lampu Latar Layar |
| **8 - GND** | **GND** | Ground Bersama |

> ⚠️ **PENTING: MENGATASI LCD SAMAR-SAMAR / BACKLIGHT MATI:**
> 1. **Pin 7 (BL) WAJIB disambung ke 3.3V ESP32 (atau GPIO 8):** Backlight LCD menarik arus 40-50mA. Jangan biarkan floating. Jika disambungkan langsung ke pin 3.3V ESP32, lampu latar dijamin menyala terang stabil terus-menerus tanpa mati.
> 2. **Pin 6 (VCC) LCD WAJIB dicolok ke 5V (VIN):** Modul LCD ini memiliki IC regulator 3.3V onboard (`U21`, tulisan: `VCC=5V -> J1 OPEN`). Jika dicolok ke 3.3V, tegangan turun ke ~2.7V sehingga layar terlihat redup / pudar (samar-samar).
> 3. **Kelupas Plastik Pelindung Layar:** Modul baru memiliki stiker pelindung layar bermotif pemandangan gunung/sunset. Pastikan plastik stiker pelindung ini sudah dikelupas agar tulisan terlihat jernih dan tajam!

---

### 2. Rotary Encoder (EC11 + Switch Button) ke ESP32-S3
| Pin Rotary Encoder | Pin ESP32-S3 | Fungsi / Deskripsi |
| :--- | :--- | :--- |
| **CLK (Fase A)** | **GPIO 1** | Pulsa Putar A |
| **DT (Fase B)** | **GPIO 2** | Pulsa Putar B |
| **SW (Push Switch)** | **GPIO 42** | Tombol Tekan Encoder (Active LOW) |
| **+ (VCC)** | **3.3V** | Power VCC Enkoder |
| **GND** | **GND** | Ground bersama |

---

### 3. Dual DAC PCM5102 ke ESP32-S3 (2 Modul DAC Fisik Terpisah)
Sistem menggunakan **2 modul fisik PCM5102 terpisah**, di mana setiap modul menghasilkan sinyal audio dengan DSP mandiri:

| Pin ESP32-S3 | Sambung Ke Modul DAC 1 & DAC 2 | Fungsi |
| :--- | :--- | :--- |
| **GPIO 15** | **BCK** (DAC 1 & DAC 2) | I2S Bit Clock Out |
| **GPIO 16** | **LCK / LRCK** (DAC 1 & DAC 2) | I2S Word Select Out |
| **GPIO 17** | **DIN** (DAC 1 & DAC 2) | I2S Serial Data Out |
| **GND** | **GND** | Ground bersama |
| **5V** atau **3.3V** | **VCC** | Power Supply DAC |

> **Topologi 2 Modul DAC (Setiap Modul Memiliki Output Stereo R & L Sendiri):**
> * **MODUL DAC 1 = CHANNEL 1:** Memiliki pipeline DSP mandiri (HPF 20Hz-20kHz 12/24/48dB, LPF 20Hz-20kHz 12/24/48dB, 3-Band Parametric EQ, Gain, Delay, Phase Invert, Limiter). Kedua output jack (R & L) pada Modul DAC 1 mengeluarkan sinyal Channel 1. Karakter suara bebas diatur: **Subwoofer**, **Low-Mid**, **Vokal**, atau **Stereo Full-Range**!
> * **MODUL DAC 2 = CHANNEL 2:** Memiliki pipeline DSP mandiri (HPF 20Hz-20kHz 12/24/48dB, LPF 20Hz-20kHz 12/24/48dB, 3-Band Parametric EQ, Gain, Delay, Phase Invert, Limiter). Kedua output jack (R & L) pada Modul DAC 2 mengeluarkan sinyal Channel 2. Karakter suara bebas diatur: **Mid/High**, **Tweeter**, **Low**, atau **Stereo Full-Range**!
> * **Bebas Tanpa Keterikatan Nama:** Anda bebas mengatur Channel 1 dan Channel 2 sesuai kebutuhan audio lapangan, tanpa dipaksa bahwa satu harus Sub dan satu harus Mid/High.

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
 │ S.NET MINI DLMS 2-CHANNEL INDEPENDENT                   │
 ├─────────────────────────────────────────────────────────┤
 │ IN  [████████████░░░░░░░░░░░░░░░░░░] -14dB              │
 │ CH1 [██████████████████░░░░░░░░░░░░] -6dB               │
 │ CH2 [████████████░░░░░░░░░░░░░░░░░░] -12dB              │
 ├─────────────────────────────────────────────────────────┤
 │ CH1: HPF 80Hz | CH2: HPF 1.2kHz | PRESET: Slot 1        │
 │ IP: 192.168.4.1                 | VOL: 0.0dB            │
 ├─────────────────────────────────────────────────────────┤
 │ [ TEKAN KNOB : MENU ]                                   │
 └─────────────────────────────────────────────────────────┘
```

### A. Kontrol Tombol Virtual di Web Dashboard (Tanpa Menunggu Knob Fisik Datang!)
Jika modul rotary encoder EC11 fisik Anda belum tiba atau sedang dalam pengiriman, sistem telah dilengkapi **Panel Kontrol Virtual Rotary Encoder** langsung di Web Dashboard (`http://192.168.4.1`):

```text
 ┌──────────────────────────────────────────────────────────────────┐
 │ 🎮 KONTROL VIRTUAL ROTARY ENCODER              [LIVE LCD TFT]    │
 ├──────────────────────────────────────────────────────────────────┤
 │ [ ⟲ PUTAR KIRI ]     [ 🔘 TEKAN KNOB ]     [ ⟳ PUTAR KANAN ]    │
 │  (Prev / Nilai -1)    [ MENU / EDIT / OK ]   (Next / Nilai +1)   │
 ├──────────────────────────────────────────────────────────────────┤
 │ [ ⏪ Cepat -5 ]   [ ↩ TEKAN LAMA (HOME) ]   [ ⏩ Cepat +5 ]     │
 └──────────────────────────────────────────────────────────────────┘
```

* **⟲ PUTAR KIRI (`left`):** Menggeser kursor menu ke atas / mengurangi nilai parameter aktif (-1).
* **🔘 TEKAN KNOB (`click`):** Membuka Menu Pengaturan, masuk ke mode edit nilai parameter, atau mengunci pilihan.
* **⟳ PUTAR KANAN (`right`):** Menggeser kursor menu ke bawah / menambah nilai parameter aktif (+1).
* **⏪ / ⏩ LANGKAH CEPAT (-5 / +5):** Mengubah frekuensi atau gain secara cepat tanpa perlu banyak klik.
* **↩ TEKAN LAMA (`long`):** Kembali seketika ke Layar Utama VU Meter.
* **🕹️ Floating Mini Remote Bar:** Tombol mengambang di pojok kanan bawah layar HP yang tetap dapat ditekan meskipun Anda sedang menggeser ke bawah layar untuk tuning EQ atau Crossover.
* **⌨️ Shortcut Keyboard Laptop/PC:**
  * **⬅ Panah Kiri:** Putar Kiri *(Tahan Shift untuk -5)*
  * **➡ Panah Kanan:** Putar Kanan *(Tahan Shift untuk +5)*
  * **[Enter]:** Tekan Knob / OK
  * **[Esc] / [Backspace]:** Tekan Lama (Kembali ke VU Meter)

---

### B. Kontrol Knob Rotary Encoder Fisik (EC11)
1. **Layar Utama (Home Monitoring)**:
   * Menampilkan VU meter 3-channel responsif 25 FPS dengan indikator `CLIP`.
   * Menampilkan parameter crossover aktif, gain level, IP address, dan status preset.
2. **Masuk ke Menu Pengaturan**:
   * **Tekan tombol encoder 1x** saat di layar utama untuk masuk ke Menu Pengaturan.
   * **Putar knob** untuk navigasi menu:
     * `CH1 GAIN` : -30 dB s/d +12 dB
     * `CH2 GAIN` : -30 dB s/d +12 dB
     * `CH1 HPF` : 20 Hz – 20.000 Hz
     * `CH1 LPF` : 20 Hz – 20.000 Hz
     * `CH2 HPF` : 20 Hz – 20.000 Hz
     * `CH2 LPF` : 20 Hz – 20.000 Hz
     * `CH1 PHASE` : Normal / Invert 180°
     * `CH2 PHASE` : Normal / Invert 180°
     * `CH1 MUTE` : Mute / Unmute
     * `CH2 MUTE` : Mute / Unmute
     * `MASTER VOL`: -60 dB s/d +12 dB
     * `ALL MUTE` : Mute Semua Suara
     * `LOAD PRESET`: Slot 1 – 5
     * `SAVE TO NVS`: Simpan konfigurasi ke memori flash NVS
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
