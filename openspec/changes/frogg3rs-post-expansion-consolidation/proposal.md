# Proposal — `frogg3rs-post-expansion-consolidation`

**Created 2026-08-12 as `frogg3rs-validation-and-upstream-uptake`; renamed 2026-08-13 when that name stopped
describing it.** It began as two things the archived `frogg3rs-bank-expansion` could not close. It has since
absorbed a rework of the Delay bank's vestigial slots, an analysis of a real Stop-sustain defect, and four
new operator-requested controls — kept together deliberately, because they are one user story rather than
five errands.

---

## Why

`frogg3rs-bank-expansion` is built, green and archived — but "green" covers only what a test can see, and
using the instrument surfaced things no test was ever going to raise. Five threads, each with a reason it
cannot live in an archived change:

1. **Two of its tasks were un-closable by an implementer from the start** — they need a human's eyes and
   ears — and four shipped ranges were chosen against no specified value.
2. **Six upstream Sheaf gaps** remain outstanding against a pinned dependency this project never forks.
3. **Three of Delay's nine original slots hold one-and-a-half controls between them.** Detune, Color and
   Halo were vestigial before the expansion and the expansion did not touch them.
4. **Stop does not silence the instrument** once Randomize All puts audio-rate modulation on the drive
   parameters — traced to a rule this project wrote and then over-read.
5. **Four new controls** the operator asked for, two of which depend on thread 3's DSP.

## What Changes

- **Delay's Detune, Color and Halo become Freeze, Reverse Blend and Diffusion** (§6). Parameter count
  unchanged at fourteen; Freeze clamps its loop gain continuously, Reverse Blend gets buffer smoothing.
  **This MODIFIES `froggers-sheaf-parameter-model`'s "One sixteen-slot bank per Froggers page"**, whose
  Delay scenario currently requires slots 0-8 to be unchanged — the three being replaced are among those
  nine.
- **The Stop-sustain defect is analysed and NOT fixed** (§7), at the operator's request — including the
  finding that all three in-loop saturator pre-gains share it, one of which had its headroom flag withdrawn
  during the expansion on exactly the reasoning that turns out to be incomplete.
- **Record, Freeze, Reset Page and Reset All** are specified (§8), with recording settled as WAV-only.
- Adds requirements that a bank parameter be **audibly effective** across its range, that a **measured bound
  be pinned by a regression test**, that a **reset returns depths to neutral rather than zero** — the
  obvious reading of which is full negative modulation — and that a control needing genuine colour
  inversion **draws itself** rather than relying on the library's selected state.
- Adds the `froggers-upstream-uptake` capability: an upstream gap must be **proven app-unreachable before
  it is treated as blocking**, and the pinned dependency is never forked or locally patched. **This rule
  exists because this project broke it**, filed an issue on a false premise and withdrew it.

## 1. Objective

Close out everything the bank expansion left behind or caused, and the operator-requested work that hangs
off it, in one change rather than five. The threads are not independent — Freeze's clamp is the same clamp
the Stop-sustain fix would need (§6.4b-i), the Freeze button cannot exist before the Freeze parameter
(T5.2a), and recording's format choice is what decides whether an upstream gap matters at all (§8.3).
Splitting them would have hidden those couplings.

**What is buildable now:** the Delay slate, Reset Page/Reset All, Record, and — after the Delay work — the
Freeze button. **What is not:** the by-ear and visual validation, which needs the operator, and the six
upstream items, which need a dependency this project does not control.

## 2. What `frogg3rs-bank-expansion` actually left open

Verified against that change's own `§EXECUTION` record, not assumed:

- **T6.2 — the over-length label rework's acceptance criterion is VISUAL.** The rendering is verified
  programmatically, but **nobody has looked at it.** The predecessor change records a prior UI change in
  this project taking four attempts precisely by asserting a weaker property than "the operator can see
  it."

  **What actually ships, read from `app/FroggersUiSurface.hpp:1218-1366` — corrected 2026-08-13, see §9.**
  An earlier version of this bullet said "six character slots emitted for `CmbOff` where four were
  before, badge-chip clearance checked algebraically against `AppendBadge`'s own formula." That sentence
  was inherited verbatim from the predecessor's own `§EXECUTION` record and is wrong in three ways
  against the code. The shipped treatment: the app strips Sheaf's own trailing 4-character label block
  (`kSheafLabelCommandsPerChar` 15 x `kSheafLabelDefaultChars` 4 = 60 commands, :1232-1239 — the "four
  before" is the only part that survives), then draws its own plate below `centerY` and renders the
  parameter's **long** name (`FroggersBankLayouts()[bankIx].params[ix].name`, :1342 — so "Comb offset",
  not the short name `CmbOff`) split by `SplitFourteenSegmentLines` across **two rows of TEN character
  slots** (`kGridColumns = 10`, :1348, passed as `numChars` at :1364), each row centred by leading-space
  padding. Badge clearance is proven against `GetBadgePosition`/`drawBadges`, not `AppendBadge` — the
  plate starts strictly below `centerY` and upper badges end at or above it, and lower gesture badges are
  structurally absent because `SetGestureCount()` is never called (:1277-1294). `AppendBadge` is cited in
  that code for its chip **colour** only (:1310).
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
wrong once is the reason it is stated carefully here.** Sheaf issues 1-6 are open upstream, as is issue 8,
filed by this project on 2026-08-13 and then downgraded to the least important of them once WAV-only
recording removed anything to configure (§8.3). Issue 7 —
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

**Provenance note, corrected twice — the second correction is why the files are now in this directory.**
An earlier version of this section stated the round-1/round-2 Delay research was unrecoverable. **That was
wrong** — all five files survived in a session scratchpad outside the repo. Rather than cite a path under
`/private/tmp` that any cleanup would destroy, **the research is now checked in beside this proposal at
[`research/`](research/)**:

| File | Covers |
|---|---|
| [`research/RESEARCH-drive-delay.md`](research/RESEARCH-drive-delay.md) | Round 1 — Drive slots 10-13, Delay slots 9/10/12/13 |
| [`research/RESEARCH2-drive-delay.md`](research/RESEARCH2-drive-delay.md) | Round 2 — Drive slots 10-13, Delay slots 10/12/13 |
| [`research/RESEARCH-audio-filter.md`](research/RESEARCH-audio-filter.md) | Round 1 — Audio slots 12-13, Filter slots 10-13 |
| [`research/RESEARCH2-audio-filter.md`](research/RESEARCH2-audio-filter.md) | Round 2 — Audio slots 12-13, Filter slot 13 |
| [`research/RESEARCH-reverb.md`](research/RESEARCH-reverb.md) | Round 1 — Reverb slots 11-12 |

**This reverses the predecessor change's own instruction to keep the research out of the tree and cite it by
summary only.** That instruction is what allowed a summary to drift from its source and then be declared
lost; the operator's ruling is that the artifacts carry their own evidence. Everything §6.3 says below is
read from these files, and any later session can now check it rather than trust it.

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
  `phased = -a*wet + x1 + a*y1` is the identical building block (`app/dsp/Drive.hpp:693`). Headroom: none,
  allpass sections being unity-gain by construction — **but only if the coefficient stays strictly inside
  the unit circle, using the same `0.98` margin `DriveBlendPhase` itself uses** (`Drive.hpp:676`, `:689`,
  `:711` — its authored mapping is `0.98f * (2*knob - 1)`, i.e. the open interval (-0.98, 0.98)).
  **Corrected 2026-08-13 (§9): an earlier version credited that margin to `dsp::Comb` as well, and `Comb`
  does not have it.** `Comb`'s own margin is `kMaxFeedbackMagnitude = 0.95f`
  (`app/dsp/FilterFx.hpp:537`) — the same ±0.95 §7b's table already lists — and `FilterFx.hpp` carries no
  0.98 stability constant at all. The other 0.98 in this codebase is `StereoDelay`'s feedback clamp
  `fbk <= 0.98` (`app/dsp/Delay.hpp:442`), which is a feedback bound, not an allpass margin. That caveat is now much more
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
**freezes** at its last values, it does not reset. Once every voice reaches `Stage::Idle`, exactly one
`ForEachStatefulUnit(Reset)` fires and zeroes all fourteen stateful units, including the delay lines,
the reverb tank, and every sub-unit this change added (`Delay.hpp`'s fbTone and crush, `Reverb.hpp`'s tilt
filters, damping and wet limiter).

**That reset coverage is complete — the new parameters' state IS reset. Reset coverage is not the gap.**
(Precision, corrected 2026-08-13 (§9): the reset is written at TWO call sites, not one —
`FroggersAppCore.hpp:799` for "already idle at the stop edge" and `:825` for the deferred
clear-on-`AllIdle` watch — and the policy fires exactly one of them per stop. An earlier version said "a
single unconditional `ForEachStatefulUnit(Reset)`", which reads as one site; it is one FIRING.)

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

### 6.4 The slate: Diffusion, Freeze and Reverse replace Halo, Color and Detune

**Operator ruling, 2026-08-13.** All three vestigial slots are replaced outright. **Color is NOT made into
a real tone control** — the operator's judgement, and the reasoning that supports it: the shipped Feedback
tone (slot 10) already damps the repeats, and a second, wet-output tone would have been a nicer-but-not-
necessary variation on a filter this bank already has. It is dropped, not deferred.

| Slot | Today | Becomes |
|---|---|---|
| 4 | Detune — static +-50 cent L/R time skew, overlaps Stereo width at ~1/12 the strength | **Freeze** |
| 7 | Color — no destination; averaged into Detune | **Reverse Blend** |
| 8 | Halo — no destination; averaged into Mod depth | **Diffusion** |

**Slot assignment is a recommendation, not a ruling.** Halo -> Diffusion is name-adjacent and the two are
the same idea (§6.4a). The other two are interchangeable; nothing in the DSP prefers one slot over the
other, and the encoder grid's row-major mapping means only the on-screen position changes.

### 6.4a Halo IS Diffusion — traced, not reasoned from the names

**The delay has two insertion points, and every candidate here is really a choice between them**
(`app/dsp/Delay.hpp`):
- **In-loop**, the feedback tap: `fbL`/`fbR` -> crush -> feedback tone -> `Saturate(fbDrive * .)` -> written
  back. Anything here **compounds once per repeat**.
- **Wet output**, post-loop: `dL`/`dR` -> `wetLimiterL`/`wetLimiterR` -> `lastWet` -> `ToReverbMono` blends
  against dry by `dmix`. Anything here applies **once, equally to every repeat**.

The research places Diffusion as "a short allpass chain on the wet tap" — the second point — and the
archived design doc records that Halo's name always promised early reflections. An allpass diffuser on the
wet tap IS the practical form of that wash, so **making Halo real and adding Diffusion were never two
things.**

**Cost.** `dsp::DriveBlendPhase` already carries the one-pole allpass recurrence and its state
(`allpassX1`/`allpassY1`, `app/dsp/Drive.hpp`) — **verified by reading, not taken from the research** —
along with the `-0.98f` coefficient this codebase already treats as its stability margin. New per-channel
state for a short chain; composes-existing, but genuinely new plumbing rather than a drop-in.
**Headroom:** allpass sections are unity-gain by construction ONLY while the coefficient stays strictly
inside the unit circle. **That must be enforced by the knob mapping, not asserted in a comment** — §7 is
this codebase's record of what happens when a bound is proven and a contraction assumed.

### 6.4b Is Freeze continuous? Yes — in one implementation, and it collides with §7

**The operator asked directly, and the honest answer has a condition attached.** Freeze passes the
continuous-range rule (§3 ruling 3 of the archived change, the rule that cut Cycle and Hard Sync) **only if
built as a crossfade, not a switch**:

```
write  = inSignal * (1 - freeze)
fb_eff = lerp(fbk, 1.0, freeze)
```

At 0 it is today's behaviour; at 1 the line recirculates with no new input; in between, new material bleeds
in at reduced level over a loop that decays more slowly. **The midpoint is a real playable state**, which is
exactly what Cycle (stepping through integer retrigger counts) and Hard Sync (whose character IS the
discontinuity) could not offer. Built instead as a write-enable toggle, it fails the rule outright.

**⚠ RETRACTED 2026-08-14 — this paragraph asserted a false binary, and it cost a packet.** It read:
*"Freeze cannot be specified without deciding how it interacts with Feedback drive: either Freeze clamps
the product to 1, or `fbDrive` multiplies through it and full Freeze is a runaway rather than a hold."*

**That premise is false, and it was never traced — the code block directly above it already disproves it.**
`fb_eff = lerp(fbk, 1.0, freeze)` is a complete specification of Freeze in Freeze's own terms: it names no
`fbDrive`, needs none, and tops out at unity feedback — lossless recirculation, which is what "freeze"
means. What `fbDrive` then multiplies is Feedback Drive's business, exactly as it is for every other value
in this loop. There was never a decision to make here.

**The cost of the false binary, recorded because it is the §1 failure mode this rule exists to catch:**
§6.4b-i took its first horn and ruled "Freeze clamps the loop-gain PRODUCT to 1", which is only expressible
as `fb_eff = fbk + (1/fbDrive - fbk) * freeze` — dragging `fbDrive` into a mapping that had no business
knowing about it. Packet P4 built exactly that, and it is **non-monotonic**: at `fbDrive` 4.0 the knob runs
0.98 -> 0.25, so turning Freeze UP takes loop gain 3.92 -> 1.00. A knob that goes down as it turns up
violates this change's own new requirement that a control move "in the direction its name implies"
(`specs/froggers-sheaf-parameter-model/spec.md`), which §3 of this proposal names as a defect class that
survives every automated test this project has. **It survived the preflight audit (§9) too**: that audit
checked §6.4b-i's citations and never checked it against the code block in the section it amends.
§1's corollary is the lesson — when honouring an instruction forces a worse structure (here, a coupling
that need not exist), the instruction is suspect, not the implementer.

**Superseded by §6.4b-iii.**

### 6.4b-i Both open Freeze/Reverse questions are now ruled on (operator, 2026-08-13)

- **⚠ Freeze clamps — SUPERSEDED 2026-08-14 by §6.4b-iii, and the reason is recorded there.** This bullet
  ruled that "the loop-gain product is clamped to 1", taking the false binary §6.4b posed. Expressing a
  product clamp requires `fbDrive` inside Freeze's own mapping, which makes the knob non-monotonic at high
  Feedback Drive. **The ruling it was trying to express — full Freeze holds rather than grows, continuously,
  never leaving a runaway behind it — is preserved intact in §6.4b-iii**, which achieves it without the
  coupling by clamping Freeze's OWN feedback coefficient at unity. What changes is the mechanism, not the
  operator's intent.
  **Still true and carried forward:** the clamp is continuous rather than a latched state change applied at
  freeze-on; un-toggling restores sub-unity loop gain and the tail decays; and the clamp does not touch the
  un-frozen product `fbk * fbDrive`, so §7's accidental Stop-sustain behaviour is left entirely alone and
  T4.1 is not settled by side effect.
- **Reverse Blend gets buffer smoothing.** The edge-of-buffer click hazard at the forward/reverse crossfade
  is answered by smoothing rather than by narrowing the control. It is specified with the parameter, not
  left to implementation taste, because it is what makes the control shippable at all.

### 6.4b-iii Freeze, specified in Freeze's own terms (operator, 2026-08-14) — SUPERSEDES §6.4b's binary and §6.4b-i's product clamp

**Operator ruling, verbatim in substance:** *"obviously an encoder turning left to right should make a
value go from zero to a higher value"*, and — on being offered a knob-vs-gate tradeoff — *"i dont care what
drive is doing here, that is drive's own business. i already gave you specifications for what freeze
encoder and button should do."* Both are corrections of this document, not new requirements.

**The whole specification, and it references nothing outside Freeze:**

```
write  = inSignal * (1 - freeze)          // knob seals the loop as it rises
fbEff  = fbk + (1.0f - fbk) * freeze      // knob: 0.98 -> 1.00, monotonically UP, clamped at unity
fbEff  = kFreezeLatchOverdrive            // button: strictly above unity
```

- **The knob rises monotonically at every Feedback Drive setting**, because `fbDrive` does not appear.
  It tops out at unity feedback — lossless recirculation, no self-amplification. That IS the clamp, and it
  is a clamp on Freeze's own contribution rather than on a product Freeze does not own.
- **The button goes above unity**, so only the latch can make Freeze amplify. The gate the operator asked
  for on 2026-08-13 (§6.4b-ii) survives exactly: the knob cannot reach what the button reaches.
- **`fbDrive` multiplies whatever it multiplies**, as it does for every other value in this loop. That the
  total product may exceed unity at high Drive is Feedback Drive's doing, present at `freeze == 0` too, and
  not Freeze's to correct.
- **`kFreezeLatchOverdrive` is a BY-EAR constant, not a derived one.** It is set to a starting value and
  flagged for the operator to tune; nothing about it is measured or justified, and it must not be written
  up as though it were.

**At `freeze == 0` this is bit-exact `fbk`** (`fbk + x * 0.0f`), so the default and §7's accidental
Stop-sustain remain untouched — the same invariant §6.4b-i protected, preserved through the change of
mechanism.

### 6.4b-ii The Freeze BUTTON overrides the clamp — ruled earlier, written down here (2026-08-13)

**Operator ruling, verbatim in substance:** *"when toggled, the freeze button should override the freeze
encoder parameter and take it to the maximum above the clamp — period. when not toggled, the clamped
encoder parameter prevails."* **Recorded during this change's own audit (§9) after the operator noted it
had been decided in an earlier iteration. It had not been written anywhere:** `d870219`, `b3bc8e4`,
`1757ba7` and `57a9771` are every commit that touched Freeze, `b3bc8e4` ruled the clamp, `1757ba7`
specified the button as "drive the delay's Freeze parameter to its maximum", and
`git log -S"override"` over this directory returns nothing but an unrelated line about the Stop release
override. A ruling that exists only in conversation is a ruling a subagent cannot execute.

**What it means, and why it is the better design.** §8.2's Freeze button was specified as a latch over the
Freeze parameter — which made it a shortcut for turning the encoder up, and nothing more. Under this
ruling the two controls do different things:

| | Freeze value the DSP sees | Loop gain | Result |
|---|---|---|---|
| Encoder at max, button off | clamped | 1 | **holds** — recirculates, decaying slowly under `Saturate`'s compression |
| Button latched | un-clamped maximum | `1.0 * fbDrive`, up to 4.0 | **grows** — the deliberate form of §7's accidental limit cycle |
| Button released | clamped again | sub-unity | tail decays; nothing is left behind |

So the accidental sound §7 documents is **kept, and promoted to a control**, which is the outcome T4.1
listed as available and did not choose between. Its depth is the Feedback Drive knob (slot 9): below that
knob's centre `fbDrive < 1` and the latch holds instead of growing. That is expected behaviour, not a
defect.

**Implementation constraint that follows from it, and it is not optional:** the override must be applied
where the freeze mapping resolves the encoder's value, so that the clamp is a property of the PARAMETER
path and the latch substitutes for it. Writing 1.0 into the Freeze parameter and letting the clamp run
cannot exceed the clamp by construction — it would silently produce the hold in both cases, which is the
exact defect this ruling exists to prevent, and it would look correct in any test asserting only "the
button drives Freeze to maximum."

### 6.4c What replacing all three entails

1. **Delete the fold.** `params.ddet = 0.5*(ddet + Color)` and `params.dmod = 0.5*(dmod + Halo)` come out of
   `MapRowsToDelayParams`. Note this changes Detune's own reachable range — today it cannot exceed 0.5
   unless Color is raised — which is moot once Detune is retired, but matters if the two land in
   separate commits.
2. **Diffusion** — allpass chain on the wet tap, per §6.4a.
3. **Freeze** — input-write attenuation plus feedback-toward-unity, per §6.4b, with the `fbDrive`
   interaction decided.
4. **Reverse Blend** — a second, backward-incrementing read pointer into the existing `lineL`/`lineR` via
   `ReadAt`, crossfaded against the forward tap. Genuinely-new: needs its own wrap handling and a
   click-free crossfade. **The research is explicit that a CONTINUOUS reverse control is its own
   extrapolation** — every shipped reference makes reverse a discrete mode — so unlike Freeze, its
   continuity is by construction rather than by precedent. It passes the rule (the midpoint is a real mixed
   forward/reverse texture), but nobody has shipped it this way.

**Net: three vestigial slots become three real controls, parameter count unchanged at fourteen.**

## 8. Four new controls — Record, Freeze, Reset Page, Reset All

**Operator request, 2026-08-13, deliberately kept in this change rather than split out: these controls and
the Delay slate above are one user story.** Every fact below was read, and three of them change the shape
of what was asked for.

### 8.1 Reset Page / Reset All — the cheapest of the four, with one trap

Below the existing Randomize row, two buttons of the same size. They set, for the page or for all pages,
every parameter value to minimum and every modulation depth to off.

- **Placement is exact:** `AppendRandomizeRow()` builds the LAST of `FroggersCellMap::kRightRows`'
  seven entries (`kRightRows[6]`, `app/FroggersUiSurface.hpp:362-369`), two `Button` nodes each at
  `Extent::Weight(2.0f)` of four weight-units — two equal halves. "Below them, same size" means a new row
  appended after `Randomize` in `kRightRows` with the same two-halves weighting. **Note `kRightRows` is a
  fixed-extent `std::array<RightRow, 7>`** — "appended" means bumping that extent to 8, not just adding an
  initializer.
- **Recorded, because T5.1 anchors to this row:** `openspec/specs/froggers-app-surface-layout/spec.md:16`
  still says Randomize All lives in the global chrome band and Randomize Page in the per-page/bank header.
  The code has had both together in this row since task 10.2. Pre-existing drift; correcting the spec
  would widen this change's delta set, so it is recorded at T5.1c rather than fixed here.
- **"Minimum" is unambiguous and uniform: 0.0.** `ClampToRange(value, range)` ignores `range` entirely and
  returns `clamp(value, 0.0f, 1.0f)`; `ParameterConfig` carries no per-parameter min/max. So a reset is
  "set the normalized commanded value to 0.0" for every parameter, with no per-parameter table.
- **⚠ THE TRAP, and it would be silent: modulation depth "off" is 0.5, NOT 0.0.** Depth parameters are
  `RangeKind::Bipolar` and their neutral is `kNeutralModulationDepthCenter = 0.5f`. **Writing literal 0.0
  to a depth is FULL NEGATIVE depth — the opposite of off.** A Reset that took the request's wording
  literally would produce a maximally-modulated patch while appearing to do the opposite. The correct
  target is neutral, and the app already has the helper: `ZeroExistingModulationDepths(Parameter&)` walks
  the materialized depths and writes the neutral centre to both scene poles.
- **The enumeration already exists and should be mirrored, not re-derived.** `RandomizePage` acts on
  `drillIn.BankRef()`; `RandomizeAll` loops `bankIx` over `kFroggersBankCount` via `model.BankAt(bankId)`.
  Both branch on `drillIn.Level()` for the modulation-grid views, and Reset must mirror that branching or
  it will mean the wrong thing while drilled in.

### 8.2 Freeze button — and why it should be a Draw node, not a Toggle

Labelled "Freeze", beside Stop. One click latches it on and inverts its colours; clicking again un-latches.
When on it drives the Delay Freeze parameter (§6.4) to 1.

- **`NodeKind::Toggle` exists in Sheaf and this app uses it nowhere.** The app's only latched visual is
  `ControlStyle::selected` on `Button` nodes (the bank tabs), which JUCE renders via `StateColourFor` as
  `brighter(0.14f)` on the background — **a brightness bump, not an inversion.**
- **Upstream item 3 ("selected buttons invert background, not text") landed only PARTIALLY.** The
  `selected`/`color`/`textStyle` fields now exist, but `TextColourForNode`
  (`External/Sheaf/projects/synth/juce/PortableJuceBackend.hpp:1036-1042`) branches on `enabled` only and
  has no `selected` branch — **text colour never changes on selection**, and `StateColourFor` (`:285-292`)
  is a `brighter(0.14f)` bump. A genuine inversion cannot be had from the library's SELECTED-STATE
  handling.
- **⚠ But an app-side route does exist, and saying otherwise would break this change's own §4 rule.**
  **Corrected 2026-08-13 (§9): an earlier version of this bullet ended at "cannot be had from the
  library's own state handling" and let that stand as the reason Freeze must be a `Draw` node. It is not
  a sufficient reason.** `ControlStyle` carries app-settable `color` AND `textStyle`
  (`External/Sheaf/projects/synth/include/synth/PortableUIBuilders.hpp:20-33`), and `GlyphColourForNode`
  (`PortableJuceBackend.hpp:1046`) prefers `node.textStyle->color` whenever the app supplies one. The app
  rebuilds its whole tree every frame and already branches a style on its own state
  (`style.selected = BankSelected(bankIx)`, `app/FroggersUiSurface.hpp:957`). So a `Button` whose `color`
  and `textStyle.color` the app swaps from its own latch WOULD invert genuinely — the library's `selected`
  flag simply would not be the mechanism. Recorded because "no lever exists" is exactly the claim §4 says
  needs the whole surface read, and this bullet is the second time this project reached for it from one
  absent branch.
- **Therefore: build Freeze as a `Draw` node, like Play and Stop already are** (`AppendTransportRow`,
  `app/FroggersUiSurface.hpp:724-744`, 28 px plates) — **as a preference, not a forced choice.** Draw
  nodes emit their own commands, so the inversion is free, and it makes Freeze consistent with the two
  controls it sits beside rather than a third visual idiom next to them. The `Button`-with-app-supplied-
  colours route above is the live alternative if the Draw path proves awkward; either satisfies the
  spec's "SHALL NOT depend on the control library's selected-state rendering."
- **Interaction with the clamp, from §6.4b-i:** the clamp is continuous, so un-toggling restores sub-unity
  loop gain and the tail decays. The button is a latch over a continuous parameter, not a mode switch.

### 8.3 Record — what was asked for is not available, and what is

**Three findings, each verified by reading rather than inferred:**

1. **Sheaf provides NO recording capability.** No file writer, no output tap, no API — an exhaustive search
   of `External/Sheaf/projects/synth/` for `AudioFormatWriter`/`WavAudioFormat`/`createWriterFor` returns
   nothing. Every "record"/"capture" hit is UI-selection memory or a test rig.
2. **The audio config page cannot carry the configuration.** `AudioConfigPage` is built by Sheaf's own
   `BuildAudioPageTree` from a closed `AudioPageSnapshot` (outputs, inputs, selected ids, device/status
   text). **There is no extension point for an app to add rows**, and this app never wires the page at all
   — it inherits it from the shared runtime chrome. **So "configuration in the audio config page Sheaf
   gives us" is not available as stated.** Per §4's rule this was checked before being called blocked, and
   the check went further than the page itself: `MainPane::Page` (`runtime/MainPane.hpp`) is a CLOSED enum
   — `None, Audio, Controllers, Sync, File` — so "add our own sidebar page instead" is not available
   either, and `ExtraPage`/`RegisterPage`/`AddPage`/`customPage` return nothing across `include/` and
   `runtime/`. Unlike the encoder label there is no composition seam: the app never calls
   `BuildAudioPageTree` and never assembles the sidebar, so there is no returned tree to append to.
   **Filed upstream as [jvictor0/Sheaf#8](https://github.com/jvictor0/Sheaf/issues/8)**, asking for either
   an optional app-supplied section on the audio page or an app-supplied sidebar page.
   **Severity, corrected — an earlier version of this section called it "not a blocker" and that was an
   overcorrection** after overstating a different upstream gap days earlier. Accurately:
   - **It DOES block the design as requested.** The operator asked for the configuration to sit with the
     audio device selection. That is not possible, so that requirement is blocked outright.
   - **It does NOT block recording from existing — but only in the degenerate case.** WAV-only needs no
     configuration at all, so a WAV recorder ships with nothing to place. The moment a format choice
     exists (v1 shipped four), the setting needs a home.
   - **The "the app has its own surface" answer is weak and was presented as though it settled the
     question.** That surface is a dense instrument panel on a fixed row map; a rarely-touched export
     setting there is somewhere to dump it, not a natural home.
   The issue also states explicitly that no recording facility is being requested, since capture is
   app-side.
3. **The app core is mechanically barred from JUCE**, and v1's recorder is entirely JUCE.
   `app/check_no_juce.cpp` compiles `Froggers.hpp` *with JUCE on the include path* and fails the build if
   `JUCE_MAJOR_VERSION` ends up defined — absence of a link dependency is explicitly not accepted as proof.
   v1's `AudioRecorder`/`AudioFormatWriter`/`FileChooser` (`desktop/Source/`) therefore **cannot be ported
   into the core as-is.**

**What v1's button actually is, for reference:** a dark-red circle with a brighter inner accent while
armed, labelled "Record audio"; click starts a lock-free capture into an in-memory float buffer capped at
~30 minutes; click again stops, reads a WAV/MP3/FLAC/OGG choice from a radio cluster, opens a save dialog
and writes via JUCE. Recording is refused unless audio is already running ("Press Play before recording").

**The shape that IS buildable, and it follows the app's existing split:**
- **Capture lives in the core** — accumulating samples into a buffer needs no JUCE and no Sheaf support.
- **Export lives in `app/FroggersMain.cpp`**, which is the JUCE host and sits OUTSIDE the no-JUCE gate.
  That is the same boundary the app already draws.
- **Format: WAV ONLY — decided by the operator, 2026-08-13.** WAV is writable by hand with no dependency
  at all, and more importantly **it needs no format choice, so there is nothing to configure and nothing
  to place.** That dissolves §8.3's placement problem rather than working around it: the export layer
  needs no JUCE audio-format dependency, and the core/host split reduces to the file dialog alone.
- **Consequently Sheaf#8 is downgraded to the least important open issue**, and says so upstream. It
  blocks a design that would have been nicer and blocks nothing being built. It becomes real again only if
  MP3/FLAC/OGG are added later — v1 shipped all four; this app does not need them.

## 9. Audit of this change against the code, 2026-08-13

**Every code-shaped claim in this proposal, `tasks.md` and the three spec deltas was re-read against the
tree at `57a9771` (`External/Sheaf` pinned at `77a3019e`, working tree clean).** Recorded here rather
than in five places, per OMNI §16.5; each corrected passage cites §9 at its own site.

**Held up under reading** — the Color/Halo fold and its exact form (`Delay.hpp:608-609`); Detune's
`kMaxDetuneCents = 50` and ±2.93% span against `widthSpread`'s 0.35 (`:422`, `:427`); all three
saturator pre-gains at 0.25–4.0 (`Delay.hpp:341`, `Reverb.hpp:172-173`, `FroggersAppCore.hpp:1403`) with
`fbk <= 0.98`, tank `fb -> ~0.99998` and comb `±0.95`; `Saturate`'s `x(27+x²)/(27+9x²)`
(`FilterFx.hpp:102`); the frozen modulation slate (`FroggersAppCore.hpp:889-891`) and the ~50 ms release
override (`:1241-1245`); 84 top-level parameters (6 × `kFroggersParamsPerBank` 14) drawing a mean 2.25
sources (`FroggersModulation.hpp:1119`) with the three VCO audio-outs eligible (`:163`, `:570-581`);
`ClampToRange` ignoring `RangeKind` and `ParameterConfig` carrying no min/max
(`ParameterModulation.cpp:449`, `ParameterModulation.hpp:260-269`); `kNeutralModulationDepthCenter = 0.5f`
and `ZeroExistingModulationDepths` skipping unmaterialized depths (`FroggersModulation.hpp:954`,
`:1036-1046`); Randomize's enumeration (`:1272-1323`); the Randomize row's two `Weight(2.0f)` halves
(`FroggersUiSurface.hpp:1383-1402`) and the transport row's 28 px `Draw` plates (`:724-744`); `Toggle`
present in Sheaf and used nowhere in `app/`; the closed `AudioPageSnapshot` and closed `MainPane::Page`,
with no `ExtraPage`/`RegisterPage`/`AddPage`/`customPage` anywhere in `include/` or `runtime/`; no
recording facility in Sheaf; `check_no_juce.cpp` compiling the core with JUCE on the include path; v1's
30-minute cap and *"Press Play before recording."* (`desktop/Source/MainComponent.cpp:201`, `:219`);
`kMaxDecaySeconds`/`kMaxGraceSeconds` at 1.0 s, `kPmLfoDepth = 0.15f`, Ring Mod at 20 Hz–5 kHz, Drive
Bias at ±0.02. **The suite was re-run, not inherited: 213 passed / 0 failed across the 10 test binaries**,
matching the baseline in `tasks.md`.

**Corrected, each because the code says otherwise:**

1. **§2 / T1.1 — the label rework was described as something else entirely.** "Six character slots for
   `CmbOff` where four were before" is ten slots across two rows, rendering the LONG name. The sentence
   came from the predecessor's `§EXECUTION` record, and §2 said it was "verified against that change's own
   `§EXECUTION` record, not assumed" — which is the laundering OMNI §1 names: verified against the
   RECORD, never against the code. It matters because T1.1 is a VISUAL task and this is what the operator
   is being asked to look at.
2. **§6.3 / T3.1c — `dsp::Comb` does not carry the 0.98 margin** it was credited with; its margin is
   ±0.95, which §7b's own table already said. The proposal contradicted itself two sections apart.
3. **§8.2 / T5.2 — "a genuine inversion cannot be had" was true only of the library's `selected`
   handling**, and was doing the work of a general impossibility claim. `ControlStyle::color` +
   `textStyle` is a live app-side route. This is the §4 failure mode inside the change that specs §4.
4. **§7a — "a single unconditional `ForEachStatefulUnit(Reset)`"** is two call sites and one firing.
5. **A `MODIFIED Requirements` delta was missing.** T3.1 is marked DECIDED and replaces three Delay
   slots, but `openspec/specs/froggers-sheaf-parameter-model/spec.md:114-117` requires "slots 0-8 are
   unchanged from the Delay bank's existing nine parameters" — Detune, Color and Halo are three of those
   nine. The delta held only `ADDED` requirements, so nothing in it contradicted the spec the change
   contradicts. Added, using the same full-restatement form the predecessor used
   (`archive/2026-08-13-frogg3rs-bank-expansion/specs/froggers-sheaf-parameter-model/spec.md:55`).
   `openspec validate --strict` passed both before and after — it checks delta STRUCTURE, not whether a
   change specified what it decided, so a green validate was never evidence here.

**Found in the code and the live specs, not in this change's artifacts.** Both surfaced while auditing
T5.1's own anchors, and the operator ruled on them separately on 2026-08-13:

- **FOLDED INTO T5.1's PACKET (T5.1d), not filed for later.** `FroggersModulation.hpp:1266` still
  describes Randomize Page as "this bank's 9 values + Crispy"; `RandomizeBankValues` loops
  `kFroggersParamsPerBank`, which is 14. T5.1 tells the implementer to mirror that enumeration, so a stale
  comment sitting on the function being mirrored is load-bearing here. One line, mechanical once found,
  and OMNI §16.2 is explicit that such a fix belongs in the next bounded dispatch rather than a
  someday-chip — so it ships in T5.1's own commit.
- **RECORDED, still unscheduled (T5.1c).** `openspec/specs/froggers-app-surface-layout/spec.md:16` puts
  Randomize All in the global chrome band and Randomize Page in the per-page header. The code puts BOTH in
  the right column's last row (`AppendRandomizeRow`, `kRightRows[6]`). Correcting it would add
  `froggers-app-surface-layout` to this change's delta set — a scope decision, not a typo fix — so it is
  recorded rather than folded. It stays load-bearing regardless: T5.1 appends two controls to the row that
  spec sentence describes wrongly.
