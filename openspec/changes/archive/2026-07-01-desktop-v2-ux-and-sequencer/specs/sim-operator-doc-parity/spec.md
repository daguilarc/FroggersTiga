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

### Requirement: sim-manual-excludes-vst-and-vcv

`SIM_MANUAL.md` and its mirrors SHALL **not** document VST plugin hosting or VCV Rack/grid-layout terminology in this change. VST v2 and VCV-inspired grid policy remain in implementation specs and **Quick Dict** only until a future doc pass.

#### Scenario: SIM_MANUAL has no VST plugin section

- **WHEN** a reader searches `SIM_MANUAL.md` for VST, plugin, DAW, or AU/VST3
- **THEN** no operator-facing VST v2 editor or DAW-mapping section is present from this change

#### Scenario: SIM_MANUAL has no VCV references

- **WHEN** a reader searches `SIM_MANUAL.md` for VCV, Eurorack, or grid-unit layout jargon
- **THEN** no VCV Rack analogies or u-cell layout documentation is present from this change

### Requirement: sim-manual-v2-sequencer-desktop

`SIM_MANUAL.md` and mirrors SHALL document **standalone desktop v2** sequencer behavior (web sim differences noted where relevant) aligned with `desktop-v2-sequencing` — **without** VST or VCV content:

- Default **internal VCO** output with **Engine** on (no MIDI/sequencer required)
- Step **gate** programming (double-click), **edit step** (single-click), right-click **Reset** / **Randomize**
- **Start Sequence** vs **Engine**
- Pair-AR page (not legacy ADSR sustain layout); per-VCO AR; gates from pattern + MIDI only while sequencer runs
- Replace stale “gate always drives ADSR” / six-float step snapshot wording

#### Scenario: SIM_MANUAL sequencer section matches Quick Dict desktop entries

- **WHEN** a reader compares `SIM_MANUAL.md` Sequencer section to Quick Dict desktop v2 Sequencer entries
- **THEN** step interaction, gate policy, and Rand-seq scope agree on shared desktop behavior
- **THEN** SIM_MANUAL does **not** duplicate Quick Dict VST-only rows

#### Scenario: SIM_MANUAL Pair-AR matches refactor

- **WHEN** reader opens desktop v2 Pair-AR section
- **THEN** seven-row A/R layout is documented (no Sus rows)
- **THEN** default open gate when sequencer stopped is stated

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
- **THEN** entries explain BPM, length, Start Sequence, record arm, edit step, gate editing, and step context menu
