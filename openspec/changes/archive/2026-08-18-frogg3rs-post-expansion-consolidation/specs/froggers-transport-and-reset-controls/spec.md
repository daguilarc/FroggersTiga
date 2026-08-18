# Delta — `froggers-transport-and-reset-controls`

**Added 2026-08-13.** A new capability covering four operator-requested controls: a Record button and a
Freeze button beside the existing transport controls, and Reset Page / Reset All below the existing
randomize controls (`../../proposal.md` §8).

Two of these carry a requirement that exists because the obvious reading is wrong. Modulation depth is
bipolar and its OFF value is its centre, not zero — a reset that set depths to literal zero would apply
full negative depth while appearing to clear them. And Sheaf's own selected-state rendering brightens a
button's background without ever changing its text colour, so a control that must genuinely invert has to
draw itself rather than rely on the library's state handling.

## ADDED Requirements

### Requirement: Reset returns parameters to minimum and modulation depths to off
The app SHALL provide a Reset Page control and a Reset All control, which return every parameter to its minimum value and every modulation depth to its neutral, unmodulated position — for the current page and for all pages respectively. Reset SHALL cover the same set of parameters the corresponding Randomize control covers at the same drill level, so that the two are exact inverses of one another in scope.

#### Scenario: Reset Page clears only the current page
- **WHEN** Reset Page is activated
- **THEN** every parameter on the current page is set to its minimum value
- **THEN** every modulation depth on the current page is returned to its off position
- **THEN** no parameter or depth on any other page changes

#### Scenario: Reset All clears every page
- **WHEN** Reset All is activated
- **THEN** every parameter in every bank is set to its minimum value
- **THEN** every modulation depth in every bank is returned to its off position

#### Scenario: A modulation depth's off position is its neutral centre, not zero
- **WHEN** a modulation depth is reset
- **THEN** it is set to the neutral centre that means "this source contributes nothing"
- **THEN** it is NOT set to zero, which for a bipolar depth is full negative modulation rather than off

#### Scenario: Reset matches Randomize's scope at the same drill level
- **WHEN** the operator is drilled into a modulation view and activates a Reset control
- **THEN** it acts on the same set of values the matching Randomize control would act on from that view

### Requirement: Freeze is a latching control that overrides the Freeze parameter's own ceiling
The app SHALL provide a Freeze control beside the transport controls. While latched it SHALL drive the delay's freeze amount to its maximum ignoring the ceiling the Freeze parameter itself is clamped to, so that the latch reaches a state the Freeze encoder cannot; while unlatched the clamped parameter value SHALL prevail. Deactivating the control SHALL restore the clamped value, so the delay's loop gain returns below unity and no sustaining loop survives the latch. The control SHALL show its latched state by inverting its own colours, and SHALL NOT depend on the control library's selected-state rendering to do so.

#### Scenario: One click latches, a second releases
- **WHEN** the Freeze control is clicked while unlatched
- **THEN** it latches on and its colours invert
- **WHEN** it is clicked again
- **THEN** it releases and its colours return to normal

#### Scenario: The latch reaches a state the encoder cannot
- **WHEN** the Freeze encoder is at its own maximum and the Freeze control is unlatched
- **THEN** the delay's loop gain is at unity and the repeats hold rather than growing
- **WHEN** the Freeze control is then latched
- **THEN** the delay's loop gain rises above unity and the repeats grow
- **THEN** the two states are audibly distinct, so the latch is not merely a shortcut for the encoder

#### Scenario: The latch amplifies where the encoder only recirculates
- **WHEN** the Freeze control is latched
- **THEN** the freeze feedback it applies is strictly greater than unity, so the loop adds energy of its own and the repeats grow
- **THEN** it is strictly greater than the value the Freeze encoder produces at its own maximum, at every feedback-drive setting

#### Scenario: Releasing Freeze leaves a decaying tail, not a sustaining one
- **WHEN** Freeze is released
- **THEN** the delay's loop gain returns below unity and its tail decays
- **THEN** no loop is left sustaining as a result of having been frozen

### Requirement: Recording captures in the app core and exports outside it
Where the app records its audio output, the capture SHALL be performed in the app core and the encoding and file writing SHALL be performed in the host layer, because the app core is mechanically barred from resolving any JUCE header. Recording SHALL be refused, with an explanation, when audio is not running.

#### Scenario: The core captures without a host dependency
- **WHEN** recording is armed and audio is running
- **THEN** output samples accumulate in the app core
- **THEN** the app core resolves no host audio-format or file-dialog dependency in doing so

#### Scenario: Recording is refused when audio is stopped
- **WHEN** recording is armed while audio is not running
- **THEN** recording does not start
- **THEN** the operator is told why rather than being left with a silent no-op

#### Scenario: Capture is bounded
- **WHEN** a recording reaches the capture buffer's capacity
- **THEN** capture stops rather than growing without limit
- **THEN** the operator is told the recording was truncated
