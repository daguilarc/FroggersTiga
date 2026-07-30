## MODIFIED Requirements

**Audit 2026-06-30:** `ScopeGridComponent` renders eight cells — six EF scopes (indices 7–12) plus two Marbles LEDs (`ScopeGridComponent.cpp`). This delta **removes the scope grid band entirely**: one triple-VCO oscilloscope relocates to the transport row (standalone) or a slim top strip (VST); Marbles LEDs relocate to the performance band. Pair/sum EF scopes (indices 10–12) are removed from UI and mod assignment menus.

### Requirement: v2-single-vco-ef-oscilloscope

Desktop v2 and VST v2 SHALL expose **one** oscilloscope showing envelope followers for VCO1, VCO2, and VCO3 on a shared time axis (mod indices **7–9** only):

| Mod index | Source | Trace color |
|-----------|--------|-------------|
| 7 | VCO1 EF | `#e06c75` (red) |
| 8 | VCO2 EF | `#3fb950` (green) |
| 9 | VCO3 EF | `#58a6ff` (blue) |

The widget SHALL read `GetCvOut(7..9)` at UI refresh rate (≥15 Hz) while audio is running.

**No other scope cells** — pair/sum EF monitors (indices 10–12) SHALL NOT exist in chrome.

#### Scenario: Three traces on one scope

- **WHEN** audio is running
- **THEN** one scope widget renders red, green, and blue traces simultaneously
- **THEN** no separate scope widgets exist for indices 10–12

#### Scenario: Mod assignment lists per-VCO EF only

- **WHEN** the operator opens a row mod-source dropdown
- **THEN** **VCO1 EF**, **VCO2 EF**, and **VCO3 EF** (indices 7–9) are available
- **THEN** **VCO1+VCO2 EF**, **VCO2+VCO3 EF**, and **VCO1+VCO2+VCO3 EF** (indices 10–12) are **not** listed
- **THEN** **Random S&H 1/2** (indices 13–14) remain assignable

### Requirement: v2-scope-in-transport-row

**Standalone** (`MainComponent`): The transport row SHALL embed the VCO EF oscilloscope to the **right** of Engine / Stop / Audio / MIDI controls in the **same** horizontal band.

- Transport + scope band height: **7u** (70px) — replaces separate 3u transport + 9u scope grid + gap
- Scope flex width: remaining panel width after buttons (~**88u** at 128u)

**VST** (`HostedMainComponentV2`): No transport row. The same triple-VCO scope SHALL render in a **5u** top strip (full width) above the performance band.

`ScopeGridComponent` and `kScopeGridH` SHALL be removed.

#### Scenario: Standalone transport layout

- **WHEN** desktop v2 renders the top chrome
- **THEN** Engine / Stop / Audio / MIDI appear on the left of the 7u band
- **THEN** the VCO EF scope occupies the right flex area of that band
- **THEN** no separate scope grid section exists below transport

#### Scenario: VST top scope strip

- **WHEN** FroggersTigaPluginV2 editor opens
- **THEN** a 5u VCO EF scope strip appears above the performance band
- **THEN** no Engine / Audio / MIDI transport controls appear in that strip

### Requirement: v2-marbles-leds-in-performance-band

Random S&H level indicators (mod indices **13–14**) SHALL render as **gated green LEDs** in `PerformanceBandV2` — not in a scope grid.

- LED footprint: **2u × 2u** each, labeled **Rnd 1** / **Rnd 2** (or **S&H 1** / **S&H 2**)
- Placement: right end of performance band row, after sequencer transport controls
- Brightness: `ModLedDisplayBrightness(level, audioRunning)` per existing engine path

#### Scenario: Marbles LEDs in performance band

- **WHEN** desktop v2 or VST v2 renders the performance band
- **THEN** two Marbles LEDs appear at the right edge of the band
- **THEN** they are not waveform scopes

#### Scenario: Performance band height unchanged

- **WHEN** LEDs are added to the performance band
- **THEN** band height remains **7u** (70px) — LEDs fit inline with existing controls

### Requirement: v2-vertical-chrome-without-scope-grid

Vertical order **without** a dedicated scope grid section:

**Standalone:**

```
7u  Transport + VCO EF scope (one widget)
7u  Performance band (scenes, gestures, seq transport, Marbles LEDs)
──  Carousel flex
13u Sequencer
4u  Global strip
```

**VST:**

```
5u  VCO EF scope strip
7u  Performance band (+ Marbles LEDs)
──  Carousel flex
13u Sequencer
4u  Global strip
```

Reclaimed height (~**60px** at 920px standalone) goes to carousel flex — approximately **one additional encoder row** at 5u row height.

#### Scenario: Carousel gains height

- **WHEN** chrome lays out at 920px standalone with 5u encoder rows
- **THEN** carousel viewport fits **ten** module rows without scroll (Filter page)
- **THEN** `ScopeGridComponent` is not in the component tree
