## MODIFIED Requirements

### Requirement: v2-global-strip-extended-controls

Desktop v2 SHALL provide global randomization actions, global **Crunchy**, and **Shift** in the **center global cluster** within the module/carousel area. Standalone desktop v2 SHALL NOT allocate a bottom global strip row for these controls.

#### Scenario: Preserved v1 randomization buttons

- **WHEN** desktop v2 renders the center global cluster at 1280×920
- **THEN** Rand All, Rand Mods, Rand waveforms, and Rand Resample are present and invoke the same `DesktopHostIO` mutations as v1 (adapted for ADSR and global Crunchy)

#### Scenario: Crunchy encoder in center cluster

- **WHEN** desktop v2 renders the center global cluster
- **THEN** a **Crunchy** encoder ring is visible and labeled **Crunchy**
- **THEN** it controls global fuegoization per `desktop-v2-encoder-rings` / global Crunchy spec

#### Scenario: Shift in center cluster

- **WHEN** desktop v2 renders the center global cluster
- **THEN** Shift is visible and toggles shift-held semantics per `desktop-v2-global-controls` v2-shift-keyboard-and-midi

## REMOVED Requirements

### Requirement: v2-sequencer-global-strip

**Reason:** Sequencer transport (BPM, Steps, Write Seq., Start Sequence) lives in `SequencerPanelComponent` toolbar per `desktop-v2-chrome-sequencer-ux`. Global strip no longer exists on standalone desktop v2.

**Migration:** Use sequencer toolbar for clock/transport; center cluster for globals.

### Requirement: v2-scene-gesture-lfo-vco-buttons

**Reason:** Gesture lanes moved to performance band; LFO/VCO hidden in current v2 build. Center cluster scope is randomize + Crunchy + Shift only.

**Migration:** Gesture controls remain in `PerformanceBandV2`.
