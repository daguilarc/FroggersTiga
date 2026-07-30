## ADDED Requirements

### Requirement: Shared C++ tables are the generated host-display authority
`sim/ParamDisplayNames.hpp` SHALL be the editable authority for host page names, page-row labels, pair-AR labels, mod-source labels, and global-strip action labels. `sim/HostPanelLayout.hpp` SHALL own the ordered host-aware mod-rack presentation table, host inclusion metadata, and scope sample capacity. Web build inputs SHALL be generated from those authorities or obtained from WASM; equivalent hand-maintained TypeScript tables SHALL NOT exist.

#### Scenario: Label change
- **WHEN** a maintainer changes a label in `ParamDisplayNames.hpp` and runs the generator
- **THEN** the web pre-audio UI, worklet screen payload, and tests consume the generated value without a second manual edit

#### Scenario: Stale generated file
- **WHEN** a checked/generated web label artifact differs from generator output
- **THEN** the preflight check fails with a regeneration instruction

### Requirement: Mod-rack topology has one cross-host authority
The shared mod-rack presentation table SHALL define host inclusion and presentation for indices `0, 1, 4, 5, 6`. Desktop standalone SHALL present all five in that order, with scopes for MIDI CC 1, MIDI CC 2, and VCO Envelope plus LED feedback for Random 1 and Random 2. Web SHALL project indices `0, 4, 5, 6` in that order. VST/AU and VCV SHALL project only internal sources `4, 5, 6`; VST/AU routes MIDI through host parameters and VCV receives modulation through native CV jacks.

#### Scenario: Web mod bay is constructed
- **WHEN** `main.ts` builds the mod-bay indicators
- **THEN** it consumes generated metadata and creates four cells in order `0, 4, 5, 6`

#### Scenario: Worklet collects display samples
- **WHEN** `froggers-processor.ts` copies mod display samples
- **THEN** mod indices and scope capacity come from generated/runtime metadata rather than `SCOPE_MOD_INDICES` or `SCOPE_SIZE` literals

#### Scenario: Hosted and Rack projections exclude CC mod cells
- **WHEN** VST/AU or VCV consumes the shared mod-rack metadata
- **THEN** it creates only the VCO Envelope, Random 1, and Random 2 cells in order `4, 5, 6`

### Requirement: Disabled supported external sources remain visible
Desktop standalone and web SHALL keep their supported MIDI CC cells present when input is disabled. Availability SHALL change a supported cell's disabled state and assignability, not remove it or shift the remaining topology. Web supports only MIDI CC 1. VST/AU and VCV SHALL render no MIDI CC mod-rack cells.

#### Scenario: Web External MIDI defaults off
- **WHEN** the web sim starts with External MIDI Off
- **THEN** the MIDI CC 1 cell remains visible and unavailable, and no MIDI CC 2 cell appears

#### Scenario: VST and VCV have no fixed CC cells
- **WHEN** a hosted plugin editor or VCV module panel is created
- **THEN** neither surface contains a MIDI CC 1 or MIDI CC 2 mod-rack cell
