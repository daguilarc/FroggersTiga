## MODIFIED Requirements

**Audit 2026-06-30:** Transport entries exist in `QUICK_DICT.md`. Scene semantics fixed. **Open:** full sequencer UX glossary + **VST v2 subsection in Quick Dict only**; SIM_MANUAL desktop updates via `sim-manual-v2-sequencer-desktop` (**no VST/VCV in SIM_MANUAL**).

### Requirement: quick-dict-v2-sequencer-step-editing

`QUICK_DICT.md` and mirrors SHALL document desktop v2 and **VST v2** sequencer step editing:

- **Edit step** vs **playhead**
- Toolbar **←** / **→**, dice (Rand-seq), **Step** / **Pattern** scope
- **Single-click** step → edit step; **double-click** → toggle gate (lit/rest)
- **Right-click** step → **Reset** (factory snapshot) / **Randomize** (full step snapshot)
- Internal VCOs drive sound by default; step gates affect AR **only while Start Sequence runs**
- Dice Rand-seq vs context-menu **Randomize** (globals vs step-only)

#### Scenario: VST sequencer subsection in Quick Dict

- **WHEN** a reader opens Quick Dict sequencer or transport entries for VST
- **THEN** text states there is **no Engine** row — DAW owns audio transport
- **THEN** text states **Start Sequence**, step grid, and step editing behave the same as standalone once the DAW is playing audio
- **THEN** text states DAW MIDI Start/Stop may toggle sequencer playback

### Requirement: quick-dict-v2-performance-sections

`QUICK_DICT.md` and its mirrors SHALL include dedicated sections for **Scenes**, **Gestures**, and **Sequencer** with operator-facing “how to use” steps.

#### Scenario: Scenes how-to in Quick Dict

- **WHEN** a reader opens Quick Dict Scenes section
- **THEN** text explains S1/S2/S3 pick L/R endpoints; active endpoints show **·L** / **·R** on buttons; blend slider ends labeled **L** / **R**
- **THEN** text states scene storage is global across all modules

#### Scenario: Gestures how-to in Quick Dict

- **WHEN** a reader opens Quick Dict Gestures section
- **THEN** text explains G1/G2 selection, weight sliders, ring capture, badges on affected rings
- **THEN** text states Rand All clears gesture selection first

#### Scenario: Sequencer how-to in Quick Dict

- **WHEN** a reader opens Quick Dict Sequencer section
- **THEN** text explains BPM, pattern length, Start Sequence, Record arm, step gates
- **THEN** with Engine on, internal VCOs drive sound by default — no MIDI or running sequencer required
- **THEN** step gates affect envelopes only while **Start Sequence** is running (stored pattern data when stopped)
- **THEN** text documents edit step, ←/→, dice Rand-seq, Step/Pattern scope after Phase H

#### Scenario: Crunchy Crispy pair-AR matrix

- **WHEN** a reader opens Quick Dict Crunchy/Crispy section
- **THEN** a short table states Crispy vs global Crunchy scope including pair-AR on v2 hosts

### Requirement: quick-dict-desktop-v2-control-map

Quick Dict SHALL map desktop v2 performance-band and transport controls to their actions so operators are not required to infer behavior from unlabeled toggles.

#### Scenario: Engine vs Start Sequence documented

- **WHEN** a reader opens Quick Dict desktop v2 transport section
- **THEN** **Engine** (top row) is defined as start/stop audio processing; with Engine on, internal VCOs drive sound by default
- **THEN** **Start Sequence** (performance band) is defined as start sequencer pattern playback — distinct from Engine; step gates shape envelopes only while it runs
- **THEN** **Stop Sequence** is defined as the playing-state label on the same button

#### Scenario: Performance band named in Quick Dict

- **WHEN** a reader opens Quick Dict on desktop v2
- **THEN** entries exist for Scene S1–S3, Scene blend, G1/G2 gesture lanes, gesture weights, BPM, Steps, Record arm, and Start Sequence transport

#### Scenario: Scene semantics documented accurately

- **WHEN** a reader opens Quick Dict scene entries
- **THEN** S1/S2/S3 are defined as selecting L/R morph endpoints (not storing current knob positions on button press)
- **THEN** ring turns without a gesture selected edit the scene slot selected by blend between those endpoints
- **THEN** text distinguishes scenes (stored knob positions), gestures (per-knob performance offsets), and sequencer steps (timed recall layer)
- **THEN** cold start notes that factory defaults come from `pageKnobDefault` seeded into all three scene slots
- **THEN** Audio cold start notes VCO1–VCO3 default to **30 Hz** with **sine / square / saw** morphs respectively

#### Scenario: v2 randomize documented

- **WHEN** a reader opens Quick Dict or SIM_MANUAL desktop v2 randomize entries
- **THEN** **Randomize** is defined as rewriting all three scene slots per musical row on the current module (Crispy skipped)
- **THEN** **Rand All** and **Rand-seq dice** also randomize L/R endpoint assignment and scene blend (distinct endpoints, blend ∈ [0,1])
- **THEN** per-page **Randomize** does not change endpoints or blend
- **THEN** text states S1/S2/S3 endpoint ordinals and scene blend are not changed by either action

#### Scenario: Sequencer edit step and Rand-seq documented

- **WHEN** a reader opens Quick Dict sequencer section
- **THEN** text explains **edit step** (selected step for authoring) vs **playhead** (playback position)
- **THEN** **single-click** a step selects edit step; **double-click** toggles step gate (lit/rest)
- **THEN** **←** / **→** move edit step within pattern length
- **THEN** **dice (Rand-seq)** writes randomized scene slots into step buffer(s) — same scene policy as Rand All (includes Crunchy slots), not mod depths; also randomizes live L/R/blend once per press
- **THEN** step **right-click → Reset** restores factory cold-start values into that step only
- **THEN** step **right-click → Randomize** randomizes all storable step fields for that step only (no live L/R/blend change)
- **THEN** **Step** scope randomizes edit step only; **Pattern** scope fills blank steps only
- **THEN** text states dice does not change live knobs until playback recalls the step

#### Scenario: Crunchy scene ring documented

- **WHEN** a reader opens Quick Dict Crunchy entry on desktop v2
- **THEN** text states Crunchy uses S1/S2/S3 scene slots and blend like module encoder rings (not a single unscened rotary)
- **THEN** text states web global Crunchy remains a single knob (desktop-only scene parity)

#### Scenario: v2 MIDI CV settings documented

- **WHEN** a reader opens Quick Dict desktop v2 MIDI section
- **THEN** text explains: MIDI In picks one device; CV Assignments map pitch (page+row), gate, MIDI CC A/B, shift/scene triggers, and QWERTY virtual channel
- **THEN** **MIDI CC A** and **MIDI CC B** are defined as incoming CC routes assignable in module mod menus
- **THEN** v1-only "two CC pairs" wording is absent from desktop v2 guidance
