**Baseline cross-reference:** this capability extends `desktop-v2-global-controls` requirement `v2-rand-all-scope`, which currently says nothing about scope-radio consumption or live-mod-depth randomization. On archive, sync a corresponding delta into `desktop-v2-global-controls` so `v2-rand-all-scope` reflects scope-radio behavior instead of relying on this capability alone.

## ADDED Requirements

### Requirement: Randomization commands use one control-core authority
Desktop v2 SHALL route Rand All, Rand Mods, and sequencer step randomization through one control-core randomization authority. UI components MUST NOT call `DesktopHostIO::EnqueueRandomizePanelMod`, `EnqueueRandomizeAllMod`, or `MessageIn::RandSequencerMods` directly except through the authority adapter layer under test.

#### Scenario: Global Rand Mods randomizes live mod depths
- **WHEN** the operator clicks Rand Mods with Current Step scope selected
- **THEN** live mod depths on eligible rows change in the control core
- **THEN** the audio engine hears the change without switching carousel pages
- **THEN** sequencer step mod snapshots change only when step scope targets written steps

#### Scenario: Global Rand Mods is not sequencer-only
- **WHEN** the operator clicks Rand Mods on the Audio page without starting the sequencer
- **THEN** visible module mod depths randomize on the live synth state
- **THEN** the action is not equivalent to only mutating sequencer snapshot storage

### Requirement: Scope radios change randomization behavior
The global-command band scope radios (`All Scenes` / `Current Scene`, `All Steps` / `Current Step`) SHALL be consumed by Rand All and Rand Mods. Decorative scope storage without behavioral effect is forbidden.

#### Scenario: Rand All respects scene scope
- **WHEN** Current Scene is selected and the operator clicks Rand All
- **THEN** only the current scene edit-target endpoint parameters randomize
- **THEN** left/right scene ordinal selections and blend position remain unchanged

#### Scenario: Rand Mods respects step scope
- **WHEN** All Steps is selected and the operator clicks Rand Mods
- **THEN** mod depths randomize on the live state and in every written sequencer step snapshot

### Requirement: Randomization affordances are not duplicated
Desktop v2 SHALL expose at most one surface per randomization command class at default 1280×920 layout: global Rand All, global Rand Mods, sequencer scene-slot dice (Rand-seq), and per-module Randomize/Randmod headers MUST NOT all remain visible simultaneously.

#### Scenario: Module header Randmod removed
- **WHEN** the operator views any carousel module page at 1280×920
- **THEN** per-page Randomize and Randmod header buttons are absent
- **THEN** global Rand All and Rand Mods remain in the global-command band

#### Scenario: No hidden duplicate command cluster
- **WHEN** desktop v2 source is audited for randomization command surfaces
- **THEN** no permanently hidden component duplicates the global-command band's Rand All / Rand Mods cluster (`CenterGlobalClusterV2` removed, not merely invisible)
