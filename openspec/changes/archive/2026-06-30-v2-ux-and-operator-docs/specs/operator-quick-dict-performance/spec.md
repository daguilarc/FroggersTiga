## ADDED Requirements

### Requirement: quick-dict-v2-performance-sections

`QUICK_DICT.md` and its mirrors (`docs/quick-dict.md`, `web/public/quick-dict.md`, desktop v2 embedded copy) SHALL include dedicated sections for **Scenes**, **Gestures**, and **Sequencer** with operator-facing “how to use” steps, not parameter-name glosses alone.

Mirror parity gates live in `sim-operator-doc-parity` (`quick-dict-covers-v2-performance-controls`); this capability owns section content only.

#### Scenario: Scenes how-to in Quick Dict

- **WHEN** a reader opens Quick Dict Scenes section
- **THEN** text explains: press S1/S2/S3 to store endpoints, blend slider morphs L↔R, encoder rings show concentric scene arcs
- **THEN** text states scene storage is global across all modules

#### Scenario: Gestures how-to in Quick Dict

- **WHEN** a reader opens Quick Dict Gestures section
- **THEN** text explains: select Gesture 1 or Gesture 2, adjust that lane's weight slider, turn an encoder ring to capture offset, badges appear on affected rings
- **THEN** text states Rand All clears gesture selection first

#### Scenario: Sequencer how-to in Quick Dict

- **WHEN** a reader opens Quick Dict Sequencer section
- **THEN** text explains: set BPM and pattern length, toggle play, arm record to capture per-step scene/gesture snapshots, step gate buttons in sequencer panel
- **THEN** text references ADSR gate source when sequencer is playing (desktop v2 only)

#### Scenario: Crunchy Crispy pair-AR matrix

- **WHEN** a reader opens Quick Dict Crunchy/Crispy section
- **THEN** a short table states: Crispy scrambles page rows 1–7 and web Audio pair-AR; global Crunchy affects all page rows and pair-AR on web and desktop v2

### Requirement: quick-dict-desktop-v2-control-map

Quick Dict SHALL map desktop v2 performance-band controls to their actions so operators are not required to infer behavior from unlabeled toggles.

#### Scenario: Performance band named in Quick Dict

- **WHEN** a reader opens Quick Dict on desktop v2
- **THEN** entries exist for Scene S1–S3, Scene blend, Gesture 1 weight, Gesture 2 weight, and Sequencer transport controls in the performance band
