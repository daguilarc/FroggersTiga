# Delta — `froggers-modulation-slate`

**Added 2026-08-19 (audit fix: the proposal promised this delta from
the start and the change did not carry it).** Sheaf PR #9 (sar-33)
landed the affirmative routed-input signal at the pinned commit, so the
deployed requirement's interim zero-channels mandate — written for the
era when a channel's existence was indistinguishable from operator
routing — is replaced by the signal it was waiting for.

## MODIFIED Requirements

### Requirement: External-audio sources stay present but inert when unavailable
When no external audio input is routed, the two external-audio modulation sources SHALL be marked
**not connected**. They SHALL remain present in the slate, SHALL be inert, SHALL render as
disconnected, and SHALL NOT be randomized. They SHALL NOT be hidden, and the slate SHALL NOT change
size.

**Availability is defined by the host's affirmative routed signal, and a host-opened device is not
enough.** The app SHALL request one audio input channel and SHALL derive the sources' connected
state exclusively from the host's routed signal — which reports routed only when the operator
affirmatively selected an input (device selection in the standalone and browser; explicit bus
routing in a DAW) — never from a channel or device merely existing. A platform-default device the
host opened unasked SHALL derive not-routed and SHALL NOT connect the sources. The connected state
SHALL be written once per routing transition, from the host's change notification, never recomputed
per sample from channel presence.

#### Scenario: Disconnection is the inert state, not a removal
- **WHEN** no input is routed
- **THEN** both sources are marked not connected
- **THEN** their grid cells are still present, carrying no depth parameter
- **THEN** those cells render in the framework's standard disconnected appearance

#### Scenario: A host-opened default device does not count as routed
- **WHEN** the host has opened a platform-default input device without any operator selection
- **THEN** the routed signal reports not routed
- **THEN** both external-audio sources stay disconnected and contribute no modulation

#### Scenario: Routing connects the sources in place
- **WHEN** the operator affirmatively routes an input and the host's routed signal reports routed
- **THEN** both sources are marked connected
- **THEN** their depth parameters materialize on next use, in the same cell positions

#### Scenario: Unrouting disconnects them again
- **WHEN** the routed input is removed and the routed signal reports not routed
- **THEN** both sources return to the inert, disconnected state
- **THEN** existing depth assignments keep their targets for the next connection

#### Scenario: Slate size never changes with cabling
- **WHEN** an input is routed or unrouted
- **THEN** no modulation cell changes position
- **THEN** the slate still contains fifteen sources in the same order

#### Scenario: Randomization skips them
- **WHEN** randomization assigns modulation depths and no input is routed
- **THEN** neither external-audio source receives depth
- **THEN** this follows from their disconnected state, with no separate randomization rule
