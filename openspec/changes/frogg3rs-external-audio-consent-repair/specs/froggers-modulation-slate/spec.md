# Delta — `froggers-modulation-slate`

**Added 2026-08-21. Rewritten 2026-08-21 after preflight.** The existing
requirement derives the sources' connected state correctly and the app honors
it. What it left implicit is the step before: the operator must be able to
decline input, and must start declined. The standalone opens a capture device
at launch instead, and offers no way to say no.

This delta states the declining half, and states the disconnected rendering
the surface already performs so that it is locked rather than assumed.

## MODIFIED Requirements

### Requirement: External-audio sources stay present but inert when unavailable
The two external-audio modulation sources SHALL connect only on the operator's
affirmative act, and declining SHALL be a real and default choice.

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

**Requesting an input channel SHALL NOT open an input device.** A host SHALL
NOT open a capture device on the strength of an application's requested
channel count. It SHALL open one only when the operator has selected a device,
and SHALL then open the channels the application requested.

**Declining input SHALL be a real choice, and SHALL be the default.** Every
host that presents an input selection SHALL offer an explicit no-input option,
SHALL start on it, and SHALL treat any other selection as the operator's
affirmative act. A host SHALL NOT present "whatever the system provides" as an
input choice at all: an unnamed device that resolves to a real microphone is
indistinguishable from a choice nobody made. Output device selection is
unaffected and keeps its system-default entry.

**A persisted input selection SHALL NOT survive a change in what it means.**
When a host reads stored state written before declining became expressible, it
SHALL treat any input device name it finds as unset and start declined, because
a name recorded when there was no way to say no is not evidence that anyone
said yes. Stored output device selections are unaffected.

**A disconnected source SHALL present no control.** While not connected, the
two external-audio cells SHALL hold their grid positions while drawing no
encoder, and SHALL carry no press or drag action, so that there is no edit to
accept and nothing that reads as adjustable.

#### Scenario: Disconnection is the inert state, not a removal
- **WHEN** no input is routed
- **THEN** both sources are marked not connected
- **THEN** their grid cells are still present, carrying no depth parameter
- **THEN** those cells draw no encoder and carry no press or drag action

#### Scenario: A host-opened default device does not count as routed
- **WHEN** the host has opened a platform-default input device without any operator selection
- **THEN** the routed signal reports not routed
- **THEN** both external-audio sources stay disconnected and contribute no modulation

#### Scenario: Requesting a channel opens no device
- **WHEN** an application requesting one input channel starts
- **THEN** the host has no input device open
- **THEN** the input diagnostic reports the requested count with zero active

#### Scenario: An upgraded install starts declined
- **WHEN** a host loads stored state written before no-input was a choice
- **AND** that state names an input device
- **THEN** the input selection reads as no input and no device is opened
- **THEN** the stored output device selection is still honored

#### Scenario: The operator can decline input, and starts declined
- **WHEN** a host that selects input devices is opened for the first time
- **THEN** the input selection reads as no input
- **THEN** both external-audio sources are disconnected
- **WHEN** the operator selects an actual device
- **THEN** that device is opened with the requested input channels
- **THEN** the routed signal reports routed
- **WHEN** the operator selects no input again
- **THEN** the input device is closed and the routed signal reports not routed

#### Scenario: Routing connects the sources in place
- **WHEN** the operator affirmatively routes an input and the host's routed signal reports routed
- **THEN** both sources are marked connected
- **THEN** their depth parameters materialize on next use, in the same cell positions

### Requirement: Randomized source count is biased toward few, and depth storage is allocated once
The randomizer SHALL affect zero modulation sources on about one call in five,
and four or more sources only on about one call in sixteen. Depth storage for a
given source SHALL be allocated once, on first use, rather than accumulating
additional storage across repeated randomization presses.

A parameter the draw leaves at zero sources SHALL carry no modulation depth and
SHALL therefore show no modulation badge, so that a randomized bank reads as a
set of deliberate choices rather than as everything touched at once.

#### Scenario: Some parameters come out of a randomize untouched
- **WHEN** Randomize All is pressed on a parameter page
- **THEN** about one parameter in five carries no modulation depth
- **AND** those parameters show no modulation badge
- **AND** the remaining parameters carry at least one non-neutral depth

#### Scenario: Wide draws stay rare
- **WHEN** modulation depths are randomized repeatedly
- **THEN** four or more sources are affected on about one call in sixteen

### Requirement: Two randomize affordances
The app SHALL provide exactly two randomize affordances: **Randomize All** (global) and **Randomize Page** (per-page). Randomize All, pressed while a parameter page is active, SHALL randomize every page parameter value in every bank plus all first-level modulation depths, SHALL leave every bank's local Crispy control and the global Crunchy control untouched, and SHALL NOT descend to the second level. Randomize All, pressed while a first-level modulation detail grid is active, SHALL randomize that parameter's depths and SHALL also materialize and randomize their second-level depths. Randomize All pressed at the second level SHALL behave identically to Randomize Page. Randomize Page SHALL always randomize exactly what is displayed: on a parameter page, that bank's values including that bank's own Crispy, with no depths; on a modulation detail grid, that grid's depths only.

#### Scenario: The global press leaves Crispy to the page press
- **WHEN** Randomize All is pressed while a parameter page is active
- **THEN** every bank's local Crispy control is unchanged
- **AND** the global Crunchy control is unchanged
- **WHEN** Randomize Page is pressed on that parameter page
- **THEN** that bank's own Crispy control is randomized
