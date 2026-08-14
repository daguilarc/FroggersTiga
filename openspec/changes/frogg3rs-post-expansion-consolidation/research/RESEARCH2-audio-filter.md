# Empty-slot parameter research — ROUND 2 — Audio bank (slots 12–13) & Filter bank (slot 13)

Read first: round 1's rejected file (self-FM and Glide killed by "self-FM obviated by
modulation level 1 ... same concern about glide"), and the operator's hard rule: **if the
15-source modulation matrix can already produce the effect by routing an existing source onto
an existing parameter, the parameter is rejected.** Every candidate below is checked explicitly
against that rule, not just against headroom/continuity.

Source read this round: `app/dsp/Vco.hpp` (full struct), `app/dsp/DspMath.hpp`,
`app/dsp/FilterFx.hpp` (full file — `PadeSaturator`, `ResonantBump`, `Comb`, `PureDelay`,
`FilterFxChain`), `app/dsp/VoiceEnvelope.hpp`, `app/FroggersAppCore.hpp` lines 1180–1399 (the
actual per-sample wiring for Audio and Filter banks, read line-by-line, not from memory), and
`openspec/changes/archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/BANK-EXPANSION-DESIGN.md`
(a prior, already-graded candidate survey — confirms which round-1 picks were taken: Filter
slot 9 Topology, 10 Scoop Freq, 11 Scoop Width, 12 Comb Drive; and confirms Peak Slope,
Cross-VCO FM, Reverb Spread, C.Sign, comb-trim-smoother-rate are all cut by prior explicit
operator ruling, not just my own past guess).

The method this round: instead of brainstorming "features a synth might want," I grepped for
**hardcoded numeric literals and arguments fixed at a call site** inside the DSP that a knob
*could* reach but currently doesn't — the exact shape of the accepted PM Rate example
(`Vco::Process`'s `pmOffset = kPmLfoDepth * PmDepthScale(pmKnob01) * StepPmLfo(pmKnob01, ...)`,
one knob driving two conflated things). Two more of that *exact* shape turned up, cited below.

---

## AUDIO bank — need 2 (slots 12–13)

### 1. PM Depth Max — `PMdp` (NEW this round, rank 1 — strongest match to the accepted shape)

`Vco.hpp:105`: `static constexpr float kPmLfoDepth = 0.15f;` — a compile-time ceiling multiplied
into the phase-mod offset at `Vco.hpp:169`:
```cpp
const float pmOffset = kPmLfoDepth * PmDepthScale(pmKnob01) * StepPmLfo(pmKnob01, sampleRate);
```
Today the Phase-mod knob (Audio slots 6–8) only ever reaches [0, 0.15] of phase excursion —
and per `PmDepthScale` (`Vco.hpp:137-150`), it saturates to that ceiling by knob position
~0.10, so **90% of the existing Phase-mod knob's travel does nothing further to depth at all**.
`kPmLfoDepth` itself has never been anything but 0.15 since the port.

- **0.0** today's ceiling unchanged (0.15, subtle vibrato/PM, current default sound preserved)
  · **0.5** ceiling ~0.5 (audible sideband buzz, FM-adjacent) · **1.0** ceiling 1.0 (full-cycle
  phase wrap — true FM-index territory, harsh sidebands/inharmonic clang).
- **Why routing can't do this**: the 15-source matrix can route any source onto the *existing*
  Phase-mod knob's value, but that only interpolates `pmKnob01` inside `PmDepthScale`, which is
  hard-capped at `1.0 * kPmLfoDepth = 0.15` no matter what drives it or how hard. `kPmLfoDepth`
  is a C++ compile-time constant multiplied at the call site — there is no parameter for any
  source to land on that would rescale it. This is structurally identical to the operator's own
  worked PM-Rate example (two things conflated inside one function; the ceiling is the piece
  nothing can reach), just the OTHER conflated constant in the same formula.
- DSP reuse: 100% reuse of the existing `pmOffset` multiply — only the literal `kPmLfoDepth`
  becomes a runtime value fed from a new knob. No new struct, no new state.
- Cost: **reuses-existing** (cheaper than PM Rate, which needed a second `ExpMapCompute` call;
  this needs zero new math, just a parameter instead of a `constexpr`).
- Headroom: **none, provably** — `EvalWaveMorph` (`Vco.hpp:68-84`) always evaluates to sine/saw/
  square of a *wrapped* phase (`WrapPhase`, `Vco.hpp:170`), each bounded to exactly [-1,1]
  regardless of how large the phase offset is. Raising this ceiling changes *timbre*, never
  amplitude — no `f(0)` concern either, this is a phase argument, not a signal path.
- Precedent: separate depth/index control independent of LFO rate is standard — Moog Sub 37's
  dedicated LFO Rate and Amount (depth) knobs —
  [moogmusic.com/products/sub-37](https://www.moogmusic.com/products/subsequent-37) (manual:
  distinct Rate vs. Amount per LFO); Yamaha DX7 modulation depth vs. speed as the two
  independent Mod Wheel-assignable axes —
  [yamaha.com DX7 documentation summaries](https://en.wikipedia.org/wiki/Yamaha_DX7) (mod wheel
  routed to depth, LFO speed set separately).
- Why it beats the alternatives: bigger, more identity-defining sonic range than PM Rate (goes
  all the way to genuine FM harshness, matching this instrument's "gritty, self-oscillating"
  brief) for *less* implementation cost (no new math function, just parameterizing an existing
  literal).

### 2. PM Rate — `PMrt` (carried over, rank 2 — the example itself)

Unchanged from round 1: `pmKnob01` currently drives both `PmDepthScale` (depth-gate) and, via
`ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, pmKnob01)` inside `StepPmLfo` (`Vco.hpp:157`), the PM
LFO's own rate. Decouples the rate mapping from the existing Phase-mod knob (which becomes pure
depth once PM Depth Max above also ships, or stays as today's odd depth/rate blend otherwise).
- **0.0** ~0.05 Hz slow drift · **0.5** a few-Hz vibrato · **1.0** ~20 Hz audio-adjacent buzz.
- **Why routing can't do this**: same argument as PM Depth Max — `ExpMapCompute(kPmLfoMinHz,
  kPmLfoMaxHz, pmKnob01)` is called with `pmKnob01` as its only free variable; no second
  parameter exists for any of the 15 sources to land on that would move rate independently of
  depth. Routing the Phase-mod knob with any source still moves both at once.
- DSP reuse: composes `Vco::StepPmLfo`'s existing `ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, ...)`
  call — feed it a second knob instead of reusing `pmKnob01`.
- Cost: **composes-existing**. Headroom: none (phase-domain only, same proof as above).
- Precedent: Oberheim OB-X vibrato speed/depth split —
  [gforcesoftware.com/blog/creative-lfo-techniques-in-ob-x](https://www.gforcesoftware.com/blog/creative-lfo-techniques-in-ob-x/).
- Ranked below PM Depth Max only because it needs a shared-across-3-VCOs knob (only 2 slots
  open) and its ceiling (20 Hz) is a smaller sonic delta than PM Depth Max's (0.15 → 1.0 full
  wrap); both are legitimate and the operator may prefer either or both.

### 3. VCO Balance — `VBal` (rank 3, flagged headroom)

`VoiceEnvelope.hpp:271-296`, `MixOscVoices`'s return: `return (v1 + v2 + v3) * (1.0f / 3.0f);` —
a hardcoded, un-parameterized equal-thirds average. No per-VCO level control exists anywhere.
- **0.0** VCO1-heavy blend · **0.5** today's fixed equal thirds (unchanged) · **1.0** VCO3-heavy.
- **Why routing can't do this**: there is no "VCO level" parameter in the bank to route a source
  onto — pitch is each VCO's only knob. The matrix modulates existing parameter *values*; it
  cannot rewrite the fixed `1/3, 1/3, 1/3` literal inside `MixOscVoices`'s return statement.
- DSP reuse: composes `dsp::MixOscVoices` — replace the fixed thirds with a normalized 3-point
  crossfade driven by one knob (e.g. a "tilt" from VCO1-heavy to VCO3-heavy through center).
- Cost: **composes-existing**. **HEADROOM FLAGGED**: must be implemented as true crossfade whose
  weights sum to a constant total (matching today's implicit total gain of 1.0), or a
  simultaneous 1.0/1.0/1.0 weighting would raise `chainIn` above what Drive/Filter's trims were
  tuned against — same caution the BANK-EXPANSION-DESIGN doc raises for any new pre-average
  Audio-bank signal (§3.1's cross-cutting flag).
- Precedent: Minimoog's three independent oscillator VOLUME knobs; Prophet-5 mixer's Oscillator
  1/2 amount controls — [grokipedia.com/page/Prophet-5](https://grokipedia.com/page/Prophet-5).

### 4. Noise Blend — `Nois` (rank 4, flagged — reachable-adjacent, needs care)

- **0.0** no noise ingredient · **0.5** audible hiss floor under the tone · **1.0** pure noise.
- **Why routing can't do this** (re-examined this round, more carefully than round 1): the 15
  sources modulate *parameter values*, not the raw signal path — noise routed onto, say, Shape
  produces random morph jitter, which is audibly different from a steady noise floor summed
  directly into the mix. So this is NOT literally "route Noise onto an existing target" in the
  banned sense. Kept, but ranked below the three above because it is the least novel
  mechanically (a fourth mixer ingredient, not a hardcoded-constant unlock) and, per the
  BANK-EXPANSION-DESIGN doc's own §3.1 flag (its "Sub-Oscillator" row), an ADDITIVE new signal
  into the pre-average mix needs the average turned 4-way (or an independent trim) or it raises
  `chainIn` above the 1.0 bound every downstream Drive/Filter trim assumes — **flag: raises
  output level if not folded into the average.**
- DSP reuse: the noise generator already exists (one of the 15 mod sources' underlying RNG);
  reused as a direct audio ingredient rather than a mod signal.
- Precedent: Prophet-5's dedicated Noise mixer input alongside Osc1/Osc2.

### 5. PM Sens (threshold width) — `PMsn` (rank 5, weakest — flagged for constraint 1)

`Vco.hpp:108-109`: `kPmLfoFloor = 0.02f`, `kPmLfoRampWidth = 0.08f` — the smoothstep window
`PmDepthScale` ramps over (`Vco.hpp:143-149`) before saturating to full depth. Today this window
is fixed at 2%–10% of knob travel, meaning the existing Phase-mod knob is "off" then "fully on"
within its first tenth — the rest of its range currently does nothing (see candidate #1).
- **0.0** narrow window (today, ~2–10%) · **0.5** medium window (~2–50%) · **1.0** wide window
  (~2–100%, i.e. the Phase-mod knob becomes a true linear-feeling depth fader end to end).
- **Why routing can't do this**: `kPmLfoFloor`/`kPmLfoRampWidth` are compile-time constants
  inside `PmDepthScale`; no modulation target exists for them.
- Cost: **composes-existing** (parameterize the smoothstep's own window bounds).
- Headroom: none (same phase-domain bound as #1/#2).
- **Flagged against constraint 1**: this candidate's *purpose* is largely superseded once PM
  Depth Max (#1) ships — with #1, the existing Phase-mod knob's saturation quirk becomes far
  less audible because the ceiling itself is now reachable via a separate knob. Listed for
  completeness, ranked last, likely redundant with #1 rather than genuinely additive.

---

## FILTER bank — need 1 (slot 13)

### 1. Scoop Depth — `ScDp` (rank 1 — same conflation shape as the accepted Scoop Freq/Width)

Confirmed directly in `FroggersAppCore.hpp`: **the same knob, `knob(FroggersBankId::Filter, 8)`,
is read twice for two different jobs**:
```cpp
// :1343-1344 — sets the notch's own dip depth
filterChain_.scoopNotch.SetHeight(std::max(0.05f, 1.0f - 0.95f * knob(FroggersBankId::Filter, 8)));
// :1356 — the SAME knob value also sets the wet/dry blend of that notch into the output
filterChain_.Process(driveOut, /*useParallel=*/true, knob(FroggersBankId::Filter, 7), knob(FroggersBankId::Filter, 8));
```
Inside `Process` (`FilterFx.hpp:748-750`), that second read is `scoopMix`:
`return mixed * (1.0f - scoopMix) + scooped * scoopMix;`. One knob simultaneously sets how deep
the notch dips **and** how much of the notched signal is blended in — exactly the
already-accepted Scoop Freq/Width pattern (one knob doing two independent jobs that happen to
share a value only because nothing separated them), just on the height/blend axis instead of
freq/width.
- **0.0** scoop fully off (height=1, mix=0 — today's floor, unchanged) · **0.5** shallow,
  broadband dip blended at ~50% (subtle EQ-style character, notch itself barely dips) · **1.0**
  today's max: deep notch (height=0.05) fully blended in (unchanged ceiling). Decoupled range
  in between lets, e.g., a *deep* notch blended only lightly (surgical, subtle) or a *shallow*
  dip blended fully (broad, gentle EQ) — states the current single knob cannot reach.
- **Why routing can't do this**: the matrix can route any source onto Filter slot 8's value, but
  both `SetHeight` and `scoopMix` always read that identical value — no routing can move one
  without the other, because they are the same call-site variable, not two parameters.
- DSP reuse: **100% reuse** — `dsp::ResonantBump::SetHeight` (already called every sample on
  `scoopNotch`) and `FilterFxChain::Process`'s existing `scoopMix` blend (already implemented);
  only which knob feeds which argument changes. Same "free real estate" story that got Scoop
  Freq/Width accepted.
- Cost: **reuses-existing** (a rewiring; zero new DSP code, same tier as the already-accepted
  Scoop Freq/Width).
- Headroom: **none** — `SetHeight`'s formula is a dip (`<=1` always, `max(0.05, 1-0.95*x)`), and
  `scoopMix` is a convex blend of two already-bounded signals (`mixed`, `scooped`) — neither can
  raise output level regardless of how the two are decoupled.
- Precedent: independent Depth vs. Mix as two controls on a filter/EQ effect is standard
  practice on notch/parametric processors — e.g. FabFilter Pro-Q's per-band Gain (cut depth) is
  always independent of the plugin's separate global Dry/Wet Mix control —
  [fabfilter.com/products/pro-q-3](https://www.fabfilter.com/products/pro-q-3-equalizer-plug-in);
  classic phaser/vibrato pedals with separate Depth and Mix knobs (e.g. Boss CE-2W) follow the
  same idiom applied to a swept notch.
- Why it beats the alternatives: cheapest possible (zero new DSP, mirrors the exact reasoning
  that got Scoop Freq/Width through), and closes out the scoop stage's parameter set
  symmetrically (freq, width, AND depth/mix all independent, instead of two of three).

### 2. Comb LP Track — `LPTr` (rank 2 — hardcoded ratio locked at a call site)

`FroggersAppCore.hpp:1351-1352`:
```cpp
const float cmlp = dsp::ExpMapCompute(4.0f * combFreq, 20000.0f / sampleRate_, knob(FroggersBankId::Filter, 6));
```
The Comb LP knob's *floor* is locked to exactly `4.0f * combFreq` — four times whatever the Comb
Delay knob (Filter slot 4) currently computes. That `4.0f` is a bare literal at the call site,
never a parameter; the existing Comb LP knob (slot 6) can only ever sweep from that floor up to
a fixed `20000/sr` ceiling.
- **0.0** ratio ~1x (LP floor can reach all the way down to the comb's own fundamental — dark,
  muffled, near-monotone resonance even with the LP knob wide open) · **0.5** ratio ~4x (today's
  fixed behavior, unchanged) · **1.0** ratio ~16x (LP floor stays bright/present even with the
  LP knob at its lowest — resonance never gets very dark, brightness "tracks" less).
- **Why routing can't do this**: the `4.0f` multiplier is a C++ literal multiplied into an
  `ExpMapCompute` argument at this one call site — there is no "Comb LP tracking" parameter for
  any of the 15 sources to land on; routing the existing Comb LP knob only moves within whatever
  range the fixed `4.0f` currently defines.
- DSP reuse: composes the existing `dsp::ExpMapCompute` call — only the `4.0f` literal becomes
  knob-driven. No new struct, no new state, reuses `OnePoleLowPass`/`Comb::SetCutoffAlpha`
  unchanged.
- Cost: **composes-existing**. Headroom: **none** — a one-pole lowpass is non-expansive at any
  cutoff (`OnePoleLowPass::Process`, `DspMath.hpp:63-67`, unity gain at DC, ≤unity elsewhere);
  changing where its floor sits changes tone, never level.
- Precedent: brightness/damping controls that scale relative to a resonator's own fundamental
  (rather than an absolute Hz range) are standard in physical-modeling/plucked-string synthesis
  — Mutable Instruments Rings' "Brightness" parameter shapes the resonator's damping relative to
  its own pitch, independent of structure/pitch —
  [pichenettes.github.io/mutable-instruments-documentation/modules/rings](https://pichenettes.github.io/mutable-instruments-documentation/modules/rings/).
- Why it beats the alternatives after Scoop Depth: genuinely new axis of comb-resonance
  character (this instrument's signature self-oscillating comb) rather than a rewiring, at
  still-trivial implementation cost and zero headroom risk.

### 3. Peak Self-FM — `PkFM` (rank 3, flagged low-confidence, Tier 3, likely too close to the killed pattern)

`ResonantBump::SetFreq`/`UpdateCoefficients` (`FilterFx.hpp:244-247,263-285`) recompute
`cos`/`sin`/`sqrt` from `freq`/`height`/`width` — today called once per sample from a knob read,
never from another signal. This candidate would feed the **comb branch's own output**
(`filterChain_.comb`'s `Process` return) into the peak's `SetFreq`, producing an audio-rate,
self-modulated resonant growl.
- **0.0** static peak freq (today) · **0.5** moderate FM wobble tied to the comb's own ringing ·
  **1.0** strong self-FM, chaotic/growl character.
- **Why routing can't do this — the narrow, load-bearing argument**: the comb's post-filter
  audio output is **not** one of the 15 registered modulation sources (those are 6× Random S&H,
  3× VCO Audio, 3× VCO Envelope Follower, Noise, 2× External — confirmed against
  `app/FroggersModulation.hpp`'s source registration cited in the BANK-EXPANSION-DESIGN doc).
  So unlike VCO self-FM (already reachable: VCO Audio is a registered source that can already
  target pitch), no routing configuration can make the comb's own signal modulate the peak's
  frequency — the comb output literally cannot be selected as a source.
- **Why it's ranked last despite passing the rule**: it rhymes strongly, in spirit, with the
  self-FM idea the operator already rejected for VCOs, and it is genuinely expensive — true
  audio-rate use means `UpdateCoefficients`'s trig/sqrt recompute runs every sample instead of
  once per knob read, a real, unbudgeted CPU cost this filter has never paid. It also compounds
  headroom: a moving center frequency invalidates the *static-height* assumption both the B1
  peak trim and the peak limiter's tuning were derived under (per the BANK-EXPANSION-DESIGN
  doc's own §3.3 row on this exact idea, which independently flags it "Tier 3... needs its own
  re-derivation"). Included only because it is the one candidate that survives the rule via a
  genuinely different argument than PM Depth Max/Comb LP Track — flagged, not recommended.
- Precedent: audio-rate filter-cutoff FM via an internal/external audio signal, distinct from
  envelope/LFO modulation — EMS VCS3/Synthi "filter self-oscillation as an audio-rate FM source"
  tradition; modular VCFs with a dedicated linear-FM CV input driven by audio (e.g. Doepfer
  A-121) — [doepfer.de/a121.htm](https://www.doepfer.de/a121.htm).

---

## Top picks summary

**Audio (pick 2 of 5)**: (1) **PM Depth Max** — unlocks `Vco.hpp`'s hardcoded `kPmLfoDepth=0.15`
ceiling, the twin of the accepted PM-Rate conflation in the same formula; routing can't rescale a
compile-time constant. (2) **PM Rate** — the operator's own worked example, decouples PM LFO rate
from depth in `StepPmLfo`'s `ExpMapCompute` call; no target for rate exists to route onto. (3) as
backup, **VCO Balance** — no per-VCO level parameter exists at all today, so routing has nothing
to attach to; needs a constant-gain crossfade to avoid raising `chainIn`.

**Filter (pick 1 of 3)**: (1) **Scoop Depth** — `Filter,8` is read twice (`SetHeight` at
`FroggersAppCore.hpp:1344` and `scoopMix` at `:1356`) for two different jobs; decoupling is a
zero-new-DSP rewiring, exactly the reasoning that got Scoop Freq/Width accepted. (2) **Comb LP
Track** — the `4.0f` tracking ratio in `cmlp`'s `ExpMapCompute` call (`:1352`) is a bare literal
no source can reach. (3) **Peak Self-FM**, flagged low-confidence — passes the routing test only
because the comb's own output isn't a registered mod source, but is expensive and headroom-heavy.
