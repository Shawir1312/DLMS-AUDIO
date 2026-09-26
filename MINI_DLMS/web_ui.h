#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>S.NET AUDIO MANAGEMENT - 2-Channel Independent DLMS</title>
<style>
:root {
  --bg: #0b1120;
  --surface: #1e293b;
  --card: #1e293b;
  --card-border: #334155;
  --primary: #38bdf8;
  --primary-hover: #0284c7;
  --ch1: #06b6d4;
  --ch2: #f59e0b;
  --accent: #818cf8;
  --text-main: #f8fafc;
  --text-muted: #94a3b8;
  --danger: #ef4444;
  --danger-bg: rgba(239,68,68,0.2);
  --warning: #f59e0b;
  --success: #10b981;
  --radius: 12px;
  --shadow: 0 4px 14px rgba(0,0,0,0.35);
}
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
body { background: var(--bg); color: var(--text-main); line-height: 1.5; padding-bottom: 60px; }

/* Top Header */
.header { background: linear-gradient(135deg, #0f172a 0%, #1e1b4b 50%, #1e293b 100%); border-bottom: 1px solid var(--card-border); padding: 18px 24px; box-shadow: 0 4px 20px rgba(0,0,0,0.5); }
.header-content { max-width: 1360px; margin: 0 auto; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 14px; }
.brand h1 { font-size: 20px; font-weight: 800; letter-spacing: 1px; color: #fff; display: flex; align-items: center; gap: 8px; }
.brand h1 span { color: var(--primary); font-size: 14px; background: rgba(56,189,248,0.15); border: 1px solid var(--primary); border-radius: 6px; padding: 2px 8px; }
.brand p { font-size: 12px; color: #94a3b8; font-weight: 600; letter-spacing: 0.5px; }

.status-badges { display: flex; gap: 8px; flex-wrap: wrap; }
.badge { background: rgba(30,41,59,0.8); border: 1px solid var(--card-border); padding: 5px 12px; border-radius: 20px; font-size: 12px; font-weight: 600; display: flex; align-items: center; gap: 6px; }
.badge .dot { width: 8px; height: 8px; border-radius: 50%; background: #64748b; }
.badge.connected .dot { background: #34d399; box-shadow: 0 0 8px #34d399; }
.badge.streaming .dot { background: #38bdf8; box-shadow: 0 0 8px #38bdf8; animation: pulse 1.5s infinite; }
@keyframes pulse { 0% { opacity: 0.4; } 50% { opacity: 1; } 100% { opacity: 0.4; } }

.container { max-width: 1360px; margin: 20px auto; padding: 0 16px; }
.grid-top { display: grid; grid-template-columns: 320px 1fr; gap: 20px; margin-bottom: 20px; }
@media (max-width: 980px) { .grid-top { grid-template-columns: 1fr; } }

.card { background: var(--card); border-radius: var(--radius); padding: 16px; border: 1px solid var(--card-border); box-shadow: var(--shadow); margin-bottom: 16px; }
.card-title { font-size: 13px; font-weight: 700; color: #cbd5e1; text-transform: uppercase; letter-spacing: 0.8px; margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center; }

/* VU Meters */
.vu-panel { display: flex; flex-direction: column; gap: 10px; }
.vu-meter-group { display: flex; flex-direction: column; gap: 4px; }
.vu-label-row { display: flex; justify-content: space-between; font-size: 11px; font-weight: 700; }
.vu-val { font-family: monospace; font-size: 12px; color: var(--primary); }
.meter-container { height: 16px; background: #090d16; border-radius: 4px; position: relative; overflow: hidden; display: flex; padding: 2px; border: 1px solid #1e293b; }
.meter-bar { height: 100%; width: 0%; border-radius: 2px; background: linear-gradient(to right, #10b981 0%, #10b981 70%, #f59e0b 85%, #ef4444 100%); transition: width 0.08s ease-out; }
.meter-peak { position: absolute; top: 0; bottom: 0; width: 3px; background: #fff; box-shadow: 0 0 6px #fff; transition: left 0.15s ease-out; }
.meter-ticks { display: flex; justify-content: space-between; font-size: 9px; color: #64748b; font-family: monospace; }
.clip-indicator { display: inline-block; padding: 2px 6px; border-radius: 4px; font-size: 10px; font-weight: 900; background: #334155; color: #94a3b8; }
.clip-indicator.active { background: var(--danger); color: #fff; box-shadow: 0 0 10px rgba(239,68,68,0.8); }

/* Controls */
.control-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px; gap: 10px; }
.control-label { font-size: 12px; font-weight: 600; color: #cbd5e1; }
.slider-container { display: flex; align-items: center; gap: 8px; width: 100%; }
input[type="range"] { flex: 1; -webkit-appearance: none; height: 6px; border-radius: 3px; background: #334155; outline: none; }
input[type="range"]::-webkit-slider-thumb { -webkit-appearance: none; width: 16px; height: 16px; border-radius: 50%; background: var(--primary); cursor: pointer; }
input[type="number"] { width: 70px; padding: 4px 6px; background: #0f172a; border: 1px solid var(--card-border); color: #fff; border-radius: 6px; font-size: 12px; font-weight: 600; text-align: right; }
select { background: #0f172a; border: 1px solid var(--card-border); color: #fff; border-radius: 6px; padding: 4px 8px; font-size: 12px; }

.btn { padding: 8px 14px; border-radius: 6px; font-size: 12px; font-weight: 700; cursor: pointer; border: 1px solid transparent; transition: all 0.2s; display: inline-flex; align-items: center; justify-content: center; gap: 6px; }
.btn-primary { background: var(--primary); color: #0f172a; }
.btn-primary:hover { background: #0284c7; color: #fff; }
.btn-outline { background: transparent; border-color: var(--card-border); color: #cbd5e1; }
.btn-outline:hover { background: #334155; }
.btn-danger { background: #b91c1c; color: #fff; }
.btn-danger.active { background: #ef4444; box-shadow: 0 0 12px rgba(239,68,68,0.6); }

/* Switch Toggle */
.switch { position: relative; display: inline-block; width: 38px; height: 20px; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider-toggle { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #334155; transition: .3s; border-radius: 20px; }
.slider-toggle:before { position: absolute; content: ""; height: 14px; width: 14px; left: 3px; bottom: 3px; background-color: white; transition: .3s; border-radius: 50%; }
input:checked + .slider-toggle { background-color: var(--primary); }
input:checked + .slider-toggle:before { transform: translateX(18px); }

/* Frequency Response Graph */
.canvas-wrapper { position: relative; width: 100%; height: 240px; background: #090d16; border-radius: var(--radius); overflow: hidden; border: 1px solid var(--card-border); margin-bottom: 16px; }
canvas#eqCanvas { width: 100%; height: 100%; display: block; }
.graph-legend { position: absolute; top: 8px; right: 12px; font-size: 11px; font-family: monospace; display: flex; gap: 12px; background: rgba(15,23,42,0.85); padding: 4px 8px; border-radius: 4px; border: 1px solid #334155; }
.legend-ch1 { color: var(--ch1); font-weight: 700; }
.legend-ch2 { color: var(--ch2); font-weight: 700; }

/* 2-Channel Columns */
.channels-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
@media (max-width: 900px) { .channels-grid { grid-template-columns: 1fr; } }

.ch-card { background: #162032; border: 1px solid #293548; border-radius: var(--radius); padding: 16px; }
.ch1-header { border-left: 4px solid var(--ch1); padding-left: 10px; margin-bottom: 14px; }
.ch2-header { border-left: 4px solid var(--ch2); padding-left: 10px; margin-bottom: 14px; }
.ch-title { font-size: 16px; font-weight: 800; }
.ch1-title { color: var(--ch1); }
.ch2-title { color: var(--ch2); }
.ch-sub { font-size: 11px; color: var(--text-muted); }

/* Sub sections inside Channel */
.sub-box { background: #1e293b; border: 1px solid var(--card-border); border-radius: 8px; padding: 12px; margin-bottom: 12px; }
.sub-box-title { font-size: 12px; font-weight: 700; color: #e2e8f0; margin-bottom: 8px; display: flex; justify-content: space-between; align-items: center; }

/* PEQ mini grid */
.peq-row { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-top: 8px; }
.peq-mini-card { background: #0f172a; border: 1px solid #334155; border-radius: 6px; padding: 8px; font-size: 11px; }

/* Presets Bar */
.presets-bar { display: flex; gap: 6px; flex-wrap: wrap; margin-top: 8px; }
.preset-btn { flex: 1; min-width: 60px; padding: 8px; border-radius: 6px; border: 1px solid var(--card-border); background: #0f172a; color: #cbd5e1; font-weight: 700; font-size: 12px; cursor: pointer; text-align: center; }
.preset-btn.active { background: rgba(56,189,248,0.15); border-color: var(--primary); color: var(--primary); }

.quick-bar { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 16px; }
</style>
</head>
<body>

<div class="header">
  <div class="header-content">
    <div class="brand">
      <h1>S.NET DLMS <span>2-CH INDEPENDENT DAC</span></h1>
      <p>Dual PCM5102 I2S DACs with Fully Independent DSP Pipelines</p>
    </div>
    <div class="status-badges">
      <div class="badge" id="badge-wifi"><span class="dot"></span> <span id="wifi-txt">WiFi: 192.168.4.1</span></div>
      <div class="badge" id="badge-bt"><span class="dot"></span> <span id="bt-txt">BT: Disconnected</span></div>
      <div class="badge" id="badge-sr"><span id="sr-txt">44.1 kHz</span></div>
    </div>
  </div>
</div>

<div class="container">
  <!-- Quick Preset Architecture Buttons -->
  <div class="quick-bar">
    <span style="font-size:12px;font-weight:700;color:var(--text-muted);display:flex;align-items:center;">Arsitektur Cepat:</span>
    <button class="btn btn-outline" style="border-color:#38bdf8;color:#38bdf8;" onclick="setMode2WayCrossover()">Mode 2-Way Crossover (CH1 Sub / CH2 Mid-High)</button>
    <button class="btn btn-outline" style="border-color:#10b981;color:#10b981;" onclick="setModeStereoFull()">Mode Stereo Full-Range (CH1 Flat / CH2 Flat)</button>
    <button class="btn btn-outline" style="border-color:#f59e0b;color:#f59e0b;" onclick="setModeActive2Way()">Mode Mid-Low & Tweeter (CH1 Low / CH2 High)</button>
  </div>

  <div class="grid-top">
    <!-- LEFT: VU METERS & MASTER -->
    <div class="left-col">
      <div class="card">
        <div class="card-title">
          <span>Level Meters (dBFS)</span>
          <span class="clip-indicator" id="clip-badge">CLIP</span>
        </div>
        <div class="vu-panel">
          <!-- INPUT -->
          <div class="vu-meter-group">
            <div class="vu-label-row"><span>INPUT PEAK</span><span class="vu-val" id="in-peak-val">-60 dB</span></div>
            <div class="meter-container"><div class="meter-bar" id="in-peak-bar"></div><div class="meter-peak" id="in-peak-tick"></div></div>
            <div class="meter-ticks"><span>-60</span><span>-30</span><span>-18</span><span>-12</span><span>-6</span><span>0</span></div>
          </div>

          <!-- CH1 DAC 1 -->
          <div class="vu-meter-group">
            <div class="vu-label-row"><span style="color:var(--ch1);">CH 1 (DAC 1 / LEFT)</span><span class="vu-val" id="ch1-peak-val">-60 dB</span></div>
            <div class="meter-container"><div class="meter-bar" id="ch1-peak-bar"></div><div class="meter-peak" id="ch1-peak-tick"></div></div>
            <div class="meter-ticks"><span>-60</span><span>-30</span><span>-18</span><span>-12</span><span>-6</span><span>0</span></div>
          </div>

          <!-- CH2 DAC 2 -->
          <div class="vu-meter-group">
            <div class="vu-label-row"><span style="color:var(--ch2);">CH 2 (DAC 2 / RIGHT)</span><span class="vu-val" id="ch2-peak-val">-60 dB</span></div>
            <div class="meter-container"><div class="meter-bar" id="ch2-peak-bar"></div><div class="meter-peak" id="ch2-peak-tick"></div></div>
            <div class="meter-ticks"><span>-60</span><span>-30</span><span>-18</span><span>-12</span><span>-6</span><span>0</span></div>
          </div>
        </div>
      </div>

      <!-- MASTER & PRESETS -->
      <div class="card">
        <div class="card-title">Master Output</div>
        <div class="control-row">
          <span class="control-label">Master Volume</span>
          <span id="master-gain-txt" style="font-weight:700;color:var(--primary);">0.0 dB</span>
        </div>
        <div class="slider-container" style="margin-bottom:12px;">
          <input type="range" id="master-gain" min="-60" max="12" step="0.5" value="0">
          <input type="number" id="master-gain-num" min="-60" max="12" step="0.5" value="0">
        </div>
        <div class="control-row">
          <span class="control-label">Master Polarity Invert</span>
          <label class="switch"><input type="checkbox" id="master-invert"><span class="slider-toggle"></span></label>
        </div>
        <button class="btn btn-danger" style="width:100%;margin-top:6px;" id="master-mute-btn">MUTE ALL AUDIO</button>

        <div style="margin-top:16px;padding-top:12px;border-top:1px solid var(--card-border);">
          <div class="card-title" style="margin-bottom:6px;">Presets Memory (NVS)</div>
          <div class="presets-bar">
            <button class="preset-btn active" onclick="loadPreset(1)">Slot 1</button>
            <button class="preset-btn" onclick="loadPreset(2)">Slot 2</button>
            <button class="preset-btn" onclick="loadPreset(3)">Slot 3</button>
            <button class="preset-btn" onclick="loadPreset(4)">Slot 4</button>
            <button class="preset-btn" onclick="loadPreset(5)">Slot 5</button>
          </div>
          <div style="display:flex;gap:6px;margin-top:8px;">
            <button class="btn btn-primary" style="flex:1;" onclick="saveCurrentPreset()">Save Active</button>
            <button class="btn btn-outline" style="flex:1;" onclick="resetCurrentPreset()">Reset</button>
          </div>
        </div>
      </div>
    </div>

    <!-- RIGHT: DUAL FREQUENCY RESPONSE GRAPH -->
    <div class="right-col">
      <div class="canvas-wrapper">
        <canvas id="eqCanvas"></canvas>
        <div class="graph-legend">
          <span class="legend-ch1">&mdash; CH 1 Response</span>
          <span class="legend-ch2">&mdash; CH 2 Response</span>
        </div>
      </div>

      <!-- LINK CROSSOVER CONTROL -->
      <div class="card">
        <div class="control-row">
          <div>
            <span style="font-weight:700;color:#fff;">Tautkan Crossover 2-Way (Link X-Over)</span>
            <div style="font-size:11px;color:var(--text-muted);">Jika ON: Mengatur frekuensi akan sinkronkan CH1 LPF dan CH2 HPF otomatis</div>
          </div>
          <label class="switch"><input type="checkbox" id="xover-link-toggle"><span class="slider-toggle"></span></label>
        </div>
        <div class="control-row" id="xover-controls-row" style="margin-top:8px;">
          <span class="control-label">X-Over Cutoff Freq:</span>
          <input type="number" id="xover-freq-num" min="20" max="20000" step="5" value="100" style="width:80px;">
          <select id="xover-slope-sel" style="margin-left:8px;">
            <option value="12">12 dB/oct</option>
            <option value="24" selected>24 dB/oct (Linkwitz-Riley)</option>
            <option value="48">48 dB/oct (Extreme 8th Order)</option>
          </select>
        </div>
      </div>
    </div>
  </div>

  <!-- INDEPENDENT 2-CHANNEL DSP CONTROLS -->
  <div class="channels-grid">
    <!-- ========================================== -->
    <!-- CHANNEL 1: DAC 1 / LEFT                   -->
    <!-- ========================================== -->
    <div class="ch-card">
      <div class="ch1-header">
        <div class="ch-title ch1-title">CHANNEL 1 (DAC 1 / LEFT)</div>
        <div class="ch-sub">Jalur Audio Independen Output Kiri</div>
      </div>

      <!-- Gain, Phase & Delay -->
      <div class="sub-box">
        <div class="control-row">
          <span class="control-label">Gain Channel 1</span>
          <span id="ch1-gain-txt" style="font-weight:700;color:var(--ch1);">0.0 dB</span>
        </div>
        <div class="slider-container" style="margin-bottom:8px;">
          <input type="range" id="ch1-gain" min="-60" max="12" step="0.5" value="0">
          <input type="number" id="ch1-gain-num" min="-60" max="12" step="0.5" value="0">
        </div>
        <div class="control-row">
          <span class="control-label">Fase 180&deg; (Invert)</span>
          <label class="switch"><input type="checkbox" id="ch1-invert"><span class="slider-toggle"></span></label>
          <button class="btn btn-danger" style="padding:4px 10px;" id="ch1-mute-btn">MUTE CH1</button>
        </div>
        <div class="control-row" style="margin-top:6px;">
          <span class="control-label">Delay Time</span>
          <span id="ch1-delay-txt" style="font-weight:700;">0.0 ms</span>
        </div>
        <div class="slider-container">
          <input type="range" id="ch1-delay" min="0" max="50" step="0.1" value="0">
          <input type="number" id="ch1-delay-num" min="0" max="50" step="0.1" value="0">
        </div>
      </div>

      <!-- HPF & LPF Filter -->
      <div class="sub-box">
        <div class="sub-box-title">
          <span>High-Pass Filter (HPF / Low Cut)</span>
          <label class="switch"><input type="checkbox" id="ch1-hpf-en" checked><span class="slider-toggle"></span></label>
        </div>
        <div class="control-row">
          <span class="control-label">Cutoff:</span>
          <input type="number" id="ch1-hpf-f" min="20" max="20000" step="5" value="25">
          <select id="ch1-hpf-s"><option value="12">12 dB</option><option value="24" selected>24 dB</option><option value="48">48 dB</option></select>
        </div>
        <input type="range" id="ch1-hpf-sl" min="0" max="1000" value="0" style="width:100%;">
      </div>

      <div class="sub-box">
        <div class="sub-box-title">
          <span>Low-Pass Filter (LPF / High Cut)</span>
          <label class="switch"><input type="checkbox" id="ch1-lpf-en"><span class="slider-toggle"></span></label>
        </div>
        <div class="control-row">
          <span class="control-label">Cutoff:</span>
          <input type="number" id="ch1-lpf-f" min="20" max="20000" step="5" value="20000">
          <select id="ch1-lpf-s"><option value="12">12 dB</option><option value="24" selected>24 dB</option><option value="48">48 dB</option></select>
        </div>
        <input type="range" id="ch1-lpf-sl" min="0" max="1000" value="1000" style="width:100%;">
      </div>

      <!-- 3-Band Parametric EQ -->
      <div class="sub-box">
        <div class="sub-box-title">3-Band Parametric EQ (CH 1)</div>
        <div class="peq-row" id="ch1-peq-container"></div>
      </div>

      <!-- Limiter -->
      <div class="sub-box">
        <div class="sub-box-title">
          <span>Peak Limiter (CH 1)</span>
          <label class="switch"><input type="checkbox" id="ch1-lim-en" checked><span class="slider-toggle"></span></label>
        </div>
        <div class="control-row">
          <span class="control-label">Threshold:</span>
          <span id="ch1-lim-th-txt" style="font-weight:700;">-1.0 dB</span>
        </div>
        <input type="range" id="ch1-lim-th" min="-30" max="0" step="0.5" value="-1" style="width:100%;">
      </div>
    </div>

    <!-- ========================================== -->
    <!-- CHANNEL 2: DAC 2 / RIGHT                  -->
    <!-- ========================================== -->
    <div class="ch-card">
      <div class="ch2-header">
        <div class="ch-title ch2-title">CHANNEL 2 (DAC 2 / RIGHT)</div>
        <div class="ch-sub">Jalur Audio Independen Output Kanan</div>
      </div>

      <!-- Gain, Phase & Delay -->
      <div class="sub-box">
        <div class="control-row">
          <span class="control-label">Gain Channel 2</span>
          <span id="ch2-gain-txt" style="font-weight:700;color:var(--ch2);">0.0 dB</span>
        </div>
        <div class="slider-container" style="margin-bottom:8px;">
          <input type="range" id="ch2-gain" min="-60" max="12" step="0.5" value="0">
          <input type="number" id="ch2-gain-num" min="-60" max="12" step="0.5" value="0">
        </div>
        <div class="control-row">
          <span class="control-label">Fase 180&deg; (Invert)</span>
          <label class="switch"><input type="checkbox" id="ch2-invert"><span class="slider-toggle"></span></label>
          <button class="btn btn-danger" style="padding:4px 10px;" id="ch2-mute-btn">MUTE CH2</button>
        </div>
        <div class="control-row" style="margin-top:6px;">
          <span class="control-label">Delay Time</span>
          <span id="ch2-delay-txt" style="font-weight:700;">0.0 ms</span>
        </div>
        <div class="slider-container">
          <input type="range" id="ch2-delay" min="0" max="50" step="0.1" value="0">
          <input type="number" id="ch2-delay-num" min="0" max="50" step="0.1" value="0">
        </div>
      </div>

      <!-- HPF & LPF Filter -->
      <div class="sub-box">
        <div class="sub-box-title">
          <span>High-Pass Filter (HPF / Low Cut)</span>
          <label class="switch"><input type="checkbox" id="ch2-hpf-en"><span class="slider-toggle"></span></label>
        </div>
        <div class="control-row">
          <span class="control-label">Cutoff:</span>
          <input type="number" id="ch2-hpf-f" min="20" max="20000" step="5" value="20">
          <select id="ch2-hpf-s"><option value="12">12 dB</option><option value="24" selected>24 dB</option><option value="48">48 dB</option></select>
        </div>
        <input type="range" id="ch2-hpf-sl" min="0" max="1000" value="0" style="width:100%;">
      </div>

      <div class="sub-box">
        <div class="sub-box-title">
          <span>Low-Pass Filter (LPF / High Cut)</span>
          <label class="switch"><input type="checkbox" id="ch2-lpf-en"><span class="slider-toggle"></span></label>
        </div>
        <div class="control-row">
          <span class="control-label">Cutoff:</span>
          <input type="number" id="ch2-lpf-f" min="20" max="20000" step="5" value="20000">
          <select id="ch2-lpf-s"><option value="12">12 dB</option><option value="24">24 dB</option><option value="48">48 dB</option></select>
        </div>
        <input type="range" id="ch2-lpf-sl" min="0" max="1000" value="1000" style="width:100%;">
      </div>

      <!-- 3-Band Parametric EQ -->
      <div class="sub-box">
        <div class="sub-box-title">3-Band Parametric EQ (CH 2)</div>
        <div class="peq-row" id="ch2-peq-container"></div>
      </div>

      <!-- Limiter -->
      <div class="sub-box">
        <div class="sub-box-title">
          <span>Peak Limiter (CH 2)</span>
          <label class="switch"><input type="checkbox" id="ch2-lim-en" checked><span class="slider-toggle"></span></label>
        </div>
        <div class="control-row">
          <span class="control-label">Threshold:</span>
          <span id="ch2-lim-th-txt" style="font-weight:700;">-1.0 dB</span>
        </div>
        <input type="range" id="ch2-lim-th" min="-30" max="0" step="0.5" value="-1" style="width:100%;">
      </div>
    </div>
  </div>
</div>

<script>
let dspState = {
  master_gain_db: 0,
  mute: false,
  polarity_inverted: false,
  xover_linked: false,
  xover_freq: 100,
  xover_slope: 24,
  ch1: {
    gain_db: 0, mute: false, polarity_inverted: false, delay_ms: 0,
    hpf: { enabled: true, freq: 25, slope: 24 },
    lpf: { enabled: false, freq: 20000, slope: 24 },
    limiter: { enabled: true, threshold_db: -1, attack_ms: 10, release_ms: 100 },
    peq: [
      { enabled: true, type: 1, freq: 50, gain_db: 0, q: 1.0 },
      { enabled: true, type: 0, freq: 100, gain_db: 0, q: 1.0 },
      { enabled: true, type: 0, freq: 250, gain_db: 0, q: 1.0 }
    ]
  },
  ch2: {
    gain_db: 0, mute: false, polarity_inverted: false, delay_ms: 0,
    hpf: { enabled: false, freq: 20, slope: 24 },
    lpf: { enabled: false, freq: 20000, slope: 12 },
    limiter: { enabled: true, threshold_db: -1, attack_ms: 2, release_ms: 50 },
    peq: [
      { enabled: true, type: 0, freq: 1000, gain_db: 0, q: 1.0 },
      { enabled: true, type: 0, freq: 4000, gain_db: 0, q: 1.0 },
      { enabled: true, type: 2, freq: 12000, gain_db: 0, q: 1.0 }
    ]
  }
};

let activePresetSlot = 1;
let debounceTimer = null;

function freqToSlider(f) {
  if (!f || f < 20) f = 20;
  if (f > 20000) f = 20000;
  return Math.round((Math.log10(f / 20) / 3.0) * 1000);
}
function sliderToFreq(s) {
  const f = 20 * Math.pow(1000, s / 1000);
  if (f < 100) return Math.round(f);
  if (f < 1000) return Math.round(f / 5) * 5;
  return Math.round(f / 50) * 50;
}

function initPeqUi() {
  ['ch1', 'ch2'].forEach(chKey => {
    const cont = document.getElementById(`${chKey}-peq-container`);
    cont.innerHTML = '';
    for (let b = 0; b < 3; b++) {
      const card = document.createElement('div');
      card.className = 'peq-mini-card';
      card.innerHTML = `
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:4px;">
          <span style="font-weight:700;color:var(--primary);">Band ${b+1}</span>
          <label class="switch"><input type="checkbox" id="${chKey}-peq-en-${b}" onchange="updatePeq('${chKey}', ${b})"><span class="slider-toggle"></span></label>
        </div>
        <select id="${chKey}-peq-t-${b}" style="width:100%;margin-bottom:4px;" onchange="updatePeq('${chKey}', ${b})">
          <option value="0">Peaking</option>
          <option value="1">Low Shelf</option>
          <option value="2">High Shelf</option>
        </select>
        <div style="display:flex;justify-content:space-between;"><span>Freq</span><span id="${chKey}-peq-fv-${b}">1000Hz</span></div>
        <input type="range" id="${chKey}-peq-f-${b}" min="20" max="20000" step="5" oninput="updatePeq('${chKey}', ${b})" style="width:100%;">
        <div style="display:flex;justify-content:space-between;"><span>Gain</span><span id="${chKey}-peq-gv-${b}">0dB</span></div>
        <input type="range" id="${chKey}-peq-g-${b}" min="-18" max="18" step="0.5" oninput="updatePeq('${chKey}', ${b})" style="width:100%;">
        <div style="display:flex;justify-content:space-between;"><span>Q</span><span id="${chKey}-peq-qv-${b}">1.0</span></div>
        <input type="range" id="${chKey}-peq-q-${b}" min="0.1" max="10" step="0.1" oninput="updatePeq('${chKey}', ${b})" style="width:100%;">
      `;
      cont.appendChild(card);
    }
  });
}

async function loadDspState() {
  try {
    const res = await fetch('/api/dsp');
    if (res.ok) {
      const data = await res.json();
      if (data.ch1) dspState.ch1 = data.ch1;
      if (data.ch2) dspState.ch2 = data.ch2;
      if (data.master_gain_db !== undefined) dspState.master_gain_db = data.master_gain_db;
      if (data.mute !== undefined) dspState.mute = data.mute;
      if (data.polarity_inverted !== undefined) dspState.polarity_inverted = data.polarity_inverted;
      if (data.xover_linked !== undefined) dspState.xover_linked = data.xover_linked;
      if (data.xover_freq !== undefined) dspState.xover_freq = data.xover_freq;
      if (data.xover_slope !== undefined) dspState.xover_slope = data.xover_slope;
      syncUiFromState();
      drawGraph();
    }
  } catch (e) {
    console.warn("Error fetching /api/dsp:", e);
  }
}

function syncUiFromState() {
  // Master
  document.getElementById('master-gain').value = dspState.master_gain_db;
  document.getElementById('master-gain-num').value = dspState.master_gain_db;
  document.getElementById('master-gain-txt').innerText = `${dspState.master_gain_db.toFixed(1)} dB`;
  document.getElementById('master-invert').checked = dspState.polarity_inverted;
  const mm = document.getElementById('master-mute-btn');
  mm.className = 'btn btn-danger' + (dspState.mute ? ' active' : '');
  mm.innerText = dspState.mute ? 'ALL AUDIO MUTED' : 'MUTE ALL AUDIO';

  // Xover Link
  document.getElementById('xover-link-toggle').checked = dspState.xover_linked;
  document.getElementById('xover-freq-num').value = Math.round(dspState.xover_freq);
  document.getElementById('xover-slope-sel').value = dspState.xover_slope;

  // Channels
  ['ch1', 'ch2'].forEach(chKey => {
    const c = dspState[chKey];
    document.getElementById(`${chKey}-gain`).value = c.gain_db;
    document.getElementById(`${chKey}-gain-num`).value = c.gain_db;
    document.getElementById(`${chKey}-gain-txt`).innerText = `${c.gain_db.toFixed(1)} dB`;
    document.getElementById(`${chKey}-invert`).checked = c.polarity_inverted;
    const mb = document.getElementById(`${chKey}-mute-btn`);
    mb.className = 'btn btn-danger' + (c.mute ? ' active' : '');
    mb.innerText = c.mute ? 'MUTED' : `MUTE ${chKey.toUpperCase()}`;

    document.getElementById(`${chKey}-delay`).value = c.delay_ms;
    document.getElementById(`${chKey}-delay-num`).value = c.delay_ms;
    document.getElementById(`${chKey}-delay-txt`).innerText = `${c.delay_ms.toFixed(1)} ms`;

    // HPF
    document.getElementById(`${chKey}-hpf-en`).checked = c.hpf.enabled;
    document.getElementById(`${chKey}-hpf-f`).value = Math.round(c.hpf.freq);
    document.getElementById(`${chKey}-hpf-sl`).value = freqToSlider(c.hpf.freq);
    document.getElementById(`${chKey}-hpf-s`).value = c.hpf.slope;

    // LPF
    document.getElementById(`${chKey}-lpf-en`).checked = c.lpf.enabled;
    document.getElementById(`${chKey}-lpf-f`).value = Math.round(c.lpf.freq);
    document.getElementById(`${chKey}-lpf-sl`).value = freqToSlider(c.lpf.freq);
    document.getElementById(`${chKey}-lpf-s`).value = c.lpf.slope;

    // Limiter
    document.getElementById(`${chKey}-lim-en`).checked = c.limiter.enabled;
    document.getElementById(`${chKey}-lim-th`).value = c.limiter.threshold_db;
    document.getElementById(`${chKey}-lim-th-txt`).innerText = `${c.limiter.threshold_db.toFixed(1)} dB`;

    // PEQ
    for (let b = 0; b < 3; b++) {
      const p = c.peq[b];
      document.getElementById(`${chKey}-peq-en-${b}`).checked = p.enabled;
      document.getElementById(`${chKey}-peq-t-${b}`).value = p.type;
      document.getElementById(`${chKey}-peq-f-${b}`).value = p.freq;
      document.getElementById(`${chKey}-peq-fv-${b}`).innerText = `${Math.round(p.freq)}Hz`;
      document.getElementById(`${chKey}-peq-g-${b}`).value = p.gain_db;
      document.getElementById(`${chKey}-peq-gv-${b}`).innerText = `${p.gain_db.toFixed(1)}dB`;
      document.getElementById(`${chKey}-peq-q-${b}`).value = p.q;
      document.getElementById(`${chKey}-peq-qv-${b}`).innerText = `${p.q.toFixed(1)}`;
    }
  });
}

function sendDspUpdate() {
  clearTimeout(debounceTimer);
  debounceTimer = setTimeout(async () => {
    try {
      await fetch('/api/dsp', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(dspState)
      });
      drawGraph();
    } catch (e) {
      console.error("Error posting DSP config:", e);
    }
  }, 40);
}

// Master Listeners
document.getElementById('master-gain').addEventListener('input', (e) => {
  const v = parseFloat(e.target.value);
  document.getElementById('master-gain-num').value = v;
  document.getElementById('master-gain-txt').innerText = `${v.toFixed(1)} dB`;
  dspState.master_gain_db = v;
  sendDspUpdate();
});
document.getElementById('master-gain-num').addEventListener('input', (e) => {
  const v = parseFloat(e.target.value);
  document.getElementById('master-gain').value = v;
  document.getElementById('master-gain-txt').innerText = `${v.toFixed(1)} dB`;
  dspState.master_gain_db = v;
  sendDspUpdate();
});
document.getElementById('master-invert').addEventListener('change', (e) => {
  dspState.polarity_inverted = e.target.checked;
  sendDspUpdate();
});
document.getElementById('master-mute-btn').addEventListener('click', () => {
  dspState.mute = !dspState.mute;
  syncUiFromState();
  sendDspUpdate();
});

// X-Over Link Listeners
document.getElementById('xover-link-toggle').addEventListener('change', (e) => {
  dspState.xover_linked = e.target.checked;
  if (dspState.xover_linked) {
    applyXoverLink();
  }
  sendDspUpdate();
});
document.getElementById('xover-freq-num').addEventListener('input', (e) => {
  dspState.xover_freq = parseFloat(e.target.value) || 100;
  if (dspState.xover_linked) applyXoverLink();
  sendDspUpdate();
});
document.getElementById('xover-slope-sel').addEventListener('change', (e) => {
  dspState.xover_slope = parseInt(e.target.value);
  if (dspState.xover_linked) applyXoverLink();
  sendDspUpdate();
});

function applyXoverLink() {
  dspState.ch1.lpf.enabled = true;
  dspState.ch1.lpf.freq = dspState.xover_freq;
  dspState.ch1.lpf.slope = dspState.xover_slope;
  dspState.ch2.hpf.enabled = true;
  dspState.ch2.hpf.freq = dspState.xover_freq;
  dspState.ch2.hpf.slope = dspState.xover_slope;
  syncUiFromState();
}

// Channel 1 & 2 Hookups
['ch1', 'ch2'].forEach(chKey => {
  document.getElementById(`${chKey}-gain`).addEventListener('input', (e) => {
    const v = parseFloat(e.target.value);
    document.getElementById(`${chKey}-gain-num`).value = v;
    document.getElementById(`${chKey}-gain-txt`).innerText = `${v.toFixed(1)} dB`;
    dspState[chKey].gain_db = v;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-gain-num`).addEventListener('input', (e) => {
    const v = parseFloat(e.target.value);
    document.getElementById(`${chKey}-gain`).value = v;
    document.getElementById(`${chKey}-gain-txt`).innerText = `${v.toFixed(1)} dB`;
    dspState[chKey].gain_db = v;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-invert`).addEventListener('change', (e) => {
    dspState[chKey].polarity_inverted = e.target.checked;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-mute-btn`).addEventListener('click', () => {
    dspState[chKey].mute = !dspState[chKey].mute;
    syncUiFromState();
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-delay`).addEventListener('input', (e) => {
    const v = parseFloat(e.target.value);
    document.getElementById(`${chKey}-delay-num`).value = v;
    document.getElementById(`${chKey}-delay-txt`).innerText = `${v.toFixed(1)} ms`;
    dspState[chKey].delay_ms = v;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-delay-num`).addEventListener('input', (e) => {
    const v = parseFloat(e.target.value);
    document.getElementById(`${chKey}-delay`).value = v;
    document.getElementById(`${chKey}-delay-txt`).innerText = `${v.toFixed(1)} ms`;
    dspState[chKey].delay_ms = v;
    sendDspUpdate();
  });

  // HPF
  document.getElementById(`${chKey}-hpf-en`).addEventListener('change', (e) => {
    dspState[chKey].hpf.enabled = e.target.checked;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-hpf-sl`).addEventListener('input', (e) => {
    const f = sliderToFreq(parseFloat(e.target.value));
    document.getElementById(`${chKey}-hpf-f`).value = f;
    dspState[chKey].hpf.freq = f;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-hpf-f`).addEventListener('input', (e) => {
    const f = parseFloat(e.target.value) || 20;
    document.getElementById(`${chKey}-hpf-sl`).value = freqToSlider(f);
    dspState[chKey].hpf.freq = f;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-hpf-s`).addEventListener('change', (e) => {
    dspState[chKey].hpf.slope = parseInt(e.target.value);
    sendDspUpdate();
  });

  // LPF
  document.getElementById(`${chKey}-lpf-en`).addEventListener('change', (e) => {
    dspState[chKey].lpf.enabled = e.target.checked;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-lpf-sl`).addEventListener('input', (e) => {
    const f = sliderToFreq(parseFloat(e.target.value));
    document.getElementById(`${chKey}-lpf-f`).value = f;
    dspState[chKey].lpf.freq = f;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-lpf-f`).addEventListener('input', (e) => {
    const f = parseFloat(e.target.value) || 20;
    document.getElementById(`${chKey}-lpf-sl`).value = freqToSlider(f);
    dspState[chKey].lpf.freq = f;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-lpf-s`).addEventListener('change', (e) => {
    dspState[chKey].lpf.slope = parseInt(e.target.value);
    sendDspUpdate();
  });

  // Limiter
  document.getElementById(`${chKey}-lim-en`).addEventListener('change', (e) => {
    dspState[chKey].limiter.enabled = e.target.checked;
    sendDspUpdate();
  });
  document.getElementById(`${chKey}-lim-th`).addEventListener('input', (e) => {
    const v = parseFloat(e.target.value);
    document.getElementById(`${chKey}-lim-th-txt`).innerText = `${v.toFixed(1)} dB`;
    dspState[chKey].limiter.threshold_db = v;
    sendDspUpdate();
  });
});

function updatePeq(chKey, b) {
  const p = dspState[chKey].peq[b];
  p.enabled = document.getElementById(`${chKey}-peq-en-${b}`).checked;
  p.type = parseInt(document.getElementById(`${chKey}-peq-t-${b}`).value);
  p.freq = parseFloat(document.getElementById(`${chKey}-peq-f-${b}`).value);
  p.gain_db = parseFloat(document.getElementById(`${chKey}-peq-g-${b}`).value);
  p.q = parseFloat(document.getElementById(`${chKey}-peq-q-${b}`).value);

  document.getElementById(`${chKey}-peq-fv-${b}`).innerText = `${Math.round(p.freq)}Hz`;
  document.getElementById(`${chKey}-peq-gv-${b}`).innerText = `${p.gain_db.toFixed(1)}dB`;
  document.getElementById(`${chKey}-peq-qv-${b}`).innerText = `${p.q.toFixed(1)}`;
  sendDspUpdate();
}

// Quick Mode Helpers
function setMode2WayCrossover() {
  dspState.xover_linked = true;
  dspState.xover_freq = 100;
  dspState.xover_slope = 24;
  dspState.ch1.hpf = { enabled: true, freq: 25, slope: 24 };
  dspState.ch1.lpf = { enabled: true, freq: 100, slope: 24 };
  dspState.ch2.hpf = { enabled: true, freq: 100, slope: 24 };
  dspState.ch2.lpf = { enabled: true, freq: 18000, slope: 12 };
  syncUiFromState();
  sendDspUpdate();
}

function setModeStereoFull() {
  dspState.xover_linked = false;
  dspState.ch1.hpf = { enabled: true, freq: 20, slope: 12 };
  dspState.ch1.lpf = { enabled: false, freq: 20000, slope: 12 };
  dspState.ch2.hpf = { enabled: true, freq: 20, slope: 12 };
  dspState.ch2.lpf = { enabled: false, freq: 20000, slope: 12 };
  syncUiFromState();
  sendDspUpdate();
}

function setModeActive2Way() {
  dspState.xover_linked = false;
  dspState.ch1.hpf = { enabled: true, freq: 80, slope: 24 };
  dspState.ch1.lpf = { enabled: true, freq: 2500, slope: 24 };
  dspState.ch2.hpf = { enabled: true, freq: 2500, slope: 24 };
  dspState.ch2.lpf = { enabled: false, freq: 20000, slope: 12 };
  syncUiFromState();
  sendDspUpdate();
}

// Preset Functions
async function loadPreset(slot) {
  try {
    const res = await fetch('/api/preset/load', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ slot })
    });
    if (res.ok) {
      activePresetSlot = slot;
      document.querySelectorAll('.preset-btn').forEach((b, i) => {
        b.classList.toggle('active', i + 1 === slot);
      });
      await loadDspState();
    }
  } catch (e) { console.error("Error loading preset:", e); }
}

async function saveCurrentPreset() {
  try {
    const res = await fetch('/api/preset/save', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ slot: activePresetSlot, config: dspState })
    });
    if (res.ok) alert(`Preset ${activePresetSlot} Berhasil Disimpan ke NVS!`);
  } catch (e) { console.error("Error saving preset:", e); }
}

async function resetCurrentPreset() {
  if (confirm(`Reset Preset ${activePresetSlot} ke standar?`)) {
    try {
      await fetch('/api/preset/reset', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ slot: activePresetSlot })
      });
      await loadDspState();
    } catch (e) { console.error("Error resetting preset:", e); }
  }
}

// Real-time VU Telemetry Polling
function startMeterPolling() {
  setInterval(async () => {
    try {
      const res = await fetch('/api/meter');
      if (res.ok) {
        const m = await res.json();
        updateVuBar('in-peak-bar', 'in-peak-tick', m.in_peak, 'in-peak-val');
        updateVuBar('ch1-peak-bar', 'ch1-peak-tick', m.ch1_peak, 'ch1-peak-val');
        updateVuBar('ch2-peak-bar', 'ch2-peak-tick', m.ch2_peak, 'ch2-peak-val');

        const clipBadge = document.getElementById('clip-badge');
        if (m.clip) clipBadge.classList.add('active');
        else clipBadge.classList.remove('active');

        document.getElementById('sr-txt').innerText = `${m.sampleRate} Hz`;
        const btTxt = document.getElementById('bt-txt');
        const badgeBt = document.getElementById('badge-bt');
        btTxt.innerText = `BT: ${m.bt_state}`;
        badgeBt.className = 'badge ' + (m.bt_state === 'STREAMING' ? 'streaming' : (m.bt_state === 'CONNECTED' ? 'connected' : ''));
        document.getElementById('wifi-txt').innerText = `WiFi: ${m.ip}`;
      }
    } catch (e) {}
  }, 70);
}

function updateVuBar(barId, tickId, db, textId) {
  const pct = Math.min(100, Math.max(0, ((db + 60) / 60) * 100));
  document.getElementById(barId).style.width = `${pct}%`;
  document.getElementById(tickId).style.left = `${pct}%`;
  document.getElementById(textId).innerText = `${db.toFixed(1)} dB`;
}

// Frequency Response Graph Calculation
function calcChannelResponse(ch, f) {
  let db = ch.gain_db;
  if (ch.hpf.enabled) {
    const order = ch.hpf.slope === 48 ? 8 : (ch.hpf.slope === 24 ? 4 : 2);
    const r = f / ch.hpf.freq;
    const mag = Math.pow(r, order) / Math.sqrt(1 + Math.pow(r, 2 * order));
    db += 20 * Math.log10(mag + 1e-9);
  }
  if (ch.lpf.enabled) {
    const order = ch.lpf.slope === 48 ? 8 : (ch.lpf.slope === 24 ? 4 : 2);
    const r = f / ch.lpf.freq;
    const mag = 1 / Math.sqrt(1 + Math.pow(r, 2 * order));
    db += 20 * Math.log10(mag + 1e-9);
  }
  ch.peq.forEach(p => {
    if (p.enabled && Math.abs(p.gain_db) > 0.05) {
      if (p.type === 0) {
        const logDiff = Math.abs(Math.log2(f / p.freq));
        const bw = 1 / p.q;
        const atten = Math.exp(-Math.pow(logDiff / (0.7 * bw), 2));
        db += p.gain_db * atten;
      } else if (p.type === 1) {
        if (f < p.freq) db += p.gain_db;
        else {
          const ratio = f / p.freq;
          db += p.gain_db / (1 + Math.pow(ratio, 2 * p.q));
        }
      } else if (p.type === 2) {
        if (f > p.freq) db += p.gain_db;
        else {
          const ratio = p.freq / f;
          db += p.gain_db / (1 + Math.pow(ratio, 2 * p.q));
        }
      }
    }
  });
  return db;
}

function drawGraph() {
  const canvas = document.getElementById('eqCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);

  const w = rect.width;
  const h = rect.height;

  ctx.fillStyle = '#090d16';
  ctx.fillRect(0, 0, w, h);

  const minF = 20, maxF = 20000;
  const fTicks = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];

  ctx.strokeStyle = '#1e293b';
  ctx.lineWidth = 1;
  ctx.fillStyle = '#64748b';
  ctx.font = '10px monospace';

  fTicks.forEach(f => {
    const x = ((Math.log10(f) - Math.log10(minF)) / (Math.log10(maxF) - Math.log10(minF))) * w;
    ctx.beginPath();
    ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
    const lbl = f >= 1000 ? `${f/1000}k` : `${f}`;
    ctx.fillText(lbl, x + 3, h - 6);
  });

  const dbTicks = [-18, -12, -6, 0, 6, 12, 18];
  dbTicks.forEach(db => {
    const y = h/2 - (db / 24) * (h/2);
    ctx.beginPath();
    ctx.strokeStyle = (db === 0) ? '#334155' : '#162032';
    ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
    ctx.fillText(`${db > 0 ? '+' : ''}${db}dB`, 6, y - 4);
  });

  const pts = 200;

  // Draw CH 1 Curve
  ctx.beginPath();
  ctx.strokeStyle = '#06b6d4';
  ctx.lineWidth = 2.5;
  for (let i = 0; i <= pts; i++) {
    const normX = i / pts;
    const f = Math.pow(10, Math.log10(minF) + normX * (Math.log10(maxF) - Math.log10(minF)));
    const db = calcChannelResponse(dspState.ch1, f);
    const x = normX * w;
    const y = h/2 - (db / 24) * (h/2);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // Draw CH 2 Curve
  ctx.beginPath();
  ctx.strokeStyle = '#f59e0b';
  ctx.lineWidth = 2.5;
  for (let i = 0; i <= pts; i++) {
    const normX = i / pts;
    const f = Math.pow(10, Math.log10(minF) + normX * (Math.log10(maxF) - Math.log10(minF)));
    const db = calcChannelResponse(dspState.ch2, f);
    const x = normX * w;
    const y = h/2 - (db / 24) * (h/2);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

window.addEventListener('resize', drawGraph);
window.addEventListener('DOMContentLoaded', () => {
  initPeqUi();
  loadDspState();
  startMeterPolling();
  setTimeout(drawGraph, 200);
});
</script>

</body>
</html>
)rawliteral";
