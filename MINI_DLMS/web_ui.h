#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>S.NET AUDIO MANAGEMENT - 2-Way DLMS Processor</title>
<style>
:root {
  --bg: #f1f5f9;
  --surface: #ffffff;
  --card: #ffffff;
  --primary: #2563eb;
  --primary-hover: #1d4ed8;
  --primary-light: #dbeafe;
  --accent: #0284c7;
  --text-main: #0f172a;
  --text-muted: #64748b;
  --border: #e2e8f0;
  --danger: #ef4444;
  --danger-bg: #fee2e2;
  --warning: #f59e0b;
  --success: #10b981;
  --radius: 10px;
  --shadow: 0 4px 6px -1px rgba(0,0,0,0.06), 0 2px 4px -2px rgba(0,0,0,0.06);
}
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; }
body { background: var(--bg); color: var(--text-main); line-height: 1.5; padding-bottom: 50px; }
.header { background: linear-gradient(135deg, #1e3a8a 0%, #2563eb 100%); color: #fff; padding: 20px 24px; box-shadow: 0 4px 12px rgba(37,99,235,0.25); }
.header-content { max-width: 1300px; margin: 0 auto; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 12px; }
.brand h1 { font-size: 22px; font-weight: 800; letter-spacing: 1px; }
.brand p { font-size: 13px; color: #93c5fd; font-weight: 600; letter-spacing: 0.5px; }
.status-badges { display: flex; gap: 8px; flex-wrap: wrap; }
.badge { background: rgba(255,255,255,0.18); backdrop-filter: blur(8px); padding: 6px 12px; border-radius: 20px; font-size: 12px; font-weight: 600; display: flex; align-items: center; gap: 6px; }
.badge .dot { width: 8px; height: 8px; border-radius: 50%; background: #94a3b8; }
.badge.connected .dot { background: #34d399; box-shadow: 0 0 8px #34d399; }
.badge.streaming .dot { background: #60a5fa; box-shadow: 0 0 8px #60a5fa; animation: pulse 1.5s infinite; }
.badge.warning .dot { background: #fbbf24; }
.badge.danger .dot { background: #f87171; }
@keyframes pulse { 0% { opacity: 0.5; } 50% { opacity: 1; } 100% { opacity: 0.5; } }

.container { max-width: 1300px; margin: 24px auto; padding: 0 16px; display: grid; grid-template-columns: 320px 1fr; gap: 20px; }
@media (max-width: 980px) { .container { grid-template-columns: 1fr; } }

.card { background: var(--card); border-radius: var(--radius); padding: 18px; border: 1px solid var(--border); box-shadow: var(--shadow); margin-bottom: 20px; }
.card-title { font-size: 14px; font-weight: 700; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.8px; margin-bottom: 14px; display: flex; justify-content: space-between; align-items: center; }

/* VU Meter Styles */
.vu-panel { display: flex; flex-direction: column; gap: 14px; }
.vu-meter-group { display: flex; flex-direction: column; gap: 6px; }
.vu-label-row { display: flex; justify-content: space-between; font-size: 12px; font-weight: 700; }
.vu-val { font-family: monospace; font-size: 13px; color: var(--primary); }
.meter-container { height: 18px; background: #0f172a; border-radius: 4px; position: relative; overflow: hidden; display: flex; padding: 2px; }
.meter-bar { height: 100%; width: 0%; border-radius: 2px; background: linear-gradient(to right, #10b981 0%, #10b981 70%, #f59e0b 85%, #ef4444 100%); transition: width 0.08s ease-out; }
.meter-peak { position: absolute; top: 0; bottom: 0; width: 3px; background: #ffffff; box-shadow: 0 0 4px #fff; transition: left 0.15s ease-out; }
.meter-ticks { display: flex; justify-content: space-between; font-size: 9px; color: var(--text-muted); padding: 0 2px; font-family: monospace; }
.clip-indicator { display: inline-block; padding: 3px 8px; border-radius: 4px; font-size: 11px; font-weight: 900; background: #e2e8f0; color: #94a3b8; text-align: center; }
.clip-indicator.active { background: var(--danger); color: #fff; box-shadow: 0 0 10px rgba(239,68,68,0.7); animation: flash 0.3s alternate infinite; }
@keyframes flash { from { opacity: 1; } to { opacity: 0.6; } }

/* Controls */
.control-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 12px; gap: 10px; }
.control-label { font-size: 13px; font-weight: 600; color: var(--text-main); }
.slider-container { display: flex; align-items: center; gap: 10px; width: 100%; }
input[type="range"] { flex: 1; -webkit-appearance: none; height: 6px; border-radius: 3px; background: #cbd5e1; outline: none; }
input[type="range"]::-webkit-slider-thumb { -webkit-appearance: none; width: 18px; height: 18px; border-radius: 50%; background: var(--primary); cursor: pointer; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }
input[type="number"] { width: 72px; padding: 6px 8px; border: 1px solid var(--border); border-radius: 6px; font-size: 13px; font-weight: 600; text-align: right; }
.btn { padding: 8px 16px; border-radius: 6px; font-size: 13px; font-weight: 600; cursor: pointer; border: 1px solid transparent; transition: all 0.2s; display: inline-flex; align-items: center; justify-content: center; gap: 6px; }
.btn-primary { background: var(--primary); color: #fff; }
.btn-primary:hover { background: var(--primary-hover); }
.btn-outline { background: transparent; border-color: var(--border); color: var(--text-main); }
.btn-outline:hover { background: #f8fafc; border-color: #cbd5e1; }
.btn-danger { background: var(--danger); color: #fff; }
.btn-danger.active { background: #b91c1c; box-shadow: 0 0 10px rgba(185,28,28,0.5); }
.btn-mute { width: 100%; padding: 12px; font-size: 14px; font-weight: 700; margin-top: 6px; }

/* Switch Toggle */
.switch { position: relative; display: inline-block; width: 44px; height: 24px; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider-toggle { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #cbd5e1; transition: .3s; border-radius: 24px; }
.slider-toggle:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .3s; border-radius: 50%; }
input:checked + .slider-toggle { background-color: var(--primary); }
input:checked + .slider-toggle:before { transform: translateX(20px); }

/* Graph Canvas */
.canvas-wrapper { position: relative; width: 100%; height: 260px; background: #0f172a; border-radius: var(--radius); overflow: hidden; margin-bottom: 20px; box-shadow: inset 0 2px 8px rgba(0,0,0,0.4); }
canvas#eqCanvas { width: 100%; height: 100%; display: block; }
.graph-overlay-info { position: absolute; top: 10px; right: 14px; font-size: 11px; font-family: monospace; color: #94a3b8; background: rgba(15,23,42,0.7); padding: 4px 8px; border-radius: 4px; pointer-events: none; }

/* PEQ Grid */
.peq-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 14px; margin-top: 14px; }
.peq-card { background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 12px; }
.peq-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
.peq-title { font-size: 13px; font-weight: 700; color: var(--primary); }
.select-input { padding: 4px 8px; border: 1px solid var(--border); border-radius: 4px; font-size: 12px; font-weight: 600; background: #fff; }

/* Presets Bar */
.presets-bar { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 10px; }
.preset-btn { flex: 1; min-width: 80px; padding: 10px; border-radius: 6px; border: 1px solid var(--border); background: #f8fafc; font-weight: 700; font-size: 13px; cursor: pointer; text-align: center; transition: all 0.2s; }
.preset-btn.active { background: var(--primary-light); border-color: var(--primary); color: var(--primary); }
</style>
</head>
<body>

<div class="header">
  <div class="header-content">
    <div class="brand">
      <h1>S.NET AUDIO MANAGEMENT</h1>
      <p>2-WAY ACTIVE CROSSOVER DLMS</p>
    </div>
    <div class="status-badges">
      <div class="badge" id="badge-wifi"><span class="dot"></span> <span id="wifi-txt">WiFi: Connecting...</span></div>
      <div class="badge" id="badge-bt"><span class="dot"></span> <span id="bt-txt">BT: Disconnected</span></div>
      <div class="badge" id="badge-sr"><span id="sr-txt">44.1 kHz</span></div>
    </div>
  </div>
</div>

<div class="container">
  <!-- LEFT COLUMN: Realtime Meters & Master Section -->
  <div class="left-col">
    <!-- INPUT & OUTPUT VU METERS -->
    <div class="card">
      <div class="card-title">
        <span>Level Meters (dBFS)</span>
        <span class="clip-indicator" id="clip-badge">CLIP</span>
      </div>
      <div class="vu-panel">
        <!-- Input Peak & RMS -->
        <div class="vu-meter-group">
          <div class="vu-label-row">
            <span>INPUT PEAK</span>
            <span class="vu-val" id="in-peak-val">-60.0 dB</span>
          </div>
          <div class="meter-container">
            <div class="meter-bar" id="in-peak-bar"></div>
            <div class="meter-peak" id="in-peak-tick"></div>
          </div>
          <div class="vu-label-row" style="margin-top:2px;">
            <span style="font-size:11px;color:var(--text-muted);">INPUT RMS</span>
            <span class="vu-val" style="font-size:11px;" id="in-rms-val">-60.0 dB</span>
          </div>
          <div class="meter-container" style="height:8px;">
            <div class="meter-bar" id="in-rms-bar"></div>
          </div>
          <div class="meter-ticks"><span>-60</span><span>-36</span><span>-24</span><span>-12</span><span>-6</span><span>0</span></div>
        </div>

        <!-- SUBWOOFER Output (DAC 1 / LEFT) -->
        <div class="vu-meter-group" style="margin-top:8px;">
          <div class="vu-label-row">
            <span>SUBWOOFER OUT (DAC 1 / LEFT)</span>
            <span class="vu-val" id="sub-peak-val">-60.0 dB</span>
          </div>
          <div class="meter-container">
            <div class="meter-bar" id="sub-peak-bar"></div>
            <div class="meter-peak" id="sub-peak-tick"></div>
          </div>
          <div class="meter-ticks"><span>-60</span><span>-36</span><span>-24</span><span>-12</span><span>-6</span><span>0</span></div>
        </div>

        <!-- MID / HIGH Output (DAC 2 / RIGHT) -->
        <div class="vu-meter-group" style="margin-top:6px;">
          <div class="vu-label-row">
            <span>MID / HIGH OUT (DAC 2 / RIGHT)</span>
            <span class="vu-val" id="mid-peak-val">-60.0 dB</span>
          </div>
          <div class="meter-container">
            <div class="meter-bar" id="mid-peak-bar"></div>
            <div class="meter-peak" id="mid-peak-tick"></div>
          </div>
          <div class="meter-ticks"><span>-60</span><span>-36</span><span>-24</span><span>-12</span><span>-6</span><span>0</span></div>
        </div>

        <!-- Limiter GR -->
        <div class="vu-meter-group" style="margin-top:6px;">
          <div class="vu-label-row">
            <span>LIMITER REDUCTION</span>
            <span class="vu-val" id="gr-val">0.0 dB</span>
          </div>
          <div class="meter-container" style="height:10px;">
            <div class="meter-bar" id="gr-bar" style="background:#f59e0b;"></div>
          </div>
        </div>
      </div>
    </div>

    <!-- MASTER SECTION -->
    <div class="card">
      <div class="card-title">Master Output</div>
      <div class="control-row">
        <span class="control-label">Master Gain</span>
        <span id="gain-db-txt" style="font-weight:700;color:var(--primary);">0.0 dB</span>
      </div>
      <div class="slider-container" style="margin-bottom:14px;">
        <input type="range" id="master-gain" min="-60" max="12" step="0.5" value="0">
        <input type="number" id="master-gain-num" min="-60" max="12" step="0.5" value="0">
      </div>

      <div class="control-row" style="margin-top:10px;">
        <span class="control-label">Polarity Invert</span>
        <label class="switch">
          <input type="checkbox" id="polarity-toggle">
          <span class="slider-toggle"></span>
        </label>
      </div>

      <button class="btn btn-danger btn-mute" id="mute-btn">MUTE AUDIO</button>
    </div>

    <!-- DELAY SECTION -->
    <div class="card">
      <div class="card-title">Audio Delay</div>
      <div class="control-row">
        <span class="control-label">Delay Time</span>
        <span id="delay-ms-txt" style="font-weight:700;color:var(--primary);">0.0 ms</span>
      </div>
      <div class="slider-container">
        <input type="range" id="delay-slider" min="0" max="50" step="0.1" value="0">
        <input type="number" id="delay-num" min="0" max="50" step="0.1" value="0">
      </div>
      <div style="font-size:11px;color:var(--text-muted);margin-top:6px;" id="delay-dist-txt">
        Distance: 0.00 m / 0.00 ft
      </div>
    </div>

    <!-- PRESETS -->
    <div class="card">
      <div class="card-title">Presets (NVS Memory)</div>
      <div class="presets-bar">
        <button class="preset-btn active" onclick="loadPreset(1)">Preset 1</button>
        <button class="preset-btn" onclick="loadPreset(2)">Preset 2</button>
        <button class="preset-btn" onclick="loadPreset(3)">Preset 3</button>
        <button class="preset-btn" onclick="loadPreset(4)">Preset 4</button>
        <button class="preset-btn" onclick="loadPreset(5)">Preset 5</button>
      </div>
      <div style="display:flex;gap:8px;margin-top:14px;">
        <button class="btn btn-primary" style="flex:1;" onclick="saveCurrentPreset()">Save Active</button>
        <button class="btn btn-outline" style="flex:1;" onclick="resetCurrentPreset()">Reset</button>
      </div>
    </div>
  </div>

  <!-- RIGHT COLUMN: EQ Graph, Filters, 5-PEQ & Limiter -->
  <div class="right-col">
    <!-- GRAPH CANVAS -->
    <div class="canvas-wrapper">
      <canvas id="eqCanvas"></canvas>
      <div class="graph-overlay-info">20 Hz - 20 kHz | Frequency Response</div>
    </div>

    <!-- HPF & LPF CARD -->
    <div class="card">
      <div class="card-title">Crossover Filters (HPF / LPF)</div>
      <div style="display:grid;grid-template-columns:repeat(auto-fit, minmax(280px, 1fr));gap:20px;">
        <!-- HPF -->
        <div style="background:#f8fafc;padding:12px;border-radius:8px;border:1px solid var(--border);">
          <div class="control-row">
            <span style="font-weight:700;color:var(--primary);">High Pass Filter (HPF / Low Cut)</span>
            <label class="switch">
              <input type="checkbox" id="hpf-enable">
              <span class="slider-toggle"></span>
            </label>
          </div>
          <div class="control-row">
            <span class="control-label">Cutoff Freq:</span>
            <input type="number" id="hpf-freq" min="20" max="20000" step="1" value="20" style="width:90px;font-weight:700;">
          </div>
          <input type="range" id="hpf-slider" min="0" max="1000" step="1" value="0" style="width:100%;margin-bottom:8px;">
          <div class="control-row">
            <span class="control-label">Slope:</span>
            <select id="hpf-slope" class="select-input">
              <option value="12">12 dB/oct (Butterworth)</option>
              <option value="24">24 dB/oct (Linkwitz-Riley)</option>
              <option value="48">48 dB/oct (Extreme 8th-Order)</option>
            </select>
          </div>
        </div>

        <!-- LPF -->
        <div style="background:#f8fafc;padding:12px;border-radius:8px;border:1px solid var(--border);">
          <div class="control-row">
            <span style="font-weight:700;color:var(--primary);">Low Pass Filter (LPF / High Cut)</span>
            <label class="switch">
              <input type="checkbox" id="lpf-enable">
              <span class="slider-toggle"></span>
            </label>
          </div>
          <div class="control-row">
            <span class="control-label">Cutoff Freq:</span>
            <input type="number" id="lpf-freq" min="20" max="20000" step="1" value="20000" style="width:90px;font-weight:700;">
          </div>
          <input type="range" id="lpf-slider" min="0" max="1000" step="1" value="1000" style="width:100%;margin-bottom:8px;">
          <div class="control-row">
            <span class="control-label">Slope:</span>
            <select id="lpf-slope" class="select-input">
              <option value="12">12 dB/oct (Butterworth)</option>
              <option value="24">24 dB/oct (Linkwitz-Riley)</option>
              <option value="48">48 dB/oct (Extreme 8th-Order - Cut Vokal Total)</option>
            </select>
          </div>
        </div>
      </div>

      <!-- Quick Crossover Presets -->
      <div style="margin-top:14px;padding-top:12px;border-top:1px dashed var(--border);display:flex;gap:8px;flex-wrap:wrap;align-items:center;">
        <span style="font-size:12px;font-weight:700;color:var(--text-muted);">Mode Cepat Crossover:</span>
        <button class="btn btn-secondary" style="padding:4px 10px;font-size:12px;background:#e0f2fe;color:#0369a1;border:1px solid #bae6fd;" onclick="applyXoverPreset(25, 80, 24)">Sub 80Hz (Cut Vokal)</button>
        <button class="btn btn-secondary" style="padding:4px 10px;font-size:12px;background:#e0f2fe;color:#0369a1;border:1px solid #bae6fd;" onclick="applyXoverPreset(30, 100, 24)">Sub 100Hz (Standar)</button>
        <button class="btn btn-secondary" style="padding:4px 10px;font-size:12px;background:#e0f2fe;color:#0369a1;border:1px solid #bae6fd;" onclick="applyXoverPreset(35, 120, 24)">Sub 120Hz (Punch Bass)</button>
        <button class="btn btn-secondary" style="padding:4px 10px;font-size:12px;background:#fae8ff;color:#86198f;border:1px solid #f5d0fe;" onclick="applyXoverPreset(25, 80, 48)">Sub 80Hz (Ekstrem 48dB)</button>
        <button class="btn btn-secondary" style="padding:4px 10px;font-size:12px;" onclick="applyXoverPreset(120, 4000, 24)">Mid / Vokal (120Hz-4kHz)</button>
        <button class="btn btn-secondary" style="padding:4px 10px;font-size:12px;" onclick="applyXoverPreset(0, 0, 24)">Full Range (Bypass)</button>
      </div>
    </div>

    <!-- 5-BAND PARAMETRIC EQ -->
    <div class="card">
      <div class="card-title">5-Band Parametric Equalizer</div>
      <div class="peq-grid" id="peq-container">
        <!-- Generated by JS for 5 bands -->
      </div>
    </div>

    <!-- LIMITER SECTION -->
    <div class="card">
      <div class="card-title">Peak Limiter & Output Protection</div>
      <div style="display:grid;grid-template-columns:repeat(auto-fit, minmax(200px, 1fr));gap:16px;">
        <div style="background:#f8fafc;padding:12px;border-radius:8px;border:1px solid var(--border);">
          <div class="control-row">
            <span class="control-label">Enable Limiter</span>
            <label class="switch">
              <input type="checkbox" id="limiter-enable" checked>
              <span class="slider-toggle"></span>
            </label>
          </div>
          <div class="control-row">
            <span class="control-label">Threshold:</span>
            <span id="lim-thresh-txt" style="font-weight:700;">-1.0 dB</span>
          </div>
          <input type="range" id="limiter-thresh" min="-30" max="0" step="0.5" value="-1" style="width:100%;">
        </div>

        <div style="background:#f8fafc;padding:12px;border-radius:8px;border:1px solid var(--border);">
          <div class="control-row">
            <span class="control-label">Attack Time:</span>
            <span id="lim-att-txt" style="font-weight:700;">5.0 ms</span>
          </div>
          <input type="range" id="limiter-att" min="0.1" max="50" step="0.5" value="5" style="width:100%;">
          <div class="control-row" style="margin-top:10px;">
            <span class="control-label">Release Time:</span>
            <span id="lim-rel-txt" style="font-weight:700;">100 ms</span>
          </div>
          <input type="range" id="limiter-rel" min="10" max="1000" step="10" value="100" style="width:100%;">
        </div>
      </div>
    </div>
  </div>
</div>

<script>
// State Object
let dspState = {
  master_gain_db: 0,
  mute: false,
  polarity_inverted: false,
  hpf: { enabled: false, freq: 20, slope: 12 },
  lpf: { enabled: false, freq: 20000, slope: 12 },
  peq: [
    { enabled: true, type: 1, freq: 80, gain_db: 0, q: 1.0 },
    { enabled: true, type: 0, freq: 250, gain_db: 0, q: 1.0 },
    { enabled: true, type: 0, freq: 1000, gain_db: 0, q: 1.0 },
    { enabled: true, type: 0, freq: 4000, gain_db: 0, q: 1.0 },
    { enabled: true, type: 2, freq: 12000, gain_db: 0, q: 1.0 }
  ],
  delay: { delay_ms: 0 },
  limiter: { enabled: true, threshold_db: -1, attack_ms: 5, release_ms: 100 }
};

let activePresetSlot = 1;
let debounceTimer = null;

// Initialize 5-band PEQ UI Cards
function initPeqUi() {
  const container = document.getElementById('peq-container');
  container.innerHTML = '';
  for (let i = 0; i < 5; i++) {
    const card = document.createElement('div');
    card.className = 'peq-card';
    card.innerHTML = `
      <div class="peq-header">
        <span class="peq-title">PEQ ${i + 1}</span>
        <label class="switch">
          <input type="checkbox" id="peq-en-${i}" onchange="updatePeq(${i})">
          <span class="slider-toggle"></span>
        </label>
      </div>
      <div style="margin-bottom:8px;">
        <select id="peq-type-${i}" class="select-input" style="width:100%;" onchange="updatePeq(${i})">
          <option value="0">Peaking (Bell)</option>
          <option value="1">Low Shelf</option>
          <option value="2">High Shelf</option>
        </select>
      </div>
      <div style="font-size:11px;font-weight:600;display:flex;justify-content:space-between;margin-top:4px;">
        <span>Freq</span><span id="peq-f-val-${i}">1000 Hz</span>
      </div>
      <input type="range" id="peq-f-${i}" min="20" max="20000" step="5" oninput="updatePeq(${i})" style="width:100%;">
      <div style="font-size:11px;font-weight:600;display:flex;justify-content:space-between;margin-top:4px;">
        <span>Gain</span><span id="peq-g-val-${i}">0 dB</span>
      </div>
      <input type="range" id="peq-g-${i}" min="-12" max="12" step="0.5" oninput="updatePeq(${i})" style="width:100%;">
      <div style="font-size:11px;font-weight:600;display:flex;justify-content:space-between;margin-top:4px;">
        <span>Q factor</span><span id="peq-q-val-${i}">1.0</span>
      </div>
      <input type="range" id="peq-q-${i}" min="0.1" max="10" step="0.1" oninput="updatePeq(${i})" style="width:100%;">
    `;
    container.appendChild(card);
  }
}

// Fetch Full DSP state from ESP32
async function loadDspState() {
  try {
    const res = await fetch('/api/dsp');
    if (res.ok) {
      dspState = await res.json();
      syncUiFromState();
      drawGraph();
    }
  } catch (e) {
    console.warn("Could not fetch /api/dsp:", e);
  }
}

// Sync UI controls from current state
function syncUiFromState() {
  document.getElementById('master-gain').value = dspState.master_gain_db;
  document.getElementById('master-gain-num').value = dspState.master_gain_db;
  document.getElementById('gain-db-txt').innerText = `${dspState.master_gain_db.toFixed(1)} dB`;

  const muteBtn = document.getElementById('mute-btn');
  if (dspState.mute) {
    muteBtn.classList.add('active');
    muteBtn.innerText = 'AUDIO MUTED';
  } else {
    muteBtn.classList.remove('active');
    muteBtn.innerText = 'MUTE AUDIO';
  }

  document.getElementById('polarity-toggle').checked = dspState.polarity_inverted;

  // HPF
  document.getElementById('hpf-enable').checked = dspState.hpf.enabled;
  document.getElementById('hpf-freq').value = Math.round(dspState.hpf.freq);
  document.getElementById('hpf-slider').value = freqToSlider(dspState.hpf.freq);
  document.getElementById('hpf-slope').value = dspState.hpf.slope;

  // LPF
  document.getElementById('lpf-enable').checked = dspState.lpf.enabled;
  document.getElementById('lpf-freq').value = Math.round(dspState.lpf.freq);
  document.getElementById('lpf-slider').value = freqToSlider(dspState.lpf.freq);
  document.getElementById('lpf-slope').value = dspState.lpf.slope;

  // PEQ
  for (let i = 0; i < 5; i++) {
    const p = dspState.peq[i];
    document.getElementById(`peq-en-${i}`).checked = p.enabled;
    document.getElementById(`peq-type-${i}`).value = p.type;
    document.getElementById(`peq-f-${i}`).value = p.freq;
    document.getElementById(`peq-f-val-${i}`).innerText = `${Math.round(p.freq)} Hz`;
    document.getElementById(`peq-g-${i}`).value = p.gain_db;
    document.getElementById(`peq-g-val-${i}`).innerText = `${p.gain_db.toFixed(1)} dB`;
    document.getElementById(`peq-q-${i}`).value = p.q;
    document.getElementById(`peq-q-val-${i}`).innerText = `${p.q.toFixed(1)}`;
  }

  // Delay
  document.getElementById('delay-slider').value = dspState.delay.delay_ms;
  document.getElementById('delay-num').value = dspState.delay.delay_ms;
  document.getElementById('delay-ms-txt').innerText = `${dspState.delay.delay_ms.toFixed(1)} ms`;
  updateDelayDistText(dspState.delay.delay_ms);

  // Limiter
  document.getElementById('limiter-enable').checked = dspState.limiter.enabled;
  document.getElementById('limiter-thresh').value = dspState.limiter.threshold_db;
  document.getElementById('lim-thresh-txt').innerText = `${dspState.limiter.threshold_db.toFixed(1)} dB`;
  document.getElementById('limiter-att').value = dspState.limiter.attack_ms;
  document.getElementById('lim-att-txt').innerText = `${dspState.limiter.attack_ms.toFixed(1)} ms`;
  document.getElementById('limiter-rel').value = dspState.limiter.release_ms;
  document.getElementById('lim-rel-txt').innerText = `${Math.round(dspState.limiter.release_ms)} ms`;
}

// Distance conversion helper
function updateDelayDistText(ms) {
  const m = (ms * 0.343).toFixed(2);
  const ft = (ms * 1.125).toFixed(2);
  document.getElementById('delay-dist-txt').innerText = `Distance: ${m} m / ${ft} ft`;
}

// Send DSP state to ESP32 with debouncing
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

// Event Listeners for UI
document.getElementById('master-gain').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value);
  document.getElementById('master-gain-num').value = val;
  document.getElementById('gain-db-txt').innerText = `${val.toFixed(1)} dB`;
  dspState.master_gain_db = val;
  sendDspUpdate();
});

document.getElementById('master-gain-num').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value);
  document.getElementById('master-gain').value = val;
  document.getElementById('gain-db-txt').innerText = `${val.toFixed(1)} dB`;
  dspState.master_gain_db = val;
  sendDspUpdate();
});

document.getElementById('mute-btn').addEventListener('click', () => {
  dspState.mute = !dspState.mute;
  syncUiFromState();
  sendDspUpdate();
});

document.getElementById('polarity-toggle').addEventListener('change', (e) => {
  dspState.polarity_inverted = e.target.checked;
  sendDspUpdate();
});

// Logarithmic slider conversion helpers (20 Hz - 20000 Hz)
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

// Quick Crossover Presets
function applyXoverPreset(hpfFreq, lpfFreq, slope) {
  if (hpfFreq <= 0 && lpfFreq <= 0) {
    dspState.hpf.enabled = false;
    dspState.lpf.enabled = false;
  } else {
    if (hpfFreq > 0) {
      dspState.hpf.enabled = true;
      dspState.hpf.freq = hpfFreq;
      dspState.hpf.slope = slope;
    } else {
      dspState.hpf.enabled = false;
    }
    if (lpfFreq > 0) {
      dspState.lpf.enabled = true;
      dspState.lpf.freq = lpfFreq;
      dspState.lpf.slope = slope;
    } else {
      dspState.lpf.enabled = false;
    }
  }
  syncUiFromState();
  sendDspUpdate();
}

// HPF Listeners
document.getElementById('hpf-enable').addEventListener('change', (e) => {
  dspState.hpf.enabled = e.target.checked;
  sendDspUpdate();
});
document.getElementById('hpf-slider').addEventListener('input', (e) => {
  const val = sliderToFreq(parseFloat(e.target.value));
  document.getElementById('hpf-freq').value = val;
  dspState.hpf.freq = val;
  sendDspUpdate();
});
document.getElementById('hpf-freq').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value) || 20;
  document.getElementById('hpf-slider').value = freqToSlider(val);
  dspState.hpf.freq = val;
  sendDspUpdate();
});
document.getElementById('hpf-slope').addEventListener('change', (e) => {
  dspState.hpf.slope = parseInt(e.target.value);
  sendDspUpdate();
});

// LPF Listeners
document.getElementById('lpf-enable').addEventListener('change', (e) => {
  dspState.lpf.enabled = e.target.checked;
  sendDspUpdate();
});
document.getElementById('lpf-slider').addEventListener('input', (e) => {
  const val = sliderToFreq(parseFloat(e.target.value));
  document.getElementById('lpf-freq').value = val;
  dspState.lpf.freq = val;
  sendDspUpdate();
});
document.getElementById('lpf-freq').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value) || 20;
  document.getElementById('lpf-slider').value = freqToSlider(val);
  dspState.lpf.freq = val;
  sendDspUpdate();
});
document.getElementById('lpf-slope').addEventListener('change', (e) => {
  dspState.lpf.slope = parseInt(e.target.value);
  sendDspUpdate();
});

// PEQ Update
function updatePeq(idx) {
  dspState.peq[idx].enabled = document.getElementById(`peq-en-${idx}`).checked;
  dspState.peq[idx].type = parseInt(document.getElementById(`peq-type-${idx}`).value);
  dspState.peq[idx].freq = parseFloat(document.getElementById(`peq-f-${idx}`).value);
  dspState.peq[idx].gain_db = parseFloat(document.getElementById(`peq-g-${idx}`).value);
  dspState.peq[idx].q = parseFloat(document.getElementById(`peq-q-${idx}`).value);

  document.getElementById(`peq-f-val-${idx}`).innerText = `${Math.round(dspState.peq[idx].freq)} Hz`;
  document.getElementById(`peq-g-val-${idx}`).innerText = `${dspState.peq[idx].gain_db.toFixed(1)} dB`;
  document.getElementById(`peq-q-val-${idx}`).innerText = `${dspState.peq[idx].q.toFixed(1)}`;

  sendDspUpdate();
}

// Delay Listeners
document.getElementById('delay-slider').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value);
  document.getElementById('delay-num').value = val;
  document.getElementById('delay-ms-txt').innerText = `${val.toFixed(1)} ms`;
  updateDelayDistText(val);
  dspState.delay.delay_ms = val;
  sendDspUpdate();
});
document.getElementById('delay-num').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value);
  document.getElementById('delay-slider').value = val;
  document.getElementById('delay-ms-txt').innerText = `${val.toFixed(1)} ms`;
  updateDelayDistText(val);
  dspState.delay.delay_ms = val;
  sendDspUpdate();
});

// Limiter Listeners
document.getElementById('limiter-enable').addEventListener('change', (e) => {
  dspState.limiter.enabled = e.target.checked;
  sendDspUpdate();
});
document.getElementById('limiter-thresh').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value);
  document.getElementById('lim-thresh-txt').innerText = `${val.toFixed(1)} dB`;
  dspState.limiter.threshold_db = val;
  sendDspUpdate();
});
document.getElementById('limiter-att').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value);
  document.getElementById('lim-att-txt').innerText = `${val.toFixed(1)} ms`;
  dspState.limiter.attack_ms = val;
  sendDspUpdate();
});
document.getElementById('limiter-rel').addEventListener('input', (e) => {
  const val = parseFloat(e.target.value);
  document.getElementById('lim-rel-txt').innerText = `${Math.round(val)} ms`;
  dspState.limiter.release_ms = val;
  sendDspUpdate();
});

// Presets Functions
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
    if (res.ok) alert(`Preset ${activePresetSlot} Saved!`);
  } catch (e) { console.error("Error saving preset:", e); }
}

async function resetCurrentPreset() {
  if (confirm(`Reset Preset ${activePresetSlot} to defaults?`)) {
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

// Real-time Meter Telemetry Loop (WebSocket or fast polling)
function startMeterPolling() {
  setInterval(async () => {
    try {
      const res = await fetch('/api/meter');
      if (res.ok) {
        const m = await res.json();
        // Input Meter
        updateVuBar('in-peak-bar', 'in-peak-tick', m.in_peak, 'in-peak-val');
        updateRmsBar('in-rms-bar', m.in_rms, 'in-rms-val');

        // 2-Way Output Meters
        const subVal = (m.sub_peak !== undefined) ? m.sub_peak : m.out_peak;
        const midVal = (m.mid_peak !== undefined) ? m.mid_peak : m.out_peak;
        updateVuBar('sub-peak-bar', 'sub-peak-tick', subVal, 'sub-peak-val');
        updateVuBar('mid-peak-bar', 'mid-peak-tick', midVal, 'mid-peak-val');

        // Clip
        const clipBadge = document.getElementById('clip-badge');
        if (m.clip) {
          clipBadge.classList.add('active');
        } else {
          clipBadge.classList.remove('active');
        }

        // Limiter GR
        const grPct = Math.min(100, Math.max(0, (-m.limiter_gr / 24) * 100));
        document.getElementById('gr-bar').style.width = `${grPct}%`;
        document.getElementById('gr-val').innerText = `${m.limiter_gr.toFixed(1)} dB`;

        // Update System Status
        document.getElementById('sr-txt').innerText = `${m.sampleRate} Hz`;
        const btTxt = document.getElementById('bt-txt');
        const badgeBt = document.getElementById('badge-bt');
        btTxt.innerText = `BT: ${m.bt_state}`;
        badgeBt.className = 'badge ' + (m.bt_state === 'STREAMING' ? 'streaming' : (m.bt_state === 'CONNECTED' ? 'connected' : (m.bt_state === 'CONNECTING' ? 'warning' : '')));

        const wifiTxt = document.getElementById('wifi-txt');
        const badgeWifi = document.getElementById('badge-wifi');
        wifiTxt.innerText = `WiFi: ${m.ip}`;
        badgeWifi.className = 'badge connected';
      }
    } catch (e) {
      // transient network poll fail
    }
  }, 70); // 70ms = ~14 updates per second for smooth VU meter ballistics
}

function updateVuBar(barId, tickId, db, textId) {
  const pct = Math.min(100, Math.max(0, ((db + 60) / 60) * 100));
  document.getElementById(barId).style.width = `${pct}%`;
  document.getElementById(tickId).style.left = `${pct}%`;
  document.getElementById(textId).innerText = `${db.toFixed(1)} dB`;
}

function updateRmsBar(barId, db, textId) {
  const pct = Math.min(100, Math.max(0, ((db + 60) / 60) * 100));
  document.getElementById(barId).style.width = `${pct}%`;
  document.getElementById(textId).innerText = `${db.toFixed(1)} dB`;
}

// Frequency Response Graph Drawing
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

  // Background
  ctx.fillStyle = '#0f172a';
  ctx.fillRect(0, 0, w, h);

  // Draw Grid Lines (Logarithmic Frequency: 20 Hz to 20,000 Hz)
  const minF = 20;
  const maxF = 20000;
  const fTicks = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];

  ctx.strokeStyle = '#1e293b';
  ctx.lineWidth = 1;
  ctx.fillStyle = '#64748b';
  ctx.font = '10px monospace';

  fTicks.forEach(f => {
    const x = ((Math.log10(f) - Math.log10(minF)) / (Math.log10(maxF) - Math.log10(minF))) * w;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, h);
    ctx.stroke();
    const lbl = f >= 1000 ? `${f/1000}k` : `${f}`;
    ctx.fillText(lbl, x + 3, h - 8);
  });

  // dB Grid Lines (-24 dB to +24 dB)
  const dbTicks = [-18, -12, -6, 0, 6, 12, 18];
  dbTicks.forEach(db => {
    const y = h/2 - (db / 24) * (h/2);
    ctx.beginPath();
    ctx.strokeStyle = (db === 0) ? '#334155' : '#1e293b';
    ctx.moveTo(0, y);
    ctx.lineTo(w, y);
    ctx.stroke();
    ctx.fillText(`${db > 0 ? '+' : ''}${db}dB`, 6, y - 4);
  });

  // Calculate Cumulative Frequency Response curve
  ctx.beginPath();
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 2.5;

  const fs = 44100;
  const pts = 200;
  for (let i = 0; i <= pts; i++) {
    const normX = i / pts;
    const f = Math.pow(10, Math.log10(minF) + normX * (Math.log10(maxF) - Math.log10(minF)));
    
    // Total response in dB
    let totDb = 0;

    // HPF
    if (dspState.hpf.enabled) {
      const order = dspState.hpf.slope === 48 ? 8 : (dspState.hpf.slope === 24 ? 4 : 2);
      const ratio = f / dspState.hpf.freq;
      const mag = Math.pow(ratio, order) / Math.sqrt(1 + Math.pow(ratio, 2 * order));
      totDb += 20 * Math.log10(mag + 1e-9);
    }

    // LPF
    if (dspState.lpf.enabled) {
      const order = dspState.lpf.slope === 48 ? 8 : (dspState.lpf.slope === 24 ? 4 : 2);
      const ratio = f / dspState.lpf.freq;
      const mag = 1 / Math.sqrt(1 + Math.pow(ratio, 2 * order));
      totDb += 20 * Math.log10(mag + 1e-9);
    }

    // 5-band PEQ
    dspState.peq.forEach(p => {
      if (p.enabled && Math.abs(p.gain_db) > 0.05) {
        if (p.type === 0) { // Peaking
          const logDiff = Math.abs(Math.log2(f / p.freq));
          const bw = 1 / p.q;
          const atten = Math.exp(-Math.pow(logDiff / (0.7 * bw), 2));
          totDb += p.gain_db * atten;
        } else if (p.type === 1) { // Low Shelf
          if (f < p.freq) totDb += p.gain_db;
          else {
            const ratio = f / p.freq;
            totDb += p.gain_db / (1 + Math.pow(ratio, 2 * p.q));
          }
        } else if (p.type === 2) { // High Shelf
          if (f > p.freq) totDb += p.gain_db;
          else {
            const ratio = p.freq / f;
            totDb += p.gain_db / (1 + Math.pow(ratio, 2 * p.q));
          }
        }
      }
    });

    const x = normX * w;
    const y = h/2 - (totDb / 24) * (h/2);

    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

window.addEventListener('resize', drawGraph);

// Bootstrap
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
