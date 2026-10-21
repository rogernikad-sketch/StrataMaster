# StrataMaster Component Specifications
Generated via `replica-design`

## 1. Top Header Bar
- **Dimensions**: Full width, height 44 px.
- **Background**: `surface` (`#141923`), bottom border `border` (`#2A3447`).
- **Elements**:
  - `BrandWordmark`: "STRATA MASTER" tracked uppercase 14 px bold in `accent` (`#00E5FF`).
  - `PresetSelector`: Flat combo box, width 180 px, displaying active preset, with Previous/Next preset step buttons.
  - `MasterAssistantButton`: Pill button, height 28 px, text "MASTER ASSISTANT", highlighted with `accent-alt` (`#FFB300`).
  - `AuditionABButton`: Toggle button "TRACK A / TRACK B" with reference mode indicator.
  - `GainMatchToggle`: "GAIN MATCH" auto-level compensation switch.
  - `GlobalBypassToggle`: Red/White power symbol, bypasses full mastering chain.

---

## 2. Module Rack Bar (The Module Navigation Strip)
- **Dimensions**: Full width, height 38 px.
- **Background**: `bg` (`#0B0E14`), subtle bottom border.
- **Items**:
  - `ModuleRackTab` for each processing module:
    - `Equalizer` (`EQ`)
    - `Dynamics` (`DYN`)
    - `Exciter` (`EXC`)
    - `Imager` (`IMG`)
    - `Maximizer` (`MAX`)
    - `Assistant` (`AI`)
  - **Tab States**:
    - Default: `surface` (`#141923`), text `text-muted` (`#8A99AD`).
    - Active / Selected: `surface-raised` (`#1E2533`), top accent highlight bar (2 px) in `accent` (`#00E5FF`), text `text` (`#F0F4F8`).
    - Hover: Lightened border.
  - **Module Power Button**: Small circular toggle (12 px) next to module name allowing instant single-module bypass directly from the rack.

---

## 3. Active Module Workspace Views
- **Dimensions**: Dynamic bounds (occupies center area, ~800 x 480 px).

### View S02: Equalizer
- **Interactive EQ Curve & Spectrum**:
  - Combined 4096-pt FFT RTA visualizer with 8-band interactive node handles.
  - Node Handles: Draggable for Frequency (X-axis, logarithmic 20 Hz - 20 kHz) and Gain (Y-axis, ±18 dB). Mouse wheel adjusts Q factor (0.1 to 18.0).
  - Selected Band Info Bar: Band number (1..8), Filter Type (High-Pass, Low-Shelf, Bell, High-Shelf, Low-Pass), Freq (Hz), Gain (dB), Q, Dynamic Toggle, Dynamic Threshold (dB), Dynamic Range (dB).
  - Processing Mode Selector: Stereo vs Mid/Side.

### View S03: Multiband Dynamics
- **4-Band Crossover Visualizer**:
  - Spectrum background with 3 vertical draggable crossover split lines (Split 1: 20-500 Hz, Split 2: 200-5000 Hz, Split 3: 1000-20000 Hz).
  - 4 Band Tabs: Low, Low-Mid, High-Mid, High.
- **Band Dynamics Control Panel**:
  - Threshold Slider (-40 to 0 dB) with real-time gain reduction meter.
  - Ratio Slider (1.0:1 to 20.0:1).
  - Attack Knob (1 to 200 ms).
  - Release Knob (10 to 1000 ms).
  - Makeup Gain Slider (-12 to +12 dB) and Auto-Makeup toggle.
  - Band Solo and Mute toggles.

### View S04: Harmonic Exciter
- **4-Band Crossover Spectrum**:
  - Band tabs (Low, Low-Mid, High-Mid, High).
- **Exciter Controls**:
  - Mode Selector: `Tape` (3rd harmonic punch), `Tube` (2nd harmonic warmth), `Warm` (subtle even-order density), `Retro` (transistor grit).
  - Drive Slider: 0 to 100%.
  - Mix Slider: 0 to 100% dry/wet.
  - Global Oversampling Selector: None (1x), 2x, 4x, 8x with latency compensation.

### View S05: Stereo Imager
- **4-Band Width Control**:
  - Width Sliders per band: 0% (Mono) to 200% (Super-wide), default 100%.
  - Haas Stereoizer: Toggle button + Delay / Width depth slider.
  - Mono-Maker Bass Anchor: Enable toggle + Cutoff frequency slider (20 Hz - 400 Hz) with slope display.
  - Real-time Phase Correlation meter (-1.0 to +1.0) with out-of-phase alerts.

### View S06: Maximizer (True-Peak Limiter)
- **Limiting Controls**:
  - Limiter Ceiling Fader: -12.0 to 0.0 dBTP (Default -1.0 dBTP).
  - Threshold Fader: -24.0 to 0.0 dB.
  - Release Time Slider: 10 to 1000 ms.
  - Lookahead Protection Toggle: 3.0 ms lookahead.
  - Pre-Limiter Soft Clipper: Enable toggle, Soft Knee slider (0 to 12 dB), Clipping Ceiling.
  - Real-time Gain Reduction Meter & History Trace.

### View S07: Master Assistant
- **Target Profiles**:
  - Streaming (-14.0 LUFS, -1.0 dBTP, balanced spectrum).
  - CD / Club (-9.0 LUFS, -0.3 dBTP, tight sub-bass).
  - High Dynamics / Classical (-18.0 LUFS, -1.5 dBTP, natural crest factor).
  - Reference Match (matches imported audio track).
- **Listening & Decision UI**:
  - "Start Listening" button with 5-second countdown circular progress bar.
  - Spectral Match Curve & Centroid difference plot.
  - "Apply Master" button which applies calculated EQ, Dynamics, Width, and Limiter settings to APVTS.
  - "Re-Analyze" button for continuous optimization.

---

## 4. Master I/O & Loudness Metering Strip
- **Dimensions**: Width 140 px, docked on right side.
- **Elements**:
  - `InputMeter`: Dual stereo peak bar (-60 to +6 dB) + Input Gain knob (±18 dB).
  - `BS1770MeterBar`:
    - Momentary LUFS bar (-48 to 0 LUFS).
    - Short-Term LUFS bar (-48 to 0 LUFS).
    - Integrated LUFS numeric readout + bar.
    - Loudness Range (LRA) numeric readout.
    - True Peak dBTP readout with red clip hold.
  - `CorrelationMeter`: Mini horizontal or vertical phase correlation indicator.
  - `OutputMeter`: Dual stereo peak bar + Output Gain knob (±18 dB).
