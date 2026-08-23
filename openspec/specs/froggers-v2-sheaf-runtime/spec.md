# froggers-v2-sheaf-runtime Specification

## Purpose
Froggers v2 exposes a Sheaf-compatible app-core facade, runtime File/Audio/MIDI pages, and locally owned Sheaf adoption without external runtime dependencies.

## Requirements
### Requirement: Sheaf-compatible Froggers v2 app core facade
Froggers v2 SHALL expose a Sheaf-compatible app-core facade that supplies runtime configuration, initialization, prepare, process, message ingress, UI-state publication, and shutdown while delegating product behavior to the existing Froggers v2 control core and host bridge during migration. Compatibility SHALL mean support for the selected Sheaf lifecycle, page model, patch manager, controller model, message envelope, and bank/depth semantics needed by Froggers, not wholesale source compatibility with every Sheaf interface.

#### Scenario: Facade preserves audio output
- **WHEN** the facade processes the same input block, parameter state, and sequencer state as the existing v2 path
- **THEN** rendered audio matches the existing path within the declared deterministic tolerance

#### Scenario: Facade preserves UI state
- **WHEN** the existing v2 path publishes carousel, mod grid, scene, sequencer, sequencer-lock, ADSR, and scope state
- **THEN** the facade publishes equivalent UI state for the same frame

### Requirement: Runtime pages are accessed through persistent right-side buttons
Desktop standalone SHALL expose File/Patch, Audio, and MIDI/Controllers runtime pages through persistent right-side buttons while the Froggers carousel remains the synth module UI.

#### Scenario: Standalone app UI remains first screen
- **WHEN** desktop v2 launches
- **THEN** the Audio/VCO carousel page is selected
- **THEN** File/Patch, Audio, and MIDI/Controllers buttons are visible on the right side
- **THEN** carousel arrows remain available for synth module navigation

#### Scenario: Hosted projection hides standalone pages
- **WHEN** FroggersTigaPluginV2 opens in a DAW
- **THEN** hardware Audio and Controllers device pages are not shown as editable device selectors
- **THEN** the plugin exposes read-only host bus/state information if the hosted runtime panel is enabled
- **THEN** DAW parameter mapping/state information is available through the host-parameter projection

#### Scenario: Hosted File Patch uses host state only
- **WHEN** FroggersTigaPluginV2 opens in a DAW
- **THEN** the plugin does not expose a plugin preset browser, direct plugin file-system preset save/load, or plugin import/export workflow for this change
- **THEN** patch state round-trips through plugin state and DAW/host preset mechanisms
- **THEN** desktop standalone File/Patch remains the projection that owns direct preset save/load/revert behavior

#### Scenario: File page owns preset save/load
- **WHEN** the user saves a desktop standalone preset
- **THEN** File/Patch owns the patch identity, dirty state, save result, and controller mapping persistence result
- **THEN** the synth carousel keeps its current module selection after the save

### Requirement: Sheaf code adoption requires local ownership
Any Sheaf runtime or synth scaffolding code used by Froggers SHALL be copied, adapted, namespaced, or otherwise locally owned in this repository before becoming part of product builds.

#### Scenario: No external Sheaf runtime dependency
- **WHEN** desktop v2 or plugin v2 builds
- **THEN** the build does not fetch Sheaf from the network
- **THEN** all runtime scaffolding used by Froggers is present in the FroggersTiga source tree
- **THEN** no product build or verification command requires installing new package dependencies for Sheaf adoption

#### Scenario: Sheaf adoption inventory is reviewable
- **WHEN** Sheaf-derived code is copied, namespaced, or translated into FroggersTiga
- **THEN** the adoption inventory names the source concept, local file owner, adaptation notes, and local tests that cover it
- **THEN** product builds consume the local FroggersTiga implementation rather than an external runtime package
