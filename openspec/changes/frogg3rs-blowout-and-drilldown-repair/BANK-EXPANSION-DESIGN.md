# Bank Expansion Design — all six banks at full 16-slot occupancy

## 1. Status banner

> **DEFERRED DESIGN. NOT SCHEDULED. NOTHING IN THIS DOCUMENT IS BUILT BY THE CURRENT CHANGE.**
>
> This is a research/design artifact recovered and completed from the archived plan
> `openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md` §J–§K. It
> proposes a full 14-named-parameter slate for every bank; none of the proposed parameters exist in
> code yet except where a row is explicitly marked "today." No task in `tasks.md` or `proposal.md`
> references this document, and none should until the operator picks items off it. Every open
> question below needs the operator, not an implementer, to close.

This document was produced read-only. No file under `app/` was modified while researching or
writing it — a second agent was actively editing `app/` concurrently, and several §K items this
document cites (peak trim, comb trim, delay in-loop saturation, delay/reverb output limiters) had
**already landed** in that work between the archived research (2026-08-05) and this read (2026-08-06).
Every evidence citation below was re-verified against the code as it stands today, not copied from
the archive — see §6 for a list of places the archive is now stale or was wrong.

---

## 2. Current occupancy — read from `FroggersBankLayouts()` (`app/FroggersParameters.hpp:145-186`)

Every bank has the **identical** shape today: 9 named parameters at slots 0-8, **5 empty slots at
9-13**, Crispy at 14, Crunchy at 15. This is enforced structurally, not per-bank: `kFroggersParamsPerBank
= 9` (`FroggersParameters.hpp:77`) sizes every bank's `params` array, and slots 9-13 are simply never
registered (`Init()`, `FroggersParameters.hpp:270-373`, only ever calls `bank.RegisterParameters` for
offset 0 (9 params), offset 14 (Crispy), and offset 15 (Crunchy) — nothing ever touches 9-13).

| Bank | Slots 0-8 (today, named) | Slots 9-13 | Slot 14 | Slot 15 |
|---|---|---|---|---|
| Audio | VCO1, VCO2, VCO3, Shape 1, Shape 2, Shape 3, Phase mod 1, Phase mod 2, Phase mod 3 | empty (5) | Crispy | Crunchy |
| Envelope | Attack VCO1, Sustain VCO1, Release VCO1, Attack VCO2, Sustain VCO2, Release VCO2, Attack VCO3, Sustain VCO3, Release VCO3 | empty (5) | Crispy | Crunchy |
| Filter | Comb offset, Peak freq, Peak gain, Peak Q, Comb delay, Comb feedback, Comb LP, Comb/Peak, Scoop | empty (5) | Crispy | Crunchy |
| Drive | Drive, Shape, SRR 1, SRR 2, XOR, Bit depth, Fuzz, Blend, Phase | empty (5) | Crispy | Crunchy |
| Delay | Delay time, Send, Feedback, Stereo width, Detune, Mod depth, Wet mix, Color, Halo | empty (5) | Crispy | Crunchy |
| Reverb | Wet/dry, Room size, Decay, Pre-delay, Damping, Stereo width, Diffusion, Mod depth, Hold | empty (5) | Crispy | Crunchy |

**30 empty slots total (5 × 6 banks).** Reaching the target shape (14 named + Crispy + Crunchy) needs
exactly 5 new named parameters per bank — 30 across the whole instrument. Crispy and Crunchy are
already fully implemented and out of scope for new proposals.

**Correction to the archive's framing:** the archived §J text said "today's Delay bank has 9 named +
5 empty" as if this were Delay-specific. It is not — all six banks are identically 9+5+1+1. Filed
under §6.

---

## 3. Per-bank 14-parameter slate

Table legend: **Tier 1** = DSP exists, merely unreachable (cite file:line of the unused constant/arg).
**Tier 2** = new code composed from pieces already in this codebase (name the pieces). **Tier 3** =
genuinely new DSP. "Trips §K?" flags whether the proposal can raise a stage's output level and
therefore needs its trim/limiter budget re-derived per the standing rule (tasks.md §K, "Standing rule
going forward").

### 3.1 Audio bank

§K today: "No limiter needed — oscillators are bounded and envelope-gated." Verified still true:
`MixOscVoices` (`app/dsp/VoiceEnvelope.hpp:217+`, ported from the frozen engine's "plain average
return") returns the **average** of the three gated voices, not their sum, so `chainIn` is bounded to
≤1 today. **This bound is load-bearing for every new Audio proposal below** — see the flag under
slots 9-13.

| Slot | Parameter | Reading: 0.0 / 0.5 / 1.0 | Status | Tier | Evidence | Trips §K? |
|---|---|---|---|---|---|---|
| 0 | VCO1 | pitch, 20 Hz–20 kHz exponential | today | — | `Vco.hpp:117-119` | — |
| 1 | VCO2 | pitch, 20 Hz–20 kHz exponential | today | — | same | — |
| 2 | VCO3 | pitch, 20 Hz–20 kHz exponential | today | — | same | — |
| 3 | Shape 1 | wave morph | today | — | `Vco.hpp:68` `EvalWaveMorph` | — |
| 4 | Shape 2 | wave morph | today | — | same | — |
| 5 | Shape 3 | wave morph | today | — | same | — |
| 6 | Phase mod 1 | PM LFO depth | today | — | `Vco.hpp:143-149` | — |
| 7 | Phase mod 2 | PM LFO depth | today | — | same | — |
| 8 | Phase mod 3 | PM LFO depth | today | — | same | — |
| 9 | **Ring Mod** | 0.0 = dry mix (no ring product) / 0.5 = equal crossfade of dry mix and VCO1×VCO2 product / 1.0 = pure ring product | proposed | Tier 2 | `v1`/`v2`/`v3` already exist as separate floats before the mix (`FroggersAppCore.hpp:933-944`), averaged at `MixOscVoices` (`:988`). A convex crossfade inserted before the average is new code, but the pieces (the raw per-VCO floats) already exist. | **Maybe.** `|a·b| ≤ max(|a|,|b|) ≤ 1` for two bounded [-1,1] signals, and a convex crossfade can't exceed its inputs — so on its own this stays ≤1. Flagged because it changes what "the audio bank's output" means for every downstream §K assumption; measure, don't assume. |
| 10 | **Hard Sync** | 0.0 = independent oscillators (today) / 0.5 = partial phase pull toward the master each cycle (`phase *= (1-amount)`) / 1.0 = full sync (slave phase snaps to master's every master cycle) | proposed, **MARGINAL per §J.6** | Tier 2 | `carrierPhase` is a public field (`Vco.hpp:110`, reset at `:210-214`), directly writable; zero-crossing detection on the master is new code. | No — a phase reset doesn't add gain. But it is a hard discontinuity in the waveform every cycle, a broadband click-like transient the master limiter (and any future stage limiter) has to absorb; flagged as a *spectral* concern, not a level one. §J.6 already flags the low end of this knob as plausibly inaudible — prototype before spending the slot. |
| 11 | **VCO Spread** | 0.0 = no spread, all three VCOs independent (today's behavior unchanged) / 0.5 = mild symmetric detune of VCO2/VCO3 from VCO1 / 1.0 = wide spread | proposed | Tier 2 | Pure mapping-layer offset applied to the `knob(FroggersBankId::Audio, ix)` reads at the `Vco::Process` call sites (`FroggersAppCore.hpp:933-944`); `Vco` itself has "zero cross-VCO terms by construction" (`Vco.hpp:22`) so no struct change. | No — pitch offset only, no gain. |
| 12 | **Cross XOR** | 0.0 = VCO1 only, dry / 0.5 = blended bit-combine of VCO1 and VCO2 / 1.0 = fully bit-combined | proposed | Tier 2 (reuses the quantize-and-XOR idiom already in `DigitalReorganizer`, `Drive.hpp:125-163` — but that operates on ONE signal against a computed hash; combining TWO live signals is new plumbing, not a drop-in reuse) | **Possibly — needs measurement.** Bit-XOR of two normalized-to-int signals converted back to float has no guaranteed amplitude relationship to either input; unlike ring mod's `|a·b|≤1` bound there is no equivalent proof here. Flag for headroom re-derivation before shipping. |
| 13 | **Sub-Oscillator** | 0.0 = no sub / 0.5 = moderate blend / 1.0 = full-level sub mixed at unity with VCO1 | proposed | **Tier 3** — nothing in `Vco.hpp` implements frequency division; a sub-oscillator needs its own divider/zero-crossing counter driven by VCO1, genuinely new DSP. | **Yes, explicitly.** This is an ADDITIVE new signal into the pre-average mix. §K's "no limiter needed" verdict for Audio depends on `MixOscVoices` averaging three ≤1 signals; a 4th signal added additively (not itself divided into the average) can push `chainIn` above 1.0 for the first time in this bank's history. Must be folded into the average (4-way) or independently trimmed before it reaches Drive/Filter. |
| 14 | Audio Crispy | fuego bit-scramble | today | — | `Fuegoize.hpp` | — |
| 15 | Crunchy | fuego bit-scramble (global) | today | — | same | — |

**Cross-cutting flag for this bank:** three of the five proposals (Ring Mod, Cross XOR, Sub-Osc)
touch the pre-mix or pre-average signal path that §K's own table currently exempts from any limiter.
If any of these three ship, §K's Audio row ("no limiter needed") must be re-verified, not assumed to
still hold — it was true only because the port never added anything to that path beyond three bounded,
averaged oscillators.

### 3.2 Envelope bank

§J.4 **DECIDED, do not re-litigate: Decay ×3 is in.** A/S/R already occupy 0-8; the operator's own
words: *"decay is an obvious one, yeah"*, and on the alternative framing offered instead of it,
*"that's so stupid. why not just add decay."*

**Slot-order decision made in this document, flagged as a proposal, not an operator ruling:** append
Decay VCO1-3 at slots 9-11 rather than interleaving A/D/S/R and renumbering the existing 0-8. Reason:
§H (deferred, not in this change's scope) records "slot index == PhysicalEncoderId" as the hardware
mapping constraint; renumbering Attack/Sustain/Release out from under their current slot indices would
collide with that constraint the moment §H is picked up. Appending avoids it. **This is exactly the
kind of decision §J.4 says needs the operator, not an implementer — flagged again in §5.**

§J.4's own text leaves the final 2 slots (12-13) open between Curve / Cycle / Grace. The rows below
show my recommended default fill (Curve + Cycle) so the table is complete, but this pick is **not**
settled — see §5.

§K today: "Not a signal stage — it is a gain envelope, nothing to bound." Still true for every
proposal below: none of Decay, Curve, or Cycle add gain; Decay only shapes the fall from peak to
sustain, Curve reshapes ramp timing, Cycle only retriggers the same bounded ASR/ADSR machine.

| Slot | Parameter | Reading: 0.0 / 0.5 / 1.0 | Status | Tier | Evidence | Trips §K? |
|---|---|---|---|---|---|---|
| 0 | Attack VCO1 | time | today | — | `VoiceEnvelope.hpp:127-131` | — |
| 1 | Sustain VCO1 | target level | today | — | `:98-104` | — |
| 2 | Release VCO1 | time | today | — | `:133-137` | — |
| 3 | Attack VCO2 | time | today | — | same | — |
| 4 | Sustain VCO2 | target level | today | — | same | — |
| 5 | Release VCO2 | time | today | — | same | — |
| 6 | Attack VCO3 | time | today | — | same | — |
| 7 | Sustain VCO3 | target level | today | — | same | — |
| 8 | Release VCO3 | time | today | — | same | — |
| 9 | **Decay VCO1** | 0.0 = no decay (Hold jumps straight to Sustain, today's ASR unchanged) / 0.5 = moderate fall time from peak to sustain / 1.0 = long, slow decay | proposed, **DECIDED IN** | Tier 2 | New `Stage::Decay` inserted into the existing `enum class Stage { Idle, Attack, Hold, Release }` (`VoiceEnvelope.hpp:60-65`) and `stepVoice` (`:139-171`), reusing the same per-sample step/`mapTime` idiom `mapAttack`/`mapRelease` already establish (`:127-137`). | No — decay only falls from peak toward sustain, never raises level. |
| 10 | **Decay VCO2** | same reading | proposed, DECIDED IN | Tier 2 | same | No |
| 11 | **Decay VCO3** | same reading | proposed, DECIDED IN | Tier 2 | same | No |
| 12 | **Curve** *(tentative — OPEN, see §5)* | 0.0 = logarithmic (fast rise, long tail) / 0.5 = linear (today's shape) / 1.0 = exponential (slow rise, fast tail), applied globally to all three envelopes' ramps | proposed, one candidate for the 2 open slots | Tier 2 | Reshapes the existing linear step accumulation (`stepVoice`'s `attackStep`/`releaseStep`, `VoiceEnvelope.hpp:142-143`) through an `ExpMapCompute`-style curve — the same idiom already used for `Vco::PitchToPhaseIncrement` and every `ExpMap` mapping in `FilterFx.hpp`/`Reverb.hpp`. | No |
| 13 | **Cycle** *(tentative — OPEN, see §5)* | 0.0 = one-shot (today, no retrigger) / 0.5 = a bounded burst of self-retriggers (2-8 repeats) / 1.0 = infinite self-retrigger loop (envelope-as-LFO) | proposed, other candidate for the 2 open slots | Tier 2 | Reuses the existing `Stage::Release → Stage::Idle` transition (`VoiceEnvelope.hpp:167-171`); Cycle > 0 re-enters `Stage::Attack` instead, gated by a burst counter. | No — retriggering the same bounded envelope doesn't raise its own gain, though a voice that never reaches `Stage::Idle` while Cycle is active interacts with the Stop-clear mechanism (`AllIdle()`-gated per B4/§0) — flagged as an interaction to check, not a §K trip. |
| 14 | Envelope Crispy | fuego bit-scramble | today | — | `Fuegoize.hpp` | — |
| 15 | Crunchy | fuego bit-scramble (global) | today | — | same | — |

**Correction to the archive's §J.3 corollary.** The archive states: *"envelope outputs are likewise
available as modulation sources, so [the Envelope bank's] slots should shape the envelope itself...
rather than route it anywhere."* The premise is **false as read** — checked against the live 15-source
registration (`app/FroggersModulation.hpp:157-171`, `480-577`): the 15 sources are 5× Random S&H, 1×
Random S&H 6 (ganged LFO), 3× VCO Audio, **3× VCO Envelope Follower** (an audio-amplitude follower
derived from each VCO's raw output, `EnvelopeFollowers.hpp`), Noise, and an External Audio pair. There
is **no ASR/ADSR-stage output** among them — "VCO EF" tracks the oscillator's own amplitude, not the
gate/envelope contour applied to it. The corollary's *conclusion* (Envelope bank slots should shape
the envelope's own machine, not route its output) still holds under the general bank-vs-modulation
rule regardless, so nothing above changes — but the stated reason was invented. Filed under §6.

### 3.3 Filter bank

**Flagship Tier 1 finding, confirmed still present today.** `FilterFxChain::Process(float input, bool
useParallel, float combPeakBlend, float scoopMix)` (`app/dsp/FilterFx.hpp:663`) takes a topology switch
as an argument, and the one production call site hardcodes it: `filterChain_.Process(driveOut,
/*useParallel=*/true, ...)` (`app/FroggersAppCore.hpp:1099-1100`). This is a real, unreachable
constant — the highest-value single item in this whole document.

§K today: Filter (comb) done (trim `1/(1+|fb|)` + in-loop saturator), Filter (peak) done (trim
`1/height` + peak limiter), Filter (composite) a known, accepted gap (comb/peak blend is convex so
bounded by `max(combPath, peakPath)`, retargeted to the shared ceiling per §K.3 rather than adding a
third limiter). **Both B1 (peak trim) and W2.2a (comb trim) have landed since the archive was
written** — verified live: `combTrimSmoother`/`peakTrimSmoother` (`FilterFx.hpp:550,563`), both driven
by `kTrimGlideCyclesPerSample` (`:626-630`), applied at `:682` (comb) and `:695-696` (peak).

| Slot | Parameter | Reading: 0.0 / 0.5 / 1.0 | Status | Tier | Evidence | Trips §K? |
|---|---|---|---|---|---|---|
| 0 | Comb offset | pre-delay timing | today | — | `FilterFx.hpp` PureDelay | — |
| 1 | Peak freq | center frequency | today | — | `ResonantBump::SetFreq` | — |
| 2 | Peak gain | height (0-2) | today | — | `ResonantBump::SetHeight` | — |
| 3 | Peak Q | width | today | — | `ResonantBump::SetWidth` | — |
| 4 | Comb delay | delay length | today | — | `FilterFx.hpp` Comb | — |
| 5 | Comb feedback | bipolar −0.95…0…+0.95 | today | — | `Comb::GetFeedback`, `:494-501` | — |
| 6 | Comb LP | damping alpha | today | — | Comb's `lp()` | — |
| 7 | Comb/Peak | parallel blend | today | — | `FroggersAppCore.hpp:1100` | — |
| 8 | Scoop | notch depth (dip, not gain) | today | — | `scoopNotch`, capped ≤1 (`FroggersAppCore.hpp:1093`) | — |
| 9 | **Filter Topology** | 0.0 = fully parallel (today) / 0.5 = half-series (peak sees a blend of raw input and the comb's output) / 1.0 = fully series (peak fed entirely by the comb output, multiplying it) | proposed | **Tier 1** | `FilterFx.hpp:663`'s `useParallel` bool, hardcoded `true` at `FroggersAppCore.hpp:1099-1100`. Exposing it continuously: `peak.Process(lerp(input, combPath, amount))` (§J.6's own construction). | **Yes, explicitly and by name in the standing rule.** Series makes the peak MULTIPLY the comb, which is exactly the case §K.3's convex-bound proof (`filterOut ≤ max(combPath, peakPath)`) does **not** cover — that proof is parallel-only. Even with both B1 and W2.2a's per-branch trims already landed, series composition needs its own headroom derivation from scratch; it is not covered by re-deriving either existing trim alone. |
| 10 | **Comb Drive** | 0.0 = unity (today) / 0.5 = moderate pre-gain into the in-loop saturator / 1.0 = heavy pre-gain (comb rings distorted) | proposed | Tier 2 | Reuses `PadeSaturator::Saturate` already inside the comb loop — same construction as the delay's already-landed feedback saturator. | **Yes, named explicitly in tasks.md's own §K standing rule**: "comb saturator drive requires re-deriving W2.2a's `1/(1+fb)` trim (computed from feedback alone)." |
| 11 | **Peak Slope** | 0.0 = single resonant bump (today) / 0.5 = partial second stage blended in / 1.0 = two cascaded bumps (steeper slope) | proposed | Tier 2 — composes a second `ResonantBump` instance (the type already exists, `scoopNotch` is a second live instance today) chained in series with the first. | **Yes.** Cascading two resonant peaks in series is the same class of concern as Filter Topology's series case — the second stage can multiply the first's output; needs its own headroom proof, cannot borrow B1's single-stage derivation. |
| 12 | **Scoop Freq Offset** | 0.0 = locked to Peak freq (today's actual behavior) / 0.5 = moderate offset / 1.0 = offset to a different register entirely | proposed | Tier 2 — `scoopNotch.SetFreq(bumpFreq)` is currently hardcoded to share the peak's own frequency (`FroggersAppCore.hpp:1094`); decoupling it is a mapping-layer change only. | No — Scoop's height is a DIP capped ≤1 regardless of frequency (`FroggersAppCore.hpp:1093,1256-1257`, "adds no gain at all"); decoupling its center frequency doesn't change that. |
| 13 | **Peak Self-FM** | 0.0 = no self-FM (today) / 0.5 = moderate audio-rate frequency wobble of the peak, driven by the comb's own output / 1.0 = strong self-FM (chaotic/growl character) | proposed | **Tier 3.** `ResonantBump::SetFreq` calls `UpdateCoefficients()` (`FilterFx.hpp:242-247`), which recomputes `cos`/`sin`/`sqrt` every call (`:249-263`) — currently called at most once per audio block from a knob read. True audio-rate self-FM means this trig recompute runs **every sample**, a real, honestly-priced performance cost this codebase does not currently pay anywhere in the filter. | **Yes.** A moving center frequency invalidates the *static-height* assumption both B1's peak trim and the peak limiter's tuning were derived under; needs its own re-derivation, not a reuse of either. |
| 14 | Filter Crispy | fuego bit-scramble | today | — | `Fuegoize.hpp` | — |
| 15 | Crunchy | fuego bit-scramble (global) | today | — | same | — |

**Correction to the archive.** §J.2's ranked-offender table lists "Comb-trim smoother rate" as Tier-1
item #3, "free but not musical." That item is unchanged and still correctly excluded from the
proposals above (the operator: *"definitely don't expose"*) — flagged again here only for
completeness against the instruction to enumerate every Tier-1 citation.

### 3.4 Drive bank

§K today: **"the main gap"** — Drive is bounded to ±1.0 by the `FrogBlock` sine-fold but 1.0 > the
master's 0.9 threshold, so it engages the master unaided even when behaving correctly. §K.1
subsequently found the REAL unbounded stage isn't the seven ported knobs at all — it's the
**already-authored, already-shipped** `DriveBlendPhase` (Blend/Phase, slots 7-8): its allpass
coefficient is read fresh every sample with no smoothing, measured up to **50×** under periodic
phase/content coincidence. That defect is being fixed independently of this document (root-cause fix:
smooth the coefficient) — recorded here because any NEW Drive parameter that feeds into or near that
stage inherits the same caution.

| Slot | Parameter | Reading: 0.0 / 0.5 / 1.0 | Status | Tier | Evidence | Trips §K? |
|---|---|---|---|---|---|---|
| 0 | Drive | gain into `PolynomialDrive` | today | — | `Drive.hpp:87` | — |
| 1 | Shape | polynomial coefficients | today | — | `Drive.hpp:92-100` | — |
| 2 | SRR 1 | sample-rate reduction | today | — | `FroggersAppCore.hpp:1010-1011` | — |
| 3 | SRR 2 | sample-rate reduction | today | — | `:1012-1013` | — |
| 4 | XOR | `DigitalReorganizer::SetFlip` | today | — | `:1014` | — |
| 5 | Bit depth | `DigitalReorganizer::SetHash` | today | — | `:1015` | — |
| 6 | Fuzz | `FrogBlock` blend | today | — | `:1016` | — |
| 7 | Blend | `DriveBlendPhase` dry/wet | today | — | `:1018-1019` | — |
| 8 | Phase | `DriveBlendPhase` allpass amount | today | — | same; **known unbounded to 50× until the §K.1 fix lands** | — |
| 9 | **Anti-Alias Brightness** | 0.0 = dark (heavier post-drive filtering) / 0.5 = today's fixed setting / 1.0 = bright (minimal filtering, more aliasing character let through) | proposed | **Tier 1** | `Oversampler2x::antiAlias.SetAlphaFromNatFreq(0.4f)` fixed in the constructor (`Drive.hpp:124`); grep-confirmed **no other call site anywhere in `app/` sets it.** | No — a one-pole lowpass is non-expansive (unity gain at DC, ≤unity elsewhere); this only removes high-frequency energy, never adds it. |
| 10 | **Sym** | 0.0 = symmetric (today) / 0.5 = mild DC bias before the waveshaper / 1.0 = strong asymmetric bias (even-harmonic-heavy clipping) | proposed | Tier 2/3 — small new DSP (a DC-offset add before `FrogBlock`/`PolynomialDrive`); no bias mechanism exists today. | **Flag, needs measurement.** §K.1 proved `FrogBlock`'s sine-fold bounds ANY real input to ~1.004 including a DC-shifted one (`Sine01` wraps unconditionally) — so peak amplitude likely stays bounded. But a DC bias can shift the OUTPUT's own DC balance, reducing effective headroom on one side of the waveform; do not assume the ~1.004 bound survives unchanged without checking. |
| 11 | **Stages** | 0.0 = one pass (today) / 0.5 = two passes / 1.0 = several cascaded passes through the existing sine-fold | proposed | Tier 2 — recursive reuse of `FrogBlock::Process`, already in the codebase. | **Likely no, but verify.** Each pass individually re-bounds its input to ~1.004 by the same fold argument as §K.1's single-pass measurement, so composition should self-bound — but that measurement characterized ONE pass's input distribution, not N cascaded passes' compounding distribution. Measure before shipping, per §K.4's own lesson ("two consecutive predictions from mechanism-shape were wrong; measurement was right both times"). |
| 12 | **SRR Ratio** | 0.0 = SRR2 fully independent of SRR1 (today) / 0.5 = SRR2 tracks SRR1 at a fixed offset (beating aliasing) / 1.0 = SRR2 locked to SRR1 (unison) | proposed | Tier 2 — a mapping-layer coupling between the two existing `SampleRateReducer::SetFreq` calls (`FroggersAppCore.hpp:1010-1013`). | No — sample-rate reduction is not a gain stage. |
| 13 | **Comp** | 0.0 = no comparator effect (today) / 0.5 = blended zero-crossing squarer / 1.0 = full hard-comparator square conversion | proposed | **Tier 3.** No comparator/squarer stage exists anywhere in `Drive.hpp`; genuinely new DSP. | **Yes, prominently.** A hard zero-crossing comparator outputs ±1 (full scale) for ANY nonzero input, including near-silent signals — this is a genuine, structural gain-raising operation at low input levels, unlike every other Drive stage which is at worst unity-ish. Needs its own headroom treatment from the ground up, not a re-derivation of an existing trim. |
| 14 | Drive Crispy | fuego bit-scramble | today | — | `Fuegoize.hpp` | — |
| 15 | Crunchy | fuego bit-scramble (global) | today | — | same | — |

**Cut from this bank's candidate list, and why:** *Wear* (level-correlated dropout noise, Chow
Tape/ToTape) is a plausible Tier 3 idea but was dropped to keep this bank at 5 new slots — it is the
least evidenced of the survey's Drive candidates and the most speculative Tier 3 item that wasn't
needed to fill the slate. Recorded as a candidate for if/when a slot opens up, not ruled out on
principle.

### 3.5 Delay bank

§K today: **in-loop saturator (B2) and per-channel output limiters (B6a, `wetLimiterL`/`wetLimiterR`)
have BOTH already landed** — verified live at `Delay.hpp:301-302` (saturator) and `:136-153` (the two
`OutputLimiter` instances, B6a's own header comment cites the derivation). This is further along than
the archive's "in flight" status records.

**Correction to the archive, load-bearing for this whole bank's slate.** The existing "Color" (slot 7)
and "Halo" (slot 8) parameters are **not independent DSP stages**. Read directly:
```
params.ddet = std::min(std::max(0.5f * (params.ddet + row7Color), 0.0f), 1.0f);   // Delay.hpp:433
params.dmod = std::min(std::max(0.5f * (params.dmod + row8Halo), 0.0f), 1.0f);    // Delay.hpp:434
```
"Color" is literally averaged into Detune's own value; "Halo" is literally averaged into Mod depth's
own value. Neither is a distinct tone filter or an early-reflections network — they are a v2-era
compatibility fold onto the two real knobs beside them. The archive's §J text discusses "Halo (early
reflections)" as a *future, expensive* idea to price — it did not notice Halo is **already a named
slot today**, doing something much smaller than that name implies. Filed under §6; this changes how
any future Halo/early-reflections proposal should be scoped (it would need to either repurpose the
existing slot's real behavior or accept that "Halo" the name is already spoken for by something else).

**Also found, not in the archive at all — a second Tier-1 item.** The delay's own read-head modulation
LFO rate is a hardcoded constant with no knob, exactly parallel to Reverb's already-known `kModLfoHz`:
```cpp
lfoInc = 2.0f * 3.14159265f * 0.25f / sampleRate;   // Delay.hpp:236, inside Configure(sampleRate)
```
Only `dmod` (Mod depth, today's slot 5) is knob-exposed; the 0.25 Hz rate itself is unreachable and
depends on nothing but sample rate.

| Slot | Parameter | Reading: 0.0 / 0.5 / 1.0 | Status | Tier | Evidence | Trips §K? |
|---|---|---|---|---|---|---|
| 0 | Delay time | `ExpMapCompute` time | today | — | `Delay.hpp:263` | — |
| 1 | Send | wet send level | today | — | | — |
| 2 | Feedback | clamped ≤0.98 | today | — | `Delay.hpp:284` | — |
| 3 | Stereo width | L/R spread | today | — | `Delay.hpp:267,281` | — |
| 4 | Detune | folded with Color, see above | today | — | `Delay.hpp:433` | — |
| 5 | Mod depth | folded with Halo, see above | today | — | `Delay.hpp:434` | — |
| 6 | Wet mix | dry/wet | today | — | `toReverbMono` call | — |
| 7 | Color | folds into Detune (0.5× average) | today | — | `Delay.hpp:433` — **not an independent tone control** | — |
| 8 | Halo | folds into Mod depth (0.5× average) | today | — | `Delay.hpp:434` — **not early reflections** | — |
| 9 | **Feedback Drive** | 0.0 = unity, today's behavior / 0.5 = moderate pre-gain, repeats saturate audibly / 1.0 = heavy pre-gain, dub-style self-distorting repeats | proposed | Tier 2 — near-Tier-1; the saturator this scales into already exists: `WriteSample(inSignal + fbk * PadeSaturator::Saturate(fbL), lineL)` (`Delay.hpp:301-302`). Only a scalar multiply before `Saturate()` is new. | **Yes, explicitly.** Same class as the Filter bank's Comb Drive — driving harder into an in-loop saturator raises the loop's sustained level in a way no existing trim accounts for (there is no delay OUTPUT trim today, only B6a's downstream limiter as a backstop). |
| 10 | **Feedback Tone** | 0.0 = dark (heavy damping on repeats) / 0.5 = neutral / 1.0 = bright (today's default — no damping at all) | proposed | Tier 2 — reuses `dsp::OnePoleLowPass` (`DspMath.hpp:56-76`), the comb's own precedent, inserted into the feedback path; grep-confirmed **no filtering of any kind exists in the delay loop today.** | No, on its own — a one-pole lowpass is non-expansive. Interacts with Feedback Drive if both ship (tone shapes what the drive stage saturates), worth testing together, not a §K trip by itself. |
| 11 | **Delay Mod Rate** | 0.0 = slow (well under today's 0.25 Hz, glacial drift) / 0.5 = today's fixed rate (~0.25 Hz) / 1.0 = fast (chorus-adjacent, several Hz) | proposed | **Tier 1** | `lfoInc` fixed at `Delay.hpp:236`, see above. | No — rate change only; `dmod`/Halo's fold already gates the depth. |
| 12 | **Diffusion** | 0.0 = discrete clean repeats (today) / 0.5 = moderately smeared repeats / 1.0 = fully diffused wash | proposed | **Tier 3** — no allpass/diffusion network exists in `Delay.hpp`; genuinely new. | Likely no if built as a standard unity-gain (Schroeder-style) allpass chain, but flag for verification — a poorly-tuned allpass network can leak gain. |
| 13 | **Freeze** | 0.0 = normal, writes continue (today) / 0.5 = partial freeze, new input bleeds in at reduced level while the loop rings on / 1.0 = full freeze, write disabled, only the captured loop plays | proposed | **Tier 3** — needs loop-mute + write-disable semantics that don't exist; relates to the same mechanism B4's Stop-clear needed. | Marginal/verify — a frozen loop with Feedback Drive engaged could ring at the saturator's own ±1 ceiling indefinitely; B6a's downstream output limiter already sits after this stage as a backstop, but "already has a backstop" is not the same as "verified safe," per the standing rule's own wording. |
| 14 | Delay Crispy | fuego bit-scramble | today | — | `Fuegoize.hpp` | — |
| 15 | Crunchy | fuego bit-scramble (global) | today | — | same | — |

### 3.6 Reverb bank

§K today: **B6b (reverb output limiter) has already landed**, matching Delay's B6a — verified live,
`Reverb.hpp` includes `Limiter.hpp` and defines `kReverbWetLimiterThreshold` etc. (`:58-115`). Further
along than the archive's "in flight" status.

§K.3's exhaustive chain sweep names the reverb's `aIn = preOut + aFb*fb` / `bIn = preOut + bFb*fb` sums
(`Reverb.hpp:348-349`) as one of only two unbounded gain-bearing operations in the ENTIRE signal chain
(the other, `DriveBlendPhase`, is §K.1's already-in-progress fix). B6b's limiter is the current
treatment for that.

| Slot | Parameter | Reading: 0.0 / 0.5 / 1.0 | Status | Tier | Evidence | Trips §K? |
|---|---|---|---|---|---|---|
| 0 | Wet/dry | dry/wet blend | today | — | | — |
| 1 | Room size | `ExpMap(0.05,1.0)` | today | — | `Reverb.hpp:268` | — |
| 2 | Decay | `ExpMap(0.1,0.98)` feedback | today | — | `:271` | — |
| 3 | Pre-delay | `ExpMap` normalized | today | — | `:274-276` | — |
| 4 | Damping | `ExpMap` alpha, direct | today | — | `:280` | — |
| 5 | Stereo width | L/R blend | today | — | | — |
| 6 | Diffusion | cross-feed between tank lines A/B | today | — | `:344-349` | — |
| 7 | Mod depth | LFO offset depth | today | — | `:321` | — |
| 8 | Hold | feedback ceiling, `<0.999` | today | — | `:342` | — |
| 9 | **Reverb Mod Rate** | 0.0 = slow / 0.5 = today's fixed 0.35 Hz / 1.0 = fast | proposed | **Tier 1** | `kModLfoHz = 0.35f` fixed (`Reverb.hpp:129`); `kModDepth` already knob-driven at slot 7. | No — rate change only. |
| 10 | **Tank Drive** | 0.0 = clean (today) / 0.5 = warm compression on the tail / 1.0 = heavily saturated tape-like tail | proposed | Tier 2 — reuses `PadeSaturator`, inserted into the `aIn`/`bIn` feedback sums (`Reverb.hpp:348-349`) that §K.3 already names as the tank's one unbounded gain path. | **Yes, explicitly.** Same class as Comb Drive / Delay Feedback Drive. B6b's output limiter is already downstream and provides a backstop, but per the standing rule this still needs its own budget re-derivation, not a bare "the limiter will catch it." |
| 11 | **Early Reflections** | 0.0 = none, tank feeds directly (today) / 0.5 = moderate slap-back taps / 1.0 = pronounced discrete early reflections before the diffuse tail | proposed | **Tier 3** — no multi-tap early-reflection network exists; genuinely new DSP. | Needs its own check once built — a multi-tap summed network is a classic place to accidentally introduce unbounded gain if tap levels aren't normalized. |
| 12 | **Shimmer** | 0.0 = no shimmer / 0.5 = moderate octave-up blend fed into the feedback path / 1.0 = full shimmer wash | proposed | **Tier 3, priced honestly per the evidence standard: this codebase has no pitch shifter anywhere** (grep-confirmed, `app/dsp/*.hpp`). This is new DSP of real size, not a small addition. | Not assessable until the shifter exists; flag that a pitch-shifted signal re-injected into feedback is its own gain-staging question, layered on top of the existing `aIn`/`bIn` concern. |
| 13 | **Tuned** | 0.0 = free-running tank (today) / 1.0 = fully pitch-tracked tank (tank delay times retuned to the played pitch) | proposed | **Tier 3, most expensive item in this document.** Needs both a pitch shifter (absent, see Shimmer) AND pitch tracking of the incoming signal (also absent — nothing in this codebase estimates pitch from audio). Two missing subsystems, not one. | Same open question as Shimmer, compounded. |
| 14 | Reverb Crispy | fuego bit-scramble | today | — | `Fuegoize.hpp` | — |
| 15 | Crunchy | fuego bit-scramble (global) | today | — | same | — |

**Cut from this bank, and why — a correction to the archive, not a restatement of an existing
ruling.** The archive's §J.5/§J.6 list "Reverb Spread" among the natively-continuous, near-free
candidates. **It duplicates the bank's own existing slot 5, "Stereo width," which is already a named
parameter today.** The archive proposed adding a knob for something the bank already has. Cut on
sight; filed under §6 as a second instance of the same failure mode as the already-known C.Sign error
(a candidate proposed by name without checking it against the live layout).

---

## 4. Cross-bank conflicts and overlaps

1. **Filter Comb Drive vs Delay Feedback Drive vs Reverb Tank Drive vs Drive bank's Fuzz/Shape.** Four
   different "push a saturator harder" controls across three banks plus the existing Drive bank. The
   archive already flagged Comb/Peak-Drive vs Drive-bank-Fuzz/Shape as needing resolution; this
   document adds two more instances of the identical pattern (Delay, Reverb) that the archive never
   considered because §J only researched Delay in depth. **All four should sound audibly distinct by
   design** (Drive bank's Fuzz operates pre-filter on the dry signal; Comb/Delay/Tank Drive each color
   a specific resonant/feedback loop) but this needs an operator ear pass across all four before any
   ship, not an assumption that "different DSP location" automatically means "different enough
   character."
2. **Drive bank's Sym vs the now-cut Filter C.Sign.** The archive flagged these as needing to "stay
   audibly distinct." Since C.Sign is confirmed cut (redundant — Comb Feedback is already bipolar,
   §J.6), **this specific overlap is now moot**: there is nothing left on the Filter side to be
   confused with Sym. Recorded so nobody re-derives a resolution to a conflict that no longer has two
   sides.
3. **Reverb Spread vs existing Reverb Stereo width.** Not a future conflict to resolve — an
   already-cut duplicate, see §3.6 and §6.
4. **Audio bank's Ring Mod / Cross XOR / Sub-Osc vs the Audio bank's own headroom exemption.** Not a
   cross-BANK conflict but a cross-CONCERN one: three of Audio's five proposed slots individually risk
   invalidating §K's current "Audio needs no limiter" verdict (§3.1's cross-cutting flag). If more than
   one of these three ships, their combined effect on `chainIn`'s bound needs a single re-derivation,
   not three independent ones that each assume the others are off.
5. **Envelope bank's Cycle vs Delay/Reverb's Freeze.** Both are "make the sound sustain indefinitely"
   controls at different points in the chain (source vs. time-effect). Not redundant under the
   bank-vs-modulation rule (neither reduces to "value X varies over time" — Cycle restructures the
   envelope's own state machine, Freeze gates writes to a delay line), but both interact with the
   Stop-transport clear mechanism (`AllIdle()`/B4) — a patch using Cycle at max AND Freeze at max
   simultaneously is worth an explicit Stop-behavior check before either ships, since neither one alone
   was designed with the other in mind.
6. **Filter Topology (series) vs Peak Slope (cascaded bumps).** Both are "make the peak's gain
   multiply onto something else" mechanisms. If both ship, a patch at Topology=1.0 (full series, peak
   fed by comb) AND Peak Slope=1.0 (two cascaded bumps) compounds: comb feeds bump 1 feeds bump 2. This
   is a THIRD headroom case beyond what either proposal's own row above prices individually — flagged
   so it isn't first discovered in a blowout repro months later, the exact failure mode §K exists to
   prevent.

---

## 5. Open questions for the operator

1. **Envelope bank's final 2 slots (12-13): Curve, Cycle, Grace — pick two, or fewer than two plus
   something else.** §J.4 left this explicitly open; this document filled it with a tentative
   Curve+Cycle default only so the table would be complete. Grace (minimum Hold duration so short gates
   don't clip) is equally evidenced and was not chosen over the other two for any principled reason —
   it needs the operator's ear/preference, not an implementer's tiebreak.
2. **Envelope slot order: append Decay at 9-11 (this document's proposal) or interleave into true
   A-D-S-R order at 0-11, renumbering the existing Attack/Sustain/Release?** Appending avoids
   colliding with the deferred §H "slot index == PhysicalEncoderId" hardware constraint; interleaving
   is the more conventional ADSR layout a player might expect from the encoder grid's visual order.
   Trade-off is legibility now vs. a slot-repack that's cheap today and potentially not once §H lands.
3. **Filter Topology and Peak Slope both trip §K in ways that compound (§4 item 6) — build both, one,
   or neither, and in what order?** Given the effort of re-deriving headroom for series composition
   twice (once per feature) versus once for their combination, is it worth designing them together
   even though each is independently useful?
4. **Which of the four "drive into a feedback loop" controls (Comb Drive, Delay Feedback Drive, Reverb
   Tank Drive, existing Drive bank Fuzz/Shape) does the operator actually want, and in what order of
   priority?** Building all four risks an instrument where four knobs do variations on the same thing;
   the operator's ear is the only way to know if that's four useful colors or three too many.
5. **Ring Mod (Audio slot 9): which oscillator pair(s)?** This document assumed VCO1×VCO2 for
   concreteness. A three-way scheme (e.g., a second knob-selected pairing, or all three multiplied)
   is possible but wasn't costed here — worth asking whether one fixed pair is enough or whether pair
   selection itself deserves thought before implementation.
6. **Reverb Shimmer and Tuned are honestly the two most expensive items in this entire document** (a
   pitch shifter for both; pitch tracking additionally for Tuned) **and also the two most
   identity-giving**, per the original survey. Worth an explicit operator call on whether that cost is
   worth paying at all, independent of scheduling — they may simply not be worth it relative to
   everything else on this list.
7. **Hard Sync (Audio slot 10) is marked MARGINAL by §J.6 itself** — is a low end that's plausibly
   inaudible (sync's character IS the discontinuity) worth a whole encoder slot, or should this be cut
   before it's ever prototyped?

8. **THE ASR ENVELOPES CANNOT MODULATE ANYTHING — is that intended?** (Added by the lead, 2026-08-06,
   drawing out a consequence of this document's own §J.3 correction.) Verified directly: the fifteen
   modulation sources (`FroggersModulatorSlot`, `app/FroggersModulation.hpp:156-171`) are Random S&H
   ×6, VCO Audio ×3, VCO **EF** ×3, Noise, External Audio, External Audio EF. **No ASR envelope
   output appears anywhere in that list.** The "VCO EF" sources are audio-amplitude envelope
   *followers* — they track the VCO's output level, which is a different signal from the ASR
   generator's own contour, and they only resemble it while a note is sounding.

   So the instrument has three envelope generators whose output gates the VCOs and **is available
   nowhere else.** You cannot use an envelope to sweep the filter, the drive, the delay send, or
   anything at all — the single most standard modulation routing in subtractive synthesis.

   **This is a modulation-SLATE question, not a bank-slot question, and it may outrank every
   proposal in this document.** Three of the fifteen slots currently hold VCO EF sources that
   partially duplicate what a true envelope source would do better. Options, for the operator:
   (a) leave it — the EF sources are the intended substitute and the omission is deliberate;
   (b) add ASR outputs as modulation sources, which needs slate capacity that is currently full at
   fifteen; (c) replace the three VCO EF slots with the three ASR outputs, on the grounds that the
   generator's own contour is more useful than a follower of its result.

   **Do not act on this without the operator.** It is recorded here because it was found while
   checking §J.3's corollary, and because every "shape the envelope itself" proposal in §3.2 is
   worth materially less if the envelope's shape still cannot reach anything downstream.

---

## 6. What was cut, and why

**Cut by explicit prior ruling (not re-litigated here):**
- **C.Sign (comb polarity)** — redundant. Comb Feedback is already bipolar (`Comb::GetFeedback`,
  `FilterFx.hpp:494-501`): 0.0→−0.95, 0.5→0, 1.0→+0.95.
- **Reverb Clear/Purge** — a momentary event, not continuous; the underlying need (Stop must flush
  reverb) is already covered by B4's transport-Stop wiring, itself confirmed landed
  (`FroggersAppCore.hpp`'s `AllIdle()`-gated clear, per the archive's own Group B findings).
- **Comb-trim smoother rate** — stays internal. Operator: *"definitely don't expose."*
- **Reverb InDiff** — duplicates Delay's Halo; Halo stays exclusive to Delay (even though, per §3.5's
  correction, today's "Halo" barely does what its name implies).
- **Cross-VCO FM (a dedicated Audio-bank FM knob)** — cut under the bank-vs-modulation rule. VCO
  Audio and VCO EF are already registered modulation sources feeding pitch parameters at audio rate
  (`FroggersModulation.hpp:535-546,551-562`), so a dedicated FM knob would be a fixed preset for
  something the 15-source modulation slate already does more flexibly.

**Newly cut in this document, by the same rules, on new evidence:**
- **Reverb Spread** — the archive proposed it as near-free; it duplicates the existing "Stereo width"
  slot (today's slot 5). Cut on sight, §3.6.
- **Wear (Drive bank)** — not cut by a rule, deprioritized for slot budget; the least-evidenced Tier 3
  idea in the Drive survey, kept as a future candidate rather than shipped in this 5-slot proposal.

**Marginal, flagged rather than cut — needs a prototype before it's trusted either way:**
- **Hard Sync (Audio)** — §J.6's own verdict: the event stays discrete, only depth is continuous, and
  the low end of the knob may be inaudible. Not cut, but the weakest "natural fit" in this whole
  document; see open question 7.

**No new items were cut under the bank-vs-modulation rule beyond the already-recorded FM case** — every
new proposal in §3 was checked against "is this reducible to a mod-source varying a value over time?"
and each survived because it's either a signal-path operation (multiply, bit-combine, phase reset,
route-through) or a structural coupling (spread, ratio-link, topology) that the 15-source slate cannot
express regardless of depth or source choice.

---

## 7. Report notes on evidence quality

Every "today" row in §3's tables was read directly from `FroggersBankLayouts()`
(`app/FroggersParameters.hpp:145-186`), not from the archive's prose. Every Tier-1 citation was
re-verified against the code as it stands today (2026-08-06), not copied from the 2026-08-05 archive —
three of the archive's original Tier-1 citations (Filter topology, Drive anti-alias, comb-trim
smoother) still hold at (shifted) line numbers; a fourth, previously uncited, was found in Delay
(§3.5, Mod Rate). Several §K items the archive still listed as "in flight" (peak trim, comb trim,
delay in-loop saturation, delay output limiter B6a, reverb output limiter B6b) have already landed —
confirmed by direct reads of `FilterFx.hpp`, `Delay.hpp`, and `Reverb.hpp`, not inferred from the
archive's own status markers.
