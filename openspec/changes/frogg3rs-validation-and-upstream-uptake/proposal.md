# Proposal — `frogg3rs-validation-and-upstream-uptake`

**Created 2026-08-12.** Supersedes the outstanding work of `frogg3rs-bank-expansion`, which is built and
archived. That change grew every bank from nine parameters to fourteen — thirty new parameters — and left
exactly two things no implementer could close, plus a standing dependency on upstream Sheaf. This change
carries both, and nothing else.

**This change is scoped to what a machine cannot finish.** Everything in `frogg3rs-bank-expansion` that
could be verified by a test IS verified by a test: 211 tests, 0 failures, 0 warnings, with a positive
control run against every measurement. What remains needs either a human's ears and eyes, or a dependency
this project does not control.

---

## Why

`frogg3rs-bank-expansion` is built, green and archived — but "green" covers only what a test can see. Two of
its tasks were written from the start as un-closable by an implementer because they need a human's eyes and
ears, four shipped ranges were chosen by an implementer against no specified value, two of its measurements
were reported but never pinned as regression tests, and six upstream Sheaf gaps remain outstanding against a
pinned dependency this project deliberately never forks. Leaving those inside an archived change would
record them as done. They are not.

## What Changes

- Adds a requirement that a bank parameter be **audibly effective** across its range and in the direction its
  name implies — a property registration, bounding and default-parity tests do not reach.
- Adds a requirement that a **measured bound be pinned by a regression test**, not left as prose.
- Adds a new capability, `froggers-upstream-uptake`, requiring that an upstream gap be **proven
  app-unreachable before it is treated as blocking**, and that the pinned dependency is never forked or
  locally patched.
- Schedules the by-ear and visual validation the predecessor could not close, and parks the six upstream
  items behind an explicit re-check gate.

## 1. Objective

Two capabilities, deliberately kept separate because one is reachable now and the other is not:

1. **Hands-on validation of the thirty new parameters.** Automated tests prove each knob is wired, bounded
   and default-neutral. They do NOT prove any of it sounds like the thing its name promises, and two tasks
   were explicitly written as un-closable by an implementer for that reason.
2. **Uptake of upstream Sheaf fixes**, when and if they land. `External/Sheaf` is pinned at `77a3019e` and
   is deliberately not forked — a fork was tried on 2026-07-27 and reverted the same day because the
   gitlink was unresolvable from any other checkout (`UPSTREAM-SHEAF-ASK.md`). So every upstream gap is an
   ask, and this change is where the app-side uptake work lives once an ask is answered.

## 2. What `frogg3rs-bank-expansion` actually left open

Verified against that change's own `§EXECUTION` record, not assumed:

- **T6.2 — the over-length label rework's acceptance criterion is VISUAL.** The rendering is verified
  programmatically (six character slots emitted for `CmbOff` where four were before; badge-chip clearance
  checked algebraically against `AppendBadge`'s own formula), but **nobody has looked at it.** The
  predecessor change records a prior UI change in this project taking four attempts precisely by asserting
  a weaker property than "the operator can see it."
- **T8.4 — Ring Mod's low-frequency end is a by-ear taste call.** It gates nothing: "off" is the shared
  zero taper at the bottom of the knob, not a frequency. The carrier range currently ships at 20 Hz - 5 kHz,
  an implementer's judgement call with no spec value behind it.

**Beyond those two, this change adds by-ear validation the predecessor never scheduled at all** — see §3.

## 3. Why by-ear validation is its own scope, not a formality

The predecessor's tests establish, for each of the thirty parameters, that it is registered, reachable,
bounded, and that its default reproduces the previous behaviour. **Three classes of defect survive all of
that**, which is why this is real work rather than a sign-off:

- **A knob wired to the wrong end of its own range** passes every bound and default test and sounds
  backwards. Nothing automated distinguishes "brighter as it rises" from "darker as it rises."
- **A range that is technically safe and musically useless.** Four parameters ship with ranges chosen by an
  implementer against no spec value: Ring Mod's carrier (20 Hz - 5 kHz), `kMaxDecaySeconds` (1.0 s), the
  Grace maximum (1.0 s), and Curve's shape family. Each is defensible and none is validated by ear.
- **A measured bound that is correct and still wrong for playing.** Drive's Bias is bounded to +-0.02
  because peak swing rises with any nonzero bias on an unbounded 5th-order polynomial — measured, +6.1% at
  the bound. Whether that range is musically worth having at all is a question no measurement answers.

**Also carried here, recorded rather than left implicit:** two of the predecessor's measurements
(Reverb Tilt, Reverb Tuned) were performed and reported but NOT pinned as regression tests — they ran in
standalone harnesses. They are correct today and nothing guards them tomorrow.

## 4. Upstream: what is actually blocked, and what is not

**Corrected relative to a claim made and withdrawn during the predecessor change, because getting this
wrong once is the reason it is stated carefully here.** Sheaf issues 1-6 are open upstream. Issue 7 —
`EncoderDraw`'s 4-character label cap — **was filed by this project and then withdrawn and closed by this
project as not-planned**, because the premise was wrong: `BuildFourteenSegmentCommands` is public with
`numChars` as an ordinary parameter, and the app owns the `std::vector<DrawCommand>` that
`BuildEncoderDrawCommands` returns, so the app composes its own label block. That is how T6.1 shipped, with
`External/Sheaf` untouched.

**The lesson is recorded because it generalizes to every item below:** "no app-side lever exists" is a
claim about a whole surface and needs the whole surface read. A missing configuration field is a different,
much smaller fact. **Before any task here is treated as upstream-blocked, that check is repeated.**

Consequently this change carries **no** work for issue 7, and treats the remaining six as genuinely
upstream-gated but re-checkable.

## 5. Non-goals

- **No new SLOTS.** All six banks are at fourteen and the slate is closed. **§6 is not an exception**: it
  proposes repurposing three of Delay's existing slots that today hold one-and-a-half controls between
  them, which changes what occupies a slot without changing the count.
- **No Sheaf fork and no local Sheaf patch.** The pinned-upstream property is worth more than the features
  (`UPSTREAM-SHEAF-ASK.md`), and this constraint is what makes §4's app-side-first check load-bearing
  rather than optional.
- **The design doc's open question 8** — the ASR envelopes cannot modulate anything, and the fifteen-source
  slate is full — remains open and out of scope. It is a modulation-slate question, not a bank-slot one,
  and it may outrank everything here.
- **Whether `kPmLfoDepth = 0.15` is the right PM depth ceiling** stays an open by-ear tuning item, fixed by
  changing the constant if it proves too shallow, not by adding a knob.

## 6. The Delay bank's three vestigial slots — Detune, Color, Halo

**Raised by the operator, 2026-08-13, and this proposal is where it lands rather than being acted on.**
The operator's position, recorded verbatim in substance: Detune was only ever tolerable while Width's two
roles were conflated; now that Width balance separates them, *"we weren't doing detune anymore."*

**Provenance note, corrected 2026-08-13.** An earlier version of this section stated the round-1/round-2
Delay research files were unrecoverable. **That was wrong** — all five survive at
`/private/tmp/claude-501/-Users-diegoaguilar-canabal-Desktop/6299e4a0-9bb0-47a0-b8b4-4ae3508fd32c/
scratchpad/` (`RESEARCH-drive-delay.md`, `RESEARCH2-drive-delay.md`, `RESEARCH-audio-filter.md`,
`RESEARCH2-audio-filter.md`, `RESEARCH-reverb.md`). They are read directly below. **They live in a session
scratchpad outside the repo and could be cleaned up at any time — if their content is to be relied on, copy
them into the repo rather than citing the path.**

### 6.1 What the three slots actually do — read, not assumed

- **Detune (slot 4)** is a static, symmetric L/R delay-time skew expressed in cents (`app/dsp/Delay.hpp`):
  `cents = ddet * kMaxDetuneCents` (50), `timeL /= 2^(+cents/1200)`, `timeR /= 2^(-cents/1200)`. Nothing is
  pitch-shifted; the two channels simply run at slightly different lengths, +-2.93% at full. The mean delay
  time is unchanged, which is the only thing distinguishing it from Stereo width.
- **Color (slot 7)** and **Halo (slot 8)** have **no destination of their own at all**:
  ```
  params.ddet = clamp(0.5f * (params.ddet + row7Color));
  params.dmod = clamp(0.5f * (params.dmod + row8Halo));
  ```
  Color exists solely as a co-input averaged into Detune; Halo solely as a co-input averaged into Mod
  depth. So the Detune knob alone cannot exceed half its range unless Color is also raised, and moving
  Color silently moves Detune. **The archived design doc already found this** and recorded that Color is
  *"not an independent tone control"* and Halo *"not early reflections"* — a v2-era compatibility fold, not
  a design.

### 6.2 Why this is the same defect this change's predecessor fixed three times

One value driven by two knobs, with neither knob owning a job, is precisely the conflation that justified
splitting Scoop freq / Scoop width / Scoop depth, splitting Width balance out of Stereo width, and
unlocking Link from Drive's own gain. **Three of Delay's nine original slots are spent on one-and-a-half
controls.** Detune survives the change's own selection rule on a technicality — no routing can produce a
static L/R asymmetry, since there is a single Delay-time knob — but it overlaps Stereo width in kind while
being roughly 12x weaker (`widthSpread` reaches 35% of base time; Detune reaches 2.93%).

### 6.3 Replacement candidates, from the archived table, none chosen here

Read from the round-1/round-2 Delay research directly. Of its six ranked candidates, three were built by
the predecessor (Feedback Drive rank 1, Feedback Tone rank 2, Crush rank 3, plus round 2's Width Balance).
**Three were never built and are available now**, each with the research's own precedent, reuse and
headroom verdict:

- **Diffusion (`Diff`, round-1 rank 4)** — sharp discrete repeats at 0.0, edges blurring at 0.5, smeared
  into "a reverb built from a delay" at 1.0. Precedent: Valhalla Delay's Diffusion section, Chase Bliss
  Mood's `Modify`. **Reuses in-tree allpass math** — `dsp::DriveBlendPhase`'s own
  `phased = -a*wet + x1 + a*y1` is the identical building block. Headroom: none, allpass sections being
  unity-gain by construction — **but only if the coefficient stays strictly inside the unit circle, using
  the same `0.98` margin `DriveBlendPhase` and `dsp::Comb` already use.** That caveat is now much more
  interesting than when it was written: §7 shows this codebase has exactly one loop-gain-above-unity
  defect already, and it came from a stage whose bound was proven and whose contraction was not.
- **Reverse Blend (`Rev`, round-1 rank 6)** — forward repeats at 0.0 through a mixed forward/reverse
  hybrid to fully backwards repeats at 1.0. Reuses `StereoDelay`'s existing `lineL`/`lineR` and `ReadAt`
  via a second, backward-incrementing read pointer. Strongest "wow factor" of any Delay candidate, ranked
  last on cost: genuinely-new, needs its own wrap handling and a click-free crossfade. **The research is
  honest that the continuous framing is its own extrapolation** — every shipped reference implements
  reverse as a discrete MODE, not a continuous knob.
- **Ducking (`Duck`, round-1 rank 5)** — repeats duck under new input. The research flags it itself as the
  weakest fit: explicitly corrective rather than characterful, working against the brief's stated bias, and
  with the weakest DSP reuse of the set. **Its own recommendation is to treat it as the first cut.**

**Freeze** also appears, from the archived `BANK-EXPANSION-DESIGN.md` rather than the research rounds —
normal writes at 0.0 through partial bleed to an infinite hold at 1.0. Note it is the deliberate form of
the accidental sustain analysed in §7.

Every one passes the selection rule: none is reachable by routing a modulation source onto an existing
parameter. **A further option needs no new parameter at all:** retire Detune and give Color and Halo real
destinations, which costs no slots and removes the fold rather than building on top of it.

**Nothing here is decided.** The operator has ruled Detune out; what replaces it, and whether Color and
Halo are repurposed or retired, is open (T3).

## 7. Stop does not silence the instrument — analysis, not a fix

**Operator-reported 2026-08-13: after pressing Stop, audio keeps sounding, but only once Randomize All has
been pressed several times and parameters are being modulated at audio rate. The operator likes the sound
and has explicitly NOT asked for a fix.** This section exists so the mechanism is on record before anyone
decides. Every claim below was traced by reading.

### 7a. The mechanism, and where it is NOT the problem

On the running→stopped edge (`FroggersAppCore.hpp`) the ASR gate closes and each VCO's Release time is
force-overridden to a fixed ~50 ms fade regardless of the operator's own Release knob
(`kStopFadeReleaseKnob`). `modulation_.Step()` then stops being called at all — the modulation slate
**freezes** at its last values, it does not reset. Once every voice reaches `Stage::Idle`, a single
unconditional `ForEachStatefulUnit(Reset)` zeroes all fourteen stateful units, including the delay lines,
the reverb tank, and every sub-unit this change added (`Delay.hpp`'s fbTone and crush, `Reverb.hpp`'s tilt
filters, damping and wet limiter).

**That reset coverage is complete — the new parameters' state IS reset. Reset coverage is not the gap.**

### 7b. The gap: a bound was mistaken for a decay, and this document is where that happened

`frogg3rs-bank-expansion` §7a stated a rule once for all three in-loop saturator pre-gains and had every
site cite it: `PadeSaturator::Saturate` hard-clamps to ±1, so a pre-gain on its ARGUMENT cannot raise the
loop's per-sample bound `|x| + |k|`. **That is true, and it is not the property that matters here.**

`Saturate` is `x*(27+x²)/(27+9x²)`, whose derivative at the origin is exactly 1 (`FilterFx.hpp`). It is a
hard output CEILING, not a contraction. The loop `y[n+1] = k · Saturate(g · y[n])` decays only while
`k · g · Saturate'(y) < 1`. Before these knobs existed, the feedback clamp alone guaranteed that
everywhere. **With a pre-gain of up to 4.0 the origin becomes an UNSTABLE fixed point:** any nonzero seed
grows until it lands on the saturator's curve and stays there — a persistent limit cycle that is bounded
and never decays. Bounded and silent are different properties, and §7a only ever established the first
while the whole change read it as clearing the second.

**All three sites that cite §7a share this, including one the investigation was not scoped to:**

| Site | Pre-gain range | Feedback reaches | Near-origin loop gain |
|---|---|---|---|
| Delay feedback (slot 9 `FbDr`) | 0.25 – 4.0 | `fbk` ≤ 0.98 | ~3.92 |
| Reverb tank (slot 10 `TkDv`) | 0.25 – 4.0 | `fb` ~0.99998 | ~4.0 |
| **Filter comb (slot 12 `CDrv`)** | 0.25 – 4.0 | `GetFeedback` ±0.95 | **~3.80** |

**Comb Drive is the third instance and arguably the worst**, because the comb sits in the always-on filter
chain rather than behind a send. `FilterFx.hpp`'s own header already records that a self-oscillating comb
at ±0.95 "can place this linearised system's loop gain arbitrarily close to" instability — Comb Drive then
multiplies that by up to four. **Comb Drive is also the flag `frogg3rs-bank-expansion` §9.6 WITHDREW as
"wrong rather than merely cautious."** The withdrawal was right about the bound and wrong about the
consequence, and it withdrew the one flag that would have caught this.

**Not implicated, checked rather than assumed:** Grit reuses `DigitalReorganizer`, whose
`Mangle(in) - Mangle(0)` form keeps zero-in/zero-out at any setting; Crush, Feedback tone and Tilt are
zero-preserving, and a lowpass in a loop can only reduce loop gain; Width balance and Tuned move read
indices and add no energy.

### 7c. Why Randomize All is the trigger

Each press draws a fresh, non-additive set of modulation-source depths across all 84 top-level parameters,
averaging ~2.25 connected sources each (`FroggersModulation.hpp`). Eligible sources include the three raw
VCO audio outputs — literally audio-rate. Repeated presses are repeated independent draws, so each one
raises the odds that a drive parameter's depth lands on a live audio-rate source *while* its base knob sits
above the instability threshold. A static patch rarely gets there; repeated randomisation reliably does.

### 7d. Options, listed and NOT chosen

1. Clamp the product (`fbk · fbDrive`, `fb · tankDrive`, `feedback · combDrive`) below 1 so contraction
   holds at every knob position.
2. Force the three drives toward unity on the Stop edge, alongside the existing release override.
3. Fire the fourteen-unit `Reset()` unconditionally on the Stop edge rather than deferring to `AllIdle()` —
   this trades away the deliberate "let the release ring through the wet path" behaviour.
4. Neutralise modulation DEPTH for these parameters on stop, not merely stop advancing the slate.
5. Extend the existing finite-only recovery watch with a post-Stop non-decaying-magnitude check.

**Option 1 is the only one that fixes the instability rather than the symptom** — the others make Stop
silent while leaving a non-decaying loop reachable during normal play. Recorded, not chosen.

### 7e. What could not be determined by reading

Whether the tail is strictly bounded by the ~1.05 s Grace-plus-fade window before `Reset()` lands, or can
run indefinitely — e.g. if some path re-seeds the loop independently of the transport gate. **Settling that
needs a runtime capture, not more reading** (T4.2). Note the upper bound of that window is itself set by
`kMaxGraceSeconds`, a value this change's predecessor picked with no specification behind it.
