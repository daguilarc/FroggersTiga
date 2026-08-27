# Delta — `froggers-sheaf-parameter-model`

The Drive bank's Tone range is a bare literal with nothing behind it: no
requirement mentions it, and the only tests that touch the control pass its
bypass default, so neither end of its travel is asserted. Stated here at the
value this change sets it to, alongside the damping requirement that already
covers the same class of mistake in the Reverb bank.

## ADDED Requirements

### Requirement: The drive tone range excludes the inaudible end

The Drive bank's Tone control SHALL map geometrically onto the one-pole
coefficient of the filter closing the drive chain, with a floor high enough that
its darkest setting is still a tone rather than a mute.

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

- **WHEN** the Tone control is at its minimum
- **THEN** the resulting cutoff is above 500 Hz at a 48 kHz sample rate

#### Scenario: An untouched control removes nothing

- **WHEN** the Tone control is at its default, fully open
- **THEN** the coefficient is exactly 1 and the stage passes its input unchanged

#### Scenario: The mapping is geometric across its whole travel

- **WHEN** the Tone control is at its midpoint
- **THEN** the resulting coefficient is the geometric mean of the coefficients
  at the control's two ends

#### Scenario: Down is darker

- **WHEN** the Tone control is lowered
- **THEN** the coefficient falls, and its cutoff with it
