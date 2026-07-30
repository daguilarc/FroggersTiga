## ADDED Requirements

### Requirement: Faceplate silkscreen visible without hover

Every FroggersTiga VCV module panel SHALL display functional silkscreen text on the faceplate at 100% rack zoom without requiring hover. This includes a **header strip** (tiny frog logo + “FroggersTiga” name), column titles, row labels, mod-rack output names, CC/MIDI/I/O port names.

#### Scenario: Product header on faceplate

- **WHEN** any FroggersTiga module is placed at 100% zoom
- **THEN** a tiny frog logo and “FroggersTiga” name are visible on the panel shell (top area), matching web/desktop branding
- **THEN** the header does not obscure knobs, jacks, or screws

#### Scenario: Main module at 100% zoom

- **WHEN** Froggers Tiga main (72 HP) is placed in Rack at 100% zoom
- **THEN** Audio, Random S&H, Filter, and Drive column titles are readable on the panel
- **THEN** each voicing row label (VCO1 … Phase mod 3, Crispy) is readable beside its knob column
- **THEN** mod-rack labels (Random, VCO Env, Random 1, Random 2) and I/O labels (Audio in, CV1, Gate, etc.) are readable on the faceplate

#### Scenario: FX module at 100% zoom

- **WHEN** Froggers Tiga FX is placed at 100% zoom to the right of main
- **THEN** Reverb and Delay column titles and row labels are readable on the faceplate
- **THEN** stereo jack labels (L in, R in, L out, R out) are readable on the faceplate

#### Scenario: Hover is supplementary

- **WHEN** the user does not hover any control
- **THEN** faceplate silkscreen still identifies every knob row and port row
- **THEN** tooltips may add detail but are not the primary label surface

### Requirement: Label authority remains ParamDisplayNames

Silkscreen strings SHALL match `ParamDisplayNames::forHostPage` and `forHostPageRow` for the corresponding page and row indices. Audio row 6 SHALL read **Phase mod 3**; row 7 SHALL read **Crispy**.

#### Scenario: PM3 parity with desktop

- **WHEN** the Audio column silkscreen is inspected
- **THEN** the seventh knob row from the top is labeled **Phase mod 3**, not VCO Envelope
- **THEN** the bottom row is labeled **Crispy**

### Requirement: Rack-compatible SVG text

Panel SVG assets SHALL NOT rely on SVG `<text>` elements or embedded fonts for silkscreen. Text SHALL be converted to paths before distribution in the vcvplugin bundle. Label anchor positions SHALL be derived from the same layout constants as `FieldParityWidget` (including header strip offset).

#### Scenario: nanosvg render

- **WHEN** Rack loads `res/FroggersTiga.svg` via `setPanel`
- **THEN** silkscreen geometry is present as paths or widget draw calls that nanosvg/Rack actually renders
