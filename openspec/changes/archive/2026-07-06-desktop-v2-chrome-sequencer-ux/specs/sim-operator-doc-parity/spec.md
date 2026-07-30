## ADDED Requirements

### Requirement: Desktop v2 transport Play label documented

`QUICK_DICT.md` and mirrors (`docs/quick-dict.md`, `web/public/quick-dict.md`) SHALL describe standalone desktop v2 top transport audio controls as **Play** and **Stop**, not **Engine**.

#### Scenario: quick-dict transport row

- **WHEN** a reader opens `QUICK_DICT.md` desktop v2 transport section
- **THEN** the audio start control is named **Play**
- **THEN** no entry names audio transport **Engine**

#### Scenario: Distinct sequencer transport names

- **WHEN** a reader opens sequencer entries in quick-dict mirrors
- **THEN** **Start Sequence** and **Stop Sequence** are documented separately from audio **Play** / **Stop**

### Requirement: Rand-seq scope documented

Operator docs SHALL document Rand-seq scope as **Step** (edit step only) and **All steps** (entire pattern, overwrites existing step data). Docs SHALL NOT describe Pattern mode as filling blank steps only.

#### Scenario: Rand-seq glossary entry

- **WHEN** reader opens `QUICK_DICT.md` Rand-seq entry
- **THEN** **Step** scope randomizes the currently selected edit step
- **THEN** **All steps** scope randomizes every step in pattern length including steps that already contain data

#### Scenario: Edit step invariant documented

- **WHEN** reader opens sequencer toolbar documentation
- **THEN** text states one edit step is always selected (default step 1 / index 0)
- **THEN** prev/next arrows wrap within pattern length

### Requirement: Write Seq. and audio Record documented

Operator docs SHALL distinguish:

- **Record audio** (transport) — audio file export (WAV/MP3/FLAC/OGG); requires audio **Play**
- **Write Seq.** (sequencer toolbar) — writes scene snapshots into step buffer; works stopped (on edit-step change) and playing (on Start Sequence + beat advance)

Docs SHALL NOT label sequencer step capture as **Record** or **Record audio**.

#### Scenario: Write Seq. stopped workflow

- **WHEN** reader opens sequencer documentation
- **THEN** text explains: arm **Write Seq.**, tweak knobs, click another step to save previous step and recall new step

#### Scenario: Write Seq. playing workflow

- **WHEN** reader opens sequencer documentation
- **THEN** text explains: arm **Write Seq.**, press **Start Sequence**; step 0 captures on start and on first beat leave; each beat saves the step being left

#### Scenario: Audio Record audio restored

- **WHEN** reader opens desktop v2 transport documentation
- **THEN** round red **Record audio** control is documented in the transport row
- **WHEN** reader opens desktop v2 **Audio** menu documentation
- **THEN** WAV / MP3 / FLAC / OGG export format toggles are documented there (not beside Record in transport)
