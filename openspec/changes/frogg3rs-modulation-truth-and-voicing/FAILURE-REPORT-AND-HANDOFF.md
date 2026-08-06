# Failure report and handoff — `frogg3rs-modulation-truth-and-voicing`

**Written 2026-08-05 by the agent that failed.** The operator ran the consolidated build and
reports the instrument is still broken in four ways. Every fix below was measured green, and every
symptom below is still present. **Read the systemic error section before touching any code** — the
individual hypotheses matter less than why a session's worth of green measurements did not move the
symptoms at all.

---

## 1. THE SYSTEMIC ERROR — read this first

**Every measurement taken this session was on an isolated stage, driven by a synthetic adversarial
input, asserting that stage's own output bound. Not one measured the real signal path end to end,
with real VCOs, real modulation routing, real Crispy, real transport.**

Concretely, the harnesses did things like: instantiate a bare `FilterFxChain`, feed it a
per-sample-random height and a unit input, assert `filterOut ≤ (A+fb)/(1+fb)`. That is a correct
test of a bound. It is **not** a test that the instrument stops blowing out.

Consequences, all visible in hindsight:
- Five stages each pass their own bound test (comb 0.0, peak 0.990, delay 0.999999, reverb
  1.000000, DriveBlendPhase 0.90) while the operator still hears blowouts. **The composite was
  never measured.**
- The `useParallel` convexity argument (§K) says a convex blend cannot exceed the max of its
  inputs, which is true — but "each stage ≤ 1.0" combined with "master threshold 0.9" still means
  the master is engaged essentially always. B7.1 was written to fix exactly that and **was never
  implemented.** The architecture on paper is not the architecture in the binary.
- The one test that WOULD have caught this — §K's B7.5, *"the master limiter's envelope stays at
  unity across a hostile patch"* — was specified and never written. **Write it first. It is the
  only acceptance criterion that matters.**

**Directive for the next agent: do not add another per-stage bound test until an end-to-end test
exists that reproduces the operator's actual repro** (Filter Crispy at max, modulation live,
transport running) **and fails.** A green suite that does not move a reported symptom is worse than
a red one.

---

## 2. Observed failures, as reported

### F1 — Randomize All produces far more than 3 badges per parameter
Operator: *"i'm seeing a LOT of outcomes of >3 badges per parameter... your distribution shaping
algorithm is trash."* Non-additivity DOES now work (confirmed by the operator) — depths reset
between presses. The **count distribution** is wrong.

The helper is `detail::RandomizeParameterModulationDepths` (`app/FroggersModulation.hpp`), spec'd
in the predecessor change as: never zero, **median 3**, sharp falloff above 4, geometric tail to
the full connected-source count, distinct sources per call. Tests
(`randomize_depth_helper_median_count_is_three_across_1000_trials`) assert the median.

**HYPOTHESIS CORRECTED 2026-08-05 after the operator pushed back. The first version of this
report blamed A3 for drawing source sets independently per pole. THAT IS WRONG — verified by
reading the loop: the source set is drawn ONCE via partial Fisher-Yates, and both poles receive
values for the SAME `eligible[i]`. A3 matches its spec. Do not chase it.**

**THE ACTUAL LIKELY CAUSE — the zeroing is incomplete, and `HasNonZeroState()` counts more than
the depth's own value.**

`Parameter::HasNonZeroState()` (Sheaf, `src/ParameterModulation.cpp`) returns true if ANY of these
differ from neutral: `currentCenter_`, `targetCenter_`, `sceneCenters_`, `gestureActiveMasks_`,
**`currentDepths_`**, **`targetDepths_`**, `currentNormalizationOffsets_`,
`targetNormalizationOffsets_`.

`currentDepths_`/`targetDepths_` on a level-1 depth parameter are **that depth's OWN sub-depths —
level 2.**

`detail::ZeroExistingModulationDepths` (`app/FroggersModulation.hpp`) writes only
`depth->SceneCenter(pole) = kNeutralModulationDepthCenter` for each pole. **It does not recurse
into the depth's own modulation depths.** Therefore any level-1 depth that ever received level-2
assignments keeps reading as "has non-zero state" forever, and keeps lighting a badge on the main
page, even though its own value was correctly zeroed.

**This explains the exact symptom pair the operator reports:** values DO reset (non-additivity
works — confirmed), but badge counts stay high and accumulate. It also means badge count is not
the count of sources with non-zero DEPTH — it is the count of sources whose depth parameter has
non-zero state ANYWHERE in its subtree, which is a different and much larger set.

**And it is coupled to F4.** Whatever level-2 assignments exist — from the fan-out working
previously, or from any earlier session — persist through every subsequent zeroing.

**SECOND CORRECTION, same day — the subtree hypothesis above does NOT explain the operator's
actual observation either.** They randomized ONCE and saw many parameters at 4-7 badges. A level-0
Randomize All does not create level-2 assignments (the fan-out lives only in the drill-in branch,
see F4), so persisting sub-depths cannot be the cause of a single fresh press — unless the loaded
patch already carried them from an earlier session, which is possible but must be verified, not
assumed.

**THE ARITHMETIC NOBODY DID.** The spec'd distribution is **10/30/30/20** for counts 1/2/3/4, then
a geometric r=0.7 tail for 5+. Therefore **P(count ≥ 4) = 30%**. Across 16 visible parameters that
is ~5 expected at 4+ per press. **"Many parameters with 4+ badges" is what this distribution
produces by design.** "Median 3" means half of all draws are 3 or below — it does NOT mean most
draws are 3 or below, and the operator's expectation ("bias towards a median/mode of 3") reads as
wanting the latter.

**The part that does NOT fit the spec is 7.** The geometric tail should put P(7) well under 1%. If
7s are appearing with any regularity, the tail is fatter than specified.

**Three candidates now, and they are distinguishable by ONE measurement:**
1. **The distribution is implemented correctly and the SPEC is what the operator dislikes.** Then
   this is a spec change (tighter distribution — e.g. mode 2, hard cap at 4), not a bug fix.
2. **The tail is implemented fatter than 10/30/30/20 + r=0.7.** A real bug in the count draw.
3. **The badge criterion is over-counting** via the subtree issue above — only viable if the loaded
   patch already carried level-2 state.

**THE DECISIVE MEASUREMENT — do this before touching anything:** from a genuinely fresh patch,
press Randomize All once, and histogram the per-parameter count three ways: (a) the count the
helper CHOSE, (b) the number of depths with non-neutral `SceneCenter`, (c) the number where
`HasNonZeroState()` is true. Compare (a) against 10/30/30/20+tail.
- (a) matches spec and (c) == (a) → **candidate 1**, spec change, talk to the operator about the
  distribution they actually want.
- (a) does NOT match spec → **candidate 2**, fix the draw.
- (c) > (b) → **candidate 3**, the zeroing/badge issue above is real and additional.

**Do not "fix" the distribution before running this.** The lead twice asserted a cause for F1 and
was wrong twice — first blaming A3's per-pole draws (disproved by reading the loop), then the
subtree state (disproved by the operator pointing out a single fresh press cannot have created
level-2 state). Both times the operator caught it. Measure first.

### F2 — Filter Crispy at max still blows out
Unchanged from the start of the session, despite: comb trim, peak trim, peak limiter, delay
saturator + wet limiter, reverb wet limiter, DriveBlendPhase smoothing + limiter.

**Every one of those was verified in isolation. The composite was never verified.** See §1.

**Hypotheses:**
1. **Stage ceilings are 1.0 while the master's threshold is 0.9** — so every stage can legitimately
   run the master into continuous limiting. **B7.1 (retarget all stages to the measured C=0.80,
   plus make-up gain after the master) was specified and never built.** This alone could account
   for the entire symptom.
2. Crispy is a bit-scramble producing **discontinuous** per-sample parameter jumps
   (`app/dsp/Fuegoize.hpp`). All the smoothers added (comb trim 0.45, peak trim 0.45, phase coeff
   0.0035 cycles/sample) were tuned against *random* sweeps, not against Crispy's actual
   scramble pattern. **Reproduce with real Crispy, not with `rand()`.**
3. Something upstream of the filter is already hot. The VCO→envelope→Drive path was never bound —
   §K explicitly excluded Audio and Envelope as "bounded and gated", which was reasoned, **not
   measured**.

### F3 — Stop does not stop. THE MOST SERIOUS.
Operator: *"it has now been over a minute since i stopped audio, and it's still coming out. even
though the oscilloscope isn't moving. something is stuck in an infinite loop with extremely harsh
loud noise."*

**The decisive clue: the scope is still, but audio continues.** The scope displays VCO output. So
the VCOs are silent and **something downstream is self-sustaining with no input.** This is not a
release tail; it is an oscillation.

**What I checked and wrongly concluded was fine:** a forced ~50 ms release is substituted while
stopped (`FroggersAppCore.hpp:970-976`), and `delay_.ClearBuffers()` + `reverb_.Reset()` fire at
`audioAdsr_.AllIdle()` (`:652-653`, `:679-680`). Two tests drive delay fb→0.98 and reverb
Hold→0.99998 and assert post-Stop silence; both green. **I reported this as "already fixed, verified
by trace." That conclusion was wrong.**

**Hypotheses, in order:**
1. **`AllIdle()` never becomes true**, so the flush never fires. If any modulation source drives an
   envelope parameter, a voice may never reach `Stage::Idle`. Audio-rate modulation of Attack /
   Sustain / Release was never considered against the Idle condition. **Check this first — it
   explains "forever" precisely.**
2. **The self-oscillator is a stage the flush does not clear.** `ClearBuffers()`/`Reset()` cover
   delay and reverb. They do **not** clear: the comb's delay line inside `FilterFxChain`, the
   `DriveBlendPhase` allpass state, `SampleRateReducer`/`DigitalReorganizer` holds, or any limiter
   envelope. A comb at fb=0.95 with its in-loop saturator is bounded but **decays only as
   fb^n** — and if its input never truly reaches zero it never decays at all.
3. **The limiters I added have 100 ms release and their own state.** A limiter cannot generate
   signal, but a limiter whose envelope is stuck low will *duck* signal — that is the opposite
   symptom. Rule out, do not chase.
4. **"Harsh loud noise" suggests a nonlinearity self-oscillating**, which points at the comb's
   in-loop saturator or `DriveBlendPhase`'s allpass with a stuck coefficient, not at a linear tail.

**First move: instrument every stage's output magnitude per block after Stop and print which one is
non-zero.** Do not reason about it. The answer is one print statement away and I never took it.

### F4 — Randomize All at drill level 1 does not fan out to level 2
Operator: *"it only appears to be randomizing knob positions, not modulation depths in level 2...
randomize all should randomize modulation depths 1 page deeper, if that exists... the same
median/mode skew of 3 non-zero depths should apply in all cases."*

So: at level 1 the depth VALUES visibly change (level-1 knobs move), but **no level-2 depth
assignments are created**, so no level-1 cell ever shows a badge. Expected per spec: randomize the
current level's depths AND fan out one level deeper where a deeper level exists — with the **same
median-3 distribution at every level.** Only at max depth (level 2) should the fan-out stop.

**Hypothesis — strong, and previously recorded but never resolved:** silent **pool exhaustion**.
`EnsureModulationDepth` returns `nullptr` when `!group_.CanAllocate()`
(Sheaf `ParameterModulation.cpp`), and the app helper `break`s with `partial = true`
(`app/FroggersModulation.hpp`). The level-2 fan-out needs up to 15 × 15 = **225** depth parameters
against a ~1200-slot pool that Randomize All has already largely consumed at level 0. **A4 made
`partial` observable via `LastRandomizePartial()` — but nothing surfaces it to the operator**, so
this fails completely silently. (The stderr log was correctly removed as a real-time violation and
never replaced with a UI-thread equivalent.)

**Second hypothesis — the fan-out may simply not be implemented as specified.** A previous agent
told the operator that fan-out-one-level-deeper was "the spec AND the code," and the operator
built expectations on that. **Verify it against the code before assuming capacity is the cause.**
`RandomizeAll`'s `Level()==1` branch was read this session as "randomizes `originalParam`'s 15
depths, then for each now-materialized depth presses into level 2 and randomizes its 15
sub-depths" — but that reading came from a subagent's report and **was never independently
confirmed by the lead, and never covered by a test.** If the fan-out loop does not exist, is
short-circuited, or silently no-ops, no amount of capacity would help.

**The spec, restated from the operator 2026-08-05, because it must hold at EVERY level:**
Randomize All randomizes the current level's depths AND fans out one level deeper wherever a
deeper level exists, stopping only at max depth (level 2). **The median-3 distribution applies
identically at every level** — not just at level 0.

**First move:** read the `Level()==1` branch directly and confirm the fan-out loop exists and
executes. Then read `LastRandomizePartial()` after a level-1 Randomize All. Capacity and
not-implemented produce the identical symptom, and only reading the code distinguishes them.

---

### F4 ADDENDUM — SCOPE ANSWERED, AND A LIKELY ROOT CAUSE (read 2026-08-05, lead read the code directly)

**Operator's question: at level 1, does Randomize All affect level-2 for EVERY parameter, or just
the drilldown you are in? ANSWER: just the drilldown you are in.**
`RandomizeAll`'s `Level()==1` branch (`app/FroggersModulation.hpp:~1097-1150`) operates solely on
`drillIn.BankRef().SelectedParameter()`:
- Step 1 randomizes that one parameter's own 15 depths (median-3 helper).
- Step 3 loops `modIx` 0..14 and, for each depth that is materialized (`ModulationDepthParameter`
  non-null — i.e. roughly the ~3 Step 1 just chose, plus any pre-existing), presses into level 2
  and randomizes ITS 15 sub-depths.
- Total 15 + 15×15 = **240 parameters, all belonging to the ONE focused parameter.** The in-source
  comment calls this "the only path in the design that creates level-2 storage."

**CONFIRMED CONSEQUENCE: a level-0 Randomize All creates NO level-2 state.** This definitively
kills the subtree hypothesis for F1 on a fresh patch — the operator was right.

**LIKELY ROOT CAUSE OF F4's SYMPTOM — the function navigates the operator OUT of the drilldown.**
Step 2 calls `drillIn.Back()` (exit to level 0) to locate the parameter's encoder id in the
parameter grid, then `PressEncoder` to reopen level 1. The Step-3 loop then does
`PressEncoder(modIx)` → level 2, randomize, `Back()` → level 1, per source. **After the loop there
is a final `drillIn.Back()`, which drops the level to 0.** So pressing Randomize All inside a
level-1 drilldown ends with the operator on the MAIN BANK PAGE.

That matches the report exactly: *"i only see the knob positions change and the badges change for
the main bank page."* **They are seeing the main bank page because the operation put them there.**
The level-2 work may well be happening correctly and simply not be visible, because the view they
would need to see it in has been exited.

**OPERATOR RULING 2026-08-05, both parts:**
1. **The SCOPE is correct as built** — *"this is desired functionality for randomize all"*.
   Level-1 Randomize All affecting only the drilled parameter (its 15 depths + their level-2
   sub-depths) is the intended behaviour. **Do not change the scope.**
2. **Navigating out is a BUG** — *"randomize all in level 1 shouldn't navigate me out wtf"*.
   Randomize All must leave the operator **exactly where they were**, in the same level-1 view of
   the same parameter, with the results visible on that page.

**So F4 splits into two fixes:**
- **F4a (navigation, almost certainly the whole visible symptom):** restore the drill-in state
  after the operation. The branch currently ends on a bare `drillIn.Back()` after the loop, which
  drops level 1 → 0. It must instead return to the level-1 view of `originalParam`. Note Step 2
  ALSO leaves the view (`Back()` to level 0 purely to look up the encoder id in the parameter
  grid) — that round trip is an implementation detail the operator should never observe, and if
  the id can be found without exiting, better still. **The whole operation should be visually
  atomic: press it, stay put, see the badges change on the page you are on.**
- **F4b (verify the work actually happened):** with navigation fixed, confirm the focused
  parameter's depths really do carry non-neutral level-2 sub-depths, and check
  `LastRandomizePartial()` for silent allocation failure. If badges appear on the level-1 cells
  once you stop being ejected, F4 was only ever a navigation defect.

**Do F4a first.** It is likely that the randomization has been working correctly all along and was
simply never visible.

**Also note the allocation pressure:** the comment records that `PressEncoder`'s view-open
**eagerly materializes ALL of a depth's connected sub-depths**, not just the ones randomize picks.
So the 240 figure is materialization, not selection — and against a ~1200-slot pool, a few level-1
randomizes could plausibly exhaust it. That remains the secondary hypothesis if (b) comes back
empty.

### F1 ADDENDUM — distribution target agreed with the operator (2026-08-05)

**"mode 2, rarely above 4 is good enough."** That supersedes the predecessor's median-3 spec.
Current spec is 10/30/30/20 + geometric r=0.7 tail, giving **P(≥4) = 30%** — which is what the
operator is seeing and objecting to. Target instead: **mode 2**, with 4+ genuinely rare (single
digits of percent, not 30%), and 7 essentially never. Still never zero, still distinct sources,
**and the same distribution must apply at EVERY level, not just level 0** (operator, explicit).
Re-derive the table to hit mode 2 and re-pin the tests on the new distribution — including a test
that the SAME distribution governs the level-1 and level-2 draws.

## 3. What was attempted, and what each fix actually proved

| Fix | Measured result | What it actually proved |
|---|---|---|
| A1 non-additive randomize | works (operator-confirmed) | genuinely fixed |
| A2 `ComputeAllParameters()` reseed | drill-in knobs move | genuinely fixed |
| A3 scene-pair semantics | both poles written | correct as specified; source set drawn ONCE, verified by reading the loop. The report's first F1 hypothesis blamed this and was wrong |
| A4 surface `partial` | atomic published | **useless in practice** — nothing shows it to the operator |
| E.1 fan-out to level 2 (predecessor change) | claimed done, spec'd | **UNVERIFIED by this session** — reported by a subagent, never read by the lead, never tested. F4 says it does not work |
| W2.2a comb trim `1/(1+fb)` | overshoot 0.0 | isolated stage only |
| B1 peak trim `1/height` | 1.819 → 1.669 | structurally insufficient, correctly reported |
| B5 peak limiter | 1.669 → 0.990 | isolated stage only |
| B2 delay in-loop saturator | loop bounded | isolated stage only |
| B6 delay/reverb wet limiters | 1.962→1.0, 66.6→1.0 | isolated stage only |
| B7.2 DriveBlendPhase | 61.2× → 0.90× | isolated stage only; **largest single find, still did not fix F2** |
| B3 release ceiling 5s→2.5s | — | unrelated to F3, which is oscillation not tail |
| C1 scene-2 mirror defaults | works | genuinely fixed |
| **B7.1 shared ceiling C=0.80** | **NEVER IMPLEMENTED** | **the missing piece that ties all the stage work together** |
| **B7.5 master-envelope-at-unity test** | **NEVER WRITTEN** | **the only test that would have caught the failure** |

---

## 4. Recommended order for the next agent

1. **Write B7.5 first** — end-to-end, operator's real repro, assert the master limiter's `envelope`
   stays at unity. It should FAIL. That failing test is the entry point for everything else.
2. **F3 (Stop)** — highest severity, and likely the easiest: instrument per-stage output after Stop
   and find which stage is non-zero. Check `AllIdle()` never latching first.
3. **F4** — check `LastRandomizePartial()`. Likely capacity, not RNG.
4. **F1** — measure the actual per-parameter badge count from a fresh patch, per pole and unioned.
   Suspect A3's two independent draws before touching the distribution.
5. **F2** — only after B7.1 lands and B7.5 is green. Reproduce with **real Crispy**, not `rand()`.

**Do not trust any assertion in this change's test suite to mean the instrument works.** 176 tests
pass and four reported failures remain. The suite measures stages; the operator hears the
instrument.

---

## 5. F5 — raise the drill-in maximum to level 3 (operator, 2026-08-05)

*"increase the drilldown level maxima to 3, not 2 (keeping with the frogg3rs as base-3 system
theme)"* and *"the omni rule compliant implementation of this should be really really simple"* —
correct, and it is simple ONCE the existing §8 violation is removed. It is not simple today.

### The violation blocking it

**The cap is TWO hardcoded `2`s with no named constant** (`app/FroggersModulation.hpp`):
- `PressEncoder`: `if (level_ >= 2)` (~`:676`)
- `Back()`: `const bool wasLevelTwo = (level_ == 2);` (~`:723`)

Same concept, two definition sites, magic number both times. Operator: *"it would be an omni rule
violation for it to be hardcoded in multiple places."* Exactly — and it is why 2→3 is not currently
a one-line change.

### The second obstacle: `Back()` remembers only ONE level

`Back()` synthesizes the one-level pop (Sheaf exposes only a full `Deselect()` — upstream ask 11)
by: full `Deselect()` to level 0, then re-press the remembered `level1Encoder_` to return to level
1. **That works only because there is exactly one intermediate level to restore.** From level 3,
popping to level 2 requires re-pressing level 1's encoder *and then* level 2's — state the current
code does not keep.

### Agreed design (operator: *"i like the array approach"*)

Replace the single remembered id with an array indexed by level, and derive everything from one
constant:

```cpp
static constexpr std::size_t kMaxDrillLevel = 3;                 // ONE definition site
std::array<synth::PhysicalEncoderId, kMaxDrillLevel> levelEncoders_{};
```
- `PressEncoder`: cap becomes `if (level_ >= kMaxDrillLevel)`; on a successful descent record
  `levelEncoders_[level_]` before incrementing.
- `Back()`: pop one level by `Deselect()` to 0, then re-press `levelEncoders_[0 .. level_-2]` —
  a loop, not a special case. **This removes the `wasLevelTwo` branch entirely**, which is the
  §8/§5 win: one mechanism that works at any depth instead of a hardcoded two-level special case.
- Raising or lowering the maximum thereafter is **one constant**, which is the property the
  operator is asking for.

**Do this refactor even if the maximum stays at 2** — it is a strict improvement and it is the
prerequisite that makes the change trivial.

### The real open question: CAPACITY

Level-3 randomize fan-out is **15 + 15² + 15³ = 3615 depth parameters for ONE focused parameter**
(vs 240 today). Worse, the recorded behaviour is that `PressEncoder`'s view-open **eagerly
materializes ALL of a depth's connected sub-depths**, not only the ones randomize selects — so
3615 is materialization, not selection. Existing sizing commentary (`FroggersParameters.hpp:215-224`)
budgets "up to 915 level-1 plus more at level 2" and notes depth storage rides
`RequestParameterStorageBatch`. **3615 per focused parameter is a different order of magnitude and
is very likely to hit `CanAllocate() == false`** — the same silent-exhaustion path already
suspected in F4.

**THE FIX IS THE SAME ONE AS F4a, AND IT IS NOT SPECULATIVE — VERIFIED BY READING THE CODE.**

`detail::RandomizeParameterModulationDepths` **does not need the modulation view open.** It takes
the `Parameter&` directly, reads eligibility from `group.GetModulators().Metadata()`, and calls
`parameter.EnsureModulationDepth(eligible[i])` itself — materializing **on demand, exactly the
sources it selects.** Nothing in it reads view state.

So the `drillIn.PressEncoder(modIx)` → randomize → `drillIn.Back()` round trip in Step 3 of
`RandomizeAll`'s level-1 branch accomplishes **nothing except** (a) triggering
`Bank::OpenModulationView`'s eager materialization of ALL 15 connected sub-depths when ~2 are
wanted, and (b) driving the level counter up and down, which is what ejects the operator (F4a).

**Delete the press/Back round trip in Step 3.** One edit fixes:
- **F4a** — no level churn, so no ejection; the operator stays in their view.
- **Allocation** — materialization drops from 15 per depth to ~2, i.e. the level-2 cost falls from
  15 + 15×15 = 240 to roughly 15 + 15×2 ≈ 45 for a focused parameter.
- **Level 3 feasibility** — sparse fan-out is ~15 + 30 + 60 ≈ 105 instead of 3615. **This is the
  difference between level 3 being impossible and being cheap.**

Step 2's `Back()`/`PressEncoder` round trip (used only to locate the encoder id in the parameter
grid) should be removed on the same grounds — find the id without leaving the view, or cache it.

**Sequence: delete the round trips first, re-measure allocation, then land the constant + array
refactor, then raise the maximum to 3.** The capacity objection to level 3 very likely disappears
entirely once the fan-out stops materializing 15 to use 2.

**Also note F1's ruling applies here:** the mode-2 distribution must govern level-3 draws too —
*"the same median/mode skew ... should apply in all cases."*
