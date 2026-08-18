# Delta — `froggers-sheaf-parameter-model`

**Added 2026-08-12.** `frogg3rs-bank-expansion` established that every bank holds fourteen parameters and
that each new parameter's default reproduces the value it replaced. Both are now built and test-verified.

This delta adds the property those requirements do not reach: a parameter can be registered, bounded,
default-neutral and still be inaudible, backwards, or musically useless across its range. Nothing in the
existing spec requires a control to actually DO anything.

**The `MODIFIED` section was added 2026-08-13 during this change's own audit (`../../proposal.md` §9).**
It had been missing. `proposal.md` §6.4 and `tasks.md` T3.1 record an operator ruling — DECIDED, not
proposed — that Delay's Detune, Color and Halo are replaced by Freeze, Reverse Blend and Diffusion. The
requirement below said "slots 0-8 are unchanged from the Delay bank's existing nine parameters," and
those three are among the nine. The change therefore contradicted a live requirement while carrying only
`ADDED` deltas. Note `openspec validate --strict` passed the whole time: it checks that a delta is
well-formed, never that a change specified what it decided.

## MODIFIED Requirements

### Requirement: One sixteen-slot bank per Froggers page
Each existing Froggers page SHALL become exactly one bank of sixteen parameter slots. Pages SHALL NOT be
merged. A bank's own parameters SHALL occupy the leading slots; remaining parameter slots MAY be empty,
or MAY hold additional named parameters where a bank's slate has been explicitly decided and expanded.

#### Scenario: Page identity is preserved
- **WHEN** the banks are enumerated
- **THEN** there is one bank per original Froggers page
- **THEN** each bank contains that page's parameters and no other page's

#### Scenario: Sparse banks are valid
- **WHEN** a bank has fewer parameters than available slots
- **THEN** the unused slots render as empty
- **THEN** the occupied slots keep their positions rather than being renumbered

#### Scenario: The Audio bank holds fourteen parameters, complete
- **WHEN** the Audio bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, not nine
- **THEN** slots 0-8 are unchanged from the Audio bank's existing nine parameters, without every one of
  the nine necessarily originating as a page row
- **THEN** three of the original nine are the VCO Shape controls rather than page rows
- **THEN** slots 9 through 11 are Ring Mod (short names `RM1`, `RM2`, `RM3`), one per VCO — each VCO's own
  ring modulator carrying an internal carrier generated inside that VCO's own ring-mod stage, never a
  signal from another VCO
- **THEN** each Ring Mod knob's resolved value sets its own VCO's internal carrier frequency across an
  audio-rate range, mapped the same exponential way the Audio bank's existing pitch knobs already map
  pitch, and each VCO's own signal is multiplied by its own carrier's output
- **THEN** the bottom of each Ring Mod knob's travel is a true zero position, gating that VCO's ring-mod
  amount to exactly zero and ramping smoothly out of it, so the knob is continuous and the VCO can be heard
  unmodulated, per the shared ramp the `froggers-vco-topology` delta requires
- **THEN** each Ring Mod knob defaults to a position at or below that zero floor, so the instrument at its
  defaults sounds exactly as it did before Ring Mod existed
- **THEN** slot 12 is PM Rate (short name `PMrt`), the phase-modulation LFO's own rate, shared across all
  three VCOs and independent of the phase-modulation depth the existing Phase-mod knobs already control
- **THEN** slot 13 is VCO Balance (short name `VBal`), a single tilt sweeping mix emphasis across VCO1
  through VCO3, replacing the fixed equal-thirds average with a constant-total-gain crossfade, subject to
  the floor and cap the "VCO Balance keeps every VCO in the mix" scenario below requires

#### Scenario: VCO Balance keeps every VCO in the mix
- **WHEN** the VCO Balance crossfade weights are computed for any knob position
- **THEN** the three weights SHALL sum to exactly 1
- **THEN** each weight SHALL stay within the range 0.10 to 0.80 inclusive
- **THEN** no knob position reduces any VCO's weight to 0, and no knob position raises any VCO's weight to
  1.0
- **THEN** the three-VCO, 10%-floor arithmetic caps any single VCO's weight at 1 minus two floors, i.e.
  0.80, so the floor and the cap are the same constraint expressed from opposite ends

#### Scenario: The Envelope bank holds fourteen parameters in interleaved ADSR order
- **WHEN** the Envelope bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, not nine
- **THEN** slots 0-3 are Attack VCO1, Decay VCO1, Sustain VCO1, Release VCO1 (short names A1, D1, S1, R1)
- **THEN** slots 4-7 are Attack VCO2, Decay VCO2, Sustain VCO2, Release VCO2 (short names A2, D2, S2, R2)
- **THEN** slots 8-11 are Attack VCO3, Decay VCO3, Sustain VCO3, Release VCO3 (short names A3, D3, S3, R3)
- **THEN** slot 12 is Curve, applying to all three voices' Attack/Decay/Release ramp shape
- **THEN** slot 13 is Grace, a minimum Hold duration so a short gate cannot clip a note before its
  envelope completes Attack and Decay
- **THEN** each voice's Attack ramps to a peak, Decay then falls from that peak to the voice's own
  Sustain target level, and Hold sustains at that level exactly as it does today

#### Scenario: The Filter bank holds fourteen parameters, complete
- **WHEN** the Filter bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, not nine
- **THEN** slots 0-8 are unchanged from the Filter bank's existing nine parameters
- **THEN** slot 9 is Topology (short name `Topo`), a continuous morph of the Comb and Peak stages from
  parallel at one end to series at the other, with no switched positions anywhere in its travel
- **THEN** at its minimum the chain behaves exactly as it does today, with the Peak stage reading the chain's
  own input
- **THEN** at its maximum the Peak stage reads the Comb stage's output instead, which is the series topology
- **THEN** the Comb/Peak blend, the Scoop blend, and every output trim and limiter in the chain stay in
  force at every position of this control, including its extremes
- **THEN** slot 10 is Scoop Freq (short name `ScFq`), the Scoop notch's own center frequency, independent
  of the Peak stage's frequency
- **THEN** slot 11 is Scoop Width (short name `ScWd`), the Scoop notch's own width, independent of the
  Peak stage's width
- **THEN** slot 12 is Comb Drive (short name `CDrv`), a pre-gain applied to the input of the Comb stage's
  own in-loop saturator, never to that saturator's output, so the loop's per-sample bound is unchanged at
  every setting
- **THEN** slot 13 is Scoop Depth (short name `ScDp`), the Scoop notch's own dip depth, independent of
  the same notch's wet/dry blend into the output

#### Scenario: The Drive bank holds fourteen parameters, complete
- **WHEN** the Drive bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, not nine
- **THEN** slots 0-8 are unchanged from the Drive bank's existing nine parameters
- **THEN** slot 9 is Anti-Alias Brightness (short name `ABrt`), the oversampling anti-alias filter's own
  cutoff
- **THEN** slot 10 is Link (short name `Link`), the coupling weight between the Drive knob's resolved
  gain and the Shape stage's own asymmetric coefficients, independent of Drive's and Shape's own values
- **THEN** slot 11 is Fold (short name `Fold`), the pre-fold scale ahead of the sine-fold stage,
  independent of the Drive knob's own gain
- **THEN** slot 12 is Tone (short name `Tone`), a post-chain one-pole lowpass applied after every other
  Drive stage
- **THEN** slot 13 is Bias (short name `Bias`), a DC offset applied before the polynomial waveshaper and
  exactly cancelled afterward so silence-in still produces silence-out

#### Scenario: The Delay bank holds fourteen parameters, complete
- **WHEN** the Delay bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, not nine
- **THEN** slots 0-8 hold nine parameters, of which six — Delay time, Send, Feedback, Stereo width, Mod
  depth and Wet mix — are unchanged from the Delay bank's original slate
- **THEN** the remaining three of those nine are Freeze, Reverse Blend and Diffusion, replacing Detune,
  Color and Halo outright; which of the three vacated slot indices each occupies is not fixed by this
  requirement
- **THEN** no Delay bank parameter's resolved value is folded into another parameter's value, so each of
  the fourteen owns exactly one destination — replacing the arrangement in which Color had no destination
  but was averaged into Detune, and Halo had none but was averaged into Mod depth
- **THEN** slot 9 is Feedback Drive (short name `FbDr`), a pre-gain applied to the input of the feedback
  loop's own in-loop saturator, never to that saturator's output
- **THEN** slot 10 is Feedback Tone (short name `FbTn`), a one-pole lowpass damping the feedback tap
  ahead of the same in-loop saturator
- **THEN** slot 11 is Mod Rate (short name `MdRt`), the delay's own modulation LFO rate
- **THEN** slot 12 is Width Balance (short name `WBal`), the ratio between the Width knob's time-offset
  spread and its cross-feed blend, independent of the Width knob's own value
- **THEN** the cross-feed weight this balance produces stays within 0 to 1 inclusive at every knob position,
  so the left/right feedback pair stays a convex combination of the two delay-line reads
- **THEN** the time-offset spread this balance produces never lengthens a read tap beyond the delay
  buffer's own capacity
- **THEN** slot 13 is Crush (short name `Crsh`), a bitcrush stage applied to the feedback tap's repeats

#### Scenario: The Reverb bank holds fourteen parameters, complete
- **WHEN** the Reverb bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, not nine
- **THEN** slots 0-8 are unchanged from the Reverb bank's existing nine parameters
- **THEN** slot 9 is Mod Rate (short name `MdRt`), the tank's own modulation LFO rate
- **THEN** slot 10 is Tank Drive (short name `TkDv`), a pre-gain applied to the input of the tank feedback
  path's own in-loop saturator, never to that saturator's output
- **THEN** slot 11 is Grit (short name `Grit`), the tank feedback path routed through a bit-scramble
  stage ahead of that same in-loop saturator
- **THEN** slot 12 is Tilt (short name `Tilt`), a bipolar post-tank tone shave applied before the
  existing wet limiter
- **THEN** slot 13 is Tuned (short name `Tund`), the tank's own delay-line lengths driven directly by
  this parameter's resolved value, with no pitch tracker


## ADDED Requirements

### Requirement: Delay's replacement controls are continuous and cannot leave a non-decaying loop
Freeze, Reverse Blend and Diffusion SHALL each be continuous across their range, with a genuinely playable midpoint rather than a discrete mode selected by a knob position. None of them SHALL be able to leave the delay's feedback loop at or above unity gain once the control is returned toward zero. The Freeze PARAMETER SHALL rise monotonically across its whole range and SHALL reach unity feedback at its maximum and no further — lossless recirculation, with no self-amplification — evaluated continuously rather than latched when Freeze is engaged. That ceiling SHALL be expressed in terms of Freeze's own feedback contribution alone and SHALL NOT depend on the delay's feedback-drive pre-gain, so that the control's direction of travel is the same at every feedback-drive setting. It SHALL NOT alter the loop at any Freeze position other than by that monotonic rise, and SHALL NOT bind the transport Freeze latch that deliberately overrides it. Diffusion's allpass coefficient SHALL be held strictly inside the unit circle by its own knob mapping, not by an assertion in a comment. Reverse Blend SHALL smooth its buffer at the forward/reverse crossfade so the control is free of edge-of-buffer clicks across its whole range.

#### Scenario: Full Freeze recirculates losslessly rather than amplifying
- **WHEN** the Freeze parameter is at its maximum
- **THEN** its own feedback contribution is exactly unity, so the loop neither loses nor adds energy of its own
- **THEN** Freeze itself does not amplify at any feedback-drive setting

#### Scenario: Freeze travels the same direction at every feedback-drive setting
- **WHEN** the Freeze parameter is swept from zero to maximum, at any feedback-drive setting including its maximum
- **THEN** the feedback the control produces increases monotonically across that sweep
- **THEN** it never decreases as the control is raised, so the control cannot read backwards at any setting

#### Scenario: Freeze at zero changes nothing
- **WHEN** the Freeze parameter is at zero and the delay's feedback-drive pre-gain is at its maximum
- **THEN** the loop's feedback and pre-gain combine exactly as they did before Freeze existed
- **THEN** the ceiling introduced for Freeze changes nothing about that combination

#### Scenario: Releasing Freeze restores a decaying tail
- **WHEN** Freeze is returned from its maximum toward zero
- **THEN** the loop's effective gain returns below unity and the tail decays
- **THEN** no state latched while Freeze was engaged keeps the loop at unity afterward

#### Scenario: Each control's midpoint is a real playable state
- **WHEN** Freeze, Reverse Blend or Diffusion is set to the middle of its range
- **THEN** Freeze passes new input in at reduced level over a loop that decays more slowly than at zero
- **THEN** Reverse Blend produces a mixed forward-and-reverse texture rather than either extreme
- **THEN** Diffusion produces partially blurred repeats rather than either sharp or fully smeared ones

#### Scenario: Diffusion's coefficient bound is enforced by the mapping
- **WHEN** Diffusion is swept across its whole range
- **THEN** the allpass coefficient it produces stays strictly inside the unit circle at every position
- **THEN** that bound is a property of the knob mapping, so no knob position can violate it

#### Scenario: Reverse Blend does not click at the crossfade
- **WHEN** Reverse Blend is swept across its range while the delay is sounding
- **THEN** the forward/reverse crossfade produces no edge-of-buffer click at any position
- **THEN** the smoothing that achieves this is part of the control, not left to implementation taste

### Requirement: Every bank parameter is audibly effective across its own range
Each parameter a bank exposes SHALL produce an audible change in the instrument's output as it is swept across its range, in the direction its name implies, with no inaudible dead span other than a deliberate zero position at a control's own floor. A control whose only effect is at one extreme, or whose sense is inverted relative to its name, does not satisfy this requirement even if its value is correctly plumbed and correctly bounded.

#### Scenario: Sweeping a parameter changes the sound
- **WHEN** any bank parameter is swept from its minimum to its maximum with the instrument sounding
- **THEN** the output changes audibly across that sweep
- **THEN** the change proceeds in the direction the parameter's name implies

#### Scenario: A deliberate zero position is the only permitted dead span
- **WHEN** a parameter defines a true zero position at the bottom of its travel
- **THEN** that zero region is inert by design and does not violate this requirement
- **THEN** every other part of that parameter's travel is still audibly effective

#### Scenario: A range chosen without a specification is confirmed by ear
- **WHEN** a parameter's range, maximum or shape was chosen by an implementer with no specified value behind it
- **THEN** that choice is confirmed by ear before the parameter is considered done
- **THEN** a range that is technically safe but musically useless is recorded as a defect, not accepted

### Requirement: A measurement that guards a bound is pinned by a regression test
Where a bound on an audio stage was established by measurement, that measurement SHALL be pinned by a regression test rather than left as a one-time result recorded in prose, so that a later change cannot silently invalidate it. A measurement performed in a standalone harness and reported only in a document does not satisfy this requirement.

#### Scenario: A measured bound survives a later change
- **WHEN** a stage's bound was established by measurement and a later change alters that stage
- **THEN** a regression test fails if the measured bound no longer holds

#### Scenario: A test that cannot observe a violation does not count
- **WHEN** a regression test is written to guard a measured bound
- **THEN** it is confirmed to fail when the bound is deliberately broken
- **THEN** a test that passes without exercising the controlling quantity is treated as absent
