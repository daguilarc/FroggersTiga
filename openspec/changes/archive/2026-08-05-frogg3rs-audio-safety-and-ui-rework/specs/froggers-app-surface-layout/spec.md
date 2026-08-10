# Delta — `froggers-app-surface-layout`

Supersedes the predecessor change's scope-band and transport requirements. Both were written before
anyone had seen the running application; the operator has now seen it, and both were wrong in ways
no reading of the code would have revealed.

## MODIFIED Requirements

### Requirement: The scope band is proportioned for reading a waveform
The scope band SHALL be **wider than it is tall**. A waveform display taller than it is wide wastes
the axis that carries information (time) on the axis that does not (amplitude), and it crowds the
rest of the surface.

The band SHALL occupy at most **one third** of the area it occupied when this requirement was
written.

The band's **position SHALL NOT change**. It stays in the upper-left column with the encoder grid in
the column to its right; only its height shrinks. The space the shrink releases **below** the scope
in the left column SHALL be left empty and SHALL NOT be given to the encoder grid — the operator
reserves it for a transport/scene control block (tasks D.6).

*(Amended 2026-07-29. This requirement originally read "the reclaimed space SHALL go to the encoder
grid rather than being left blank". An implementer reasonably read that as licence to restructure,
moved the scope to a full-width top band and gave the grid the whole width beneath. Operator:
"WHEN DID I ASK FOR YOU TO CHANGE THE LOCATION OF IT? i said just the height should change." The
original wording is withdrawn; the reserved-empty region is intended, not an oversight.)*

There SHALL be **one** scope panel. An earlier requirement described two bands, a post-gate audio
band above an envelope-follower band below; the second band is withdrawn. If no second band earns
its space, the single audio-rate panel stands alone.

#### Scenario: The band is landscape, not portrait
- **WHEN** the surface is laid out at the default window size
- **THEN** the scope panel's width exceeds its height
- **THEN** its area is at most a third of its former extent

#### Scenario: Shrinking the band does not move it
- **WHEN** the scope band shrinks
- **THEN** it remains in the upper-left column with the encoder grid to its right
- **THEN** the space it releases below itself in that column is left empty
- **THEN** the encoder grid does not expand into that space

#### Scenario: The band's location is asserted, not assumed
- **WHEN** the layout is under test
- **THEN** an assertion pins the scope panel's position, not only its dimensions
- **THEN** a change that moved the panel while keeping its size would fail

### Requirement: The scope shows only what is sounding
The scope SHALL trace the signal **after** envelope gating, so that a silent instrument renders flat
traces. A scope that animates while nothing is audible misreports the instrument's state; the
operator observed traces moving before the transport had ever been started.

Obtaining the post-gate per-voice values SHALL NOT duplicate the envelope computation at a second
call site.

#### Scenario: Silence renders flat
- **WHEN** the transport has not been started, or the envelope gate is closed
- **THEN** every trace is a flat line

#### Scenario: The envelope is evaluated once
- **WHEN** the post-gate per-voice values are obtained for the scope
- **THEN** they come from the same evaluation that produces the audio output

### Requirement: Trace colours are distinguishable with colour-vision deficiency
Oscillator traces SHALL use **cyan, pink, and yellow**. Hues that differ mainly along the red-green
axis SHALL NOT be used to distinguish traces, because they collapse together for red-green colour
blind viewers — the previous red / orange / yellow set was unreadable for exactly that reason.

#### Scenario: Traces are separable without red-green discrimination
- **WHEN** the three oscillator traces are displayed
- **THEN** they are cyan, pink and yellow
- **THEN** they remain mutually distinguishable when red-green discrimination is unavailable

### Requirement: Transport controls are visible icons that respond to a single click
Play and Stop SHALL be visible, icon-like, and operable with **one** click. An earlier
implementation achieved icons at the cost of requiring a double click; its replacement achieved
single click by removing the icons. Both are failures — the requirement is conjunctive.

Where the pinned toolkit renders custom drawing only on controls that require a double click, the
icon SHALL be expressed as glyph text on a single-click control rather than sacrificing either
property or waiting on an upstream change.

#### Scenario: One click, and it looks like a transport control
- **WHEN** the operator single-clicks Play, then single-clicks Stop
- **THEN** the transport starts and then stops
- **THEN** each control displays a transport glyph rather than a word alone

### Requirement: Controls do not change size with state
A control's label SHALL NOT change length in response to transport or playback state. The
auto-flowed chrome positions controls by width, so a label that grows or shrinks re-flows its
neighbours and makes the surface appear to jump.

#### Scenario: The tempo control holds its position
- **WHEN** the transport starts or stops
- **THEN** the tempo control's label is unchanged
- **THEN** no neighbouring control moves

## ADDED Requirements

### Requirement: No unrequested user-visible behaviour
Features that change what the operator sees or how controls behave SHALL NOT be added on an
implementer's initiative. A discoverability annotation was added to the tempo control that the
operator had not asked for; it was invisible in the backend that shipped it, and once made visible
it re-flowed its neighbours on every transport change.

Where an implementer believes such a change is warranted, it SHALL be proposed and approved before
being built.

#### Scenario: Additions are requested, not volunteered
- **WHEN** a change introduces user-visible behaviour not present in the approved scope
- **THEN** it has been proposed to and approved by the operator beforehand
