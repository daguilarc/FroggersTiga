# Delta — `froggers-modulation-slate`

The existing requirement says a disconnected source draws no encoder at all. The
operator has ruled that wrong: an unavailable source should be visible and
visibly unavailable, not blank. A blank cell in a 4x4 grid reads as an empty slot
or a rendering fault, and gives no clue that connecting an input would put
something there.

What the original requirement was protecting stays exactly as it is: a
disconnected source must not be adjustable, must carry no depth parameter, and
must not accept an edit. That was always about INERTNESS, and inertness does not
require invisibility.

## MODIFIED Requirements

### Requirement: External-audio sources stay present but inert when unavailable

The two external-audio sources SHALL remain in the fifteen-source slate at all
times, SHALL be inert while unavailable, and SHALL NOT be eligible for
randomization while unavailable.

**A disconnected source SHALL render as a disabled control, not as nothing.**
While not connected, the two external-audio cells SHALL hold their grid
positions and SHALL draw a visibly de-emphasised encoder — carrying no value
readout and no modulation or gesture indicators, so it reads as present and
unavailable rather than as adjustable.

The de-emphasis SHALL be an explicit neutral colour rather than a dimming of
the source's own. A disconnected source publishes no colour, so there is
nothing to dim: scaling its published value leaves the cell drawn exactly as a
connected one would be. The disabled cell is drained of hue while a connected
cell carries its source's, which is what distinguishes them on screen.

A disabled cell draws no value arc, and that is not an omission: the arc is a
per-voice layer and a disconnected source publishes no voices. It SHALL NOT be
given a synthesised voice to draw one, which would report a value it does not
have.

They SHALL carry no press or drag action and no depth parameter while
disconnected, so there is no edit to accept. Visibility is a rendering choice and
SHALL NOT make a disconnected source reachable: nothing about the disabled
rendering may create an action, a parameter, or a randomization candidate.

The rendering SHALL be the surface's own choice rather than a change to the
shared encoder drawing, so that no other application's disconnected cells change
appearance.

#### Scenario: Disconnection is the inert state, not a removal
- **WHEN** no input is routed
- **THEN** both sources are marked not connected
- **THEN** their grid cells are still present, carrying no depth parameter
- **THEN** those cells carry no press or drag action

#### Scenario: A disconnected cell is drawn, and drawn as unavailable
- **WHEN** the modulation view shows a source that is not connected
- **THEN** that cell emits draw commands rather than none
- **AND** it is visibly de-emphasised against a connected cell in the same view,
  drawn in a neutral colour where the connected cell carries its source's
- **AND** it shows no value readout

#### Scenario: Drawing it does not make it reachable
- **WHEN** a disconnected cell is pressed or dragged
- **THEN** no action is dispatched and no depth parameter is created

#### Scenario: A host-opened default device does not count as routed
- **WHEN** the host has opened a platform-default input device without any operator selection
- **THEN** the routed signal reports not routed
- **THEN** both external-audio sources stay disconnected and contribute no modulation
