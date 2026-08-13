# Proposal — `frogg3rs-bank-expansion`

**Created 2026-08-11.** Picks up section J of the archived
`openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md`, continued as the
DEFERRED design artifact `openspec/changes/archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/
BANK-EXPANSION-DESIGN.md` ("the design doc" below). That document proposed a full 14-named-parameter
slate for all six banks and left every item open for the operator. **This change records the operator's
rulings on parts of it, corrects two of the design doc's own claims, and specifies only what is now
actually decided** — most of the design doc's candidate slots remain open and are **not** filled in
here; see §8.

**This document is self-contained.** The design doc is read once, in full, as background (§0); every
binding fact this proposal relies on is re-verified here by reading the current code, cited by symbol.

**Markdown only.** No file under `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/` or `External/Sheaf` is
touched by this change, no other source file is touched, and nothing is built.

---

## 0. Why the design doc's own citations cannot be trusted verbatim

The design doc itself was careful about this (its own §7: "every Tier-1 citation was re-verified against
the code as it stands today ... not copied from the archive"). Since it was written (2026-08-06), two
further things moved the ground under it:

1. **A VCO `std::array` refactor.** `FroggersAppCore.hpp`'s three separately-named `audioVco1_/
   audioVco2_/audioVco3_` members were collapsed into `std::array<dsp::Vco, 3> audioVcos_`, confirmed
   live by reading (the collapsing comment is attached to the member declaration and to the
   `RouteAudioSample` loop that iterates it). Every citation into that function that predates the
   refactor is off by however many lines the loop saved.
2. **A header row added to `FroggersUiSurface.hpp`'s chrome band** ("STEP 1, 2026-08-09": a modulation
   header row inserted between the bank-tabs row and the first encoder row), confirmed live by reading —
   the file's own comment says the right column's row count grew from what the CELL MAP originally had.
   This shifts **chrome pixel-layout row numbers**, not the parameter bank's own slot indices (0-15),
   which are owned by `FroggersParameters.hpp`/`kFroggersSlotsPerBank` and are unaffected by chrome
   layout changes — worth stating plainly since both are called "rows" and are easy to conflate.

Independent confirmation that citation drift is real, not hypothetical: the design doc's own Filter-bank
flagship finding cites `FroggersAppCore.hpp:1099-1100` for the hardcoded `useParallel=true` call. Reading
the current file finds the same two-line construct, same symbols, unchanged behavior — at
`FroggersAppCore.hpp:1355-1356`. **Every citation in this proposal is therefore by symbol; any line
number given is a snapshot from this session's own reading, 2026-08-11, and goes stale the moment
anything above it in the same file changes.**

## 0a. The central selection rule — session 2, binding on this change and every later candidate

**Added 2026-08-11 (session 2).** The operator rejected several of the design doc's and this session's
own researched candidates with one principle. It is recorded here, prominently, because it is now the
change's central criterion — every future bank-fill proposal must be checked against it before it is
written, not after:

**"If the modulation matrix can already produce the effect by routing one of the 15 sources onto an
existing parameter, the parameter is REJECTED."**

Operator, verbatim:
- *"self-FM obviated by modulation level 1."*
- *"i have the same concern about glide, it seems irrelevant without s&h in modulation level 1."*
- *"cross couplers dont make sense anymore because of mod lvl 1."* (the same rule's first application,
  already recorded at §3 ruling 2 before this session generalized it.)

What survives this test is a degree of freedom **structurally unreachable by routing** — not merely a
sound a patient operator could already approximate by hand or by patching a source onto an existing
parameter.

**Worked example the operator accepted, re-verified live against `app/dsp/Vco.hpp`:** `Vco::Process`
computes
```
const float pmOffset = kPmLfoDepth * PmDepthScale(pmKnob01) * StepPmLfo(pmKnob01, sampleRate);
```
The SAME knob, `pmKnob01`, sets both the PM LFO's rate (`StepPmLfo`'s own `ExpMapCompute(kPmLfoMinHz,
kPmLfoMaxHz, pmKnob01)`, confirmed `kPmLfoMinHz = 0.05f`, `kPmLfoMaxHz = 20.0f`) and, through
`PmDepthScale(pmKnob01)`, that same LFO's depth. No modulation route can separate two values one
function multiplies together from a single knob's input — this is exactly why a dedicated "PM rate"
control is a structurally new degree of freedom (a live candidate, §9.5), while self-FM and Glide, which
only re-create effects the matrix's existing sources already reach when routed onto an existing
parameter, are not.

**Also recorded here, verified by reading `app/dsp/RandomShLane.hpp`:** only ONE of the five Random S&H
lanes the modulation matrix exposes is slewed. `lanes::MakeSource5` is constructed with `kSlowCutoff =
0.002f`, while `lanes::MakeSource1` through `MakeSource4` are all constructed with `kFastCutoff = 0.45f`
(near-instant, effectively unslewed). So the matrix does supply one slewed source — part of why
Glide/portamento/slew was cut, per the operator's own "irrelevant... without s&h" framing above — but
that slew rate is fixed, not variable, and lanes #1-#4 supply no slew at all. This is the factual basis
for the operator's framing, not an invented justification, and it is why the matrix's coverage of "a
slewed source" is real but incomplete.

**Session 5 addendum — the same rule applied to the lead's own proposals, not only to candidates.** Three
times in this change the lead invented a special-cased mechanism beside the general parameter+modulation
mechanism `Parameter::GetRaw` already provides: Reverb's "Tuned" originally coupled to VCO1's pitch instead
of being an ordinary parameter (corrected at §3 ruling 5); the cut cross-couplers assumed a dedicated
coupling term instead of ordinary matrix routing (§3 ruling 2); and Ring Mod's carrier was framed as an
open choice among other named VCOs instead of an internal, per-VCO oscillator (corrected at §3 ruling 1,
§4.2, session 5). The operator's own selection rule already names the fix for the first case: if the
general mechanism already reaches the effect, a special case is not needed. This is the same rule, applied
to the lead's own drafting rather than to a candidate the operator is choosing among.

## 1. Objective

Record the operator's rulings on the design doc's open questions (§3), correct two of the design doc's
claims that the operator's own words disprove (§3, rulings 3 and 5), and specify — as an OpenSpec change,
not yet implemented — only the portion of the 14-parameter slate that is now actually decided: the
**Envelope bank's full expansion to fourteen parameters**. Everything else the design doc proposed stays
exactly as open as it was, restated in §8 so it is not mistaken for resolved by omission.

**Session 2 (2026-08-11, this update) adds:** the central selection rule that now governs every
candidate (§0a); a further round of REJECTED candidates found by applying that rule (§3 ruling 6-7); a
DECIDED slate for **Filter** (Topology, Scoop Freq, Scoop Width, Comb Drive — §9.1), **Drive**
(Anti-Alias Brightness — §9.2), **Delay** (Feedback Drive, Mod Rate — §9.3), and a now-**COMPLETE**
fourteen-parameter **Reverb** bank (§9.4); and a new scope item, the over-length short-label rework
(§10), which the operator explicitly folded into this change rather than splitting out.

**Session 3 (2026-08-11, this update) adds:** the round-2 research (`RESEARCH2-audio-filter.md`,
`RESEARCH2-drive-delay.md`, outside the repo, summarized not linked per that research's own instruction)
fills every remaining **PENDING RESEARCH** slot left open by session 2 — **Filter slot 13** (Scoop Depth,
§9.1), **Drive slots 10-13** (Link, Fold, Tone, Bias, §9.2), **Delay slots 10, 12, 13** (Feedback Tone,
Width Balance, Crush, §9.3), and **Audio slots 12-13** (PM Rate, VCO Balance, §9.5) — leaving **zero**
PENDING RESEARCH markers anywhere in this change. Filter, Drive, and Delay each now reach a **COMPLETE**
fourteen-parameter slate, matching Envelope and Reverb. Session 3 also records a new REJECTED candidate
(PM Depth Max, §3 ruling 8, including a correction of an earlier mischaracterization of what the candidate
does) and a new §9.6 totals section. Ring Mod (Audio slots 9-11) was, at the end of this session, recorded
as the only remaining blocker in this entire change — everything else that was open going into this
session was either decided or rejected coming out of it (§4.2, restated prominently below at the time).
**Session 5 corrects this: that "blocker" framing was itself wrong and is removed throughout this document
— see the session-5 note below.**

**This change now puts every bank at fourteen parameters.** Envelope, Reverb, Filter, Drive, Delay, and —
as of session 5 — Audio are all fully decided at fourteen each. Ring Mod (Audio slots 9-11) is DECIDED
(§3 ruling 1, §9.5): per-VCO, an internal carrier, an audio-rate knob range, and ordinary matrix
modulation like every other parameter. §9.6 counts the target slate as thirty new parameters across six
banks, all thirty of which this change (sessions 1-5 combined) now specifies.

**Session 4 (2026-08-11, this update) adds:** a single new operator ruling, a binding hard floor/cap on
VCO Balance's crossfade — no VCO may fall below 10% or reach 100% of the mix, capping any single VCO at
80% (§4.4, §9.5). This is not a new candidate or a new slot; it tightens VCO Balance's own already-decided
specification. Its effect ripples through §4.4 (rewritten: the mechanism is now a provable invariant, not
a to-be-measured risk), §9.5 (VCO Balance's own entry, rewritten), §9.6 (the headroom-flagged list drops
from three items to two — **Comb Drive and Bias only**), and §11 (updated to match). No other bank slot,
ruling, or open item changes in this session.

**Session 5 (2026-08-12, this update) corrects Ring Mod's design entirely — the design this document
previously recorded (an open "carrier" choice among next-VCO-cyclically, the mix of the other two VCOs, or
a dedicated carrier; a pre-gate/post-gate sub-question; and a claimed collision with `froggers-vco-
topology`'s "No hardcoded cross-VCO coupling" requirement) was WRONG and is removed throughout this
document, not merely superseded.** It was the same error already named and fixed twice elsewhere in this
change — a special-cased mechanism invented beside the general parameter+modulation mechanism
`Parameter::GetRaw` (§2) already provides, the same error behind Reverb Tuned's original mischaracterization
(§3 ruling 5) and the cut cross-couplers (§3 ruling 2). The corrected design, operator's own words: *"why
wouldn't ring mod for each vco be like any other parameter -- derive its value from the knob position, and
that position attenuates any modulation it's getting"*; *"the signal is the value derived from knob
position and/or modulation source"*; and decisively, *"the ring mod range of the knob should be audio
rate, then it gets modulation sources from levels 1-2-3 like any other parameter"*. **Each VCO has its own
ring modulator with an INTERNAL carrier** — an oscillator generated inside that VCO's own ring-mod stage,
never reading another VCO's signal — **whose frequency the Ring Mod knob sets across an audio-rate range**,
the same exponential-map shape `Vco::PitchToPhaseIncrement` already uses for pitch (`ExpMapCompute(20.0f /
sampleRate, 20000.0f / sampleRate, pitchKnob01)` = `min * (max/min)^value`, `app/dsp/DspMath.hpp:43-46`,
`app/dsp/Vco.hpp:131-134` — equivalently `f = 20 * 1000^knob`). The resolved knob value is then modulatable
from the matrix exactly like any other bank parameter, through `Parameter::GetRaw`'s `center + Σ(depth ×
source)` (§2). **Consequences: there is no carrier decision (the carrier is internal and per-VCO), there is
no cross-VCO coupling, and therefore no collision with `froggers-vco-topology`'s requirement — that claimed
collision was never real and is deleted below, not merely softened. Ring Mod is no longer a blocker; Audio
slots 9-11 are DECIDED, completing all six banks at fourteen parameters each (thirty of thirty, up from
twenty-seven of thirty).** ~~One genuine implementation detail remains open, not a blocker (§8, §9.5):
whether the knob's range reaches down to sub-audio rate (an effectively-clean position at the bottom,
mirroring PM LFO's `kPmLfoMinHz = 0.05f`-`kPmLfoMaxHz = 20.0f` span, `app/dsp/Vco.hpp:103-104`) or stays
audio-rate across the whole sweep.~~ **Struck by the session-6 audit: a sub-audio carrier is not a clean
position (a product has no unity position at any frequency), so that item's premise was false. What is
actually open — how Ring Mod is turned off at all — IS a blocker and an operator question (§1a finding 2,
§4.2, §8, T8.0).**

## 1a. Session 6 (2026-08-12) — the OMNI §14 PREFLIGHT audit's own findings, recorded before execution

**§11 previously declared this change "Ready for OMNI §14 PREFLIGHT." The audit ran; this section is its
result. It did not reject the change's decisions — it rejected the claim that the change was ready to
execute, and §11 is rewritten accordingly. Three of the five findings below needed the operator's judgement;
all three were put to the operator; ONE came back answered (ruling 9 — see §3a, and §4.5 for the shared
zero-off gate it requires) and TWO are still out (§3a items 10-11). A brief version of this document recorded
all three as answered, misreading one reply as three — corrected at §3a and §11.** What held up and what did not:

**Held up under re-verification (checked directly against the current tree, not taken from any prior
session's report):** every one of the roughly dozen hardcoded-literal claims this change rests on. `0.25f *
(computedGain - 1.0f)` in BOTH `coefs[1]` and `coefs[3]` (`Drive.hpp:101,103`); `const float sinIn = out /
4.0f;` (`Drive.hpp:378`); `p.dwid * baseSeconds * 0.35f` and `p.dwid * 0.5f` (`Delay.hpp:316,330`);
`kPmLfoDepth = 0.15f` (`Vco.hpp:105`, used at `:169`); `kModLfoHz = 0.35f` (`Reverb.hpp:142`, used at
`:350`); `antiAlias.SetAlphaFromNatFreq(0.4f)` (`Drive.hpp:133`); `lfoInc = 2.0f * 3.14159265f * 0.25f /
sampleRate` (`Delay.hpp:285`); the single hardcoded `/*useParallel=*/true` call site
(`FroggersAppCore.hpp:1356`); and the doubled `knob(FroggersBankId::Filter, 8)` read — `SetHeight` at
`:1344`, `scoopMix` at `:1356`. Also confirmed: `rawCombTrim`'s formula and the comb's in-loop saturator
(`FilterFx.hpp:721,406`), `MixOscVoices`'s equal-thirds return (`VoiceEnvelope.hpp:294`), Attack's
`sustainLevel` ceiling (`:205`), `Sine01`'s own phase wrap making it bounded to `[-1, 1]` for ANY argument
(`DspMath.hpp:28-33`), `EvalWaveMorph`'s `[-1, 1]` bound (`Vco.hpp:68-84`), and §10's short-label count —
**23 of 54, Drive 5 of 9, and every per-bank split, re-counted independently and all exact.**

**Also held up: the two claims §11 most depended on.** `froggers-vco-topology`'s *"No hardcoded cross-VCO
coupling"* requirement IS satisfied, not violated, by an internal per-VCO carrier (§4.2's correction is
sound). And VCO Balance's convexity argument IS valid: `Σw = 1` with `w ≥ 0` gives `|Σ w·v| ≤
max(|v1|,|v2|,|v3|)`, the identical supremum the equal-thirds average has, so the ⚠ flag is genuinely
discharged by construction (§4.4). Both were re-derived here, not accepted.

**Did not hold up — five findings, each recorded at its own section rather than only here:**
1. **The collision hunt looked at one requirement, not the spec.** Sessions 3-5 checked Ring Mod against
   `froggers-vco-topology`'s coupling requirement and never read that file's other requirements. **PM Rate
   (Audio slot 12) collides with the requirement "Froggers oscillator topology is preserved," which states
   the PM LFO's frequency "is an exponential function of the PM knob," and with its scenario "Phase
   modulation is self-contained," which PM Rate's one-knob-shared-across-three-VCOs shape breaks.** A real
   collision was missed while a phantom one was litigated three times. New spec delta:
   `specs/froggers-vco-topology/spec.md`; new operator gate T7.0 (§9.5).
2. **Ring Mod as specified has no off position and no depth control**, and §8's claim that a sub-audio low
   end gives "an effectively-clean position at the very bottom" is FALSE — a 0.05 Hz sine carrier is a
   20-second full-depth tremolo through zero, not a clean position (§4.2, §8, §9.5, new gate T8.0).
3. **Nothing required an unlocked literal's new parameter to DEFAULT to the literal it replaces**, so all
   twelve Tier-1-style unlocks would have changed every existing patch's sound on upgrade — silently, with
   every stored value intact (§4.1, new spec requirement, new task block T9).
4. **Decay is a three-part change, not two:** `attackStep` is `sustainLevel / attackTime`, so raising
   Attack's ceiling without changing that numerator stretches attack time by `1/sustainLevel` — up to 10x at
   the `kMinSustainLevel = 0.10f` floor (§4.3, T1.1).
5. **Link's "Headroom: none, provably" argument is invalid** — bounded coefficients do not bound an
   unbounded 5th-order polynomial, which is this document's own reason for flagging Bias. The conclusion
   survives on a measurement instead (§9.2). **Fold's "no matter how small the divisor gets" invites a
   division by zero** into a NaN path this codebase has already been silenced by once (§9.2).

## 2. Verified facts — re-confirmed by reading, 2026-08-11

All of the following were re-read against the current tree, not carried over from the design doc or the
task brief without a fresh check:

- `kFroggersParamsPerBank = 9`, `kFroggersCrispySlot = 14`, `kFroggersCrunchySlot = 15`,
  `kFroggersSlotsPerBank = 16` (`app/FroggersParameters.hpp`). `FroggersBankLayouts()` registers exactly
  nine parameters per bank at offset 0 plus Crispy at 14 and Crunchy at 15; slots 9-13 are never
  registered for any bank. All six banks are identically shaped today. Confirmed unchanged from the
  design doc's §2.
- **The 16-slot encoder grid is a 4-column, row-major layout**: `EncoderRow(row)` returns
  `"froggers.layout.right.row." + row`, with the file's own comment "row 0 = slots 0-3, row 1 = slots
  4-7" (`app/FroggersUiSurface.hpp`), and the topology comment confirms the mapping is `ix / kColumns`,
  `ix % kColumns`. **This is load-bearing for the Envelope ruling below** — "first three rows on that
  page" means slots 0-3, 4-7, 8-11.
- `Parameter::GetRaw` (`External/Sheaf/projects/synth/src/ParameterModulation.cpp`) is
  `ClampToRange(currentCenter_ * centerScale + normalizationOffset + ApplyActive(...), range)` — the
  knob position is the CENTER, modulation adds on top. Confirmed unchanged.
- Reverb tank delay lengths: `sizeNorm = RoomSizeFromKnob(sizeKnob01)`, `dA = 180 + sizeNorm*1300`,
  `dB = 260 + sizeNorm*1800` samples (`app/dsp/Reverb.hpp`), then offset by `applyMod`, itself driven by
  `modDepthKnob01` — confirmed at the call site to be Reverb bank **slot 7** (`knob(FroggersBankId::
  Reverb, 7)` at `FroggersAppCore.hpp`'s `RouteAudioSample`, matching `FroggersBankLayouts()`'s Reverb
  row 7 = "Mod depth"). At the app's own sample rates (`config.preferredSampleRate = 48000.0`, fallback
  `kFallbackSampleRateHz = 44100.0`), `dA` spans roughly 3.75-30.8 ms and `dB` roughly 5.4-42.9 ms —
  independently computed here from the confirmed formula, not copied from any prior document.
- `FilterFxChain::Process(float input, bool useParallel, float combPeakBlend, float scoopMix)`
  (`app/dsp/FilterFx.hpp`) — the one production call site still hardcodes `/*useParallel=*/true`
  (`FroggersAppCore.hpp`'s `RouteAudioSample`, cited above at its current, re-verified location). Real,
  unreachable DSP, confirmed still true.
- **The modulation slate's three VCO-audio sources are separate `dsp::Vco vco1_/vco2_/vco3_` members
  inside `FroggersModulation.hpp`'s source-stepping class** — distinct objects from `FroggersAppCore.hpp`'s
  `audioVcos_` (the audible signal path). `Step()` drives them from only the Audio bank's own
  pitch/shape/PM knob reads (`vco1.pitch01` etc.), with **no `adsr.apply` call anywhere in that file** —
  confirmed by reading the whole `Step()` body. `RouteAudioSample`'s own comment states the single
  license for calling `adsr.apply`: "do not re-apply `adsr.apply` anywhere else." **No ASR envelope
  output, and no pitch CV, exists among the fifteen modulation sources** — confirmed by reading
  `FroggersModulatorSlot`'s full enumeration (`kModSlotRandomSh1`...`kModSlotExternalAudioEf`, 15 values,
  `app/FroggersModulation.hpp`): 6 Random S&H, 3 VCO Audio, 3 VCO EF (envelope *followers*, not the ASR
  generator's own contour), Noise, 2 External Audio.
- **The reverb tank's feedback taps are already wrapped in `PadeSaturator::Saturate`**, confirmed live:
  `const float aIn = preOut + fb * PadeSaturator::Saturate(aFb);` / same for `bIn`
  (`app/dsp/Reverb.hpp`), with the surrounding comment naming it "S2a.1" and citing the same operator
  decision the task brief quotes. This closes the design doc's "Tier 2, needs measurement" framing for
  Reverb Tank Drive down to "a pre-gain knob is the only missing piece," matching the brief.
- `VcoAdsrState`'s `enum class Stage { Idle, Attack, Hold, Release }` (`app/dsp/VoiceEnvelope.hpp`) —
  confirmed, no `Decay` stage exists today.

**Session 2 additions (2026-08-11), re-verified live against the current tree:**
- `Oversampler2x`'s constructor hardcodes `antiAlias.SetAlphaFromNatFreq(0.4f)` (`app/dsp/Drive.hpp`) —
  the one-pole anti-alias filter inside the Drive bank's `PolynomialDrive` oversampling path has a fixed
  cutoff, never exposed as a parameter. Real, unreachable DSP — the Drive-bank Tier-1 finding behind
  Anti-Alias Brightness (§9.2).
- `StereoDelay`'s modulation LFO rate is hardcoded: `lfoInc = 2.0f * 3.14159265f * 0.25f / sampleRate`
  (`app/dsp/Delay.hpp`) — a fixed 0.25 Hz, feeding `modSeconds = std::sin(lfoPhase) * p.dmod * baseSeconds
  * 0.08f`. Real, unreachable DSP — the Delay-bank Tier-1 finding behind Mod Rate (§9.3). The same file's
  feedback loop is `WriteSample(inSignal + fbk * PadeSaturator::Saturate(fbL), lineL)` (also `fbR`/`lineR`)
  — confirmed unchanged from the predecessor change's own citation, load-bearing for §9.3's withdrawn
  headroom flag on Feedback Drive.
- `filterChain_.scoopNotch.SetFreq(bumpFreq)` / `.SetWidth(bumpWidth)` (`FroggersAppCore.hpp`'s
  `RouteAudioSample`) are fed the identical local variables passed to `filterChain_.peak.SetFreq(bumpFreq)`
  / `.SetWidth(bumpWidth)` — confirmed live: Scoop has no frequency or width of its own today, only its
  height/mix knob. A parity-era wiring compromise, not a design choice — the Filter-bank finding behind
  Scoop Freq/Scoop Width (§9.1).
- The comb branch's output trim is `rawCombTrim = 1.0f / (1.0f + std::fabs(comb.feedback))`, smoothed by
  `combTrimSmoother` (`app/dsp/FilterFx.hpp`) — measured and tuned against `|comb| <= A + |fb|` at
  **unity input gain** into the comb's own in-loop `PadeSaturator::Saturate(filter.Process(tapped))` call.
  Load-bearing for §9.1's Comb Drive headroom flag: a pre-gain ahead of that saturator raises the trim's
  worst case above what was measured.

## 3. Operator rulings — recorded as DECIDED, verbatim quotes preserved

1. **Ring Mod is per-VCO, three parameters at Audio slots 9-11, DECIDED — corrected in full, session 5.**
   *"it's supposed to be ring mod for each vco separately"*. Closes the design doc's open question 5.
   **The design this document originally recorded here — an open choice among next-VCO-cyclically, the mix
   of the other two VCOs, or a dedicated carrier, plus a pre-gate/post-gate sub-question — was WRONG and is
   corrected below, not merely softened (see the session-5 note in §1 and §0a, and §4.2, §9.5 below).**
   Ring Mod is an ordinary parameter like any other: **each VCO has its own ring modulator with an INTERNAL
   carrier**, and the Ring Mod knob's range IS that carrier's frequency, mapped across audio rate — the
   same exponential shape `Vco::PitchToPhaseIncrement` already uses for pitch (`ExpMapCompute(20.0f /
   sampleRate, 20000.0f / sampleRate, pitchKnob01)`, `app/dsp/Vco.hpp:131-134`, equivalently `f = 20 *
   1000^knob`). The resolved value is then modulatable from the matrix exactly like every other bank
   parameter, through `Parameter::GetRaw`'s `center + Σ(depth × source)` (§2). Operator, verbatim: *"why
   wouldn't ring mod for each vco be like any other parameter -- derive its value from the knob position,
   and that position attenuates any modulation it's getting"*, *"the signal is the value derived from knob
   position and/or modulation source"*, and *"the ring mod range of the knob should be audio rate, then it
   gets modulation sources from levels 1-2-3 like any other parameter"*. **There is no carrier decision** —
   the carrier is internal and per-VCO, not one of the other named VCOs — **and therefore no cross-VCO
   coupling and no collision with `froggers-vco-topology`'s "No hardcoded cross-VCO coupling" requirement**
   (ruling 2, below); that claimed collision was never real. **Ring Mod is no longer a blocker; Audio slots
   9-11 are DECIDED** (§9.5). ~~One implementation detail remains genuinely open, not a blocker — whether the
   knob's low end reaches sub-audio rate~~ — **session 6 audit: that framing is withdrawn (its premise, that
   a sub-audio carrier is an effectively-clean position, is false). The ruling itself stands; what is open is
   how Ring Mod is turned off, which IS a blocker and an operator question — §4.2, §8, T8.0.**
2. **Cross XOR is CUT.** *"cross couplers dont make sense anymore because of mod lvl 1"* — the
   modulation system already routes VCO audio sources onto any parameter, so a dedicated cross-VCO
   coupler is redundant. **This ruling is not merely a taste call — it restates an existing, in-force
   requirement.** `openspec/specs/froggers-vco-topology/spec.md`'s "No hardcoded cross-VCO coupling"
   requirement already says: *"The oscillator section SHALL contain no hardcoded VCO-to-VCO coupling
   terms. All inter-oscillator routing SHALL be expressed only through the modulation matrix."* Cross XOR
   would have violated a requirement that already exists; no spec change is needed to cut it, and no
   code exists to remove (it was never built). **Ring Mod does not create a tension with this requirement
   either** (ruling 1, above, corrected session 5) — its carrier is internal and per-VCO, not a coupling
   between named VCOs, so this requirement closes the question for both Cross XOR and Ring Mod alike; a
   prior version of this document claimed otherwise and that claim was never real (§4.2).
3. **Continuous range is the selection criterion for any slot.** *"the answer to this and for anything
   else depends on which parameters offer continuous ranges. i think that's curve and grace, not sure
   what cycle is."* Consequences, all DECIDED:
   - **Cycle is CUT** — a retrigger count (the knob's middle steps through 2, 3, 4... repeats), not a
     continuous range.
   - **Hard Sync is CUT** under the same rule — its low half is inaudible and sync's character IS the
     discontinuity, so it fails the continuous-range test the same way Cycle does. (The design doc's own
     §J.6 had already flagged Hard Sync as MARGINAL; this ruling settles it.)
   - **Envelope slots 12-13 are Curve + Grace.** This resolves the design doc's open question 1 (it had
     left Curve/Cycle/Grace as "pick two" with a tentative Curve+Cycle default); Cycle's cut above
     leaves Curve and Grace as the only two continuous-range survivors, so the pick is no longer
     tentative.
4. **Envelope uses interleaved ADSR order, not appended Decay.** *"labels should be A1 D1 S1 R1 etc for
   first three rows on that page"* — three grid rows of four columns (§2's confirmed row-major mapping:
   slots 0-3, 4-7, 8-11) give A/D/S/R × VCO1-3 at slots 0-11, two-character short names (fits the
   14-segment display's 4-char cap — confirmed live: `FourteenSegment`'s text helper defaults to
   `numChars = 4` and `UpperShortLabel`'s own default is `maxChars = 4`,
   `External/Sheaf/projects/synth/include/synth/EncoderDraw.hpp`). Curve and Grace take 12-13 per
   ruling 3.
   **Consequence recorded explicitly, per the task brief's own instruction — but corrected by this
   session's own re-verification, not merely restated (§4.1): interleaving DOES renumber which slot
   index Sustain and Release occupy. Whether that renumbering is safe for existing saved patches turns
   out to depend on how patches are actually persisted, which this session verified by reading rather
   than assuming — see §4.1 for the finding.** This does not resolve the migration/reset question (§8
   keeps it open); it changes what evidence that decision is made against.
5. **Reverb "Tuned" is re-scoped and the design doc was wrong.** *"why would the tuned pitch come from
   vco1 instead of the knob position value xor that position's attenuation of modulation source depth?"*
   From `GetRaw` (§2): the knob position is the center and modulation adds on top, so **Tuned is an
   ordinary parameter whose resolved value maps to the tank delay length** — not a coupling to VCO1's
   pitch, not an attenuator. This removes the pitch-tracker requirement the design doc costed Tuned at
   ("the most expensive item in this document") entirely; the corrected cost is a mapping-layer change
   into the same `dA`/`dB`/`applyMod` mechanism §2 confirms already exists. **This correction does not
   promote Tuned onto the decided list** — it remains one of the still-open Reverb-bank candidates (§8);
   only its cost estimate changes.
   **Caution carried forward:** modulating Tuned at audio rate sweeps a delay length inside a feedback
   loop that now has an in-loop saturator (§2) and an existing Mod-depth offset with precedent for
   bounded modulation of the same `dA`/`dB` pair — but "has precedent" is not "measured safe" for this
   specific new use, per the standing rule (§7).
   **Shimmer still needs a pitch shifter and none exists anywhere in `app/dsp/`** (grep-confirmed, no
   file under that directory implements pitch shifting) — that half of the design doc's costing stands
   unchanged.
6. **Peak Slope is CUT — session 2 promotes this from recommendation to ruling.** *"are they materially
   different in any way? i dont understand what these are supposed to be and how they are different,
   sound sstupid."* Distinction, restated for the record: today Comb and Peak run in PARALLEL, blended
   by the Comb/Peak knob; Filter Topology (already Tier 1, hardcoded off — §2) lets Peak process Comb's
   OUTPUT instead (series), so Peak sculpts and multiplies Comb's resonance. Peak Slope only chains a
   second identical resonant bump for a steeper single peak — a much smaller idea riding the same slot
   budget, and the operator's session-1 wording was already skeptical. **Session 1 recorded this as "the
   operator has not ruled on this"; session 2's task brief now lists Peak Slope among the operator's
   REJECTED candidates, closing that gap.** Kept: Filter Topology. Cut: Peak Slope.
7. **Self-FM, Glide/portamento/slew, VCO Spread, and Sub-Oscillator are CUT — session 2.** The first two
   are direct applications of §0a's central rule, with the operator's own verbatim quotes reproduced
   there in full (*"self-FM obviated by modulation level 1"*; *"i have the same concern about glide, it
   seems irrelevant without s&h in modulation level 1"*). VCO Spread and Sub-Oscillator appear on the
   same session-2 REJECTED list without an accompanying verbatim quote specific to either one; the most
   defensible reason on record for each, consistent with §0a's spirit and this document's own prior §8
   framing, is recorded here rather than invented after the fact:
   - **VCO Spread** — a static per-VCO detune is already directly settable by hand on each of VCO1-3's
     own pitch knob; this instrument's three independent, individually-tunable VCOs make a dedicated
     "spread" abstraction redundant with a parameter that already exists three times over. (This is the
     "dialable by hand" reasoning session 1's §8 had already flagged as VCO Spread's likely fate under
     ruling 2 — session 2 confirms the cut.)
   - **Sub-Oscillator** — the same three-independent-VCO structure applies: dedicating one of VCO1-3 to
     play an octave below another already produces a sub-oscillator's classic effect without a fourth,
     purpose-built generator. Session 1's §8 had also flagged this candidate's DSP cost as "Tier 3,
     additively raises the pre-average mix" (a genuinely new headroom concern, distinct from the
     modulation-routing test) — a second, independent reason this candidate was never a cheap add.
   **None of these four is implemented; nothing exists to remove.**
8. **PM Depth Max is CUT — session 3.** Operator, verbatim: *"that's stupid as fuck, why would we want to
   interpolate between 0-0.15 pm depth max."* **Correction of this document's own earlier
   mischaracterization, recorded here rather than silently fixed, per the standard §0's own citation-drift
   discipline sets for this whole change:** the operator's phrasing describes the candidate as
   interpolating a knob's travel *between* 0 and 0.15. Re-checked directly against `RESEARCH2-audio-
   filter.md`'s own worked range and against `Vco.hpp:105`'s `kPmLfoDepth = 0.15f`, that is not what the
   candidate does — it does the opposite. `kPmLfoDepth` is today's fixed CEILING (`Vco.hpp:169`'s
   `pmOffset = kPmLfoDepth * PmDepthScale(pmKnob01) * StepPmLfo(...)`); the candidate would have moved that
   ceiling from 0.15 UP TO 1.0 as the new knob swept 0 to 1 (0.0 → ceiling stays at today's 0.15, 1.0 →
   ceiling reaches 1.0, full-cycle phase wrap) — raising the ceiling ABOVE 0.15, not interpolating within
   it. Whichever presentation of the candidate produced the operator's "interpolate between 0-0.15"
   framing mischaracterized it; the record here is corrected to the candidate's actual shape rather than
   left standing on the wrong description.
   **The correction does not reverse the ruling — PM Depth Max is rejected on independent grounds even
   under its correct description:** a knob whose only job is rescaling another knob's own range is a
   meta-control, not a sound in its own right — a qualitatively different objection from "the range is
   wrong," and one that would apply regardless of which numeric ceiling the knob swept between. Practically
   compounding that objection: PM depth already occupies Audio slots 6-8 (the existing Phase-mod knob,
   three-VCO-wide), so a second knob whose entire purpose is rescaling the first knob's ceiling would be a
   meta-control layered on top of an already-occupied concept, not a new degree of freedom in the sense
   §0a's central rule looks for.
   **The underlying question is real and is recorded separately, as an open by-ear tuning item — NOT a
   parameter, and not scheduled by any task in `tasks.md`:** is `kPmLfoDepth = 0.15` actually the right
   maximum PM depth for this instrument? If by-ear testing finds it too shallow, the fix is to change the
   constant itself (a one-line tuning edit, same class of change as `kMaxAttackSeconds`'s own 2.5s → 1.0s
   operator-judged lowering, `VoiceEnvelope.hpp`), not to add a knob that lets the player rescale it
   live. See §8 for where this open item is carried forward.

## 3a. Session 6 operator rulings — the three audit questions, answered

9. **Ring Mod gets a true zero position at the bottom of its own knob, implemented as a SHARED function with
   PM — DECIDED, session 6.** Operator: *"i choose #1 and this should be implemented in an omni rule
   compliant way as a shared function between PM and RM and any other, and we can abstract from this to
   establish an array of limiter functions that vary depending on an array of parameter types."* This closes
   §1a finding 2 (no off position). It costs no slots: the bottom of the Ring Mod knob's travel gates the
   ring-mod amount to exactly zero and ramps smoothly out of it, and the rest of the travel sweeps the
   carrier frequency — the same shape `Vco::PmDepthScale` already implements for the PM knob, which is why
   the operator required one function rather than two. Mechanism, the §8 enumeration behind it, and the
   answer to the operator's own "isn't this already part of the parameter class?" question: **§4.5**.
**Process note, recorded rather than tidied away: this document briefly recorded all three as rulings on the
strength of one reply that only covered the first.** The operator's *"i choose #1"* answered Ring Mod; the
other two questions had not been seen. Reading one answer as three is the same failure §0a's session-5
addendum already names twice in this change — the lead inventing what the operator decided. The questions
were re-put and answered separately; all three rulings below are now the operator's own.

10. **PM Rate keeps Audio slot 12; `froggers-vco-topology` is relaxed to match — DECIDED, session 6.**
    Operator: *"A:1"*. The delta at `specs/froggers-vco-topology/spec.md` stands as written: the PM LFO's
    rate comes from a rate control, each VCO keeps its own LFO instance and its own depth, and the shared
    rate control is shared by design. The trade the operator accepted, stated plainly so a later session does
    not treat it as an oversight: **one PM Rate knob moves all three VCOs' LFO rates together** — per-VCO
    rates would need three slots the bank does not have (§9.5). That same delta independently carries ruling
    9's shared zero position.
11. **Patch compatibility is a NON-GOAL for this change — DECIDED, session 6.** Operator: *"who gives a
    shit, i haven't saved any old patches?"* **Verified rather than taken on the operator's word, since it
    voids a gate that has blocked T1 since session 1:** `~/Library/Sheaf/synth/sheaf-patch/patches/frogg3rs/`
    is empty (zero patch files), and the runtime config's MIDI-instrument block is `"controllers":[]` (zero
    MIDI-learn mappings) — the two surfaces §4.1 identified as slot- or name-sensitive both hold nothing.
    **Consequences, each recorded where it lands:**
    - **What is empty is the DATA, not the machinery — stated explicitly so a later session does not read
      this ruling as "the app cannot save."** Both features exist and both already live at the Sheaf level,
      not in this app: `External/Sheaf/projects/synth/src/PatchPersistence.cpp` (`BuildPatchJSON` /
      `LoadPatchJSON`, via `ParameterManager::ParameterValuesToJSON`) and
      `External/Sheaf/projects/synth/src/MidiController.cpp` (encoder in/out configs carrying `slotIx`).
      `app/` contains no patch or MIDI persistence file of its own — confirmed by listing both directories.
      So this is shared, pinned infrastructure the app gets for free and cannot change; the operator has
      simply never saved a patch or learned a controller on this machine. **And when they do, neither needs
      anything from this change** — see the corrected §4.1 bullet: parameter values are keyed by qualified
      name, so a newly registered parameter saves and loads with no per-parameter work, and MIDI mappings are
      keyed by controller slot and physical encoder position, so they are indifferent to which parameter sits
      in which bank slot.
    - **T1.0 — the Envelope migrate-or-reset gate — is MOOT, and T1.1-T1.6 are unblocked.** It asked how to
      handle saved patches across the Envelope renumbering. There are none, and no MIDI mapping to scramble
      either. The ergonomics half ("which physical encoder is Sustain under") survives only as a thing the
      operator will notice on the hardware, not as a decision anyone must make first.
    - **The default-parity RULE survives, on a different and much smaller justification.** Not migration —
      the instrument's own baseline voice. A knob whose default does not reproduce today's literal changes
      how the synth sounds at startup, and choosing the range so the default lands on the literal costs
      nothing at write time. Kept for that reason alone — as one spec requirement and one `defaultValue` line
   per registration task (T2.6/T3.6/T4.3/T5.8/T7.3/T8.3), not as the task block the audit first wrote.
    - **Everything written as migration machinery comes out**, per the operator's own framing: the Scoop
      Freq/Width and Scoop Depth "deliberate behaviour change to existing patches" apparatus (§4.1, §9.1) is
      now just "pick sensible defaults."
    - **Session 6, second pass — the operator pushed again: does bank expansion need to solve saved state at
      all, or does Sheaf already handle these parameters as-is? It already handles them, and the whole T9
      task block is deleted as over-scaffolding.** Verified across all three surfaces:
      `ParameterManager::ParameterValuesToJSON` iterates **every registered `Parameter*`** and keys by
      qualified `Name()`, so each of the thirty new parameters saves and loads with **zero per-parameter
      work**; modulation depths are keyed by modulation-source index, untouched by any target's slot; and
      MIDI mappings are keyed by hardware position, not parameter identity (corrected §4.1). **What is left
      is not a problem to solve, it is one field per registration:** `FroggersParamSpec::defaultValue`,
      already in the struct every task here already writes. Folded into T2.6/T3.6/T4.3/T5.8/T7.3/T8.3 as one
      line each. The single spec requirement stays, because "a new knob's default reproduces the literal it
      replaced" is a real property worth stating once — but it needed no task block behind it.

12. **Filter Topology is a CONTINUOUS morph, not a switch — DECIDED, session 6.** Operator: *"idiotic, i
    pick (a)"*, on the question they raised themselves (is Topology even continuous? — it was a `bool`,
    §9.1). **The implementation is NOT "compute both branches and crossfade," and this document says so
    before an implementer tries it:** `Comb::Process` advances a delay line and index, `ResonantBump::Process`
    is a `BiquadDf1` carrying `x[n-1]`/`x[n-2]`/`y[n-1]`/`y[n-2]`, and `pureDelay`, `combTrimSmoother`,
    `peakTrimSmoother` and `peakLimiter` are all stateful too (`app/dsp/FilterFx.hpp`, read this session).
    Calling any of them twice in one sample double-advances its state. A naive two-branch crossfade therefore
    costs a **second full set of instances** — two combs ringing independently — which is a different
    instrument, not a morph.
    **The construction that is actually continuous, single-instance, and strictly less code than today
    (§9.1 carries it in full): morph what the peak stage READS, not which branch runs.** Today's parallel
    branch already computes `combPath` (trimmed) and `peakPath = peakLimiter(peak(input) * peakTrim)`, then
    blends them by slot 7 and applies Scoop. Feed `peak` a crossfade between `input` and `combPath` instead
    of `input` alone, driven by the Topology knob, and: at 0 it is bit-identical to today's parallel; at 1
    the peak is processing the comb's output, which IS series; in between it is a genuine continuous morph.
    **Every trim, the peak limiter, the Comb/Peak blend and the Scoop blend stay in force at every Topology
    position** — which is the whole reason the old `useParallel == false` branch was dangerous. **So the dead
    series branch is DELETED rather than brought to parity** (per OMNI §1's corollary: when honouring an
    instruction forces a worse structure — here a duplicated stateful chain — challenge the instruction; the
    operator's "(a)" is satisfiable in a cleaner shape than the one the question offered). One code path, one
    set of units, one new lerp.
    **What still needs measuring, and why it stays flagged:** at high Topology the peak biquad sees the
    comb's output rather than the raw input, so its own worst-case input changes even though `peakTrim` and
    `peakLimiter` remain in place — a new operating point for the stage this file's blowout history is
    about (T2.1a).

## 4. New findings from this session's own re-verification (beyond the ones the task named)

### 4.1 Patch persistence is keyed by parameter NAME, not by bank-slot index

Traced the full path from `~/Library/Sheaf/synth/sheaf-patch/` patch files to the code that reads and
writes them:

```
PatchPersistence.cpp: BuildPatchJSON()
  root.SetNew("parameterValues", manager.ParameterValuesToJSON(arena))
    -> ParameterManager::ParameterValuesToJSON (ParameterModulation.cpp):
       for each Parameter*: root.SetNew(parameter->Name().c_str(), parameter->ToValueJSON(arena))
       -- keyed by the qualified Name() string (e.g. "Envelope Sustain VCO1"), NOT by slot index.

PatchPersistence.cpp: LoadPatchJSON()
  manager.LoadParameterValuesFromJSON(root.Get("parameterValues"))
    -> ParameterManager::LoadParameterValuesFromJSON (ParameterModulation.cpp):
       for each JSON member: Parameter* p = FindParameterByName(key); p->LoadValuesFromJSON(...)
       -- FindParameterByName is a linear scan comparing parameter->Name(), also NOT slot-indexed.
```

**Consequence: reordering which `paramIx` slot a named parameter occupies in `FroggersBankLayouts()`'s
array does not, by itself, cause a saved patch to silently reinterpret another parameter's stored value
at load.** The loader looks up "Envelope Sustain VCO1" by that string and applies its stored value to
whichever Parameter object currently owns that name, regardless of which slot that object is registered
into on this launch. `Parameter::LoadValuesFromJSON`'s own nested `modDepths` object is keyed by
**modulation-source index** (`modulationDepths_[modIx]`, one of the 15 `FroggersModulatorSlot` values,
`External/Sheaf/projects/synth/src/ParameterModulation.cpp`) — an entirely separate index space from the
target's own bank/slot position, so it is unaffected by the Envelope renumbering too. The full patch JSON
schema is `{schema, schemaVersion, patchName, parameterValues}` (`BuildPatchJSON`) — there is no
slot-index-keyed field anywhere in it.

**This is a real correction to the risk as the task brief framed it ("silently reinterpret those
slots"), not a reason to consider the migration question closed.** What this finding does NOT cover,
and what remains genuinely open (§8 keeps it open, deliberately):
- A patch made before this change has no "Envelope Decay VCO1" / "Envelope Curve" / "Envelope Grace" key
  at all; those parameters load at their ordinary `defaultValue` on an old patch, same as any other
  newly-added parameter (`froggers-sheaf-parameter-model`'s existing "No other parameter departs from its
  ordinary default" scenario already covers this case in general).
- The renumbering still changes which PHYSICAL encoder Sustain/Release sit under — an ergonomics/muscle-
  memory question, not a data-integrity one, and this finding says nothing about whether that alone
  justifies a migration UX (e.g., a one-time notice) independent of any file-format risk.
- ~~This finding audited the patch-file path specifically; it does not claim every other stateful surface in
  the app (e.g., any MIDI-learn mapping, which `MidiController.cpp`'s `MessageIn` JSON does serialize by raw
  `bankIx`/`slotIx`) is equally safe.~~ **WRONG — corrected session 6, after the operator asked why any of
  this is bank expansion's problem. The MIDI caveat was session 1's, and session 6 repeated it twice before
  reading the header; a repeated claim is the repeater's claim (OMNI §1).** `EncoderMidiMapping` is
  `{MidiControlAddress control; std::size_t slotIx; std::size_t position;}`
  (`External/Sheaf/projects/synth/include/synth/MidiController.hpp:227-231`), and in context — beside
  `visibleEncoderCount = 16` in every profile-options struct in that header — **`slotIx` is the CONTROLLER
  slot and `position` is the physical encoder position, both hardware-side. Neither is a bank-parameter
  index.** A learned CC maps to "encoder position 3 of controller 0", which then drives whatever parameter
  the current bank holds there. **So MIDI mappings are entirely insensitive to which parameter occupies which
  bank slot**: renumbering Envelope cannot scramble them, and encoder positions 9-13 already exist and
  currently drive empty slots. There was never a second at-risk surface.

**Session 6 audit — this finding was load-bearing for a guarantee it does not actually provide; then ruling
11 made the whole question moot. Both halves recorded, in order.** T2/T7/T8's headers each cite this finding
as "patch-safety by construction." It does guarantee that no stored VALUE is misapplied; it does not
guarantee that a patch still SOUNDS the same, and three mechanisms in this change break that. **But there are
zero saved patches and zero MIDI-learn mappings on disk (verified, §3a ruling 11), so none of the three has
anything to break.** What survives is the third bullet's rule, on the far smaller ground that a default sets
the instrument's own startup voice:

1. **Twelve of the thirty new parameters exist to expose a value that is hardcoded today** (§9.6's own
   Tier-1-style tally). A patch saved before them has no key for them, so they load at their
   `defaultValue` — and unless that default maps to the literal it replaced, the patch's sound changes.
   `FroggersParamSpec` already carries a `defaultValue` field and `FroggersBankLayouts()` already uses it
   for non-neutral cases (the Audio bank's own pitch defaults `0.2468f`/`0.3471f`/`0.4058f`, and every
   Sustain's `1.0f`), so this is cheap to get right and cheap to get wrong. **This is the one part ruling 11
   keeps** — not for patches, but because a default that misses the literal changes what the instrument
   sounds like on a fresh launch. Binding spec requirement ("A newly exposed hardcoded value defaults to the
   value it replaces"), carried out as one `defaultValue` line in each bank's own registration task rather
   than as a separate work package.
2. **Two of the twelve have no literal to default to: Scoop Freq and Scoop Width** (a live-patch problem
   before ruling 11, now only a question of what number they start at). Today `scoopNotch`
   is fed the *same runtime locals* as `peak` — `bumpFreq`/`bumpWidth`, themselves derived from Filter
   slots 1 and 3 (`FroggersAppCore.hpp:1341-1342` vs `:1292`/`:1327`). Their old behaviour is a function of
   two other knobs, not a constant, so no fixed default reproduces it across those knobs' range. **Post-ruling
   11 this is not a patch problem at all** — pick the value the Peak knobs' own defaults produce today and
   the fresh-launch voice is unchanged; there is nothing else to preserve (§9.1).
3. **Scoop Depth narrows an existing parameter's meaning** — T2.5 keeps slot 8 ("Scoop") on `SetHeight` and
   moves `scoopMix` to the new slot 13. Worth knowing, but with no saved patches it costs nothing: give slot
   13 the same default slot 8 has and a fresh launch is unchanged. **Recorded as a note, not as the
   migration hazard an earlier version of this section made it** (ruling 11).

### 4.2 Ring Mod's design, corrected in full — session 5

**Everything this section previously recorded — a second open pre-gate/post-gate sub-question, and a
claimed collision with `froggers-vco-topology`'s "No hardcoded cross-VCO coupling" requirement — was WRONG
and is removed here, not merely softened.** Both were the product of the same mistake: treating the design
doc's ambiguous "`v1`/`v2`/`v3` already exist as separate floats before the mix" evidence as license to
invent a special-cased Ring Mod mechanism, rather than reading the operator's actual instruction that Ring
Mod is an ordinary parameter (§3 ruling 1; §0a's session-5 addendum names this as the same error pattern
behind Reverb Tuned and the cut cross-couplers).

**The claimed collision was never real: say so plainly rather than deleting it silently.** There is no
cross-VCO coupling in the corrected design — each VCO's ring modulator carries its own INTERNAL carrier,
generated inside that VCO's own ring-mod stage, and never reads another VCO's signal. `froggers-vco-
topology`'s requirement (quoted in full at §3 ruling 2) is satisfied by this design, not violated by it:
*"The oscillator section SHALL contain no hardcoded VCO-to-VCO coupling terms. All inter-oscillator routing
SHALL be expressed only through the modulation matrix."* Ring Mod's carrier is not inter-oscillator routing
at all — it never touches another VCO's signal path — so this requirement was never in tension with it once
the design is read correctly. No exception, amendment, or postflight remediation is needed; there was never
a violation to remediate.

**The pre-gate/post-gate sub-question this section previously recorded is likewise dropped, not answered —
it was an artifact of the wrong framing (choosing what a cross-VCO carrier multiplies against) and does not
survive the correction.** It is not carried forward as an open item at §8; whether an implementer multiplies
a VCO's own pre-gate or post-gate signal by its own internal carrier is an ordinary implementation detail,
not a design fork this document needs to record, because either choice is the SAME VCO's own signal either
way — there is no longer a second party to the multiplication whose identity is in question.

**One genuine implementation detail remains open, recorded at §8 as an implementation question, not a
design blocker:** whether the bottom of the Ring Mod knob's range reaches sub-audio (so a low setting reads
as slow tremolo, with an effectively-clean position at the very bottom, the way `Vco`'s own PM LFO spans
`kPmLfoMinHz = 0.05f` to `kPmLfoMaxHz = 20.0f`, `app/dsp/Vco.hpp:103-104`) or stays audio-rate across the
entire sweep. Both satisfy the operator's continuous-range rule (§3 ruling 3); the choice only affects
whether Ring Mod can be dialled effectively off at one end of its travel, not whether it can be built. See
§9.5 for the full DECIDED entry.

**Session 6 audit — the paragraph immediately above is WRONG in its premise, and the real open question is a
different and larger one. Recorded here plainly, not deleted.** The claim that a sub-audio low end gives "an
effectively-clean position at the very bottom" does not survive contact with what the stage computes. Ring
modulation is a *product*: `vcoOut * carrier`. At a 0.05 Hz carrier the product is not clean — it is a
20-second full-depth tremolo that passes through zero and inverts phase twice per cycle. Lowering the
carrier's frequency lowers the *rate* of the amplitude modulation, never its *depth*. Worse, a sine carrier
evaluates to `0` at phase zero, so the neighbourhood of "DC" is silence, not unity. **There is no knob
position, at any frequency, at which the design as specified passes the VCO through unchanged**, because the
knob's only job is frequency and nothing in the design carries depth or mix.

**So the genuinely open item is not "sub-audio or not" — it is "what makes Ring Mod inaudible?", and that is
an operator question, not an implementation detail.** Every existing patch is affected: the moment Ring Mod
ships, all three VCOs are ring-modulated unconditionally, and no default value restores today's sound (the
§4.1 default-parity rule cannot help — there is no neutral position to default to). This is recorded as a
blocking gate, T8.0, and **deliberately not answered here**: a depth or dry/wet leg per VCO would cost three
more slots the bank does not have, and folding depth into the same knob (e.g. a dead zone at the bottom of
its travel) would re-open the operator's own continuous-range rule (§3 ruling 3). Both are the operator's
call under the same rule that cut Cycle and Hard Sync. §8 carries the item; the old sub-audio framing is
withdrawn from that list, since it was a smaller question resting on a false premise.

### 4.3 Decay is a materially bigger Tier-2 item than the design doc credited

The design doc's evidence for Decay ("New `Stage::Decay` inserted into the existing enum ... reusing the
same per-sample step/`mapTime` idiom") undercounts the work. Reading `VcoAdsrState::stepVoice` in full:
**Attack's target ceiling today is `sustainLevel` itself** — `m_level = min(sustainLevel, m_level +
attackStep)` — there is no separate peak above Sustain for a Decay stage to fall FROM. Inserting
`Stage::Decay` between Attack and Hold without also raising Attack's ceiling would produce a Decay stage
that starts and ends at the same level: no audible decay at all.

**Session 6 audit correction — it is a THREE-part change, not two, and the third part is the one that
silently changes existing patches.** `attackStep` is not a constant slope: it is `sustainLevel /
std::max(mapAttack(attackKnob) * m_sampleRate, 1.0f)` (`VoiceEnvelope.hpp`, confirmed by reading — the
numerator is `sustainLevel`, the same value that is also today's ceiling, which is exactly why the ramp
takes `attackTime` to arrive). Raise the ceiling to `1.0` and leave that numerator alone and the ramp still
climbs at `sustainLevel/attackTime` per second toward a target of `1.0`, so **the actual attack time becomes
`attackTime / sustainLevel` — up to 10x longer at the `kMinSustainLevel = 0.10f` floor.** Every existing
patch's Attack knob would quietly change meaning. So part (c): `attackStep`'s numerator becomes the new peak,
not the sustain level. T1.1 is updated.

A working Decay needs Attack's ceiling raised to an independent peak (the natural choice is `1.0`, the
same ceiling `Vco`'s own output already has), with Decay then ramping from that peak down to
`MapSustain(sustainKnob)` (`app/dsp/VoiceEnvelope.hpp`'s existing public static, already floored at
`kMinSustainLevel = 0.10f` — added 2026-08-07, one day after the design doc's read, per that operator's
own comment: *"the sustain minimum value is too low ... audio rate modulation ... would result in
silence"*). Still Tier 2 (reuses the same divide-by-mapped-time idiom `attackStep`/`releaseStep` already
establish) but a two-part change (raise Attack's ceiling; add the falling stage), not a one-part
insertion.

**Not a §7 (headroom) trip in the strict sense** — the peak level still never exceeds `1.0`, the same
bound `Vco`'s own amplitude already has, so `chainIn`'s average-of-three-gated-signals bound (§2) is
unaffected. **It is a real, worth-flagging dynamics change**: a patch with a low Sustain setting today
never rises above that low ceiling for the whole note; once Decay ships, the same patch's attack
transient reaches full scale before falling to that low Sustain level. Audible, not unsafe.

### 4.4 VCO Balance's headroom question is discharged by construction — session 4 supersedes this section's own earlier framing

`MixOscVoices` (`app/dsp/VoiceEnvelope.hpp`) returns `(v1 + v2 + v3) * (1.0f / 3.0f)` — confirmed live,
a hardcoded, un-parameterized equal-thirds average, with no per-VCO level control anywhere in the signal
path. The predecessor change's own §K per-stage headroom table (`openspec/changes/archive/2026-08-06-
frogg3rs-modulation-truth-and-voicing/tasks.md`) rules Audio **"No limiter needed — oscillators are
bounded and envelope-gated"** — that verdict's entire load-bearing assumption is that the three gated
voices are always combined by this exact equal-thirds average, which is provably `<= max(|v1|,|v2|,|v3|)`
regardless of any one voice's individual level.

**Superseded — session 4 operator ruling closes this as an open question.** This section originally
required VCO Balance's crossfade to be MEASURED at implementation time because a naive shape (each
voice's own weight scaling toward 1.0 independently) could, in principle, raise the weighted sum above
what the equal-thirds average produces today. **The operator has since ruled out that naive shape
entirely, by requiring a hard floor**, verbatim: *"but VCO balance needs a hard clamp such that there is
no maximum that is 100% only vco 1 or 2 or 3"* and *"in fact, the minimum balance of any vco in the mix
in the range of vco balance should be 10% of total vco mix."* This is not merely a musical constraint —
it is the thing that makes the crossfade's bound provable rather than dependent on which shape an
implementer happens to pick.

**The arithmetic, stated explicitly.** With three VCOs and a 10% floor on each (`w1, w2, w3 >= 0.10`,
`w1 + w2 + w3 = 1`), the maximum any single VCO can reach is capped at `1 - 0.10 - 0.10 = 0.80` — no knob
position can put any one VCO at 100% of the mix, and none can silence a VCO to 0%. **A convex weight
triple (weights non-negative, summing to exactly 1) is bounded by `max(|v1|,|v2|,|v3|)` by the same
arithmetic the plain equal-thirds average already satisfies** — `|w1*v1 + w2*v2 + w3*v3| <= w1*|v1| +
w2*|v2| + w3*|v3| <= (w1+w2+w3)*max(|v1|,|v2|,|v3|) = max(|v1|,|v2|,|v3|)` for ANY convex triple, not only
the equal `1/3,1/3,1/3` split. **§K's "no limiter needed for Audio" verdict therefore survives untouched
for the whole floored range, not merely at the equal-thirds default** — the floor plus the convexity
constraint together turn what was a "must be measured, shape-dependent" risk into a provable invariant of
the parameter's own definition.

**Deliberate parallel to `dsp::VcoAdsrState::kMinSustainLevel` (§4.3, above), per the operator's own
framing.** `kMinSustainLevel` exists so modulation cannot gate a voice's envelope down to silence — this
floor exists so the Balance knob cannot gate a VCO out of the mix entirely. Same reasoning, same class of
constraint: a knob that can zero out a signal path is a hazard this codebase has already named and fixed
once. Confirmed by reading the archived `2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/
tasks.md` (`S4.6`: *"kMinSustainLevel = 0.05 was chosen by argument, not by measurement"*) against the
current `VoiceEnvelope.hpp` (`kMinSustainLevel = 0.10f`, dated 2026-08-07, the operator's own quoted
concern): the operator raised that floor from 0.05 to 0.10 in the predecessor change, so a 10% floor here
is consistent with their own established preference, not a fresh number picked for this change alone.

**Consequence for what implementation must prove:** the crossfade is no longer merely required to sum to
a constant total — it is required to keep `w1 + w2 + w3 == 1` with each `wi` in `[0.10, 0.80]` across the
whole knob sweep. This is an assertable invariant of the mapping from knob position to weights, checkable
by inspecting the mapping function directly, not a property that needs measuring `chainIn`'s output level
at runtime. §9.5 and §9.6 are updated accordingly: VCO Balance is no longer on the headroom-flagged list.

**Session 6 audit — the convexity argument is CORRECT and the flag stays discharged; what was missing is the
§4.3-shaped note that goes with it.** Re-derived independently: for any `w ≥ 0` with `Σw = 1`, `|Σ wᵢvᵢ| ≤ Σ
wᵢ|vᵢ| ≤ max(|v1|,|v2|,|v3|)`, and that supremum is exactly the equal-thirds average's own supremum (both are
attained when the three voices are identical), so §K's verdict survives across the whole floored range. The
bound does not move. **The level does.** At the tilt's extreme one voice's weight goes from `1/3` to `0.80`
— **+7.6 dB on that voice's own contribution, and about +3 dB on the three-voice sum for uncorrelated VCOs**
(`√(0.80² + 0.10² + 0.10²) = 0.81` versus `√3/3 = 0.58`). That is precisely the case §7's standing rule and
§4.3's Decay finding exist to name: *a proposal can fail to raise the sample ceiling while still changing
perceived loudness.* Recorded here in the same terms §4.3 used — **audible, not unsafe** — so that "came off
the headroom-flagged list" is not read as "changes nothing downstream." Everything after the mix (the Drive
chain's trims, the comb trim, the wet limiters) sees a louder average at the tilt's ends than it does today
at any setting.

### 4.5 The shared zero-off gate, and what the parameter class already provides — session 6

**The operator's question, verbatim: *"this is probably already a thing, like part of the class constructor
of parameters, no?"* Answered by reading `External/Sheaf/projects/synth/include/synth/
ParameterModulation.hpp` and its `.cpp`, not from memory. The answer is: half of it exists, one half of that
half is inert, and the app uses none of it.**

- **A per-parameter type tag DOES exist in the constructor.** `ParameterConfig` carries `RangeKind range`
  (`Unipolar`/`Bipolar`) and `std::size_t switchValues` alongside `name`/`shortName`/`defaultValue`
  (`ParameterModulation.hpp:260-269`). So the shape the operator remembers is real.
- **But the one type-driven post-processing function ignores the type.** `ClampToRange(float value,
  RangeKind range)` is, in full: `(void)range; return std::clamp(value, 0.0f, 1.0f);`
  (`ParameterModulation.cpp:449-452`). Bipolar and Unipolar clamp identically. **This is the function
  `Parameter::GetRaw` applies to every resolved value (§2), so today "parameter type" changes nothing about
  how a value is bounded.** Recorded as a finding, not a complaint: `External/Sheaf` is pinned and
  unpatchable (`tasks.md` §0), so this is not ours to fix — it is the reason the gate cannot live there.
- **The "array of functions that vary by parameter type" the operator described also already exists — one
  layer up, as mapping rather than limiting.** `ParameterManager` exposes six of them: `GetLinear`,
  `GetExponential`, `GetZeroBasedExponential`, `GetBipolarLinear`, `GetBipolarExponential`,
  `GetBipolarZeroBasedExponential` (`ParameterModulation.hpp:805-811`). **This app calls none of them —
  zero hits across `app/`.** `FroggersAppCore.hpp`'s `RouteAudioSample` instead reads a raw knob
  (`knob()` → `CachedKnobValue(0)`) and open-codes the map at each site: **six knob-fed
  `dsp::ExpMapCompute` calls in that one function.** That is a §8 repetition finding with a ready-made
  destination. **Session 6, second look: this was written up as an §8 finding and then withdrawn** — §8 asks
whether the same logic is WRITTEN twice, and in this app it is written once and called six times, which is
what §8 asks for; the second copy is in a pinned dependency, and consuming it would put a throwing,
out-of-line call in a per-sample audio path. The rejection is recorded in `tasks.md`'s "Recorded, not
scheduled." **What survives from this whole line of inquiry is the answer to the operator's question, above:
the parameter class carries a type tag, its one type-driven function ignores it, and the zero-off ramp
therefore lives app-side.**

**So the gate is app-side, and it is ONE function, per the operator's instruction.** Proposed home:
`app/dsp/DspMath.hpp`, beside `ExpMapCompute`/`ZeroedExpCompute`/`WrapPhase`/`Sine01` — the file that already
holds this app's shared scalar primitives, so it adds no new file and no new concept location.

**§8 enumeration, by operand rather than by syntax, before writing anything.** Searching for the concept's
operands (`Floor`, `RampWidth`, the smoothstep `3.0f - 2.0f * t`, and the zero-threshold comparisons) finds
**one** full instance and two near-misses that are deliberately different — classified, per §8, rather than
collapsed:
1. **`Vco::PmDepthScale`** (`app/dsp/Vco.hpp:137-150`) — floor `kPmLfoFloor = 0.02f`, ramp
   `kPmLfoRampWidth = 0.08f`, smoothstep `t*t*(3-2t)`. **The one true instance.** Ring Mod's gate makes two,
   which is what promotes this from a private static to a shared primitive.
2. **`StereoDelay::Process`'s `p.dsnd <= 0.0001f` early-out** (`app/dsp/Delay.hpp:303`) — NOT the same
   concept and must not be folded in: it is a whole-stage bypass for cost, with no ramp, and giving it one
   would make the delay partially process where it currently does not run at all.
3. **`MapSustain`'s `kMinSustainLevel` floor** (`app/dsp/VoiceEnvelope.hpp:177`) — the *opposite* concept, a
   floor that forbids zero. It is the same family as VCO Balance's 10%/80% clamp (§4.4) and as Fold's
   strictly-positive divisor (§9.2). **So there are two families, not one**, which is the useful half of the
   operator's "array of limiter functions" idea:
   - **Family A — true zero at the bottom, smooth ramp up.** PM depth; Ring Mod depth. Shared function.
   - **Family B — never reach zero, floor the value.** Sustain; VCO Balance weights; Fold's divisor. Each
     has a different floor for a different reason, so this family shares a *rule*, not yet a function —
     three call sites with three different bounds do not pass §6's 2-of-4 as one helper.
**Recorded honestly: found 3, sharing 1 (plus Ring Mod's new one), leaving 2 classified as deliberately
different** — a partial de-duplication reported as partial, per §8.

**§6 2-of-4 check on the new helper, since this change is what creates it:** (1) reused — PM and Ring Mod,
two uses on day one ✓; (2) isolates a distinct transformation stage — knob position → depth taper, separate
from knob position → frequency ✓; (4) prevents repetition of structurally similar code ✓. Three of four; the
helper is allowed. It takes the floor and ramp width as arguments rather than baking PM's `0.02`/`0.08` in,
so Ring Mod can be tuned by ear (T8.4) without touching PM's parity-critical constants — and PM's own call
passes exactly today's two constants, so **PM's behaviour is bit-identical after the refactor**, which is
what T8.0a's test asserts.

## 5. What this change specifies for implementation vs. leaves untouched

**Specified (spec delta in `specs/froggers-sheaf-parameter-model/`):**
- Envelope bank's target 14-parameter layout (§3 rulings 3-4), added as a MODIFIED requirement.
- **Session 2 additions:** Reverb bank's target **fourteen**-parameter layout, COMPLETE (§9.4).
- **Session 3 additions:** Filter bank's target **fourteen**-parameter layout, now COMPLETE (§9.1); Drive
  bank's target **fourteen**-parameter layout, now COMPLETE (§9.2); Delay bank's target **fourteen**-
  parameter layout, now COMPLETE (§9.3); Audio bank's target **eleven**-of-fourteen-parameter layout at
  that point — PM Rate and VCO Balance specified at slots 12-13, slots 9-11 (Ring Mod) not yet corrected
  — all five added or extended as MODIFIED requirement scenarios in the same spec delta.
- **Session 5 addition:** Audio bank's target **fourteen**-parameter layout, now COMPLETE (§9.5) — Ring
  Mod (slots 9-11) corrected and specified, an ordinary per-VCO parameter with an internal carrier, added
  as a MODIFIED requirement scenario in the same spec delta.
- A new, general requirement formalizing §4.1's finding — patch persistence safety under a bank-slate
  expansion — because it is true today, independent of Envelope specifically, and every bank-fill
  decided across sessions 2-5 (Filter/Drive/Delay/Reverb/Audio) relies on the same guarantee rather than
  re-deriving it.
- **Session 6 additions (the §14 preflight audit):** a second requirement in the same delta, "A newly
  exposed hardcoded value defaults to the value it replaces," with scenarios for the two cases no default can
  absorb (§4.1); the `[0, 1]` cross-feed bound and the buffer-capacity bound on Delay's Width Balance (§9.3);
  and **a second spec delta, `specs/froggers-vco-topology/spec.md`** — required because PM Rate collides with
  two in-force clauses of that spec and Ring Mod's internal carrier is worth asserting there rather than only
  claiming here (§9.5). The vco-topology delta relaxes a live requirement and is gated on the operator
  (T7.0).

**Not specified, not implemented, no task below closes it:**
- Cross XOR, Cycle, Hard Sync, Peak Slope, self-FM, Glide/portamento/slew, VCO Spread, Sub-Oscillator, PM
  Depth Max — all cut (§3 rulings 2-3, 6-8); nothing to build or remove for any of them. PM Depth Max's
  own underlying question (is `kPmLfoDepth = 0.15` the right ceiling?) is carried forward in §8 as an
  open by-ear tuning item, not a parameter.
- Reverb Tuned is part of the COMPLETE Reverb slate (§9.4) and IS specified — no longer merely a cost
  correction (§3 ruling 5's correction still stands as the reasoning that unblocked it).
- **Ring Mod (Audio 9-11) is now specified, not on this list — session 5 (§3 ruling 1, §9.5).** Through
  session 4 it was wrongly listed here as "the only remaining blocker in the entire change"; that framing
  is corrected and removed, not merely softened (see the session-5 note in §1, §0a, §4.2).
- **Zero slots remain marked PENDING RESEARCH anywhere in this change** — Filter 13, Drive 10-13, Delay
  10/12/13, Audio 12-13, and now Audio 9-11 (Ring Mod, session 5) are all specified (§9.1-9.3, §9.5).
- Everything remaining in §8.

## 6. Constraints

- **Markdown only.** No file under `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/` or `External/Sheaf`,
  and no other source file, is touched by this change. Nothing is built.
- **Cite by SYMBOL, not by line** — §0 demonstrates why; every line number in this directory goes stale
  on the next edit anywhere above it in the same file.
- **Subagents: Sonnet or Haiku, never Opus** — applies to whichever future session implements the tasks
  below; nothing was dispatched to build anything under this change.
- **`nice make -j2`, never higher** (8-core/16 GB) — applies to a future implementing session; this
  change does not build.
- **No AI attribution on commits** — applies when this change (or its follow-on implementation) is
  committed.
- **An implementer may not close a task whose resolution requires an operator decision** — see tasks.md;
  the Envelope implementation task is written but explicitly blocked on the still-open migration/reset
  question (§8), and the Ring Mod line item is not written as an implementation task at all, only as a
  recorded, blocked design note, because two prerequisite questions (§4.2) are unanswered.

## 7. Standing headroom rule (carried forward, restated because it binds anything picked up from §8)

Any proposal that can raise a stage's output level needs its trim/limiter budget re-derived, not assumed
— the design doc's own "Trips §K?" column exists for exactly this, and §4.3 shows a proposal can fail to
literally raise the sample ceiling while still changing perceived loudness in a way worth measuring.
"Already has a downstream limiter as a backstop" is not the same as "measured safe" (the design doc's own
words, carried forward unchanged: *tasks.md's own §K standing rule*).

### 7a. The in-loop saturator pre-gain rule — stated ONCE, for all three sites (session 6)

**Why this exists: three parameters in this change are the same construct, and sessions 2-3 reasoned about
them three separate times and reached two different answers.** The operator caught it by asking why knob
extremes are new problems. This is §8's own failure mode at the level of *arguments* rather than code — one
concept, restated per site, drifting between restatements — so the fix is to state it once here and have
every site cite it.

**The construct.** Three feedback loops in this app write `x + k * PadeSaturator::Saturate(·)`:
- `Comb::Process`: `input + feedback * Saturate(filter.Process(tapped))` (`app/dsp/FilterFx.hpp`)
- `StereoDelay::Process`: `inSignal + fbk * Saturate(fbL)` (same for `fbR`) (`app/dsp/Delay.hpp`)
- `Reverb`'s tank: `preOut + fb * Saturate(aFb)` (same for `bFb`) (`app/dsp/Reverb.hpp`)

**The rule.** `PadeSaturator::Saturate` ends in `std::max(-1.0f, std::min(1.0f, output))`
(`app/dsp/FilterFx.hpp`) — an unconditional hard clamp for any finite input. So each loop's per-sample bound
is `|x| + |k|`, **for any gain applied to the saturator's ARGUMENT**. A pre-gain into an in-loop
`Saturate(·)` therefore cannot raise that loop's bound, and needs no re-derivation. This is not a new
argument: it is the one this change already accepted, in `Delay.hpp`'s own words, to withdraw the round-1
flag on Feedback Drive (§9.3).

**The placement this rule depends on, pinned here because the proposal previously left it ambiguous
("a pre-multiply inserted before that call" reads both ways):** the gain multiplies what goes INTO
`Saturate`, not what comes out of it.
- **Inside** — `Saturate(g * tapped)` — bound `|x| + |k|`, unchanged for any `g`. This is also the musically
  correct reading of "drive": more signal into a nonlinearity, same ceiling.
- **Outside** — `g * k * Saturate(·)` — bound becomes `|x| + g*|k|`, and the comb's own trim would have to
  become `1/(1 + g*|fb|)`. Recorded so a later implementer who puts it there knows what else must change,
  rather than silently invalidating a formula.

**What the rule does NOT cover, kept explicit so it is not over-applied:** it bounds the peak, not the
perceived level. Heavier drive pins the saturator nearer ±1 more of the time, so the loop's typical output
rises toward a worst case that itself does not move — **audible, not unsafe**, the same distinction §4.3
(Decay) and §4.4 (VCO Balance) already draw. And it says nothing about a gain applied anywhere other than a
saturator's input: Reverb Tuned still needs its measurement (§9.4), because sweeping a delay LENGTH inside a
loop is a different mechanism entirely.

## 8. Non-goals — explicitly STILL OPEN, not filled in by this change

Restated and, where later sessions changed the facts, updated — because inventing an answer to any of
these would be exactly the mistake this change exists to avoid. Items resolved (decided or rejected) have
moved out of this list into §3 (rejections) or §9 (decisions); **as of session 5, no bank slot anywhere in
this change is still marked PENDING RESEARCH, and Ring Mod's carrier framing — previously the one item
left on this list — was itself wrong and is removed, not resolved by an operator answer** (§3 ruling 1,
§4.2, §9.5). What remains here is what is still genuinely open for reasons other than "the research hadn't
been written yet" or "the framing was wrong":

- ~~How Ring Mod is turned off.~~ **ANSWERED, session 6 — §3a ruling 9, §4.5, §9.5.** A true zero position at
  the bottom of Ring Mod's own knob, implemented as one function shared with PM. Costs no slots, keeps the
  knob continuous, and gives default parity for free. Session 5's "whether the knob's low end reaches
  sub-audio" item stays withdrawn (false premise) and what replaced it is now decided, so nothing about Ring
  Mod remains on this list except the by-ear frequency range (T8.4), which no longer gates anything.
- ~~Whether relaxing `froggers-vco-topology` for PM Rate is acceptable.~~ **ANSWERED — §3a ruling 10.** PM
  Rate keeps Audio slot 12; the `specs/froggers-vco-topology/spec.md` delta stands as written.
- ~~What Filter Topology should be.~~ **ANSWERED — §3a ruling 12: a continuous morph.** Raised by the
  operator, answered by the operator in the same session. The dead series branch is deleted rather than
  brought to parity, because morphing the peak stage's input reaches the same sound with one set of stateful
  units instead of two (§9.1).
- **Whether `kPmLfoDepth = 0.15` is the right maximum PM depth — session 3, an open by-ear tuning item, NOT
  a parameter (§3 ruling 8).** PM Depth Max (a knob to rescale this ceiling live) is CUT — a meta-control,
  not a sound. The ceiling's own correctness is a separate, still-open question: if by-ear testing finds
  0.15 too shallow, the fix is changing the constant directly, the same class of change as
  `kMaxAttackSeconds`'s own operator-judged 2.5s → 1.0s lowering (`VoiceEnvelope.hpp`) — not adding a knob.
- **The design doc's open question 8**, which may outrank everything here: the ASR envelopes cannot
  modulate anything, and the 15-source slate is full, with three slots (VCO EF) partially duplicating
  what a true envelope source would do better. A modulation-slate question, not a bank-slot question.
  Untouched by sessions 2-5.
- ~~Whether the Envelope renumbering migrates or resets saved patches.~~ **MOOT — §3a ruling 11, session 6.**
  There are no saved patches and no MIDI-learn mappings on disk (verified, not assumed), so there is nothing
  to migrate or reset. **T1.0, the gate that has blocked the Envelope implementation since session 1, is
  closed and T1.1-T1.6 are unblocked.** What is left is the ergonomics of Sustain and Release moving to
  different physical encoders — something the operator will notice at the hardware, not a decision anyone
  must make before building.

- ~~Default parity for the twelve unlocked literals; how its three unabsorbable cases are handled.~~
  **ANSWERED — §3a ruling 11: patch compatibility is a non-goal, verified against an empty patch directory
  and an empty MIDI-controller list.** The default-parity rule survives only as a fresh-launch-voice
  convenience, one `defaultValue` per registration; the migration machinery and the T9 block written for
  it are both deleted.

**Reverb, Filter, Drive, Delay, and — as of session 5 — Audio are no longer on this list.** Session 1 left
Reverb fully untouched; session 2 made it COMPLETE (§9.4); session 3 made Filter, Drive, and Delay COMPLETE
too (§9.1-9.3), closing out every PENDING RESEARCH slot those three banks carried into this session.
**Filter/Drive/Delay's "which of the four drive-into-a-feedback-loop controls to build" question (session
1's lead recommendation) is resolved by construction**, not by picking from the four: Comb Drive (Filter,
§9.1), Delay Feedback Drive (§9.3), and Reverb Tank Drive (§9.4, already decided in session 1) are now all
three DECIDED — the fourth option named in that recommendation, the existing Drive-bank Fuzz/Shape
controls, was never an empty-slot candidate to begin with. **Session 5 closes Audio too** — PM Rate and
VCO Balance (slots 12-13) were already decided (§9.5); Ring Mod (slots 9-11) is now DECIDED as well, its
prior blocker framing corrected and removed (§3 ruling 1, §4.2). **All six banks are now fully decided at
fourteen parameters each; this change records zero remaining bank-slot blockers.** **Session 6 audit: still
true of the SLOTS — no bank slot is undecided — but not true of execution. Three operator gates now block it:
T7.0, T8.0 and the default-parity question (§11) — all three since answered.**

## 9. Filter, Drive, Delay, Reverb, Audio bank decisions — sessions 2-3

**Provenance note.** Five research files, written outside this repo and read in full across sessions 2-3,
supply the candidate lists this section draws from — cited here by summary, not by link, per this
change's own instruction, since none are part of the tree. **Round 1 (session 2):**
`RESEARCH-audio-filter.md` (Audio slots 12-13, Filter slots 10-13), `RESEARCH-drive-delay.md` (Drive slots
10-13, Delay slots 9/10/12/13), and `RESEARCH-reverb.md` (Reverb slots 11-12). **Round 2 (session 3, this
update):** `RESEARCH2-audio-filter.md` (Audio slots 12-13, Filter slot 13) and `RESEARCH2-drive-delay.md`
(Drive slots 10-13, Delay slots 10/12/13) — each explicitly re-reads round 1's rejected-candidates file
and the operator's central rule before proposing anything, and each ranks its candidates by the same
three criteria round 1 used: DSP-reuse cost (reuses-existing / composes-existing / genuinely-new),
precedent strength, and headroom risk, screened against continuity across the whole knob sweep, no
duplication of the 15-source modulation matrix, and not on the already-rejected list. **§0a's central
selection rule, finalized after round 1 was written, retroactively cuts two of round 1's own top-ranked
Audio candidates** — Glide (its rank 1) and self-FM/"Feedback" (its rank 2) — leaving PM Rate (round 1's
rank 3) as the highest-ranked survivor carried into this session. **Round 2's own top-ranked Audio
candidate, PM Depth Max, is REJECTED by this session** (§3 ruling 8) on grounds independent of both
rounds' own routing-based ranking method — a meta-control objection round 2's own cost/precedent/headroom
framework was not designed to catch, since PM Depth Max passes all three of those tests cleanly. This is
why PM Rate (round 1's carryover, round 2's own rank 2), not round 2's own top pick, is the candidate that
ships at §9.5.

**Every slot round 1 left PENDING RESEARCH is filled this session — zero PENDING RESEARCH markers remain
anywhere in this change.** Filter slot 13, Drive slots 10-13, Delay slots 10/12/13, and Audio slots 12-13
are decided below, each citing round 2's own ranked candidate list and re-verifying every literal against
the current tree rather than trusting either round's research verbatim.

### 9.1 Filter bank — COMPLETE, all fourteen slots DECIDED

- **Slot 9 — Topology (`Topo`) — DECIDED as a CONTINUOUS morph, session 6 (§3a ruling 12); the operator
  asked whether it was continuous, and it was not.** `FilterFxChain::Process`'s `useParallel` bool is
  hardcoded `/*useParallel=*/true` at its one production call site (`FroggersAppCore.hpp`'s
  `RouteAudioSample`) — verified, and unchanged. What sessions 1-5 never did is read the branch it would
  switch INTO.
  - **It was a `bool`, which fails §3 ruling 3's continuous-range test** — the rule that cut Cycle and Hard
    Sync. Topology carried a "Tier 1, already covered" tag from session 1, i.e. from before that rule existed
    in session 2, and **no session re-tested it after the rule arrived.** §3 ruling 6's "Kept: Filter
    Topology" was Topology vs Peak Slope, not a continuity check.
  - **The two branches were never the same filter wired differently — one carries the entire blowout defence
    and the other has none.** Confirmed by reading `FilterFxChain::Process` in full (`app/dsp/FilterFx.hpp`):
    the parallel branch applies `combTrim` (`rawCombTrim`, smoothed), `peakTrim` (`1/peak.height`, smoothed)
    and `peakLimiter`, then the Comb/Peak blend and the Scoop blend. The series branch is, entire:
    `output = pureDelay.Process(input); output = comb.Process(output); output = peak.Process(output); return
    output;` — **no comb trim, no peak trim, no peak limiter, and `combPeakBlend` and `scoopMix` unused**, so
    a switch to series would also have silently disabled slot 7 and all four Scoop parameters (8/10/11/13).
    Dead code today, never executed, and the port's pre-blowout-fix shape.
  - **DECIDED shape (§3a ruling 12) — morph the peak stage's INPUT, keep one instance of everything:**
    ```
    combPath = combTrim  * comb.Process(pureDelay.Process(input))     // unchanged, once per sample
    peakIn   = input * (1 - topo) + combPath * topo                   // the new line
    peakPath = peakLimiter(peak.Process(peakIn) * peakTrim)           // unchanged, once per sample
    mixed    = peakPath * (1 - combPeakBlend) + combPath * combPeakBlend   // unchanged
    return     mixed * (1 - scoopMix) + scooped * scoopMix                 // unchanged
    ```
    `topo = 0` is bit-identical to today; `topo = 1` is series (peak processing the comb's output); between
    is a real morph. **Every unit is still processed exactly once per sample** — which matters because
    `Comb::Process` advances a delay line and `ResonantBump::Process` is a `BiquadDf1` with four state terms,
    so the naive "run both branches and crossfade" would need a duplicate stateful chain (§3a ruling 12).
    **The `useParallel` parameter and the dead series branch are deleted**, not kept beside the morph.
  - **Kept live by construction, where the switch would have killed them:** slot 7's Comb/Peak blend still
    mixes dry comb against the peak path at every Topology position, and Scoop still applies. Cost:
    reuses-existing plus one lerp; no new state, no new struct.
  - **⚠ HEADROOM FLAGGED, and this one is real rather than provisional (§9.6, T2.1a):** at high Topology the
    peak biquad's input is the comb's output rather than the raw input, so the stage's worst case moves even
    though `peakTrim` and `peakLimiter` stay in place. Measure across the whole Topology sweep, not only at
    its ends — the midpoint is its own operating point.
- **Slot 10 — Scoop Freq (`ScFq`), pure reuse.** `filterChain_.scoopNotch.SetFreq(bumpFreq)` is fed the
  identical local variable passed to `peak.SetFreq(bumpFreq)` (§2, session 2 addition) — Scoop has no
  frequency of its own today, only its height/mix knob. `dsp::ResonantBump::SetFreq` (`app/dsp/
  FilterFx.hpp`) already exists and is already called on `scoopNotch` every sample; only the value fed to
  it changes. Cost: reuses-existing. Headroom: none — frequency doesn't touch the notch's dip-gain
  formula.
- **Slot 11 — Scoop Width (`ScWd`), pure reuse.** Same gap, same `dsp::ResonantBump::SetWidth`,
  same 100%-reuse story as Scoop Freq. Cost: reuses-existing. Headroom: none.
- **Session 6 audit — Scoop Freq and Scoop Width are the two parameters with no literal to default to.**
  Today's scoop frequency and width are not constants: they are `bumpFreq`/`bumpWidth`, the same runtime
  locals `peak` uses, derived from Filter slots 1 and 3 (`FroggersAppCore.hpp:1341-1342` vs `:1292`/`:1327`).
  Once slots 10/11 own them, the scoop stops tracking the Peak knobs — the whole point of the decoupling.
  **With no saved patches (§3a ruling 11) this needs no migration story, only a default:** set them to the
  values the Peak knobs' own defaults produce today and a fresh launch sounds unchanged (T2.6).
- **Slot 12 — Comb Drive (`CDrv`), composes-existing. ⚠ FLAG WITHDRAWN — session 6, and the flag was wrong
  rather than merely cautious.** Reuses `dsp::PadeSaturator`, already the comb's own in-loop saturator
  (`feedback * PadeSaturator::Saturate(filter.Process(tapped))`, `app/dsp/FilterFx.hpp`) — a pre-multiply on
  that call's ARGUMENT (§7a pins the placement).
  **Why the flag was wrong, recorded rather than quietly dropped.** Sessions 2-3 flagged this on the reasoning
  that `rawCombTrim = 1.0f / (1.0f + std::fabs(comb.feedback))` was measured against `|comb| <= A + |fb|` at
  unity input gain, so a pre-gain raises the worst case. **It does not: `Saturate` hard-clamps to ±1 before
  the multiply by `feedback`, so `|output| <= |input| + |fb|` for ANY pre-gain, and the trim's own premise is
  untouched.** That is the identical argument this same document used to withdraw the round-1 flag on Delay
  Feedback Drive (§9.3) — over the identical construct, `x + k * Saturate(·)`. **Keeping one and withdrawing
  the other was an inconsistency, not a difference between the two stages**; §7a now states the rule once so a
  fourth site cannot drift again. The operator found this by asking why knob extremes are new problems.
  **What remains, and it is not a headroom item:** heavier drive pins the saturator nearer ±1 more of the
  time, so the comb branch's typical level rises toward a worst case that does not move — **audible, not
  unsafe** (§7a, §4.3's own phrasing). Default is unity gain, so today's voice is the knob's starting point
  (T2.6).
- **Slot 13 — Scoop Depth (`ScDp`), reuses-existing — session 3.** Confirmed by reading
  `FroggersAppCore.hpp`'s `RouteAudioSample`: `knob(FroggersBankId::Filter, 8)` is read **twice**, at two
  different call sites, for two different jobs:
  ```
  filterChain_.scoopNotch.SetHeight(std::max(0.05f, 1.0f - 0.95f * knob(FroggersBankId::Filter, 8)));
  ...
  filterChain_.Process(driveOut, /*useParallel=*/true, knob(FroggersBankId::Filter, 7),
                        knob(FroggersBankId::Filter, 8));
  ```
  The second read feeds `FilterFxChain::Process`'s fourth parameter — confirmed by reading its signature,
  `float Process(float input, bool useParallel, float combPeakBlend, float scoopMix)`
  (`app/dsp/FilterFx.hpp`) — and `scoopMix` is used at `return mixed * (1.0f - scoopMix) + scooped *
  scoopMix;`. One knob simultaneously sets the notch's own dip depth (`SetHeight`) **and** how much of
  the notched signal blends into the output (`scoopMix`) — the identical conflation shape that justified
  Scoop Freq and Scoop Width: two independent jobs that happen to share a value only because nothing ever
  separated them. Decoupling lets a deep notch blend in only lightly (surgical, subtle) or a shallow dip
  blend in fully (broad, gentle EQ) — states today's single knob cannot reach. Cost: reuses-existing —
  both `dsp::ResonantBump::SetHeight` and `FilterFxChain::Process`'s existing `scoopMix` blend already run
  every sample; only which knob feeds which argument changes, zero new DSP code, matching Scoop Freq/
  Width's own precedent exactly. Headroom: none — `SetHeight`'s formula is a dip bounded to `<= 1.0`
  (`max(0.05f, 1.0f - 0.95f * x)`) regardless of the knob value, and `scoopMix` is a convex blend of two
  already-bounded signals (`mixed`, `scooped`); decoupling the two cannot raise output level under any
  combination. **Session 6 audit — the headroom claim holds (both formulas re-read and confirmed).** T2.5 leaves slot
  8 driving `SetHeight` and moves `scoopMix` to slot 13; give slot 13 the same default slot 8 carries and the
  fresh-launch voice is unchanged, which is all that is required now that patch compatibility is a non-goal
  (§3a ruling 11). T2.6 records the number.

### 9.2 Drive bank — COMPLETE, all fourteen slots DECIDED

- **Slot 9 — Anti-Alias Brightness (`ABrt`), Tier 1, §2 session-2 addition.** `Oversampler2x`'s
  constructor hardcodes `antiAlias.SetAlphaFromNatFreq(0.4f)` (`app/dsp/Drive.hpp`) — the one-pole
  anti-alias filter inside the Drive bank's oversampling path has a fixed cutoff, never exposed. Real,
  unreachable DSP.
- **Slot 10 — Link (`Link`), reuses-existing — session 3.** Confirmed by reading `PolynomialDrive::
  SetCoefs` (`app/dsp/Drive.hpp`): `coefs[1] = 10.0f * Sine01(coefsKnob * 1.618f + 0.25f * (computedGain -
  1.0f));` and `coefs[3]` carries the identical `0.25f * (computedGain - 1.0f)` term. Drive's own resolved
  gain (`computedGain`) leaks into two of Shape's five coefficients at a hardcoded `0.25f` weight — nothing
  lets that leakage be dialed independently of Drive's own gain or Shape's own base coefficients. Link
  replaces the literal `0.25f` with a knob-driven scalar. Cost: reuses-existing — the existing `SetCoefs`
  call already runs every block; only the fixed weight becomes a knob-fed multiply. Headroom: none,
  provably — confirmed by reading `PolynomialDrive::Process`, `gain * (input * coefs[0] + input2 *
  coefs[1] + input3 * coefs[2] + input4 * coefs[3] + input5 * coefs[4])`: every term carries a positive
  power of `input`, so `Process(0) == 0` exactly regardless of the coefficients, at every Link setting; and
  `coefs[1]`/`coefs[3]` are `10 * Sine01(...)`, bounded to `[-10, 10]` for any argument, so scaling the
  coupling weight cannot push them outside the range they already reach today.
  **Session 6 audit — the literal and the reuse story are confirmed exactly (`Drive.hpp:101,103`); the
  "provably" is WITHDRAWN, and replaced with a measurement rather than deleted.** Two defects in the argument
  as written: (a) bounded coefficients do not bound this stage's output, because `PolynomialDrive::Process`
  is an unbounded 5th-order polynomial — *this document's own Bias entry (slot 13, below) rejects exactly
  this inference*, so clearing Link on it is internally inconsistent; and (b) "the range they already reach
  today" is the wrong set. Today the coupling term is `0.25 * (gain - 1)` with `gain = ExpMapCompute(1, 5,
  knob)` (`Drive.hpp:90`), i.e. a phase offset spanning exactly `[0, 1]` — one full `Sine01` cycle — but
  **locked to gain**: at `gain = 5` the offset is exactly `1.0 ≡ 0`, the same coefficient curve as `gain =
  1`. Link unlocks that pairing, so maximum gain can be combined with coefficient phases that today occur
  only at intermediate gains. The reachable `(gain, coefs)` set grows; boundedness of `coefs` says nothing
  about it.
  **Measured rather than argued, per §7 and OMNI §9.1.** A grid search over `|input| ≤ 1`, `gain ∈ [1, 5]`,
  `shape ∈ [0, 1]` (401 shape steps, 201 gain steps, 0.005 input steps) gives today's worst-case
  `|Process|` as **200.5**, at `gain = 5`; with the offset freed at `gain = 5` (400 offset steps) the
  worst case is **212.1** — **+6%, +0.5 dB.** The controlling quantity moved across its whole range and the
  measured maximum moved with it, so this is a live measurement, not a vacuous one. **Conclusion: Link stays
  OFF the ⚠ list, on the strength of that number rather than of the invalid proof** — a 6% enlargement of a
  worst case this stage already reaches today, into the same oversampler and downstream limiter that already
  handle 200x. T3.2 records the measurement as the reason, so a later session does not re-derive it from the
  bound.
- **Slot 11 — Fold (`Fold`), reuses-existing — session 3.** Confirmed by reading `FrogBlock::Process`
  (`app/dsp/Drive.hpp`): `const float sinIn = out / 4.0f;` immediately followed by `Sine01(sinIn) * (1.0f
  - fuzz) + fuzz * PadeSaturator::Saturate(out)`. The pre-fold divisor is a bare `4.0f` literal; Fold
  replaces it with a knob-mapped value. Cost: reuses-existing — one literal becomes one knob-fed divisor
  inside code that already runs every sample; `Sine01`/`PadeSaturator::Saturate` unchanged. Headroom:
  **provably free** — `Sine01(0 / anything) == sin(0) == 0`, so the folded branch stays `0` at silence
  regardless of the divisor, and `Sine01`'s output is bounded to `[-1, 1]` for any argument by definition,
  so no matter how small the divisor gets, this branch can never exceed the range it already occupies
  today; `PadeSaturator::Saturate` remains bounded by its own clamp on the other branch.
  **Session 6 audit — the literal is confirmed (`Drive.hpp:378`) and the amplitude argument holds
  (`Sine01(phase)` is `sin(2π · (phase - floor(phase)))`, `DspMath.hpp:28-33`, so it is genuinely bounded for
  any finite argument). "No matter how small the divisor gets" is the problem: the divisor must be floored
  away from zero, and the entry as written invites the mapping that isn't.** At divisor `0`, `out / 0` is
  `±inf` (or `NaN` at `out == 0`), and `Sine01`'s own `phase - std::floor(phase)` turns `inf` into `NaN`,
  which then propagates through the whole chain. This codebase has already been silenced once by exactly
  that path: `FroggersAppCore.hpp`'s own `scoopNotch` comment records a non-finite filter state reaching
  `SanitizeOutputSample`, which masked every later sample to `0.0f` — *"permanent silence with no
  recovery."* **Fold's knob mapping SHALL keep the divisor strictly positive** (an exponential map, the same
  shape every other range in this bank uses, does this by construction — `ExpMapCompute`'s `min` is never
  reached from below). T3.3 is updated.
- **Slot 12 — Tone (`Tone`), composes-existing — session 3.** A post-chain one-pole lowpass, appended
  after `FrogBlock`'s existing stages. Genuinely a new stage in the signal path (not an unlock of an
  existing hardcoded literal), but built entirely from `dsp::OnePoleLowPass` (`DspMath.hpp`), the same
  struct `Oversampler2x`'s own anti-alias filter already uses (§2 session-2 addition). Cost:
  composes-existing. Headroom: none — a lowpass only removes energy; `f(0) = 0` trivially (zero state,
  zero input, zero output).
- **Slot 13 — Bias (`Bias`), composes-existing, ⚠ HEADROOM FLAGGED — session 3.** A DC offset applied
  before `PolynomialDrive::Process` makes the drive stage map `0 -> nonzero`, which is **exactly the
  defect that caused the full-scale runaway fixed in the predecessor change**
  (`DigitalReorganizer::Process(0.0f)` was nonzero and the feedback loops amplified it, `app/dsp/
  Drive.hpp`'s own divergence-note comment). The fix generalizes that same predecessor construction —
  confirmed by reading `DigitalReorganizer::Process`, `return Mangle(input, flip, hashBits) - Mangle(0.0f,
  flip, hashBits);` (`app/dsp/Drive.hpp:336-338`) — to whatever nonlinearity Bias precedes here, i.e.
  `PolynomialDrive::Process(input + bias) - PolynomialDrive::Process(bias)`: apply the offset, waveshape,
  subtract the shaped offset alone. **This closes the acute `f(0) != 0` hazard exactly** — `Process(0 +
  bias) - Process(bias) == 0` at every Bias setting, silence stays silence — the same fix, applied to a
  different function than the one it was proven on. **What it does not close, and what stays flagged:**
  confirmed by reading `PolynomialDrive::Process` again, it is an unbounded 5th-order polynomial, not a
  bounded lookup like `Mangle` — the subtract-after construction cancels the *constant* term exactly but
  does not by itself bound *peak swing* identically across the whole Bias sweep. This softer, narrower
  residual must be MEASURED before shipping, per the standing rule (§7) — do not reach for the plain
  offset-before/subtract-after construction some research proposed independent of the
  `Mangle`-precedented one; the `PolynomialDrive`-generalized construction above is the one this document
  records as correct.

### 9.3 Delay bank — COMPLETE, all fourteen slots DECIDED

- **Slot 9 — Feedback Drive (`FbDr`), DECIDED IN.** The loop is `WriteSample(inSignal + fbk *
  PadeSaturator::Saturate(fbL), lineL)` (`app/dsp/Delay.hpp`, confirmed unchanged at §2's session-2
  addition). **A round-1 headroom flag on this candidate is WITHDRAWN as wrong**, not merely softened — and
  session 6 promoted the reasoning below into §7a, because two sibling parameters share this exact construct:
  the same file's own comment states *"Saturate clamps to ±1 unconditionally, so this line can never
  write more than `|inSignal| + fbk` regardless of how many round trips have already run — a per-sample
  bound."* Pre-gain into that saturator cannot raise this bound; `Saturate`'s output is clamped before the
  addition, not after. **The stage that actually burned this codebase historically was the REVERB tank,
  which had no in-loop saturator at the time** — a structurally different situation from Delay's loop,
  which has always had one. No headroom re-derivation is needed for this slot.
- **Slot 10 — Feedback Tone (`FbTn`), composes-existing — session 3.** A one-pole lowpass damping the
  feedback tap ahead of `WriteSample`, alongside the existing `PadeSaturator::Saturate` call, using
  `dsp::OnePoleLowPass` (`DspMath.hpp`). Genuinely a new stage in the loop, not a literal unlock. Cost:
  composes-existing. Headroom: **none** — a lowpass placed inside a feedback loop only removes
  recirculating energy, which improves the loop's own stability margin rather than eroding it, and cannot
  raise `Saturate`'s already-clamped per-sample bound.
- **Slot 11 — Mod Rate (`MdRt`), Tier 1, §2 session-2 addition.** `lfoInc = 2.0f * 3.14159265f * 0.25f /
  sampleRate` (`app/dsp/Delay.hpp`) — a fixed 0.25 Hz driving `modSeconds = std::sin(lfoPhase) * p.dmod *
  baseSeconds * 0.08f`. Real, unreachable DSP.
- **Slot 12 — Width Balance (`WBal`), reuses-existing — session 3.** Confirmed by reading `StereoDelay::
  Process` (`app/dsp/Delay.hpp`): the Width knob (`p.dwid`) feeds **two** different stereo mechanisms at
  two different hardcoded weights — `const float widthSpread = p.dwid * baseSeconds * 0.35f;` (an L/R
  time-offset spread) and `const float cross = p.dwid * 0.5f;` (the cross-feed blend `fbL = dL * (1.0f -
  cross) + dR * cross`, same for `fbR`). One knob, two roles, a fixed `0.35 : 0.5` ratio between them —
  Width Balance splits that ratio into its own knob, still driven by the existing `p.dwid` value. Cost:
  reuses-existing — both literals become knob-fed weights inside code that already runs every sample; no
  new struct, no new buffers. Headroom: none — `fbL`/`fbR` stay a convex combination whenever the balance
  stays in `[0, 1]`, bounded by `max(|dL|, |dR|)` regardless of how the balance knob splits the ratio, and
  `widthSpread` only offsets a `ReadAt` tap position, never adds gain — an interpolated read of two
  buffer samples is bounded by those samples at any offset. Precedent: Bitwig's stereo Delay ships `Width`
  and `Cross Feedback` as two independent controls rather than one fixed-ratio knob.
  **Session 6 audit — both literals confirmed (`Delay.hpp:316,330`); two bounds this entry states in prose
  but never made binding, now in the spec.** (a) The convexity of `fbL`/`fbR` is conditional — this entry's
  own words, *"whenever the balance stays in `[0, 1]`"* — and today it is guaranteed by the code, not by a
  requirement: `cross = p.dwid * 0.5f` cannot exceed `0.5`. Replace that `0.5f` with a knob-derived weight and
  the guarantee has to come from the mapping instead, so the spec now requires it. (b) `widthSpread` adds to a
  read tap that is **already able to run past the buffer**: `timeR = max(0.001, baseSeconds + modSeconds +
  widthSpread)`, `baseSeconds` reaches `kMaxDelaySeconds = 2.0f`, `widthSpread` adds up to `0.7 s`, and
  `capacity` is `ceil(kMaxDelaySeconds * sampleRate)` — 2.0 s (`Delay.hpp:276,309,316`), after which `ReadAt`
  takes the position modulo capacity and reads the wrong repeat. **That wrap is pre-existing, not introduced
  here** (so it is not this change's bug to fix, per §7's scope), **but a Width Balance that can raise the
  `0.35` weight makes it reachable at more settings, which this change IS responsible for** — hence the
  second spec line: the spread must not lengthen a tap past capacity. Per OMNI §16.2 the pre-existing wrap is
  worth a scoped look while the file is open; T4.2a says so without widening this change to own it.
- **Slot 13 — Crush (`Crsh`), composes-existing — session 3.** Reuses `dsp::SampleRateReducer` and/or
  `dsp::DigitalReorganizer` (`app/dsp/Drive.hpp`) on the feedback tap's repeats, for lo-fi bitcrushed
  decay. **A round-1 caveat on this candidate is now MOOT, not merely softened:** round 1 flagged the risk
  of a hand-rolled bit-scramble reintroducing the `f(0) != 0` defect; confirmed by reading
  `DigitalReorganizer::Process` (§9.2's Bias entry, above), that fix already ships in the struct as it
  stands today (`Mangle(input, ...) - Mangle(0.0f, ...)`), so reusing it as-is for Crush is simply correct,
  not merely "safer if done carefully." Cost: composes-existing. Headroom: none — `SampleRateReducer`/
  `DigitalReorganizer` outputs never exceed their input's magnitude.

**Slot-ordering note, Drive and Delay (per the operator's own instruction, not presented as decided beyond
what was actually specified):** the operator gave explicit slot NUMBERS for each of these eight new
parameters (Drive 10-13, Delay 10/12/13) and this document records exactly those numbers. Which physical
row/column within the bank's existing row-major encoder grid (§2's confirmed `ix / kColumns`, `ix %
kColumns` mapping) each slot index lands on follows automatically from the slot number alone — nothing
further about internal ordering is decided or implied by this document beyond the numbers given.

### 9.4 Reverb bank — COMPLETE, all fourteen slots DECIDED

- **Slot 9 — Mod Rate (`MdRt`), Tier 1.** `kModLfoHz = 0.35f` (`app/dsp/Reverb.hpp`), fixed, feeding the
  tank's own LFO phase increment (`modLfoPhase = WrapPhase(modLfoPhase + kModLfoHz / sampleRate)`). Real,
  unreachable DSP.
- **Slot 10 — Tank Drive (`TkDv`), effectively Tier 1.** The predecessor change already installed the
  in-loop saturator: `const float aIn = preOut + fb * PadeSaturator::Saturate(aFb);` (same for `bIn`,
  `app/dsp/Reverb.hpp`) — only a pre-gain on that call's argument is missing. **Session 6: this is the third
  instance of §7a's construct, and this entry previously said nothing at all about its bound — silence that
  reads as "no concern" and is indistinguishable from an oversight.** It is bounded for the same reason the
  other two are: `Saturate` clamps to ±1 before the multiply by `fb`, so `|aIn| <= |preOut| + |fb|` at any
  drive setting. Cited, not re-derived.
- **Slot 11 — Grit (`Grit`), composes-existing.** Tank feedback routed through `dsp::DigitalReorganizer::
  Process` (`app/dsp/Drive.hpp`), inserted ahead of the existing `PadeSaturator::Saturate` call on
  `aFb`/`bFb`. **This candidate would have been unsafe before the predecessor change's fix made `f(0) =
  0` hold for `DigitalReorganizer` — it is safe now precisely because of that fix**, not despite the same
  risk still being present. Precedent: Qu-Bit Nautilus's per-line selectable feedback-path bitcrusher.
- **Slot 12 — Tilt (`Tilt`), composes-existing.** A bipolar post-tank tone shave: `dsp::OnePoleLowPass`
  instantiated twice (one direct lowpass tap, one complementary highpass via `input - lowpass(input)`),
  crossfaded around the knob's center, applied to `wet`/`mixedOut` before the existing `wetLimiter.
  Process()` call. Precedent: Make Noise Erbe-Verb's `Tilt`, Noise Engineering Desmodus Versio's `Tone`.
- **Slot 13 — Tuned (`Tund`), re-scoped per §3 ruling 5.** The tank's `dA`/`dB` driven directly by the
  parameter's own resolved value — no pitch tracker, per ruling 5's correction of the design doc's
  original (more expensive) framing. **Caution carried forward, not resolved by this session:**
  modulating Tuned at audio rate sweeps a delay length inside a feedback loop, which must be MEASURED,
  not assumed safe, even though the loop now has an in-loop saturator and precedent for bounded
  modulation of the same `dA`/`dB` pair via the existing Mod-depth mechanism.

### 9.5 Audio bank — COMPLETE, all fourteen slots DECIDED, session 5

- **Slots 9-11 — Ring Mod (`RM1`/`RM2`/`RM3`), one per VCO, DECIDED — corrected in full, session 5.**
  **This document previously recorded Ring Mod here as blocked on an open carrier choice (next-VCO-
  cyclically, the mix of the other two VCOs, or a dedicated carrier), a pre-gate/post-gate sub-question,
  and a claimed collision with `froggers-vco-topology`'s "No hardcoded cross-VCO coupling" requirement —
  all of that was WRONG and is corrected here, not merely softened (§4.2 records the correction in full).**
  Ring Mod is an ordinary parameter, structurally identical to every other bank parameter: **each VCO has
  its own ring modulator with an INTERNAL carrier**, generated inside that VCO's own ring-mod stage, never
  reading another VCO's signal. **The Ring Mod knob's range IS the carrier's frequency, mapped across
  audio rate** — the same exponential shape `Vco::PitchToPhaseIncrement` already uses for pitch:
  `ExpMapCompute(20.0f / sampleRate, 20000.0f / sampleRate, pitchKnob01)` = `min * (max/min)^value`
  (`app/dsp/DspMath.hpp:43-46`, `app/dsp/Vco.hpp:131-134`), equivalently `f = 20 * 1000^knob`. The
  resolved value is then modulatable from the matrix exactly like every other bank parameter, through
  `Parameter::GetRaw`'s `center + Σ(depth × source)` (§2; `External/Sheaf/projects/synth/src/
  ParameterModulation.cpp:1207-1216`). Operator, verbatim: *"why wouldn't ring mod for each vco be like
  any other parameter -- derive its value from the knob position, and that position attenuates any
  modulation it's getting"*; *"the signal is the value derived from knob position and/or modulation
  source"*; *"the ring mod range of the knob should be audio rate, then it gets modulation sources from
  levels 1-2-3 like any other parameter"*. **There is no carrier decision** — the carrier is internal and
  per-VCO — **and therefore no cross-VCO coupling, so no collision with `froggers-vco-topology`'s
  requirement** (§3 ruling 2; §4.2 states plainly the claimed collision was never real). Cost: genuinely
  new — a new internal-oscillator instance and a new multiply stage per VCO, not an unlock of an existing
  literal. Headroom: none — multiplying two signals each bounded to `[-1, 1]` (the VCO's own audio output,
  and the internal carrier's own `EvalWaveMorph`-bounded output, per the PM Rate entry's citation below)
  produces a result bounded to `[-1, 1]` by `|a*b| <= |a|*|b| <= 1`, unlike a sum, which can exceed either
  operand. **Session 6 audit: the headroom argument holds and is re-confirmed** (`EvalWaveMorph` is bounded
  to `[-1, 1]` for any phase, `Vco.hpp:68-84`), **but "one implementation detail remains open, not a
  blocker" was wrong on both counts — the detail as framed rested on a false premise, and what replaces it
  IS a blocker (§4.2, new gate T8.0).** A product has no unity position: at every carrier frequency,
  including sub-audio, the VCO is amplitude-modulated to zero twice per carrier cycle, so the design as
  specified had **no off position and no depth control**, and every existing patch would be ring-modulated
  the moment this shipped, with no default able to restore it.
  **RESOLVED — session 6 operator ruling 9 (§3a), and it costs no slots.** The bottom of the Ring Mod knob's
  own travel gates the ring-mod amount to exactly zero and ramps smoothly out of it; above that ramp the
  same knob sweeps the carrier frequency as already specified. This is `Vco::PmDepthScale`'s exact shape, and
  per the operator's instruction it ships as **one shared function used by both PM and Ring Mod**, not a
  second copy — mechanism, §8 enumeration, and §6 justification at §4.5; tasks at T8.0/T8.0a. **Default
  parity follows for free** (ruling 11): a default at or below the gate's floor means an existing patch loads
  with Ring Mod inert and sounds exactly as it does today. The knob's low frequency end (T8.4) is now a
  by-ear taste question with nothing riding on it, since "off" no longer depends on it.
- **Slot 12 — PM Rate (`PMrt`), composes-existing.** §0a's worked example, carried into a decision this
  session: `Vco::StepPmLfo`'s rate (`const float hz = ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, pmKnob01);`,
  confirmed live against `Vco.hpp`) and depth (`kPmLfoDepth * PmDepthScale(pmKnob01)`, `Vco.hpp:169`) are
  both driven by the same `pmKnob01` today; decoupling them into a separate rate knob is a structurally
  new degree of freedom, not something the modulation matrix can already reach by routing (§0a). **Honest
  compromise, recorded rather than glossed over:** only one new Audio slot is spent on this, and
  `Vco::Process` takes its PM knob per-VCO (three separate calls, one per VCO, each with its own
  `pmKnob01`) — a genuine per-VCO rate decouple would need three new slots, one per VCO, and only two
  remain across the whole bank (split here with VCO Balance below). PM Rate therefore ships as **one knob
  shared across all three VCOs' `StepPmLfo` calls**, not three independent per-VCO rate controls — the
  same "only one slot, not three" constraint VCO Balance below is explicit about. Cost: composes-existing
  — feeds a second knob into the same `ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, ...)` call already made
  once per VCO, no new struct or state. Headroom: none — phase-domain only, same proof as §3 ruling 8's
  PM Depth Max analysis: `EvalWaveMorph` always evaluates a wrapped phase to `[-1, 1]` regardless of what
  drives the phase offset.
  **Session 6 audit — ⚠ SPEC COLLISION, real this time, and it is this parameter rather than Ring Mod.**
  Sessions 3-5 checked Ring Mod against exactly one requirement of `openspec/specs/froggers-vco-topology/
  spec.md` and never read the rest of the file. Read in full, that spec collides with PM Rate twice:
  - Its requirement **"Froggers oscillator topology is preserved"** specifies *"per-VCO phase modulation
    driven by that VCO's **own** dedicated sine LFO whose frequency is an exponential function of the PM
    knob."* PM Rate's entire purpose is to make that frequency a function of a different knob. As written,
    the requirement forbids the parameter — this is not a wording quibble, it is the requirement's operative
    clause.
  - Its scenario **"Phase modulation is self-contained"** asserts that raising *"any VCO's phase-modulation
    control"* changes no other VCO's output. PM Rate ships as ONE knob across all three VCOs' `StepPmLfo`
    calls (this entry's own recorded compromise), so moving it moves all three.
  **A new spec delta is added for this change: `specs/froggers-vco-topology/spec.md`** — the topology
  requirement re-worded so the LFO rate comes from a rate control (each VCO keeping its own LFO instance and
  its own depth), the self-contained scenario re-scoped to depth, and a new scenario stating plainly that the
  shared rate control is shared by design. **This delta RELAXES an in-force requirement, so it needs the
  operator's confirmation before an implementer acts on it — new gate T7.0.** The same delta also adds a
  scenario making Ring Mod's internal carrier a spec-level assertion rather than a proposal-level claim; that
  half strengthens the spec and needs no ruling.
- **Slot 13 — VCO Balance (`VBal`), composes-existing. NOT headroom-flagged — session 4 discharges the
  flag by construction.** Confirmed live by reading `dsp::MixOscVoices` (`app/dsp/VoiceEnvelope.hpp`):
  `return (v1 + v2 + v3) * (1.0f / 3.0f);` — a hardcoded, un-parameterized equal-thirds average; **there
  is no per-VCO level control anywhere in the signal path today.** Three independent per-VCO levels would
  need three slots, and only one remains (split here with PM Rate above) — so this ships as a single TILT
  sweeping emphasis across VCO1 → VCO2 → VCO3, not a true three-fader mixer. **Recording the compromise
  honestly, as instructed, rather than presenting it as the three-independent-level control the design
  doc's original framing implied.** Cost: composes-existing — replaces the fixed thirds with a normalized
  3-point crossfade driven by one knob.
  **Binding requirement, session 4 operator ruling, verbatim:** *"but VCO balance needs a hard clamp such
  that there is no maximum that is 100% only vco 1 or 2 or 3"* and *"in fact, the minimum balance of any
  vco in the mix in the range of vco balance should be 10% of total vco mix."* The crossfade weights
  `w1, w2, w3` SHALL sum to exactly 1 at every knob position (unchanged from the earlier framing) AND each
  SHALL stay in `[0.10, 0.80]` — no VCO may be reduced to 0% and none may reach 100%. With three VCOs, a
  10% floor on each arithmetically caps any single VCO at `1 - 0.10 - 0.10 = 0.80` (80%), stated here
  explicitly per the operator's own instruction. Deliberate parallel to `dsp::VcoAdsrState::
  kMinSustainLevel` (0.10, raised from 0.05 in the predecessor change, `VoiceEnvelope.hpp`) — same
  reasoning, same class of constraint: a knob must not be able to gate a signal path to zero. **This
  discharges the headroom question by construction, not by measurement**: any convex weight triple
  (non-negative, summing to 1) is bounded by `max(|v1|,|v2|,|v3|)` by the same arithmetic the plain
  equal-thirds average already satisfies (§4.4 derives this in full) — the predecessor change's own §K
  "No limiter needed for Audio" verdict survives untouched across the whole floored range. **This is no
  longer on the headroom-flagged list (§9.6)** — it moves from "must be measured" to "an invariant the
  implementation asserts": `sum(w) == 1` with each `w` in `[0.10, 0.80]` across the whole knob sweep,
  checkable by inspecting the mapping function directly. A positive-control measurement task is still
  required (OMNI §9.1) — not to discover an unknown bound, but to prove the test rig varies the knob
  across its full range and would actually observe a violation if the invariant were ever broken; an
  invariant-only test that never exercises the knob's extremes is worthless as a regression guard.

### 9.6 Totals — sessions 1-5 combined, the thirty-parameter target slate, now COMPLETE

**Every bank now has a target of fourteen parameters (30 new parameters total across the six banks: `(14 -
9) * 6 = 30`, since every bank started at nine). This session's own count, verified against §9.1-9.5 and
`proposal.md` §3's Envelope and Ring Mod rulings, not asserted from the "14 * 6" arithmetic alone:**

- **30 of the 30 are DECIDED and specified by this change as of session 5:** Envelope (5: Decay×VCO1-3,
  Curve, Grace — §3 rulings 3-4), Filter (5: Topology, Scoop Freq, Scoop Width, Comb Drive, Scoop Depth —
  §9.1, COMPLETE), Drive (5: Anti-Alias Brightness, Link, Fold, Tone, Bias — §9.2, COMPLETE), Delay (5:
  Feedback Drive, Feedback Tone, Mod Rate, Width Balance, Crush — §9.3, COMPLETE), Reverb (5: Mod Rate,
  Tank Drive, Grit, Tilt, Tuned — §9.4, COMPLETE since session 2), Audio (5: Ring Mod×3, PM Rate, VCO
  Balance — §9.5, COMPLETE since session 5).
- **Zero of the 30 remain undecided.** Ring Mod (Audio slots 9-11) was the sole remaining item through
  session 4; its "blocked" framing was itself wrong (§4.2) and is corrected, not resolved by an operator
  answer to the invented carrier question — the design was always an ordinary parameter (§3 ruling 1).

**Tier-1-style unlocks of already-written DSP (a hardcoded literal exposed directly, or an existing
function/struct fed a new value at its existing call site, with no new signal-path assembly) vs.
genuinely new code (a new stage, new state, or new math assembled from existing primitives), tallied
against each item's own recorded Cost tag above:**

- **Tier-1-style unlocks — 12 of 30:** Filter Topology, Scoop Freq, Scoop Width, Scoop Depth (4); Drive
  Anti-Alias Brightness, Link, Fold (3); Delay Feedback Drive, Mod Rate, Width Balance (3); Reverb Mod
  Rate, Tank Drive (2). Zero of Audio's 5 decided items qualify — Ring Mod×3, PM Rate, and VCO Balance
  each need a new call-site assembly (a per-VCO carrier instance and multiply stage; a second
  `ExpMapCompute` call; a new crossfade), not merely a literal-to-knob swap.
- **Genuinely new code — 18 of 30:** Envelope Decay×3, Curve, Grace (5, all new state or new ramp-shape
  math — §4.3, T1.4); Filter Comb Drive (1); Drive Tone, Bias (2); Delay Feedback Tone, Crush (2); Reverb
  Grit, Tilt, Tuned (3); Audio Ring Mod×3, PM Rate, VCO Balance (5, session 5 adds Ring Mod's three
  per-VCO carrier-and-multiply stages).

**The complete set of headroom-flagged items (⚠, this document's own reserved tag for a parameter whose
own decision text requires a re-derived or newly-measured bound before shipping, not merely the general
§7 standing rule every new audio-affecting parameter is already subject to) — verified against every
artifact above, **exactly TWO as of session 6: Bias and Filter Topology. Comb Drive came off (flag wrong,
§7a), VCO Balance came off in session 4 (discharged by construction), and Topology came on in session 6:****

1. **Bias** (Drive slot 13, §9.2) — new since session 3, unbounded-polynomial peak-swing residual survives
   the `f(0) = 0` fix.
2. **Filter Topology** (Filter slot 9, §9.1) — **new and now settled as a flag, session 6, operator ruling
   12.** Not for the reason the audit first raised: the dangerous series branch is deleted, so nothing
   bypasses `combTrim`/`peakTrim`/`peakLimiter` any more. The residual is narrower and real — at high
   Topology the peak biquad reads the comb's output instead of the raw input, a new operating point for the
   stage whose ceiling this codebase has already had to lower twice (T2.1a).

**Comb Drive came OFF this list in session 6 — the third time this list has changed, and the only time it
changed because a flag was WRONG rather than because a decision discharged it.** Sessions 2-3 flagged it on
the belief that a pre-gain raises `rawCombTrim`'s worst case; `Saturate`'s unconditional ±1 clamp means it
does not, which is the same argument this document already used to withdraw the flag on Delay Feedback
Drive over the identical construct. §7a now states that argument once for all three saturator pre-gains
(Comb Drive, Feedback Drive, Tank Drive) instead of relitigating it per site — the drift between two
restatements of one concept is what produced the contradiction. **The list is two items.**

**The other two are re-fittable, not open-ended, and what each re-fit IS is recorded so the flag reads as
scheduled work rather than unbounded risk:** Topology re-sweeps `peakLimiter`'s threshold and attack/release
against comb-output input (the `1/height` trim is algebraic and does not move), T2.1a; Bias either takes a
measured output trim across its sweep, the way the comb branch already has one, or a bounded knob range that
keeps peak swing inside today's maximum, T3.5.

**Session 6 audit — the count stays at two, but one of the twenty-eight unflagged items was cleared for the
wrong reason and one clearance was conditional on a bound nobody had written down.** Neither becomes a ⚠
item; recorded here so the list's own claim ("verified against every artifact above") stays true:
- **Link** (Drive slot 10) was cleared on "bounded coefficients," an inference this very list's Bias entry
  rejects for the same function. Re-cleared on a measurement instead: today's worst-case `|Process|` 200.5
  vs 212.1 with the gain/phase pairing unlocked, +0.5 dB (§9.2, T3.2). Off the list on evidence, not on the
  argument that was there.
- **Width Balance** (Delay slot 12) was cleared on a convexity that holds *"whenever the balance stays in
  `[0, 1]`"* — true, and enforced today only by the `0.5f` literal the parameter replaces. The bound is now
  a spec requirement rather than a parenthetical (§9.3).
- **Fold** (Drive slot 11) keeps its clearance for amplitude, which is sound, and gains a
  strictly-positive-divisor requirement to keep it out of the `NaN` path (§9.2, T3.3).
- **Ring Mod** (Audio 9-11) is still unconditionally bounded — that check was right. What it lacks is an off
  position, which is a design gate (T8.0), not a headroom item (§4.2).

**VCO Balance (Audio slot 13, §9.5) came OFF this list in session 4 — recorded here so a later session
does not wonder why the count dropped from three to two.** It carried the flag in session 3 because
whether its crossfade preserved §K's "no limiter needed for Audio" verdict depended on which shape an
implementer chose at build time — a naive shape could have raised `chainIn` above the equal-thirds
bound, and only measurement could have ruled that out. The operator's session-4 ruling — a mandatory
a mandatory floor of 10% and cap of 80% on every VCO's own weight (§4.4, §9.5) — makes the
crossfade a convex combination by definition, which is bounded by `max(|v1|,|v2|,|v3|)` regardless of
implementation shape. The risk that justified the flag is now closed by the parameter's own specification,
not by a future measurement — so it is an asserted invariant (`sum(w) == 1`, each `w` in `[0.10, 0.80]`),
not a headroom flag. A positive-control measurement task remains (T7.2) to prove the test rig can actually
observe a violation, per OMNI §9.1 — that is not the same as the flag itself remaining open.

**Ring Mod (Audio slots 9-11, §9.5) is not on this list either — checked and found unconditionally
bounded, not merely unflagged by omission, session 5.** Multiplying two signals each bounded to `[-1, 1]`
(the VCO's own output, and the internal carrier's own `EvalWaveMorph`-bounded output) produces a result
bounded to `[-1, 1]` by `|a*b| <= |a|*|b| <= 1`, for any carrier frequency — unlike a sum, a product of two
bounded signals cannot exceed either operand's own bound, so no re-derivation is needed the way Comb
Drive's and Bias's own headroom arguments require.

**Not on this list, but carrying their own separate pre-ship measurement requirements recorded elsewhere
(a real distinction, not an oversight):** Reverb Tilt (T5.5 re-sweeps `wetLimiter`'s margin against Tilt
at its brightest) and Reverb Tuned (T5.7 measures audio-rate modulation of a delay length inside a
feedback loop) each need verification before shipping, per §9.4's own carried-forward cautions, but
neither is tagged with this document's ⚠ HEADROOM FLAGGED marker — that tag is reserved for items whose
own headroom argument is genuinely unresolved pending measurement, distinct from items whose bound is
already understood and only needs confirming against a new setting.

## 10. Over-length short-label rework — new scope, added to this change per the operator

**The operator explicitly added this to this change rather than splitting it out:** *"i want it to be
part of this change, user testing is user testing."*

**The problem, confirmed by reading `External/Sheaf/projects/synth/include/synth/EncoderDraw.hpp`:** the
hardware-style 14-segment display truncates short labels at 4 characters. `FourteenSegment`'s text helper
defaults to `numChars = 4`, and `UpperShortLabel`'s own default is `maxChars = 4`. Any `shortName` longer
than 4 characters is silently truncated on that rendering path today.

**Verified by reading `app/FroggersParameters.hpp`'s current `FroggersBankLayouts()` directly (not
carried over from the task brief without a fresh check): of the 54 existing page parameters (six banks x
nine, the pre-this-change layout), 23 have a `shortName` longer than 4 characters — not the 22-of-53 the
task brief stated.** The brief's Filter (8 of 9), Delay (6 of 9), and Reverb (4 of 9) counts are each
independently confirmed exact by this re-count. **The correction is in Drive: this session counts 5 of 9
over-length, not 4** — `Drive`, `Shape`, `BitDp`, `Blend`, and `Phase` are all 5 characters; the brief's
"Drive 4 of 9" undercounts by one. (`SRR1`, `SRR2`, `XOR`, `Fuzz` are the Drive-bank labels that are
already compliant.) Audio and Envelope remain confirmed clean (0 of 9 each), matching the brief. Full
per-bank enumeration is in `tasks.md`.

**Session 6 audit — the count is exact (all 23 re-counted independently from `FroggersBankLayouts()`,
including the Drive correction), but its scope silently excludes two more violations on the same rendering
path.** `Crispy` and `Crnchy` are six characters each (`FroggersParameters.hpp`, registered at slots 14 and
15 — one Crispy per bank plus the shared Crunchy), and they render through the same `UpperShortLabel`
`maxChars = 4` path every counted label does, so they are truncated today too. They are not among the "54
page parameters" this count is scoped to, which is why they were not missed by an arithmetic error — but "23
over-length labels" reads as the total, and the total is 25 label sites (23 page + Crispy + Crunchy). T6 is
updated to say which of the two it fixes and which it deliberately leaves.

**Acceptance criterion, recorded here because a prior UI change in this project took four attempts by
asserting a weaker property than "the operator can see it":** this is a VISUAL acceptance criterion, not
a code-shape one. The operator's own direction: *"they can just be slightly larger colored boxes with
smaller text."* The affected cell also renders the modulation badge chips (per-source depth
indicators) — any resize or font change must be checked against that chip rendering too, not just against
the label text in isolation, because the two share the same cell. See `tasks.md` for the enumerated task
and its acceptance criterion.

## 11. OMNI §14 PREFLIGHT — RUN, session 6: one question answered, two still out

**Correction, session 6, recorded before the section it corrects: this section briefly said all three audit
questions were answered. Only one was.** The operator's *"i choose #1"* answered the Ring Mod question
(ruling 9); the PM Rate and default-parity questions were not seen when they were asked. Reading one answer
as three is the lead inventing operator intent — the same error §0a's session-5 addendum already names twice
in this change. Items 10 and 11 in §3a are open questions with an attached recommendation, not rulings, and
nothing in `tasks.md` may close them by choosing. **They are questions, not standing gates** — the operator
was explicit about that distinction — so the rest of the change proceeds; these two decide the shape of PM
Rate's slot and of every unlocked literal's default, not whether the other twenty-eight parameters can be
built.

### 11a. What the audit found and what the one answer changed

**Session 6 supersedes this section's session-5 verdict: the preflight audit ran (§1a), found five things
that did not hold, and put the three needing the operator's judgement to the operator. One came back
answered (ruling 9); two are still out (§3a items 10-11). None is a standing blocker; each was a question
this document should have asked instead of declaring itself ready.** The audit did not disturb the change's
decisions — every hardcoded-literal claim held, Ring Mod's non-collision with `froggers-vco-topology`'s
coupling requirement held, and VCO Balance's convexity held.

What the one answer changed, and what the two open questions still decide:

1. **OPEN — PM Rate's collision with `froggers-vco-topology`** (§3a item 10, §9.5, T7.0). The delta relaxing
   the two clauses is written to the audit's recommendation and awaits the operator.
2. **Ring Mod gains a true zero position** at the bottom of its own knob, implemented as ONE function shared
   with PM per the operator's own instruction — the change's only new named concept, justified against §6's
   2-of-4 and enumerated against §8 before writing (ruling 9, §4.5, T8.0/T8.0a).
3. **OPEN — default parity for the twelve unlocked literals** (§3a item 11, §4.1, T9). The spec requirement
   and T9 are written to the audit's recommendation; if the operator picks the reset instead, both come out.

**One item the audit raised and then withdrew, recorded rather than quietly dropped:** the app open-codes six
knob-fed `ExpMapCompute` calls in `RouteAudioSample` while `ParameterManager` ships a mapping family it never
calls (§4.5). Written up as an §8 duplication finding (task block T10), then **checked against §8's own test
and rejected**: the formula is written ONCE in this app and called six times, which is what §8 asks for; the
second copy lives in a pinned dependency; and adopting it would put a throwing, out-of-line, id-looking-up
call into a per-sample audio path to delete a four-line helper. T10 is deleted and the rejection is recorded
in `tasks.md`'s "Recorded, not scheduled" so it is not re-proposed.

Everything else stands as written and re-verified: every DECIDED slot (§9.1-9.5) carries a symbol-cited trace
of the code it unlocks or composes, re-read against the tree in session 6 and found accurate; the
headroom-flagged list is still exactly two items (Comb Drive, Bias), with Link's clearance now resting on a
measurement instead of an invalid proof and three other clearances tightened (§9.6); and this change remains
markdown-only end to end (§6), with nothing built or executed. §VERIFY at the end of `tasks.md` records
session 6's own `openspec validate --all --strict` result.

### 11a. Superseded — session 5's readiness claim, kept for the record

This change is markdown-only end to end (§6); nothing under it has been built or executed. Per the OMNI
workflow pipeline (`omni-rule.md` §13: structural intent check → OpenSpec → **proposal** → **preflight
audit** → execution → postflight audit), this proposal and its spec delta are now the artifact a §14
PREFLIGHT audit reviews before any execution session picks up `tasks.md`'s T1-T8 blocks. As of session 5:
every DECIDED slot (§9.1-9.5), including Ring Mod (§9.5, corrected in full this session), carries a
symbol-cited trace of the existing code it unlocks or composes (§14's own requirement that a proposal's
claims be cited to file:line/symbol, not asserted); **this change now records zero remaining bank-slot
blockers** — Ring Mod's prior "blocked" framing was itself wrong and is removed, not resolved by an
operator decision (§4.2) — so every DECIDED slot has a corresponding task block (T1-T8) in `tasks.md`; and
the two remaining headroom-flagged items (§9.6 — VCO Balance came off in session 4, discharged by the
operator's own 10%/80% floor-and-cap ruling, §4.4; Ring Mod checked and found unconditionally bounded by
construction, session 5, §9.6) each carry an explicit re-derivation or measurement task in `tasks.md`
where still needed, rather than an assumed-safe verdict. §VERIFY (below `tasks.md`'s own end) records this
session's `openspec validate --all --strict` result as the structural-consistency half of what a preflight pass
would check; the trace-completeness half is this document itself.
