# Delta — `froggers-sheaf-parameter-model`

## ADDED Requirements

### Requirement: Comb knob travel is spent on audible change

The comb feedback and Comb/Peak blend knobs SHALL spend their travel on
audible change rather than crowding it into one end. Comb feedback maps each
half of its bipolar travel so the feedback gap falls geometrically — equal
knob steps multiply the loop's ring time by equal ratios — with the center
still exactly zero and the rails still exactly the loop's maximum magnitude.
The Comb/Peak blend crossfades with equal power, so the comb is clearly
present by mid-travel while each extreme remains exactly the single
unblended path. The default patch is unaffected: every default knob position
maps to a value numerically identical to the previous curve's, and the comb
remains inaudible at launch by routing, which is intended.

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
- **THEN** at either extreme the output is exactly the single selected
  branch, bit-identical to the unblended path

#### Scenario: The launch patch is untouched

- **WHEN** the instrument launches with the default patch
- **THEN** every Filter-bank knob maps to a value numerically identical to
  the one the previous curves produced, and the launch sound is unchanged
