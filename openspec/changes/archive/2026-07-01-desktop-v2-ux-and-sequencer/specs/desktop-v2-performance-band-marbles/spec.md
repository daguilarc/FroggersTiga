## ADDED Requirements

**Merged from scope-grid relocation (`desktop-v2-scope-visualization`).** Marbles Random S&H level LEDs move from `ScopeGridComponent` into `PerformanceBandV2`.

### Requirement: v2-performance-band-marbles-leds

`PerformanceBandV2` SHALL display two Marbles level LEDs for mod indices **13** and **14** at the right end of the band, after BPM / Steps / sequencer transport controls.

Labels: **S&H 1** and **S&H 2** (or **Rnd 1** / **Rnd 2** if width requires shorter copy).

#### Scenario: LEDs reflect host CV

- **WHEN** audio is running and Random S&H 1 outputs 0.4
- **THEN** S&H 1 LED brightness equals `ModLedDisplayBrightness(0.4, true)`

#### Scenario: VST parity

- **WHEN** FroggersTigaPluginV2 performance band renders
- **THEN** the same two LEDs appear in the same band positions as standalone
