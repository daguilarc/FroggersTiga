# froggers-modulation-slate Specification

## Purpose
The fixed fifteen-source modulation slate plus Target/Back cell, app-owned registration (not Sheaf's fixed-count standard-modulator aggregate), a two-level drill-in cap, external-audio sources that stay present-but-inert when unavailable, and the two context-sensitive randomize affordances (Randomize All, Randomize Page).

## Requirements
### Requirement: Fifteen modulation sources plus a Target/Back cell
The app SHALL expose exactly fifteen modulation sources, filling the framework's fifteen modulator slots, with a sixteenth grid cell reserved for Target/Back. The slate SHALL be, **in this order**:

| Slot | Source |
|---|---|
| 1st–6th | Random S&H 1 through Random S&H 6 |
| 7th–9th | VCO 1, VCO 2, VCO 3 audio output |
| 10th–12th | VCO 1, VCO 2, VCO 3 envelope follower |
| 13th | Noise |
| 14th–15th | External audio — audio rate, then envelope follower |
| 16th cell | Target / Back |

The two external-audio sources SHALL be last before Target/Back, because they are the only sources that can become unavailable.

#### Scenario: Slate is complete and ordered
- **WHEN** the modulation detail grid is opened for any parameter
- **THEN** fifteen modulation source cells are present in exactly the order above
- **THEN** the sixteenth cell is Target/Back

### Requirement: The app owns modulator registration
The app SHALL register all fifteen modulation sources itself rather than delegating to the framework's standard-modulator aggregate, whose random count is fixed and cannot supply six sample-and-hold sources. Reusable framework pieces — the noise processor and the standard visualizers — SHALL still be used directly rather than reimplemented.

#### Scenario: No standard-modulator aggregate
- **WHEN** the app's modulator registration is inspected
- **THEN** the framework's standard-modulator aggregate is not used
- **THEN** its fixed-rate random walks and constant source do not appear in the slate

#### Scenario: Framework visualizers are still reused
- **WHEN** the noise and smooth-random sources are rendered
- **THEN** they use the framework's own noise and ganged-random visualizers
- **THEN** no equivalent is reimplemented in the app

### Requirement: Depth cells inherit source color and visualizer
Each modulation depth cell SHALL render in its modulation source's color and SHALL display that source's visualizer as an underlay, without per-cell app code.

#### Scenario: Stepped Random S&H cells show their loop visualizer
- **WHEN** the detail grid is open
- **THEN** each stepped Random S&H source's depth cell shows that source's remembered-loop visualizer
- **THEN** the depth cell's ring uses that source's color

#### Scenario: The smooth source keeps the framework visualizer
- **WHEN** the detail grid is open
- **THEN** the smooth Random S&H source's depth cell shows the framework's ganged-random visualizer
- **THEN** it is visually distinguishable from the stepped sources

### Requirement: Modulation drill-in is capped at two levels
The app SHALL permit drilling from a top-level parameter into its modulation depth grid, and from a depth cell into that depth parameter's own modulation — and SHALL refuse any deeper drill-in. The cap SHALL be enforced by the app, because the underlying framework provides no depth limit and no level stack.

#### Scenario: Second level is permitted
- **WHEN** the operator drills into a modulation depth cell from the detail grid
- **THEN** that depth parameter's own modulation view opens

#### Scenario: Third level is refused
- **WHEN** the operator attempts to drill in from a depth cell at the second level
- **THEN** no further modulation view opens
- **THEN** the current level remains the active editing context

#### Scenario: Target/Back exits to the parameter grid
- **WHEN** the operator activates Target/Back from any modulation level
- **THEN** the bank's parameter grid is restored
- **THEN** no intermediate modulation level is re-entered

Note: a one-level pop is deliberately **not** provided. The framework's deselect returns to the parameter grid, and the call that would re-open an intermediate level is not part of its public surface, so synthesizing a pop would mean working around a private API. Full exit from any level is the accepted behavior (design D5).

### Requirement: External-audio sources stay present but inert when unavailable
When no external audio input is available, the two external-audio modulation sources SHALL be
marked **not connected**. They SHALL remain present in the slate, SHALL be inert, SHALL render as
disconnected, and SHALL NOT be randomized. They SHALL NOT be hidden, and the slate SHALL NOT change
size.

**Availability is defined narrowly, and a host-opened device is not enough.** A channel merely
existing because the host opened some device — including a platform-default device the operator
never chose — SHALL NOT by itself make a source "available". Availability requires a signal that the
operator affirmatively routed audio in. Until the host exposes that signal to the app, the app SHALL
request **zero** audio input channels, so that no device — default or otherwise — is ever opened
without an explicit operator action, and the two sources are disconnected **by construction** rather
than by a compensating flag that a future edit could silently invalidate.

#### Scenario: Disconnection is the inert state, not a removal
- **WHEN** an external-audio source is unavailable
- **THEN** it is marked not connected
- **THEN** its grid cell is still present, carrying no depth parameter
- **THEN** that cell renders in the framework's standard disconnected appearance

#### Scenario: A host-opened default device does not count as available
- **WHEN** the app requests zero audio input channels
- **THEN** no input device, default or otherwise, is opened by the host
- **THEN** both external-audio sources are disconnected as a direct consequence of zero channels
  existing, independent of any separate opt-out flag

#### Scenario: Reconnection restores it in place
- **WHEN** an external audio input becomes available — the host exposes a signal that the operator
  affirmatively routed audio in, and the app requests at least one channel again
- **THEN** the source is marked connected again
- **THEN** its depth parameter materializes on next use, in the same cell position

#### Scenario: No external input connected
- **WHEN** no external audio input is available
- **THEN** both external-audio cells are still shown, rendered as disconnected
- **THEN** they contribute no modulation
- **THEN** the slate still contains fifteen sources in the same order

#### Scenario: Slate size never changes with cabling
- **WHEN** an external input becomes available or unavailable
- **THEN** no modulation cell changes position
- **THEN** existing depth assignments keep their targets

#### Scenario: Randomization skips them
- **WHEN** randomization assigns modulation depths and no external input is available
- **THEN** neither external-audio source receives depth
- **THEN** this follows from their disconnected state, with no separate randomization rule

### Requirement: Randomization uses the framework's authority
The framework's own randomization entry point SHALL remain the sole mutator of randomized modulation-depth values. The app SHALL NOT implement a parallel randomization mutator, and SHALL NOT introduce a per-source randomizability flag beside the connection state the framework already consults. The app SHALL, however, choose the target set passed to a randomization call and the aggregate reach of that call.

#### Scenario: One randomization authority
- **WHEN** the app is inspected for randomization logic
- **THEN** every randomized modulation-depth value is written by the framework's mutator
- **THEN** the app's role is limited to selecting which targets and how much reach are passed into that call, never writing a randomized value itself

### Requirement: Two randomize affordances
The app SHALL provide exactly two randomize affordances: **Randomize All** (global) and **Randomize Page** (per-page). Randomize All, pressed while a parameter page is active, SHALL randomize every parameter value in every bank — including each bank's local Crispy control and excluding the global Crunchy control — plus all first-level modulation depths, and SHALL NOT descend to the second level. Randomize All, pressed while a first-level modulation detail grid is active, SHALL randomize that parameter's depths and SHALL also materialize and randomize their second-level depths. Randomize All pressed at the second level SHALL behave identically to Randomize Page. Randomize Page SHALL always randomize exactly what is displayed: on a parameter page, that bank's values only, with no depths; on a modulation detail grid, that grid's depths only.

#### Scenario: Global press does not create second-level depth
- **WHEN** Randomize All is pressed while a parameter page is active
- **THEN** every bank's parameter values and first-level modulation depths are randomized
- **THEN** no second-level modulation depth is materialized

#### Scenario: A first-level press materializes that parameter's second-level depths
- **WHEN** Randomize All is pressed while a first-level modulation detail grid is active
- **THEN** that parameter's depths are randomized
- **THEN** their second-level depths are materialized and randomized
- **THEN** no other parameter's second-level depths are materialized

#### Scenario: The global Crunchy control is never randomized
- **WHEN** Randomize All or Randomize Page is pressed, at any level
- **THEN** the global Crunchy control's value is unchanged

#### Scenario: A bank's Crispy control is randomized by Randomize All
- **WHEN** Randomize All is pressed while a parameter page is active
- **THEN** each bank's local Crispy control is randomized along with that bank's other parameters

#### Scenario: Randomize Page on a parameter page changes no depths
- **WHEN** Randomize Page is pressed while a parameter page is active
- **THEN** that bank's parameter values change
- **THEN** no modulation depth, at either level, changes

### Requirement: Randomized source count is biased toward few, and depth storage is allocated once
The framework's randomizer SHALL affect zero modulation sources on about half of its calls, and four or more sources only on about one call in sixteen. Depth storage for a given source SHALL be allocated once, on first use, rather than accumulating additional storage across repeated randomization presses.

#### Scenario: Repeated presses do not grow allocated depth parameters
- **WHEN** a randomize affordance is pressed repeatedly against the same target
- **THEN** the number of allocated depth parameters for a given source does not increase across presses
- **THEN** only the depth values change
