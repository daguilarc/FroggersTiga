## ADDED Requirements

### Requirement: Operator docs match ParamDisplayNames

All sim operator documentation (`SIM_MANUAL.md`, `QUICK_DICT.md`, `docs/sim-manual.md`, `docs/quick-dict.md`, `web/public/sim-manual.md`, `web/public/quick-dict.md`) SHALL list Audio page row 7 as **Phase mod 3** with the gloss: VCO2 → VCO3 PM depth when cross-coupler is CW (2→3).

#### Scenario: docs sim-manual Audio table

- **WHEN** a reader opens `docs/sim-manual.md` Page 1 — Audio table
- **THEN** row 7 is **Phase mod 3**, not VCO level or VCO Envelope

#### Scenario: docs quick-dict Audio section

- **WHEN** a reader opens `docs/quick-dict.md` Audio section
- **THEN** an entry for **Phase mod 3** exists and no entry implies row 7 is OLVL/VCO-only level

#### Scenario: Crispy naming consistency

- **WHEN** sim manuals reference knob 8 on any page
- **THEN** the sim-facing name is **Crispy** (matching UI), with Field OLED name FUEG noted parenthetically where relevant

### Requirement: PM3 vs Crispy vs VCO Envelope glossary

Sim manuals SHALL include a short note that:

- **Phase mod 3** (row 7) is a dedicated PM3 depth knob on sim hosts.
- **Crispy** (row 8) scrambles knobs 1–7 and controls mix topology when external input is present; it no longer controls PM3 on sim hosts.
- **VCO Envelope** is a mod source (slow CV from VCO mix), shown as a scope on desktop/web mod rack — not the row 7 knob.

#### Scenario: Desktop embedded manual

- **WHEN** user opens Manual from the desktop app Help menu
- **THEN** embedded content matches root `SIM_MANUAL.md` PM3/Crispy/VCO Envelope distinction

#### Scenario: Web help modal

- **WHEN** user opens Manual on the website
- **THEN** fetched `sim-manual.md` matches the same PM3 row 7 semantics

### Requirement: VCO Envelope scope documented

Operator docs SHALL state that the VCO Envelope mod rack scope displays **mod CV** (0–100% slow envelope of VCO mix level), not an audio waveform.

#### Scenario: Mod bay scope description

- **WHEN** user reads Mod sources / Mod bay section
- **THEN** VCO Envelope is described as continuous CV scope trace of slow level from VCO mix, distinct from Phase mod 3 knob
