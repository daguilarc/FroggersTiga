# froggers-v2-app-manifest Specification

## Purpose
Froggers v2 declares shared product controls, projection overlays, the permanent 15-lane modulation source catalog, fixed 16-step sequencer structure, and MIDI targets through one manifest family with generated and checked projections.

## Requirements
### Requirement: Froggers v2 manifest family authority
Froggers v2 SHALL declare shared product controls plus desktop and VST/AU projection overlay fields through one manifest family. The product manifest SHALL own module pages, rows, ranges, defaults, stable IDs, display names, mod eligibility, MIDI assignment targets, exactly 16 sequencer snapshot slots, sequencer written/unwritten state, sequencer lock fields, scene support, clock sync declarations, sequencer direction/speed controls, and global randomization scene/step scope controls. Projection overlays SHALL declare context-specific visibility, host mapping, hardware controls, and layout groups.

#### Scenario: Manifest covers every visible module row
- **WHEN** the manifest validation command runs
- **THEN** every desktop v2 carousel row has one manifest entry
- **THEN** every manifest row maps to a valid module page and row index

#### Scenario: No duplicate structural tables
- **WHEN** the OMNI duplicate-authority check scans desktop-v2 sources
- **THEN** no independent hand-written table duplicates manifest-owned row labels, stable host IDs, MIDI target IDs, sequencer snapshot fields, or mod eligibility
- **THEN** any desktop or VST/AU variation is declared as a projection overlay rather than an untracked table

#### Scenario: Manifest declares global randomization scope controls
- **WHEN** manifest validation runs
- **THEN** the global controls projection declares the `All Scenes` / `Current Scene` scope pair
- **THEN** the global controls projection declares the `All Steps` / `Current Step` scope pair
- **THEN** each pair has exactly one selected state at runtime
- **THEN** the scope controls are consumed by Randomize All and Randomize Mod rather than by a separate duplicate randomization table

#### Scenario: Manifest declares fixed sequencer structure and controls
- **WHEN** manifest validation runs
- **THEN** the sequencer projection declares exactly 16 step slots
- **THEN** each step slot declares written/unwritten state
- **THEN** no pattern-length or step-count control is declared
- **THEN** the sequencer projection declares direction choices `<`, `>`, and `RND` with `>` as the default
- **THEN** the sequencer projection declares speed choices `/2`, `/1.5`, `1`, `x1.5`, and `x2` with `1` as the default

#### Scenario: Context overlay hides standalone controls in plugin
- **WHEN** the VST/AU projection overlay is validated
- **THEN** hardware audio selectors, standalone MIDI device selectors, and standalone record/export controls are marked hidden
- **THEN** host parameters still map to product-manifest stable control IDs

### Requirement: Manifest projections are generated or checked by projection type
The system SHALL generate mechanical manifest projections and verify platform-code projections before implementation is considered complete. Generated outputs SHALL include a sorted JSON snapshot and Markdown reviewer report. Checked platform outputs SHALL include desktop rows, host parameter inventory, controller targets, sequencer fields, and hosted projection behavior.

#### Scenario: Host parameter inventory projection
- **WHEN** the host parameter inventory validator runs
- **THEN** `HostParameterInventoryV2` contains exactly the continuous controls declared by the manifest
- **THEN** each entry uses the manifest stable ID and grouped display name

#### Scenario: Reviewer artifacts are produced
- **WHEN** the manifest report command runs
- **THEN** it writes `build/manifest/froggers-v2-manifest.snapshot.json`
- **THEN** it writes `build/manifest/froggers-v2-manifest-report.md`
- **THEN** both files sort controls by projection, page, row, and stable ID

#### Scenario: Sequencer snapshot projection
- **WHEN** the sequencer snapshot validator runs
- **THEN** exactly 16 step snapshots are present
- **THEN** exactly 16 written/unwritten state flags are present
- **THEN** every manifest control marked sequencer-persistent appears in `SequencerStepSnapshot`
- **THEN** every manifest control marked sequencer-lockable has a declared lock field
- **THEN** no snapshot or lock field exists without a manifest-owned control or route entry

### Requirement: Stable IDs do not derive from labels
Manifest stable IDs SHALL be explicit durable identifiers. Display labels SHALL NOT be used to derive automation IDs, state keys, MIDI target IDs, or sequencer snapshot keys.

#### Scenario: Label edit preserves stable ID
- **WHEN** a manifest display label changes while its stable ID remains unchanged
- **THEN** host automation ID, state key, MIDI target key, and sequencer snapshot key remain unchanged

#### Scenario: Duplicate ID rejected
- **WHEN** two manifest entries declare the same stable ID
- **THEN** manifest validation exits nonzero

### Requirement: Manifest owns the permanent modulation source catalog
The Froggers v2 product manifest SHALL declare the permanent modulation source catalog, including source stable ID, display label, color, group, signal-rate class, availability rule, default depth policy, and whether the source can be selected by randomization. The catalog SHALL contain exactly fifteen source lanes for this change: VCO 1+2, VCO 2+3, VCO 1+3, VCO 1 EF, VCO 2 EF, VCO 3 EF, VCO 1+2 EF, VCO 2+3 EF, LFO 1, LFO 2, LFO 3, Random/Marbles 1, Random/Marbles 2, External Audio (audio rate), and External Audio (envelope follower).

#### Scenario: Source catalog validates exact lane set
- **WHEN** manifest validation runs
- **THEN** the permanent source catalog contains the fifteen required lanes
- **THEN** raw VCO 1, VCO 2, and VCO 3 audio-rate lanes are absent
- **THEN** VCO 1+2+3 EF is absent
- **THEN** MIDI CC A and MIDI CC B are absent from the permanent source catalog

#### Scenario: VCO-owned targets avoid audio-rate self-feedback
- **WHEN** manifest validation runs
- **THEN** VCO 1-owned targets that accept audio-rate VCO modulation allow only VCO 2+3 among VCO pair buses
- **THEN** VCO 2-owned targets that accept audio-rate VCO modulation allow only VCO 1+3 among VCO pair buses
- **THEN** VCO 3-owned targets that accept audio-rate VCO modulation allow only VCO 1+2 among VCO pair buses
- **THEN** non-VCO targets can allow VCO 1+2, VCO 2+3, and VCO 1+3 unless their manifest row narrows eligibility

#### Scenario: External audio availability is manifest-declared
- **WHEN** manifest validation runs
- **THEN** External Audio (audio rate) declares an availability rule tied to active/available external audio input
- **THEN** External Audio (envelope follower) declares an availability rule tied to active/available external audio input
- **THEN** both external-audio lanes declare depth-zero/off as their default behavior when unavailable
