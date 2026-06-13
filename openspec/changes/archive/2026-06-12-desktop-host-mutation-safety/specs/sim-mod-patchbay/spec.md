## ADDED Requirements

### Requirement: Host mutation queue

`DesktopHostIO` SHALL expose a lock-free mutation queue for all burst UI actions that modify engine or page mod state. The audio thread SHALL drain the queue at the start of `tickControls()` before `ProcessBlock`. Message-thread callers SHALL enqueue only; they SHALL NOT call `PageManager` randomize, mod assign, or morph write APIs directly.

#### Scenario: Global Randomize mod enqueued

- **WHEN** the user clicks **Randomize mod (all)** on the global strip during playback
- **THEN** the message thread enqueues one mutation
- **AND** `PageManager` mod indices are updated inside `tickControls` on the audio thread

#### Scenario: Patch assign enqueued

- **WHEN** the user completes a patch-cable drop that assigns a mod source
- **THEN** `SetPageModSource` or `DelayState::setModSource` runs on the audio thread via the queue

#### Scenario: Morph randomize still queued

- **WHEN** the user clicks **Randomize waves**
- **THEN** morph values change inside `tickControls` on the audio thread (same as today)

#### Scenario: Coalesce duplicate randomize-all

- **WHEN** the user clicks **Randomize mod (all)** twice before the next audio block drains the queue
- **THEN** at most one pending global mod-randomize mutation is applied on drain

### Requirement: Sim randomize mod picker

Sim hosts SHALL pick mod sources with **P(none)=0.5** and **P(each of 0, 4, 5, 6)=0.125** when not none. Indices `{1, 2, 3}` SHALL never be assigned by sim randomize paths.

#### Scenario: Desktop per-panel Randomize mod

- **WHEN** the user clicks **Randomize mod** on the Filter panel during playback
- **THEN** only Filter mod assignments are re-randomized with the sim picker on the audio thread

#### Scenario: Delay Randomize mod includes sources

- **WHEN** the user clicks **Randomize mod** on the Delay panel
- **THEN** all eight Delay rows receive new sim-valid `modSource` and `modDepth` values on the audio thread

#### Scenario: Firmware unchanged

- **WHEN** `RandomizeMod` runs on Daisy firmware
- **THEN** full hardware index range `0–6` remains available

### Requirement: Legacy ghost route sanitize

On desktop host initialization, the host SHALL clear any stored mod assignment with index in `{1, 2, 3}` on all core pages and Delay rows.

#### Scenario: Ghost cleared at launch

- **WHEN** persisted state has Drive row 2 with `m_modIndex == 2`
- **THEN** after `DesktopHostIO::Init()` that row reads `255` with no cable drawn

## MODIFIED Requirements

### Requirement: Desktop patch cables (VCV Rack interaction)

Desktop SHALL assign modulation via **VCV Rack-style patch cables**. The user MAY start a drag from **either** a mod **output** jack or a parameter **input** jack (including empty gray inputs). Dragging a connected plug to empty space deletes that route. Drag shorter than 4px from any jack SHALL NOT create or change a cable.

Mod sources SHALL render as module boxes with **output jacks**. Each parameter row on each panel SHALL have a **mod input jack**, including **FUEG** (position 7).

#### Scenario: Drag from empty input instantiates cable

- **WHEN** the user mousedown-drags from a parameter input with `modIndex == 255` (gray ring)
- **THEN** a bezier cable appears after the 4px threshold

#### Scenario: Drop empty-input drag on mod output connects

- **WHEN** the user releases over a mod rack output while dragging from an empty parameter input
- **THEN** that row is assigned to the output mod index and a persistent cable is drawn

#### Scenario: Hover highlights valid output

- **WHEN** the user drags from a parameter input over a mod rack output jack
- **THEN** that output jack is visually highlighted

#### Scenario: Same output drop cancels reassign

- **WHEN** the user grabs a connected input assigned to Marbles 1 and drops on the Marbles 1 output jack
- **THEN** the assignment is unchanged and the cable remains

#### Scenario: Sim-invalid mod index rejected

- **WHEN** the host receives a patch to mod indices 1, 2, or 3
- **THEN** the assignment is rejected

#### Scenario: Port hit radius

- **WHEN** the user releases within `PatchCableOverlay::kPortHitRadius` of a jack center while dragging
- **THEN** the drop counts as a valid connection target

## REMOVED Requirements

### Requirement: Desktop patch cables — empty input no-op scenario

**Reason:** Empty gray input jacks must start cables (VCV parity).

**Migration:** Drag from any input jack or mod output; no pre-patch required.
