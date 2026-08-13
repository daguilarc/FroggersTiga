# Delta — `froggers-vco-topology`

**Added 2026-08-12 (session 6, the OMNI §14 preflight audit).** This delta did not exist through session 5.
It is added because the audit found a **real** collision between two of this change's own DECIDED Audio-bank
parameters and requirements that are already in force in `openspec/specs/froggers-vco-topology/spec.md` —
while sessions 3-5 checked Ring Mod against exactly one requirement in that file (*"No hardcoded cross-VCO
coupling"*, correctly concluding it is satisfied, `../../proposal.md` §4.2) and never read the file's other
requirements at all. The collision the audit found is with **PM Rate** (Audio slot 12), not Ring Mod:

- The in-force requirement *"Froggers oscillator topology is preserved"* states the per-VCO PM LFO's
  *"frequency is an exponential function of the PM knob."* PM Rate's entire purpose is to make that
  frequency a function of a DIFFERENT knob (`../../proposal.md` §0a, §9.5). As written, the requirement
  forbids the parameter.
- Its scenario *"Phase modulation is self-contained"* asserts that when *"any VCO's phase-modulation control
  is raised ... no other VCO's output changes as a result."* PM Rate ships as ONE knob shared across all
  three VCOs' `StepPmLfo` calls (`../../proposal.md` §9.5's own recorded compromise), so moving it changes
  all three VCOs at once.

**This delta relaxes an in-force requirement and therefore needs the operator's confirmation before any
implementer acts on it** (T7.0 in `../../tasks.md`). It is written here rather than left unrecorded because
an unrecorded collision with a live requirement is exactly what a §14 preflight exists to catch; the
alternative — shipping PM Rate against a requirement that forbids it — is not available.

The second MODIFIED requirement below **strengthens** rather than relaxes: it makes Ring Mod's internal
carrier an asserted property of the spec instead of a claim living only in a proposal, so a future
implementer cannot satisfy "Ring Mod" by reading another VCO's signal.

**Session 6 operator ruling adds the third MODIFIED requirement below** (`../../proposal.md` §3a ruling 9,
§4.5). Ring Mod as specified through session 5 was a pure product with no position at which a VCO passed
through unchanged. The operator's answer — a true zero position at the bottom of Ring Mod's own knob,
**implemented as one function shared with phase modulation, not a second copy** — is a generalization of the
requirement this spec already carries for PM, so it is recorded by broadening that requirement rather than by
adding a competing one. Its "one shared function" clause is the spec-level form of the operator's own
instruction; `Vco::PmDepthScale` is the existing implementation being promoted, and its behaviour must not
change in the promotion.

## MODIFIED Requirements

### Requirement: Froggers oscillator topology is preserved
The app SHALL implement the Froggers three-oscillator topology: per-VCO pitch on an exponential map spanning roughly 20 Hz to 20 kHz; a continuous waveform **Shape** morph crossfading sine to saw over the lower half of its range and saw to square over the upper half; and per-VCO phase modulation driven by that VCO's **own** dedicated sine LFO whose frequency is an exponential function of a phase-modulation **rate** control. Each VCO SHALL keep its own LFO instance and its own phase-modulation depth control; the rate control MAY be a single control shared by all three LFOs.

#### Scenario: Shape morph sweeps continuously
- **WHEN** a VCO's Shape control is swept from minimum to maximum
- **THEN** the waveform morphs continuously from sine through saw to square without discontinuity

#### Scenario: Phase-modulation depth is self-contained
- **WHEN** any VCO's phase-modulation depth control is raised
- **THEN** only that VCO's phase is modulated
- **THEN** no other VCO's output changes as a result

#### Scenario: The phase-modulation rate control is shared by design
- **WHEN** the shared phase-modulation rate control is changed
- **THEN** every VCO whose own phase-modulation depth is above zero changes its LFO rate together
- **THEN** a VCO whose own phase-modulation depth is at zero stays unmodulated, unaffected by the rate

### Requirement: No hardcoded cross-VCO coupling
The oscillator section SHALL contain no hardcoded VCO-to-VCO coupling terms. All inter-oscillator routing SHALL be expressed only through the modulation matrix.

#### Scenario: Cross-VCO routing requires an explicit assignment
- **WHEN** no modulation assignment links two VCOs
- **THEN** changing one VCO's parameters does not alter another VCO's output

#### Scenario: Default patch ships ordinary modulation assignments, not topology
- **WHEN** the app starts for the first time
- **THEN** the initial patch includes cross-oscillator modulation assignments at minimal depth
- **THEN** those assignments are ordinary modulation-matrix entries the operator can remove like any other
- **THEN** removing them restores full oscillator independence, with changing one oscillator no longer altering another

#### Scenario: Ring-modulation carriers are internal to their own VCO
- **WHEN** a VCO's ring modulator is processing
- **THEN** its carrier is an oscillator generated inside that VCO's own ring-mod stage
- **THEN** no other VCO's signal is read by that stage, so ring modulation adds no inter-oscillator routing

### Requirement: Phase modulation has a true zero position
The phase-modulation control SHALL be fully inert at its minimum position, with a smooth ramp from that floor into its active range. Every other per-VCO modulation amount that carries a zero position — ring-modulation depth today — SHALL behave the same way, and all of them SHALL derive that ramp from one shared function rather than from a per-control copy of it, each passing its own floor and ramp width so one control can be tuned without changing another's behaviour.

#### Scenario: Minimum position is silent modulation
- **WHEN** a VCO's phase-modulation control is at minimum
- **THEN** that VCO's phase receives zero modulation depth

#### Scenario: Ring modulation is inert at the bottom of its own control
- **WHEN** a VCO's ring-modulation control is at or below its own zero floor
- **THEN** that VCO's signal passes through its ring-mod stage unchanged, at any carrier frequency
- **THEN** raising the control past the floor ramps the ring-modulation amount up smoothly, with no step

#### Scenario: One ramp function serves every such control
- **WHEN** the phase-modulation and ring-modulation depth ramps are computed
- **THEN** both call the same shared ramp function, given their own floor and ramp width
- **THEN** the phase-modulation control's own behaviour is unchanged from before that function was shared
