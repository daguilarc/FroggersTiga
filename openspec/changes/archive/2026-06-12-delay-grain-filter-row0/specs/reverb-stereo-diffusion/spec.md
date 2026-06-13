## ADDED Requirements

### Requirement: Reverb rows 5–6 are Stereo width and Diffusion

Reverb page rows 5 and 6 SHALL display **Stereo width** and **Diffusion**. **LFO depth** and **LFO rate** SHALL NOT appear in sim UI.

#### Scenario: Desktop Reverb panel

- **WHEN** the Reverb submodule panel is visible
- **THEN** rows 5–6 read **Stereo width** and **Diffusion**

#### Scenario: Web Reverb page

- **WHEN** host page is Reverb (2)
- **THEN** knob columns 5–6 and OLED rows show **Stereo width** and **Diffusion**

### Requirement: Reverb engine removes delay-time LFO

`FroggersEngine::ProcessReverb` SHALL NOT modulate delay line lengths with an internal sine LFO driven by rows 5–6. LFO phase state for reverb SHALL be removed.

#### Scenario: Static tail

- **WHEN** rows 5–6 are fixed and audio is playing
- **THEN** reverb tail does not exhibit periodic chorus from an internal reverb LFO

### Requirement: Stereo width separates dual-line outputs

Row 5 (**Stereo width**) SHALL control how much the two reverb delay lines are collapsed to mono vs emitted as separate L/R wet components. At minimum width, wet output SHALL approximate equal mix of both lines. At maximum width, left wet SHALL favor line A output and right wet SHALL favor line B output (or equivalent documented pan law).

#### Scenario: Width at maximum on stereo host

- **WHEN** stereo width is high, wet/dry mix favors reverb, and host outputs two channels
- **THEN** reverb wet is wider than width at minimum on the same material

### Requirement: Diffusion increases cross-feed in the tank

Row 6 (**Diffusion**) SHALL control additional cross-mixing between the two reverb delay lines in the feedback path. Higher diffusion SHALL yield a denser, more smeared tail without requiring LFO modulation.

#### Scenario: Diffusion sweep

- **WHEN** diffusion increases with other reverb params fixed
- **THEN** the tail character becomes more diffuse (less distinct echo of discrete line pitches)

### Requirement: Quick Dict

`QUICK_DICT.md` SHALL list **`Stereo width : Reverb L/R spread`** and **`Diffusion : Cross-feed between reverb lines`**.

#### Scenario: Quick Dict Reverb section

- **WHEN** user opens Quick Dict Reverb rows
- **THEN** rows 5–6 use the new labels and glosses

### Requirement: Firmware OLED names unchanged

Field hardware SHALL continue to show `RMOD` and `RRAT` on the OLED for knobs 6–7. Sim display names differ; parameter indices 5–6 unchanged.
