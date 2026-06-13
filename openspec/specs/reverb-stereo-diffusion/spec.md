# reverb-stereo-diffusion Specification

## Purpose

Reverb rows 5–6: **Stereo width** and **Diffusion** replace internal LFO on delay-line lengths. Landed in `delay-grain-filter-row0`; canonical spec lives here after consolidation.

## Requirements

### Requirement: Reverb rows 5–6 are Stereo width and Diffusion

Reverb page rows 5 and 6 SHALL display **Stereo width** and **Diffusion**. **LFO depth** and **LFO rate** SHALL NOT appear in sim UI.

#### Scenario: Desktop Reverb panel

- **WHEN** the Reverb submodule panel is visible
- **THEN** rows 5–6 read **Stereo width** and **Diffusion**

#### Scenario: Web Reverb page

- **WHEN** host page is Reverb (2)
- **THEN** knob columns 5–6 show **Stereo width** and **Diffusion**

### Requirement: Reverb engine removes delay-time LFO

`FroggersEngine::ProcessReverb` SHALL NOT modulate delay line lengths with an internal sine LFO driven by rows 5–6.

#### Scenario: Static tail

- **WHEN** rows 5–6 are fixed and audio is playing
- **THEN** reverb tail does not exhibit periodic chorus from an internal reverb LFO

### Requirement: Stereo width separates dual-line outputs

Row 5 (**Stereo width**) SHALL control how much the two reverb delay lines are collapsed to mono vs emitted as separate L/R wet components.

#### Scenario: Width at maximum on stereo host

- **WHEN** stereo width is high and wet/dry mix favors reverb
- **THEN** reverb wet is wider than width at minimum on the same material

### Requirement: Diffusion increases cross-feed in the tank

Row 6 (**Diffusion**) SHALL control cross-mixing between the two reverb delay lines in the feedback path.

#### Scenario: Diffusion sweep

- **WHEN** diffusion increases with other reverb params fixed
- **THEN** the tail becomes more diffuse

### Requirement: Firmware OLED names unchanged

Field hardware SHALL continue to show `RMOD` and `RRAT` on the OLED for knobs 6–7.
