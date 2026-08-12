# Delta — `froggers-sheaf-parameter-model`

The main spec's "One sixteen-slot bank per Froggers page" requirement currently states every bank holds
nine parameters at slots 0-8 with slots 9-13 empty, uniformly across all six banks. That was true of
every bank until now. This delta carves out the Envelope bank's target 14-parameter slate — decided by
the operator (`../../proposal.md` §3, rulings 3-4) — from that uniform rule, and adds a new requirement
formalizing a fact this change's own research found true today and load-bearing for every future
bank-slate expansion, not only this one: patch persistence is name-keyed, not slot-index-keyed, so
reordering or growing a bank's occupied slots does not corrupt an existing saved patch's values.

**Session 2 (2026-08-11) additions:** Filter, Drive, Delay, and Reverb each carve their own decided slots
out of the same uniform rule, per `../../proposal.md` §9. Reverb's slate reaches COMPLETE at fourteen
parameters, matching Envelope's pattern below; Filter, Drive, and Delay are each partially decided at
this point.

**Session 3 (2026-08-11) additions:** the round-2 research fills every slot session 2 left
open. Filter, Drive, and Delay each now reach **COMPLETE** at fourteen parameters, matching Envelope and
Reverb — **no bank scenario below leaves any slot marked pending any longer except Audio's Ring Mod
gap.** Audio's slate gains two of its remaining five slots (PM Rate, VCO Balance, `../../proposal.md`
§9.5) — its scenario below is updated from the main spec's nine-parameter baseline to eleven, with slots
9-11 (Ring Mod) still explicitly empty, decided in kind but blocked on two open sub-questions
(`../../proposal.md` §4.2). Ring Mod is now the only remaining gap in this entire change's bank-slate
work. The ASR-modulation-source question also remains open (`../../proposal.md` §8) and is deliberately
not reflected here.

**Session 4 (2026-08-11, this update) additions:** a single new binding requirement on VCO Balance
(Audio slot 13) — a mandatory floor and cap on its crossfade weights, per the operator's own ruling
(`../../proposal.md` §4.4, §9.5). Added as a new scenario, "VCO Balance keeps every VCO in the mix," under
the same MODIFIED requirement the other per-bank scenarios already extend. No slot count changes this
session; VCO Balance was already specified at Audio slot 13 as of session 3, this only tightens what it
must do.

**Session 5 (2026-08-12, this update) additions:** Ring Mod (Audio slots 9-11) is corrected in full and
specified — the Audio bank scenario below previously left these three slots empty, described as "decided
in kind but not yet buildable pending its carrier choice and pre/post-gate insertion point." **That
framing was WRONG and is removed, not merely softened** (`../../proposal.md` §3 ruling 1, §4.2, §9.5): Ring
Mod is an ordinary parameter — each VCO has its own ring modulator with an internal carrier, and the knob's
range is that carrier's frequency, mapped across audio rate. There was never a carrier decision, never a
cross-VCO coupling, and never a collision with `froggers-vco-topology`'s "No hardcoded cross-VCO coupling"
requirement. The Audio bank scenario below is rewritten to hold fourteen parameters, complete, matching
Envelope/Filter/Drive/Delay/Reverb. This is the last of the six per-bank scenarios to reach fourteen; the
main spec's uniform nine-parameter rule now has no bank left unmodified by this delta.

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
- **THEN** slot 9 is Topology (short name `Topo`), selecting between the Comb and Peak stages running in
  parallel or in series
- **THEN** slot 10 is Scoop Freq (short name `ScFq`), the Scoop notch's own center frequency, independent
  of the Peak stage's frequency
- **THEN** slot 11 is Scoop Width (short name `ScWd`), the Scoop notch's own width, independent of the
  Peak stage's width
- **THEN** slot 12 is Comb Drive (short name `CDrv`), a pre-gain into the Comb stage's own in-loop
  saturator
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
- **THEN** slots 0-8 are unchanged from the Delay bank's existing nine parameters
- **THEN** slot 9 is Feedback Drive (short name `FbDr`), a pre-gain into the feedback loop's own in-loop
  saturator
- **THEN** slot 10 is Feedback Tone (short name `FbTn`), a one-pole lowpass damping the feedback tap
  ahead of the same in-loop saturator
- **THEN** slot 11 is Mod Rate (short name `MdRt`), the delay's own modulation LFO rate
- **THEN** slot 12 is Width Balance (short name `WBal`), the ratio between the Width knob's time-offset
  spread and its cross-feed blend, independent of the Width knob's own value
- **THEN** slot 13 is Crush (short name `Crsh`), a bitcrush stage applied to the feedback tap's repeats

#### Scenario: The Reverb bank holds fourteen parameters, complete
- **WHEN** the Reverb bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, not nine
- **THEN** slots 0-8 are unchanged from the Reverb bank's existing nine parameters
- **THEN** slot 9 is Mod Rate (short name `MdRt`), the tank's own modulation LFO rate
- **THEN** slot 10 is Tank Drive (short name `TkDv`), a pre-gain into the tank feedback path's own
  in-loop saturator
- **THEN** slot 11 is Grit (short name `Grit`), the tank feedback path routed through a bit-scramble
  stage ahead of that same in-loop saturator
- **THEN** slot 12 is Tilt (short name `Tilt`), a bipolar post-tank tone shave applied before the
  existing wet limiter
- **THEN** slot 13 is Tuned (short name `Tund`), the tank's own delay-line lengths driven directly by
  this parameter's resolved value, with no pitch tracker

## ADDED Requirements

### Requirement: Bank-slate growth is safe for existing saved patches by construction
A patch saved before a bank's occupied parameter slots grow SHALL continue to load every parameter it
named at its own previously-saved value, whether that growth added new parameters or reassigned existing
parameters to different slot indices within the same bank. Parameter identity for the purpose of saving
and loading SHALL be the parameter's own name, never its bank-slot position, so that slot reassignment
cannot cause one parameter's stored value to be silently applied to a different parameter.

#### Scenario: Reordering an occupied bank's existing slots does not swap values
- **WHEN** a bank's existing named parameters are reassigned to different slot indices within that bank
- **THEN** a patch saved before the reassignment still applies each parameter's stored value to that same
  parameter, not to whatever parameter now occupies its old slot index

#### Scenario: A newly added parameter loads at its ordinary default from an older patch
- **WHEN** a patch saved before a bank gained a new parameter is loaded
- **THEN** the new parameter is not present in that patch's saved data
- **THEN** the new parameter reads its own ordinary default value, exactly as any other parameter absent
  from a loaded patch already does

#### Scenario: Modulation depth assignments follow their own source, not the target's slot
- **WHEN** a bank's target parameter is reassigned to a different slot index
- **THEN** any modulation depth already assigned to that parameter from a given source keeps that same
  source's assignment
- **THEN** this holds because modulation depth is stored per modulation-source index, not per target
  slot index
