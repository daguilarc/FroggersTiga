# Delta — `froggers-sheaf-parameter-model`

## ADDED Requirements

### Requirement: Comb knob travel is spent on audible change

The comb feedback and Comb/Peak blend knobs SHALL spend their travel on
audible change rather than crowding it into one end. Comb feedback maps each
half of its bipolar travel so the feedback gap falls geometrically — equal
knob steps multiply the loop's ring time by equal ratios — with the center
still exactly zero and the rails still exactly the loop's maximum magnitude.
The Comb/Peak blend crossfades with equal power across a floored range:
the knob traverses 0.05 to 0.95 of the crossfade, so the comb is clearly
present by mid-travel and neither branch is ever fully absent — each
extreme holds the other branch near −22 dB. Launch knob positions are
unchanged, and launch, Reset All, and New agree with each other through
the one shared mapping; the launch output carries the comb at the blend
floor, which is accepted. Comb feedback SHALL default to its center — the
zero-feedback point of its bipolar travel — so the floored comb bed is a
single short echo of the dry signal at startup, not a ring.

#### Scenario: Feedback travel is log-linear in ring time

- **WHEN** the comb feedback knob moves outward from center in equal steps
  on either half, with the loop fed then silenced at each step
- **THEN** the measured ring times form equal ratios step to step, within
  tolerance
- **THEN** the center still produces zero feedback and each rail still
  produces the loop's maximum magnitude, numerically identical to before

#### Scenario: The comb is present by mid-blend

- **WHEN** the Comb/Peak blend sits at the middle of its travel
- **THEN** the comb branch contributes at equal power with the peak branch,
  not at half amplitude
- **THEN** at either extreme the selected branch dominates while the other
  branch stays present at the blend floor, near −22 dB, never fully absent

#### Scenario: Launch, Reset All, and New stay in agreement

- **WHEN** the instrument launches with the default patch, and the state is
  also reached through Reset All and through New
- **THEN** all three present the same knob values and the same output,
  including the floored comb bed — a single short echo, feedback at its
  centered zero default — because all three read the one shared mapping
