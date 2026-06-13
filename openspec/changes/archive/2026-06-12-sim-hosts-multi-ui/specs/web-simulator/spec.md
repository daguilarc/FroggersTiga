## ADDED Requirements

### Requirement: Paged Field-ish UI

The web simulator SHALL present eight vertical knobs, SW1/SW2 page buttons, and an eight-row OLED mock. It SHALL NOT use the desktop multi-panel layout.

#### Scenario: Page cycle via SW2

- **WHEN** the user presses SW2 repeatedly from Audio page
- **THEN** page labels cycle Audio → Marbles → Reverb → Filter → Drive → Audio

### Requirement: WASM owns host logic

The browser TypeScript layer SHALL send transport events only (`setKnob`, `pageNext`, `pagePrev`, `marbles`, mod assign). Page and mod semantics SHALL execute in C++ `PagedHostIO` inside WASM.

#### Scenario: No duplicate page logic in TS

- **WHEN** reviewing `web/src` for calls to page-index arithmetic
- **THEN** no TS code implements `StopModTracking` or page wrap logic

### Requirement: AudioWorklet processing

Audio I/O SHALL use Web Audio `AudioWorklet` running WASM `ProcessBlock` at the context sample rate. The engine sample rate SHALL be set to `audioContext.sampleRate` on start.

#### Scenario: Start without external audio

- **WHEN** the user clicks Play with **External: Off** (default)
- **THEN** processed VCO output is audible, no `getUserMedia` prompt runs, and external input to `ProcessBlock` is zero

#### Scenario: External enables ring mod

- **WHEN** the user turns **External** on, grants mic access, and audio is running
- **THEN** mic feeds the external input path and processed output reflects ring-mod character without NaN

### Requirement: GitHub Pages deployment

CI SHALL build WASM + Vite and publish static assets to `docs/` on `main` for GitHub Pages.

#### Scenario: Pages artifact exists

- **WHEN** the Pages workflow succeeds on `main`
- **THEN** `docs/index.html` and `docs/froggers.wasm` exist in the repo

### Requirement: Transport Play and Stop

The web simulator SHALL provide **Play** (green) and **Stop** (red). Stop SHALL halt audio output and tear down the AudioContext/worklet.

#### Scenario: Stop after Play

- **WHEN** the user presses Stop after Play
- **THEN** output is silent and Play is enabled again

### Requirement: External audio toggle

The web simulator SHALL expose **External: Off | On** (renamed from Mic in v2). Default SHALL be **Off**. When off, the AudioWorklet SHALL pass zero for external input samples.

#### Scenario: External off avoids permission prompt on Play

- **WHEN** the user clicks Play with External off
- **THEN** the browser does not request microphone permission

### Requirement: Internal mod meters and dropdown assignment

Web v2.1 SHALL show meters for **three internal** mod sources (VCO feat, Marbles 1, Marbles 2) and per-knob mod dropdowns on the current page: `None | VCO feat | Marbles 1 | Marbles 2`. Each dropdown SHALL sit **below** its vertical knob slider in the knob column, not beside the parameter name on the OLED row. No MIDI mod in browser v2.1. Assignment SHALL use dropdowns (not patch cables). Depth SHALL be set via the knob when a source is selected. One source MAY assign to many knobs (independent depths).

#### Scenario: Mod dropdown on knob row

- **WHEN** the user selects VCO feat on knob 2's mod dropdown
- **THEN** WASM stores mod source index 4 for that row on the current page

#### Scenario: No M1–M7 labels on web

- **WHEN** the web mod UI is rendered
- **THEN** sources are named VCO feat / Marbles 1 / Marbles 2 — not M5/M6/M7 hardware codes

### Requirement: Global strip vocabulary

Web SHALL expose **Randomize all**, **Randomize mod**, **Marbles**, and **Randomize waves** with the same names as desktop.

#### Scenario: Marbles button label

- **WHEN** the web UI is loaded
- **THEN** a button labeled "Marbles" is visible (not "m" key only)

### Requirement: Sim-only VCO morph controls

The web simulator SHALL show wave glyph buttons beside `V1VO`/`V2VO`/`V3VO` on the Audio page OLED rows; click cycles morph. No separate A′/B′ morph strip required in v2.

#### Scenario: Wave glyph cycles on Audio page

- **WHEN** the user is on the Audio page and clicks the V1VO wave button
- **THEN** VCO1 morph advances via WASM

#### Scenario: Morph not in DaisyIO

- **WHEN** inspecting `DaisyIO.hpp` after implementation
- **THEN** no VCO morph knob or CV routing exists on device
