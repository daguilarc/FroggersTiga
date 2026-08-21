# Delta — `froggers-modulation-slate`

**Added 2026-08-21.** The existing requirement was correct and was violated:
the standalone reports routed for a platform-default device, and offers no way
to select no input. This delta restates it with the two things that were left
implicit — that declining is a real, default choice, and that a disconnected
source is visibly unusable rather than merely inert underneath.

## MODIFIED Requirements

### Requirement: External-audio sources stay present but inert when unavailable
The two external-audio modulation sources SHALL connect only on the operator's
affirmative act, SHALL offer declining as a real and default choice, and SHALL
be visibly unusable while disconnected.

When no external audio input is routed, the two external-audio modulation
sources SHALL be marked **not connected**. They SHALL remain present in the
slate, SHALL be inert, SHALL render as disconnected, and SHALL NOT be
randomized. They SHALL NOT be hidden, and the slate SHALL NOT change size.

**Availability is defined by the host's affirmative routed signal, and a
host-opened device is not enough.** The app SHALL request one audio input
channel and SHALL derive the sources' connected state exclusively from the
host's routed signal — which reports routed only when the operator
affirmatively selected an input (device selection in the standalone and
browser; explicit bus routing in a DAW) — never from a channel or device
merely existing. A platform-default device the host opened unasked SHALL
derive not-routed and SHALL NOT connect the sources. The connected state SHALL
be written once per routing transition, from the host's change notification,
never recomputed per sample from channel presence.

**Declining input SHALL be a real choice, and SHALL be the default.** Every
host that presents an input selection SHALL offer an explicit no-input option,
SHALL start on it, and SHALL treat any other selection as the operator's
affirmative act. A host SHALL NOT present "whatever the system provides" as
the only starting state, because that is indistinguishable from a choice
nobody made.

**A disconnected source SHALL be visibly unusable, not merely inert.** While
not connected, the two external-audio cells SHALL render in a disabled
appearance distinguishable from a connected cell at a glance, and SHALL reject
edits rather than accepting an adjustment that does nothing.

#### Scenario: Disconnection is the inert state, not a removal
- **WHEN** no input is routed
- **THEN** both sources are marked not connected
- **THEN** their grid cells are still present, carrying no depth parameter
- **THEN** those cells render in the framework's standard disconnected appearance

#### Scenario: A host-opened default device does not count as routed
- **WHEN** the host has opened a platform-default input device without any operator selection
- **THEN** the routed signal reports not routed
- **THEN** both external-audio sources stay disconnected and contribute no modulation

#### Scenario: The operator can decline input, and starts declined
- **WHEN** a host that selects input devices is opened for the first time
- **THEN** the input selection reads as no input
- **THEN** both external-audio sources are disconnected
- **WHEN** the operator selects an actual device
- **THEN** the routed signal reports routed

#### Scenario: A disconnected cell refuses to be edited
- **WHEN** no input is routed and the operator adjusts an external-audio cell
- **THEN** the cell renders as disabled
- **AND** the adjustment is rejected rather than silently having no effect

#### Scenario: Routing connects the sources in place
- **WHEN** the operator affirmatively routes an input and the host's routed signal reports routed
- **THEN** both sources are marked connected
- **THEN** their depth parameters materialize on next use, in the same cell positions
