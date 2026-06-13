## MODIFIED Requirements

### Requirement: Sim hosts show full parameter column titles

Desktop submodule panels and web knob columns SHALL display the **sim display name** from `ParamDisplayNames` for each row, not the 4-character firmware OLED label.

#### Scenario: Audio VCO rows

- **WHEN** the user views the Audio panel or web Audio page
- **THEN** rows 0–2 are labeled **VCO1**, **VCO2**, **VCO3**

#### Scenario: Audio PM3 row

- **WHEN** the user views the Audio panel or web Audio page row 6
- **THEN** the label reads **Phase mod 3**
- **AND** the label does not read **VCO Envelope** or **VCO level**

#### Scenario: Reverb panel readable labels

- **WHEN** the user views the Reverb panel or web Reverb page
- **THEN** rows include **Wet/dry**, **Room size**, **Decay**, **Pre-delay**, **Damping**, **Stereo width**, **Diffusion**, **Crunch**
- **AND** labels do **not** show `RVMX`, `RMOD`, `RRAT`, or **LFO depth** / **LFO rate**

#### Scenario: Filter panel

- **WHEN** the user views the Filter panel or web Filter page
- **THEN** row 0 is **Comb offset**, rows 1–3 are **Peak freq**, **Peak gain**, **Peak Q**, row 4 is **Comb delay**, rows 5–6 are **Comb feedback**, **Comb LP**, row 7 is **Crunch**

#### Scenario: Drive panel

- **WHEN** the user views the Drive panel or web Drive page
- **THEN** row 4 is **XOR**, row 5 is **Bit depth**
- **AND** **Reorganizer** does not appear

#### Scenario: Delay page

- **WHEN** the user views the Delay panel or web page 6
- **THEN** rows show **Delay time**, **Send**, **Feedback**, **Stereo width**, **Detune**, **Mod depth**, **Wet mix**, **Crunch**
