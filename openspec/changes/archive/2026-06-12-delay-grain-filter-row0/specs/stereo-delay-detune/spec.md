## ADDED Requirements

### Requirement: Delay row 4 is Detune not Tone

Sim Delay page row 4 SHALL display **Detune**. The label **Tone** SHALL NOT appear in sim UI for that row.

#### Scenario: Desktop Delay panel

- **WHEN** the Delay submodule panel is visible
- **THEN** row 4 label reads **Detune**

#### Scenario: Web Delay page

- **WHEN** host page is Delay (5)
- **THEN** knob column 4 and OLED row 4 show **Detune**

### Requirement: Detune applies stereo pitch offset to wet reads

`StereoDelay` SHALL apply a static cents offset to left and right delay read times based on row 4 knob value. At minimum knob, wet reads SHALL match unison (no offset). At maximum knob, left and right SHALL use opposite-sign offsets so repeats sound detuned vs each other.

#### Scenario: Detune at zero

- **WHEN** row 4 is minimum and delay is audible
- **THEN** L/R wet pitch offset is inaudible vs unity read

#### Scenario: Detune at maximum

- **WHEN** row 4 is maximum, **Send** and **Feedback** are high, and **Delay time** is mid
- **THEN** successive repeats exhibit clear stereo pitch divergence vs detune at zero

### Requirement: Detune distinct from Mod depth

Row 4 detune SHALL be a static cents mapping. Row 5 **Mod depth** SHALL remain periodic LFO modulation of delay time. Detune SHALL NOT replace **Mod depth**.

#### Scenario: Mod without detune

- **WHEN** row 4 is zero and row 5 is high
- **THEN** delay time flutters without static L/R pitch split

### Requirement: Quick Dict

`QUICK_DICT.md` SHALL list **`Detune : Stereo pitch offset on repeats`**.

#### Scenario: Quick Dict Delay section

- **WHEN** user opens Quick Dict Delay rows
- **THEN** **Detune** entry exists with pitch-offset gloss
