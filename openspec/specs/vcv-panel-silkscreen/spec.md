# vcv-panel-silkscreen Specification

## Purpose
Require VCV Rack panel SVG silkscreen text and branding that remain readable at 100% zoom without hover tooltips.
## Requirements
### Requirement: VCV silkscreen omits removed MIDI and page concepts
VCV panel SVGs and local VCV docs SHALL NOT present Froggers-owned MIDI, CC enable controls, or page navigation as current VCV features. Labels SHALL use section and extension terminology.

#### Scenario: No CC enable labels
- **WHEN** `vcv/res/FroggersTiga.svg` and dark variant are inspected
- **THEN** they contain no CC enable, MIDI In, MIDI Out, or selected-page label surface

#### Scenario: Section terminology
- **WHEN** a VCV faceplate labels a control group
- **THEN** it uses section or extension names such as Audio, Random, Filter, Drive, Reverb, Delay, Global, VCO AR, Main, or FX

### Requirement: Faceplate silkscreen visible without hover

Every FroggersTiga VCV module panel SHALL display functional silkscreen text on the faceplate at 100% rack zoom without requiring hover. This includes a **header strip** (tiny frog logo + “FroggersTiga” name), section titles, row labels, mod-rack output names, global Crunchy and global Crunchy CV labels, extension names, and audio/CV/gate/stereo port names.

#### Scenario: Product header on faceplate

- **WHEN** any FroggersTiga module is placed at 100% zoom
- **THEN** a tiny frog logo and “FroggersTiga” name are visible on the panel shell (top area), matching web/desktop branding
- **THEN** the header does not obscure knobs, jacks, or screws

#### Scenario: Main module at 100% zoom

- **WHEN** Froggers Tiga main is placed in Rack at 100% zoom
- **THEN** its global controls, global Crunchy knob/CV, mod-rack labels (Random, VCO Env, Random 1, Random 2), and I/O labels (Audio in, Audio out, CV1, Gate, etc.) are readable on the faceplate
- **THEN** no MIDI, CC-enable, or selected-page label is present

#### Scenario: Right section and FX extension at 100% zoom

- **WHEN** a Froggers Tiga right extension is placed at 100% zoom to the right of main
- **THEN** each visible section title and row label is readable on the faceplate
- **THEN** Reverb and Delay titles, row labels, and stereo jack labels (L in, R in, L out, R out) are readable when the FX extension is present

#### Scenario: Left VCO AR extension at 100% zoom
- **WHEN** Froggers Tiga VCO AR is placed at 100% zoom to the left of main
- **THEN** VCO AR title, Attack/Release row labels, local Crispy, Randomize, and Randmod labels are readable on the faceplate

#### Scenario: Hover is supplementary

- **WHEN** the user does not hover any control
- **THEN** faceplate silkscreen still identifies every knob row and port row
- **THEN** tooltips may add detail but are not the primary label surface

### Requirement: Label authority remains ParamDisplayNames

Silkscreen strings SHALL match shared display-name authority for corresponding sections and rows. Audio row 6 SHALL read **Phase mod 3**; row 7 SHALL read **Crispy**. New VCV-only global and extension labels SHALL come from shared C++ label tables or dedicated VCV display-name helpers, not ad hoc literals in Rack widget placement code.

#### Scenario: PM3 parity with desktop

- **WHEN** the Audio column silkscreen is inspected
- **THEN** the seventh knob row from the top is labeled **Phase mod 3**, not VCO Envelope
- **THEN** the bottom row is labeled **Crispy**

#### Scenario: Global Crunchy label authority
- **WHEN** the VCV main faceplate is generated
- **THEN** global Crunchy knob and CV input labels come from the shared label authority used by VCV panel generation
- **THEN** the labels are not hardcoded only in `vcv/src/plugin.cpp`

#### Scenario: VCO AR label authority
- **WHEN** the VCO AR left extension faceplate is generated
- **THEN** Attack/Release labels for VCO1, VCO2, and VCO3 come from the shared label authority used by VCV panel generation
- **THEN** the labels are not hardcoded only in `vcv/src/plugin.cpp`

### Requirement: Rack-compatible SVG text

Panel SVG assets SHALL NOT rely on SVG `<text>` elements or embedded fonts for silkscreen. Text SHALL be converted to paths before distribution in the vcvplugin bundle. Label anchor positions SHALL be derived from the same layout constants as `FieldParityWidget` (including header strip offset).

#### Scenario: nanosvg render

- **WHEN** Rack loads `res/FroggersTiga.svg` via `setPanel`
- **THEN** silkscreen geometry is present as paths or widget draw calls that nanosvg/Rack actually renders
