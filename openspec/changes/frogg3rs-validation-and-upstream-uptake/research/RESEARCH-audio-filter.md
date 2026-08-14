# Empty-slot parameter research — Audio bank (slots 12–13) & Filter bank (slots 10–13)

Source read: `app/FroggersParameters.hpp` (bank layout, slot numbering, D5a),
`app/dsp/Vco.hpp`, `app/dsp/VoiceEnvelope.hpp` (MixOscVoices/VcoAdsrState),
`app/dsp/FilterFx.hpp` (PadeSaturator/ResonantBump/Comb/PureDelay/FilterFxChain),
`app/dsp/DspMath.hpp` (OnePoleLowPass/ExpMapCompute/BiquadDf1),
`app/dsp/Drive.hpp` (FrogBlock — confirms what the Drive bank already owns, so
Filter-bank candidates don't re-tread it), and `app/FroggersAppCore.hpp` lines
~1180–1355 (confirms current wiring: `MixOscVoices` is an unweighted 1/3+1/3+1/3
average with no balance control, and `scoopNotch.SetFreq/SetWidth` are fed the
exact same `bumpFreq`/`bumpWidth` locals as `peak` — i.e. Scoop currently has
**no independent frequency or width of its own**, only its height/mix knob).

All candidates below were checked against the hard constraints: continuous
across the whole 0–1 sweep with no inaudible/degenerate half, no duplication of
the 15-source modulation matrix (no cross-parameter routing, no new LFO), and
no re-proposal of Peak Slope / VCO Spread / Sub-Osc / Cross XOR / Hard Sync /
Cycle.

---

## AUDIO bank — need 2 (slots 12–13)

Existing 0–11: VCO1-3, Shape1-3, PM1-3, RingMod1-3 (newly decided, out of scope).

### 1. Glide — `Glid` (rank 1, cheapest & safest)
- **0.0** instant retune (today's behavior) · **0.5** audible portamento swoop
  between pitch changes · **1.0** long siren-like glide.
- Precedent: universal analog-synth portamento (Minimoog Glide); Eurorack slew
  modules built specifically for this — ph modular **GLIDE** (2HP utility,
  0–~7s) — [phmodular.com/en/glide-2](https://phmodular.com/en/glide-2/); glide
  described generically as "a slew generator, slew limiter, slope generator, or
  lag" — [learningmodular.com/glossary/portamento](https://learningmodular.com/glossary/portamento/).
- DSP reuse: `dsp::OnePoleLowPass` (already in `DspMath.hpp`) applied to the
  pitch knob/phase-increment target before `Vco::PitchToPhaseIncrement`.
- Cost: **reuses-existing**. Headroom: none — pitch-domain smoothing only.
- Why it earns a slot: cheapest possible implementation (one existing struct,
  no new state beyond what `OnePoleLowPass` already provides) for a control
  every hardware-style synth's operator will expect and currently can't get.

### 2. Feedback — `FBck` (rank 2, most characterful)
- **0.0** clean sine/saw/square per Shape (today) · **0.5** buzzing harmonic
  edge layered onto whatever Shape is dialed · **1.0** chaotic, near-digital
  edge-of-chaos texture.
- Precedent: Yamaha DX7 operator self-feedback —
  [musictech.blog/fm-synthesis-explained](https://musictech.blog/fm-synthesis-explained/),
  [righto.com DX7 reverse-engineering](http://www.righto.com/2021/12/yamaha-dx7-chip-reverse-engineering.html);
  Mutable Instruments Plaits, "2-operator FM with continuously variable
  feedback path" —
  [pichenettes.github.io/.../plaits](https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/);
  Braids FM feedback paths —
  [pichenettes.github.io/.../braids](https://pichenettes.github.io/mutable-instruments-documentation/modules/braids/).
- DSP reuse: composes `dsp::Vco` — one new state field (previous carrier
  output) feeding a phase offset added to `carrierPhase` before
  `EvalWaveMorph`, the same pattern the existing PM-LFO offset already uses.
- Cost: **composes-existing** (small addition to an already-ported struct).
  Headroom: **not flagged** — `EvalWaveMorph`'s three waveforms are each
  bounded to [-1,1] regardless of phase input; feedback rewarps phase, not
  amplitude.
- Why it earns a slot: this instrument's whole vocabulary is XOR/fuzz/bit-
  scramble grit at the Drive stage — self-FM feedback is the oscillator-level
  version of that same idea, and nothing in the Audio bank currently touches a
  single VCO's own harmonic content beyond the Shape morph.

### 3. PM rate — `PMrt` (rank 3)
- **0.0** ~0.05 Hz slow drift · **0.5** a few-Hz vibrato wobble · **1.0** ~20 Hz
  audio-adjacent buzz (the fastest the LFO reaches today).
- Today `pmKnob01` drives *both* the PM LFO's rate (`StepPmLfo`'s
  `ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, ...)`) and its depth
  (`PmDepthScale`) from the same knob — this decouples them, letting the
  existing Phase-mod knobs stay pure depth.
- Precedent: separate rate/depth is close to universal — Oberheim OB-X
  "Destination 1 Depth knob... vibrato speed being set by adjusting the LFO
  rate knob" —
  [gforcesoftware.com/blog/creative-lfo-techniques-in-ob-x](https://www.gforcesoftware.com/blog/creative-lfo-techniques-in-ob-x/);
  rate/depth as the two canonical LFO parameters —
  [theproaudiofiles.com/essential-lfo-parameters](https://theproaudiofiles.com/essential-lfo-parameters/).
- DSP reuse: composes `dsp::Vco::StepPmLfo` — feed a second knob into the
  existing `ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, ...)` call instead of
  reusing `pmKnob01`.
- Cost: **composes-existing**. Headroom: none. One design note: only 2 slots
  remain, so this would be a single knob shared across all three VCOs' PM
  LFOs rather than a per-VCO triple.

### 4. Pulse width — `PW` (rank 4)
- **0.0** thin buzzy pulse · **0.5** symmetric square (today's fixed behavior
  at this point in the morph) · **1.0** inverted thin pulse.
- Precedent: Roland Juno-106 "Pulse Width Modulation Knob" —
  [support.roland.com/.../Juno-106-Technical-Specifications](https://support.roland.com/hc/en-us/articles/201966419-Juno-106-Technical-Specifications);
  Sequential Prophet-5 oscillator PWM via Poly-Mod —
  [sequential.com/product/prophet-5](https://sequential.com/product/prophet-5/).
- DSP reuse: composes `dsp::EvalWaveMorph` (`Vco.hpp`) — the square branch's
  fixed `phaseWrapped01 < 0.5f` threshold becomes a knob-controlled duty
  cycle.
- Cost: **composes-existing** (touches one existing function shared by all
  three VCOs, no new struct). Headroom: none, waveform stays bounded to ±1.
  Implementation caution (not a headroom issue): keep a small floor (~2%)
  around the extremes so duty cycle never fully degenerates to a DC pulse
  train — same spirit as the DC-anchoring fix already applied to
  `DigitalReorganizer` in `Drive.hpp`.

### 5. VCO balance — `Bal` (rank 5, flagged headroom)
- **0.0** VCO1-heavy blend · **0.5** today's fixed equal 1/3+1/3+1/3 mix ·
  **1.0** VCO3-heavy blend.
- Precedent: Minimoog mixer VOLUME knobs per oscillator; Prophet-5 mixer's
  "Oscillator 1 amount / Oscillator 2 amount" controls —
  [grokipedia.com/page/Prophet-5](https://grokipedia.com/page/Prophet-5).
- DSP reuse: composes `dsp::MixOscVoices` — replace the fixed `(1/3,1/3,1/3)`
  weights with a normalized 3-point crossfade.
- Cost: **composes-existing**. **HEADROOM FLAGGED**: must be implemented as a
  true crossfade whose weights always sum to the same total gain as today —
  independent per-VCO gains that can all sit near 1.0 simultaneously would
  raise `chainIn`'s peak above what every downstream stage (Drive's
  polynomial gain, the Filter trims, the master limiter) was tuned against.
- Why it earns a slot: VCO1/2/3's only current variable is pitch — there is
  no way to lean the mix toward one oscillator at all today.

### 6. Noise blend — `Nois` (rank 6, flagged high risk — likely fails constraint 2)
- **0.0** no noise in the audio path · **0.5** audible hiss under the tone ·
  **1.0** pure noise, oscillators inaudible.
- Precedent if kept: Prophet-5's dedicated Noise mixer input alongside
  Osc1/Osc2.
- **RISK**: the noise generator already exists as one of the 15 modulation
  sources reachable by any parameter. Adding it a second time as a direct
  audio-mix ingredient risks reading as exactly the kind of duplication
  constraint 2 rules out ("no route A to B... cross couplers don't make sense
  anymore because of mod lvl 1"), even though this is an audio-mix blend
  rather than a modulation route. Listed for completeness, ranked last, and
  flagged as likely to be rejected on a strict reading.

**Considered and explicitly not listed**: an oscillator-stage wavefolder was
considered and dropped — it would duplicate the Drive bank's existing "Shape"
wavefolder (`dsp::PolynomialDrive::SetCoefs`, `Drive.hpp`), just at a different
point in the signal path.

---

## FILTER bank — need 4 (slots 10–13)

Existing 0–9: Comb offset, Peak freq/gain/Q, Comb delay/feedback/LP,
Comb/Peak blend, Scoop, Filter Topology (parallel↔series, newly decided).

### 1. Scoop freq — `ScFq` (rank 1, reuses-existing, cheapest possible)
- **0.0** scoop parked near 20 Hz (shaves only sub-bass) · **0.5** scoop
  tracks near mid-band, independent of wherever Peak Freq sits · **1.0** scoop
  near 20 kHz (shaves only top air).
- Confirmed gap: `FroggersAppCore.hpp` currently calls
  `filterChain_.scoopNotch.SetFreq(bumpFreq)` — the *identical* local variable
  fed to `peak.SetFreq(bumpFreq)`. Scoop has no frequency of its own today;
  this is a parity-era compromise (the code's own comment: "the scoop notch
  shares the peak bump's freq and width verbatim"), not a creative choice.
- Precedent: independent per-band frequency is baseline parametric-EQ
  practice — [geofex.com "Simple, Easy Parametric and Graphic EQ's"](http://www.geofex.com/article_folders/eqs/paramet.htm);
  Doepfer A-124 Wasp filter's own independent frequency control on its
  notch/bandpass output —
  [signalsounds.com/doepfer-a-124-eurorack-wasp-filter-module](https://www.signalsounds.com/doepfer-a-124-eurorack-wasp-filter-module).
- DSP reuse: **100% reuse** — `dsp::ResonantBump::SetFreq` (`FilterFx.hpp`)
  already exists and is called on `scoopNotch` every sample; only the value
  fed to it changes.
- Cost: **reuses-existing** (a rewiring, zero new DSP code). Headroom: none —
  frequency doesn't touch the notch's dip-gain formula
  (`max(0.05, 1 - 0.95*scoop)`).

### 2. Scoop width — `ScWd` (rank 2, reuses-existing)
- **0.0** wide, gentle EQ-style dip · **0.5** today's shared width · **1.0**
  narrow surgical notch.
- Same gap and precedent as Scoop freq (parametric-EQ Q/bandwidth control).
- DSP reuse: `dsp::ResonantBump::SetWidth`, same struct, same 100%-reuse
  story. Cost: **reuses-existing**. Headroom: none.
- Why #1/#2 earn slots: this is free real estate — the DSP unit
  (`scoopNotch`) is already instantiated and already exposes both setters;
  the only reason Scoop is a single-knob mix today is that nothing wired an
  independent freq/width to it yet.

### 3. Comb drive — `CDrv` (rank 3, flagged headroom)
- **0.0** today's unity-gain feedback loop · **0.5** the saturator starts
  rounding off the comb's resonant peaks, adding grit to the ring · **1.0**
  the feedback loop is driven hard into `PadeSaturator`'s compressive knee,
  roughening every repeat into a buzzier, more clipped resonance.
- Precedent: Korg MS-20 filter Drive knob feeding its nonlinear resonance
  path — "the drive knob's effect... sound starts to 'break up'... the
  incoming sound and the filter resonance are competing" —
  [wiki.synthdiy.com/modules/vcf/ms-20-filter](https://wiki.synthdiy.com/modules/vcf/ms-20-filter/);
  Moog Ladder's input-overdrive Drive control feeding the filter's own
  nonlinearity —
  [prosoundweb.com/the-classic-sound-of-the-moog-ladder-filter](https://www.prosoundweb.com/the-classic-sound-of-the-moog-ladder-filter/).
- DSP reuse: composes `dsp::PadeSaturator` — it already sits inside
  `Comb::Process`'s feedback path (`feedback * PadeSaturator::Saturate(filter.Process(tapped))`);
  add a pre-multiply before that call.
- Cost: **composes-existing**. **HEADROOM FLAGGED**: the comb branch's output
  trim (`combTrimSmoother`, `rawCombTrim = 1/(1+|fb|)`) was measured and tuned
  against a specific worst-case bound (`|comb| <= A + |fb|` at *unity input
  gain* — `FilterFx.hpp`'s W2.2a comment). Driving extra gain into the
  saturator before that bound is computed invalidates the measurement this
  codebase already got burned by once (the extensive comb/peak blowout
  history documented in `FilterFx.hpp`) — the trim would need re-derivation
  and re-measurement against the new worst case before shipping.

### 4. Peak drive — `PkDv` (rank 4, flagged headroom, backstop exists)
- **0.0** clean peaking EQ (today) · **0.5** audible compression/rounding at
  the resonant frequency · **1.0** aggressively overdriven resonant peak,
  closer to screaming analog-filter self-oscillation breakup.
- Precedent: same Moog/MS-20 drive-into-resonance idiom as Comb Drive above,
  applied to the peak branch instead.
- DSP reuse: composes `dsp::PadeSaturator`, inserted after `peak.Process(input)`
  and before `peakTrimSmoother`.
- Cost: **composes-existing**. **HEADROOM FLAGGED**, same class of issue as
  Comb Drive: `peakTrimSmoother`'s `1/height` trim and `peakLimiter`'s tuning
  (`kPeakLimiterThreshold` etc.) were measured against `|peakRaw| <= A*height`
  at unity input gain (the B1 finding cited in `FilterFx.hpp`); a pre-gain
  stage raises that worst case and the limiter's margin needs re-validation —
  though `peakLimiter` (already present downstream, unlike the comb branch)
  gives this one a structural backstop the comb doesn't have.

### 5. Filter input drive — `FDrv` (rank 5, flagged headroom — most severe, likely redundant)
- **0.0** clean (today) · **1.0** the whole Filter bank driven hard, coloring
  everything downstream at once.
- Precedent: the same Moog Ladder lineage, but applied at the "front door" —
  one Drive knob ahead of the whole filter rather than per-branch, which is
  literally how the Moog Ladder's single Drive control works.
- DSP reuse: composes `dsp::PadeSaturator`-style gain, but at a genuinely new
  insertion point (the top of `FilterFxChain::Process`, no existing call site
  to extend).
- Cost: **composes-existing** tooling, new plumbing. **HEADROOM FLAGGED, more
  severely than #3/#4**: this is the earliest gain stage in the chain, so it
  invalidates the worst-case assumptions behind *every* downstream trim/
  limiter at once (comb trim, peak trim, peak limiter) rather than just one
  branch's. Also flagged for **likely redundancy**: the Drive bank already
  owns a dedicated "Drive" parameter (`dsp::PolynomialDrive::SetGain`) — a
  second, differently-implemented "drive" concept in the Filter bank risks
  muddying which knob is "the drive knob" in this instrument's own
  vocabulary. Ranked below Comb/Peak Drive for this reason.

### 6. Comb-offset feedback ("Comb 2") — flagged low-confidence, included only to show it was considered
- Would give `PureDelay` (currently only "Comb offset," a straight
  feed-forward delay) its own feedback path, effectively creating a second
  comb filter ahead of the first.
- **RISK**: this is the same shape of thing as "Peak Slope," which the
  operator already rejected as *"a second cascaded resonant bump."* A second
  cascaded comb is the comb-filter analogue of that same rejected pattern.
  Included last, and only so the option is on record as considered and set
  aside rather than silently skipped.

---

## Top 3 per bank (summary)

**Audio**: (1) **Glide** — reuses `dsp::OnePoleLowPass` outright, zero headroom risk, universal precedent (Minimoog/Eurorack slew). (2) **Feedback** — self-FM via `dsp::Vco`'s own phase state, the oscillator-level version of this instrument's existing grit vocabulary (DX7/Plaits precedent). (3) **PM rate** — decouples an already-existing but conflated rate/depth coupling inside `Vco::StepPmLfo` (OB-X precedent).

**Filter**: (1) **Scoop freq** and (2) **Scoop width** — both 100% reuse of `dsp::ResonantBump::SetFreq/SetWidth`, already instantiated as `scoopNotch`, currently just hard-tied to Peak's own freq/width by parity-era wiring, not by design. (3) **Comb drive** — reuses `dsp::PadeSaturator` already inside the comb feedback loop for MS-20/Moog-style resonance breakup, but flagged: it invalidates the comb branch's measured output-trim bound and needs headroom re-derivation before shipping.
