## MODIFIED Requirements

### Requirement: Mod-then-fuego pipeline documented

Sim manuals SHALL state that on fuego-enabled pages (including Delay), modulation is applied first, then Crispy scrambles low bits of the result. Crispy itself can be modulated (scramble intensity follows effective Crispy). Pair-AR knobs accept modulation and, on hosts where `UsesV2Fuego` is true (Web, DesktopV2, VstV2), receive the same **Crunchy + Audio-page Crispy** fuego stack as Audio musical rows after modulation.

#### Scenario: Crispy mod gloss in manual

- **WHEN** reader opens Delay or Global controls section
- **THEN** text explains Crispy mod affects scramble intensity on rows 1–7 and pair-AR on web

#### Scenario: Pair-AR Crispy inclusion on v2 fuego hosts

- **WHEN** reader opens Audio pair-AR section
- **THEN** pair-AR knobs described as moddable and Crispy-scrambled on web (Audio page Crispy row)

#### Scenario: Pair-AR Crunchy inclusion on v2 fuego hosts

- **WHEN** reader opens Global Crunchy or pair-AR section
- **THEN** text states global Crunchy affects pair-AR on web and desktop v2
- **THEN** text states v1 desktop pair-AR remains without Crunchy (no global Crunchy control on v1)

## ADDED Requirements

### Requirement: quick-dict-covers-v2-performance-controls

`QUICK_DICT.md` mirrors SHALL document Scene, Gesture, and Sequencer workflows per `operator-quick-dict-performance` capability (content authority for section text).

#### Scenario: Quick Dict has scene entry

- **WHEN** reader searches Quick Dict for "Scene"
- **THEN** at least one entry explains S1/S2/S3 and blend usage

#### Scenario: Quick Dict has gesture entry

- **WHEN** reader searches Quick Dict for "Gesture"
- **THEN** entries explain both lanes and weight sliders

#### Scenario: Quick Dict has sequencer entry

- **WHEN** reader searches Quick Dict for "Sequencer"
- **THEN** entries explain BPM, length, play, and record arm
