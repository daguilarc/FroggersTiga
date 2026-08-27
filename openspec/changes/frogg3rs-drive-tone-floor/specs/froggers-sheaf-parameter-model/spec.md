# Delta — `froggers-sheaf-parameter-model`

The Drive bank's Tone range and the Delay bank's Feedback tone range are bare
literals with nothing behind them: no requirement mentions either, and the only
tests that touch these controls pass their bypass default, so neither end of
either travel is asserted. They are also the same control in two places. Stated
here as one requirement over both, at the range this change sets them to, and
alongside the damping requirement that already covers the same class of mistake
in the Reverb bank.

## ADDED Requirements

### Requirement: Tone controls share one range, and it excludes the inaudible end

The Drive bank's Tone and the Delay bank's Feedback tone SHALL map geometrically
onto the one-pole coefficient of the filter each closes, with a floor high
enough that the darkest setting is still a tone rather than a mute.

Both SHALL resolve their coefficient through ONE shared mapping rather than each
computing the range. They are the same control in two positions — a post-stage
low-pass whose knob top is exact bypass — so two expressions of the range would
be two things to keep in agreement by hand. The Reverb bank's damping filter
SHALL NOT share it: its range is narrower and its knob inverted, because it
darkens a tail rather than shaping a signal and never fully opens.

Turning the control DOWN SHALL darken the driven signal: the control's bottom
end maps to the smallest coefficient, and a smaller coefficient is a lower
cutoff. At the control's top the coefficient SHALL be exactly 1, which makes the
stage an exact identity, so an untouched Tone control removes nothing.

The floor exists for the same reason the Reverb bank's damping floor does:
randomization draws each parameter uniformly across its travel, and under a
geometric mapping half of all draws land below the range's geometric mean. A
floor an order of magnitude below anything musical therefore spends most of the
control's travel, and most randomized patches, behind a filter that removes the
signal rather than shaping it. The floor SHALL be chosen so that the geometric
mean of the range is a cutoff a driven signal can still be heard through.

#### Scenario: The darkest setting is still a tone

- **WHEN** either tone control is at its minimum
- **THEN** the resulting cutoff is above 500 Hz at a 48 kHz sample rate

#### Scenario: An untouched control removes nothing

- **WHEN** either tone control is at its default, fully open
- **THEN** the coefficient is exactly 1 and the stage passes its input unchanged

#### Scenario: The mapping is geometric across its whole travel

- **WHEN** either tone control is at its midpoint
- **THEN** the resulting coefficient is the geometric mean of the coefficients
  at that control's two ends

#### Scenario: Down is darker

- **WHEN** either tone control is lowered
- **THEN** the coefficient falls, and its cutoff with it

#### Scenario: The two controls agree by construction

- **WHEN** the Drive Tone and the Delay Feedback tone are set to the same knob
  position, anywhere across the travel
- **THEN** they resolve to the same coefficient, because they read the same
  mapping rather than each computing the range
