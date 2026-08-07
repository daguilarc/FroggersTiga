# Proposal — `frogg3rs-blowout-and-drilldown-repair`

**Created 2026-08-06. Supersedes `frogg3rs-modulation-truth-and-voicing`**, archived
*superseded, FAILED* at `../archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/`.
See `SUPERSESSION-RECORD.md` for what carried and why.

**This document is self-contained.** Everything binding from the predecessor's failure report has
been carried here. You do not need to read the archive to execute this change; read it only if you
want a derivation.

## Why this change exists

The predecessor did not run out of time. It shipped, claimed success, and did not work. Four
operator-reported failures survived a session in which every fix was measured green:

1. Randomize All produces far more modulation badges per parameter than intended.
2. Filter Crispy at max still blows out.
3. **Stop does not stop** — audio continues over a minute after the transport is stopped.
4. Randomize All inside a level-1 drilldown ejects the operator to the main page.

Plus one new operator ask (5): raise the drill-in maximum to level 3, keeping the base-3 theme.

**176 tests pass. Four reported symptoms remain.**

---

## Method constraints (BINDING — read before anything else)

These are not style guidance. Each one is a specific way the predecessor failed, and an agent that
repeats them will reproduce the failure regardless of how good its DSP reasoning is.

### M1 — Never assert what a direct look would settle

Every wrong call in the predecessor had a twenty-line read that would have settled it. It claimed
"Drive is a bounded waveshaper, no cap needed" and talked the operator out of their own cap
suggestion — without ever opening the authored `DriveBlendPhase` stage, which measured **50-61×**.
It claimed "Stop is already fixed, verified by trace" — it is not fixed. It blamed F1 on per-pole
source draws; reading the loop disproved it in one look. It blamed F1 next on subtree state; the
operator disproved it by pointing out a single fresh press cannot create level-2 state.

**If a claim can be settled by reading code or printing a number, do that before writing the claim
down. "Traced" and "verified" mean the lead read it, not that a subagent said so.**

### M2 — A subagent report is a lead, not a fact

Several load-bearing claims entered the predecessor's plan because a subagent said them and the
lead never checked: the level-1 fan-out behaviour, the Drive bound, and the test count — miscounted
**four separate times**, caught each time only by counting `TEST_CASE` declarations directly.

### M3 — Measure the thing that is broken, not the thing that is easy to measure

Every measurement the predecessor took was on an **isolated stage, driven by synthetic adversarial
input, asserting that stage's own output bound.** Not one measured the real signal path end to end
with real VCOs, real modulation routing, real Crispy, real transport.

Five stages each pass their own bound test (comb 0.0, peak 0.990, delay 0.999999, reverb 1.000000,
`DriveBlendPhase` 0.90) while the operator still hears blowouts. **The composite was never
measured.**

**Directive: do not add another per-stage bound test until an end-to-end test exists that
reproduces the operator's actual repro and FAILS.** A green suite that does not move a reported
symptom is worse than a red one. This is task **B7.5**, and it comes first.

### M4 — Do not request a listening pass on a knowingly-incomplete build

Operator: *"why did you defer shit and still have me test it? all of these bugs are
interconnected."* One-variable-at-a-time is right only when the instrument is otherwise sound. It
is wrong when several defects stack into a blowout that masks every individual result.
**Nothing goes to the operator until the whole list lands.**

### M5 — When honouring an instruction requires adding a branch, suspect a literal reading

An instruction's rationale is part of the instruction. When the rationale dies, re-derive rather
than mechanically preserve. The extra branch is the tell.

**This recurs inside the code, not only in plans** — see F1.0, where an in-code comment forbids
exactly what the operator's current ruling requires.

### M6 — Quote the operator's words into any brief where a decision already exists

Told to "reconsider from scratch," a research agent built a reasoned case against adding Decay —
which the operator had explicitly approved — by framing it as the "naive" move. The lead relayed
the inversion without catching it. Check proposals against the operator's actual words before
relaying them.

### M7 — Correct-sounding prose is not correctness

The predecessor's plan is full of confident, well-cited paragraphs that were wrong. Citations prove
something was read, not that the conclusion follows. **Prefer a printed number over a paragraph.**

---

## Data flow (§1 trace)

The audio chain, single-sourced, as the code actually runs it
(`FroggersAppCore.hpp::RouteAudioSample`, one caller, per sample):

```
VCO1/2/3  ──► MixOscVoices (ASR gate, per voice)  ──►  FrogBlock (Drive)
   │                                                        │
   │                                                  DriveBlendPhase   ◄── allpass, recursive
   ▼                                                        │
scope tap (post-gate)                              FilterFxChain
                                                   ├─ comb  ◄── recirculating delay line, fb ±0.95
                                                   ├─ peak  ◄── stateful biquad
                                                   └─ scoop (convex dip)
                                                            │
                                                     StereoDelay  ◄── fb ≤ 0.98, in-loop saturator
                                                            │
                                                        Reverb    ◄── Hold fb ≈ 0.99998
                                                            │
                                                    outputLimiter_ (master, threshold 0.9)
                                                            │
                                                      trailing clamp
```

**Every internal mix is convex** (comb/peak blend, scoop return, delay dry/wet, reverb dry/wet), so
no summation can exceed the max of its inputs. **The only unbounded gain-bearing operations in the
entire chain** are `DriveBlendPhase` when its coefficient is unsmoothed, and the reverb's
`aIn/bIn = preOut + aFb*fb` sums. Verified by exhaustive sweep; nothing else was found.

### Stateful units — the authoritative enumeration

**CORRECTED 2026-08-07 (count) and SUPERSEDED BY `1c37657` (structure).** This paragraph
previously said *"`FroggersAppCore.hpp:1368` `RecoverPoisonedUnitState` lists all thirteen"* and
then enumerated **fourteen** names. Fourteen is right; "thirteen" was a miscount that also
propagated into `SUPERSESSION-RECORD.md`. The units are `audioVco1_`, `audioVco2_`, `audioVco3_`,
`driveBlendPhase_`, `drive_.oversampler`, `drive_.sampleRateReducer1`,
`drive_.sampleRateReducer2`, `filterChain_.peak`, `filterChain_.scoopNotch`, `filterChain_.comb`,
`filterChain_.peakLimiter`, `delay_`, `reverb_`, `outputLimiter_` — **10 Tier-2 (`Magnitude`) +
4 Tier-1 (`FiniteOnly`) = 14**, matching F3.1's measured count exactly.

**The single definition site is now `FroggersAppCore::ForEachStatefulUnit`**
(`grep -n "void ForEachStatefulUnit" app/FroggersAppCore.hpp` → `:1480` as of `4cde39c`), which
composes `dsp::Drive`'s and `dsp::FilterFxChain`'s own enumerations rather than re-listing their
members. `RecoverPoisonedUnitState` still exists (`:1425`) but is now a **consumer** of that
enumeration, not the definition of it.

**F3.3 has closed the duplication this paragraph described.** The Stop flush no longer carries a
truncated second copy: it calls the same enumeration and resets all 14. F3 remains open, but **not
for this reason** — F3.1 refuted the enumeration as its cause.

### Modulation write path

Randomize All/Page cross threads correctly by construction: the UI thread sets a pending atomic,
`FroggersAppCore::ProcessFrame` (audio thread) drains it, and `ComputeAllParameters()` is called
**once**, at the end of the drain, guarded by `randomizeRan`
(`FroggersAppCore.hpp:474-509`). Depth writes go through
`detail::RandomizeParameterModulationDepths` (`FroggersModulation.hpp:889-971`), which zeroes the
parameter's existing depths at both scene poles, draws a count, then draws distinct sources by
partial Fisher-Yates.

**Verified 2026-08-06, and it is what makes F4/F5 cheap:**
`RandomizeParameterModulationDepths` **does not need the modulation view open.** It takes
`Parameter&` directly, reads eligibility from `group.GetModulators().Metadata()`, and calls
`parameter.EnsureModulationDepth(eligible[i])` itself — materializing on demand, exactly the
sources it selects. Nothing in it reads view state.
**Line cites corrected 2026-08-07:** this paragraph said `:910` and `:952`; the true lines are
**`:914`** and **`:961`** (`grep -n "GetModulators().Metadata()\|EnsureModulationDepth(eligible"
app/FroggersModulation.hpp`, as of `4cde39c`). `tasks.md`'s F4 block already carried the correct
pair — the two documents disagreed and this one was wrong. Locate by symbol, never by these
numbers.

### What is traced, and what is NOT

| Item | Trace status |
|---|---|
| F1 distribution | **TRACED.** `FroggersModulation.hpp:926-942`. 10/30/30/20 then geometric r=0.7. P(count ≥ 4) = **30%** — the operator is seeing what the spec produces. The tail is genuinely thin, so counts of 7 need a separate explanation |
| F3 Stop flush | **TRACED.** Flush at `FroggersAppCore.hpp:656,683` clears 2 of 13 units and is one-shot. Root-cause candidate is concrete; the confirming measurement is F3.1 |
| F4 ejection | **TRACED.** `FroggersModulation.hpp:1112` and `:1152` — bare `Back()` drops level 1 → 0 |
| F5 drill cap | **TRACED.** `FroggersModulation.hpp:676` (`level_ >= 2`), `:723` (`level_ == 2`), `:736` (singular `level1Encoder_`) |
| F2 blowout | **TRACED** — see the gain-staging table below. The master engages by construction; B7.1 is the fix and was never built |

### F2's mechanism, derived from the constants as they currently stand

`OutputLimiter::DesiredMagnitude` (`dsp/Limiter.hpp:143-149`) is
`threshold + headroom·(1 − exp(−(|x| − threshold)/headroom))`, whose asymptote is
`threshold + headroom = ceiling`. **A stage's threshold is where limiting begins; its CEILING is
what it can actually deliver.** Every per-stage limiter currently ships `ceiling = kSharedCeiling
= 1.0` (`dsp/Limiter.hpp:59`):

**Line cites re-verified 2026-08-07 against `4cde39c`** — the Drive and Filter rows had drifted;
the corrected numbers are below. Locate by symbol (`grep -n "kOutputLimiterThreshold\|kPeakLimiterThreshold\|kDelayWetLimiterThreshold\|kReverbWetLimiterThreshold" app/dsp/*.hpp`).

| Stage | threshold | ceiling | max deliverable |
|---|---|---|---|
| Drive output limiter (`dsp/Drive.hpp:449`, configured `:503`) | 0.7 | **1.0** | → 1.0 |
| Filter peak limiter (`dsp/FilterFx.hpp:188-189`) | 0.7 | **1.0** | → 1.0 |
| Filter comb branch — trim `1/(1+\|fb\|)`, saturator in loop | — | — | `(A+fb)/(1+fb)` = **1.0** at A = 1 |
| Delay wet limiter (`dsp/Delay.hpp:105-106`) | 0.9 | **1.0** | → 1.0 |
| Reverb wet limiter (`dsp/Reverb.hpp:115-116`) | 0.9 | **1.0** | → 1.0 |
| **Master** (`dsp/Limiter.hpp:88-89`) | **0.9** | 1.0 | — |

`filterOut ≤ max(combPath, peakPath)` because every internal mix is convex, so nothing downstream
reduces these. **Therefore every upstream stage is permitted to deliver more than the master's 0.9
threshold, and the master engages continuously on any patch that drives any stage past 0.9.** That
is not a per-parameter defect and no range trim addresses it — it is a headroom-budget defect, and
it is why five green per-stage bound tests moved nothing.

**B7.1 is the fix: retarget every per-stage `ceiling` to the measured `C = 0.80`, leaving the
master at 0.9 so it becomes a rarely-firing backstop.** `dsp/Limiter.hpp:66-70` records it as
outstanding. Measured cost on the default patch is **−0.115 dB** — effectively free — with make-up
gain `1/C` = 1.25× applied AFTER `outputLimiter_.Process()` and BEFORE the trailing clamp.

**Trap for the implementer — RESTATED 2026-08-07, the original statement was wrong.** This
paragraph previously claimed that `headroom == 0` makes `DesiredMagnitude` evaluate `0/0` → NaN.
**It does not.** `absX == threshold` is caught by the function's own early return, so with
`headroom == 0` the division always has a strictly positive numerator: `+x/0.0f → +inf`,
`exp(-inf) → 0`, and the term vanishes, returning exactly `threshold` — a silent brickwall, not a
NaN.

**The real hazard is the opposite sign: `headroom < 0`, i.e. a threshold left ABOVE the new
ceiling** — which is the default outcome for delay and reverb, whose thresholds are both `0.9`
against the proposed `C = 0.80`. Negative headroom flips the exponent and turns the limiter into
an **exponential amplifier** (|x| = 1.5 → 41.1, a 27× gain), staying finite — and therefore
invisible to `SawNaN` and `RequireFiniteStereo` — up to |x| ≈ 9.8. That is F2's own symptom,
reintroduced by F2's own fix, past every guard this change has built.

`C = 0.80` is the **ceiling**; each stage's threshold must stay strictly below it, and
`OutputLimiter::Configure` does not check that. **`tasks.md` F2.1a adds a compile-time
`static_assert` on every threshold/ceiling pair before F2.1b touches any constant.**

**Preflight ruling: all of F1–F5 are traced and may execute.** B7.5 is not the trace — it is the
falsification gate. It still comes first, because M3 requires a failing end-to-end test before any
fix is trusted, and because it is the only thing that can prove B7.1 actually closed F2.

---

## What this change delivers

- **B7.5 — the end-to-end acceptance test, first.** The master limiter's `envelope` stays at unity
  across a hostile patch: all maxima, modulation live, transport running, the operator's real
  Crispy repro. **It must FAIL when written.** It is the only end-to-end proof that the per-stage
  headroom architecture works in the binary rather than on paper.
- **F0 — preflight remediation** of five defects the 2026-08-06 audit found in the predecessor's
  shipped work. Cheap, mechanical, and two of them are prerequisites for F1 and B7.5.
- **F1 — randomize count distribution: mode 2**, 4+ genuinely rare, 7 essentially never, never
  zero, distinct sources, **and the same distribution at every level.**
- **F2 — Crispy at max stops blowing out.** B7.1's `C = 0.80` ceiling retarget plus make-up gain,
  gated on B7.5.
- **F3 — Stop stops.** Highest severity.
- **F4 + F5 — one shared edit.** Deleting the level-1 round trips fixes the ejection, cuts level-2
  materialization from 240 to ~45, and makes level 3 feasible (3615 → ~105).
- **F6 — operator verification.** Nothing visual or audible closes at an implementer.

## Out of scope

- `External/Sheaf` modifications — pinned at `77a3019e`; needs go to `/UPSTREAM-SHEAF-ASK.md`.
- Frozen trees stay byte-identical: `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- §H mobile-web layer, §I VST layer, §J bank parameter expansion, D.4 publish pipeline — all
  deferred, all readable in the archive.
- W4's second Sheaf pin bump (`508d9d68`) and `kExternalAudioOptedIn` removal — carried open,
  deliberately after this change so bump breakage stays attributable.
- Parameter-VALUE randomization (no coin flip, every knob every time) — operator-confirmed correct.
- **Randomize All's level-1 SCOPE.** Operator: *"this is desired functionality."* Level-1
  Randomize All affects only the drilled parameter. Do not change it.
- **The master limiter.** Threshold, attack, release and position stay as they are; it is the only
  thing between reverb Hold and the output. F2 changes what feeds it, never it.

## Durable rulings carried forward (do not re-litigate)

- **Every parameter must be continuous.** No switch-type parameters even though Sheaf supports
  them — a discrete parameter cannot be meaningfully modulated by the 15-source slate.
- **Bank slots vs the modulation layer.** The modulation layer expresses "this parameter's value
  varies over time"; anything reducible to that is redundant as a dedicated parameter. Bank slots
  are for signal-path operations — multiplying, summing, phase resetting, bit-combining.
- **Any new bank parameter that can raise a stage's output level ships with its trim/limiter budget
  re-derived.**
- **Crispy exposes reachable extremes; it cannot create out-of-range values.** It is a bit-scramble
  of the normalized value, output always in [0,1]. The treatment target is the maxima and stage
  headroom, never Crispy itself.

## Success criteria (falsifiable, with who checks)

1. **B7.5 is green**, having first been red for the right reason. Implementer, with the red run
   recorded.
2. Stop silences the instrument within the ~50 ms stop fade, from any patch, including reverb Hold
   at max and comb feedback at max. Test-pinned per stage; **operator confirms by ear.**
3. Randomize All inside a level-1 drilldown leaves the operator in the same level-1 view of the
   same parameter, with the results visible on that page. **Operator confirms visually.**
4. After Randomize All, the per-parameter badge count has mode 2, with 4+ rare and 7 essentially
   never, at every drill level. Test-pinned on a histogram; **operator confirms visually.**
5. Filter Crispy at max no longer blows out. **Operator's ears decide**, and B7.5 must agree.
6. The drill-in maximum is 3, changeable by one constant, with no `wasLevelTwo`-style special case
   anywhere.
7. Suite green across all ten binaries at every landing; `External/Sheaf` clean at its pin; frozen
   trees byte-identical.
8. **No task whose spec requires operator eyes or ears is closed by an implementer.**
