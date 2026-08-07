# Tasks — `frogg3rs-blowout-and-drilldown-repair`

> **For agentic workers:** REQUIRED SUB-SKILL — use `superpowers:subagent-driven-development` to
> implement this plan task by task. **Read `proposal.md`'s "Method constraints" (M1–M7) before the
> first dispatch.** They are the specific ways the predecessor failed while measuring everything
> green. Steps use `- [ ]` for tracking.

**Goal:** Make the instrument stop when Stop is pressed, stop blowing out at Filter Crispy max,
stop ejecting the operator out of drilldowns, randomize with mode-2 density at every level, and
drill three levels deep.

**Architecture:** One end-to-end failing test defines "fixed" before any fix lands. Then five
repairs, each traced to a specific line before it is written. The per-stage headroom architecture
already exists in the binary; the one constant that ties it together (`C = 0.80`) was specified and
never built, and that is F2's entire fix.

**Tech stack:** C++20 header-only app tree (`app/`), out-of-tree Sheaf app, `nice make -j2`,
ten test binaries.

## §0 Standing constraints (binding on every task)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch (OMNI §4, §15).
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **Build/test runs go through a subagent** — counts and failure tails only, never raw logs
  (OMNI §16.1). Count that all ten binaries ran; `make test` has no `-k`.
- **`External/Sheaf` pinned at `77a3019e` and clean.** We do not patch Sheaf; needs go to
  `/UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **No unrequested user-visible behaviour. Propose first.**
- **An implementer may not close a task whose spec requires operator eyes or ears.**
- **Systematic debugging is binding for F1–F3:** no fix before the recorded root cause.
- **The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`.**
- **Nothing goes to the operator until the whole list lands** (M4). One build, one listening pass.

## Execution order

| # | Task | Why here |
|---|---|---|
| 1 | **B7.5.0** patch-application anomaly | Every test below is invalid if the test idiom does not reach the DSP |
| 2 | **B7.5** end-to-end failing test | M3. Must be RED before any fix. The only acceptance criterion that matters |
| 3 | **F0** preflight remediation | Mechanical; F0.3 is a hard prerequisite for F1 |
| 4 | **F3** Stop does not stop | Highest severity, fully traced |
| 5 | **F4 + F5** drilldown | One shared edit; highest value per line |
| 6 | **F1** randomize distribution | Needs F0.3 |
| 7 | **F2** B7.1 ceiling retarget | Turns B7.5 green. Last, because it is the one B7.5 exists to judge |
| 8 | **F6** operator verification | Single pass on the complete build |

---

## B7.5.0 — Resolve the patch-application anomaly (BLOCKING)

The predecessor recorded this and never investigated it. **If it is real, every test in the suite
that sets up a patch is asserting against a patch it never applied**, and B7.5 would be worthless.

**The evidence:** a 4096-block limiter diagnostic returned **bit-identical** `first_engage_block`
(133), `min_envelope` (0.976694) and `peak_output` (0.998264) for two structurally different
patches — P2 delay-driven (Delay slot 2 Feedback = 1.0, slot 1 Send = 1.0) and P4 drive-only
(Drive slot 0 = 1.0). Slot indices were verified correct against `FroggersBankLayouts()`. Two
different patches producing byte-identical trajectories means at least one set of writes is not
reaching the DSP. P1 vs P3 DID differ (101 vs 43), so the failure is **partial**, which is worse —
it looks like it works.

**The mechanism to check first:** `model.PageParameter(bank, slot).SceneCenter(0) = value` writes
the *commanded* value. W1.0 established that commanded values do not reach the *display* without a
`ComputeAllParameters()` pass. The open question is whether they reach the **DSP** without one —
`knob()` reads `currentKnobValues_`, refreshed by `ProcessLitePhase1` per sample, but only for
parameters registered in `topLevelParameters_`.

- [ ] **Step 1: Print, do not reason** (M1/M7). In a scratch harness, set
      `model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f`, run one block, and
      print the value `knob(FroggersBankId::Drive, 0)` actually returns inside `RouteAudioSample`.
      Repeat with `Delay` slot 2. Report the two numbers.
- [ ] **Step 2:** Repeat both with a `ComputeAllParameters()` call inserted after the writes and
      before the first block. Report the same two numbers.
- [ ] **Step 3: Decide from the four numbers, not from a paragraph.**
      - Values match the write in both cases → the idiom is sound; the anomaly is elsewhere
        (re-run the original P2/P4 diagnostic and find it before proceeding).
      - Values only match after `ComputeAllParameters()` → **the idiom is broken.** Every affected
        test is green-while-wrong. Record which tests use it (`grep -rn "SceneCenter(0) =" app/`)
        and fix the idiom once, in one shared test helper, not per test.
- [ ] **Step 4: Commit** the finding into this file under this task before writing any test.

**Do not fix anything under this task beyond the test idiom itself.** Report first.

---

## B7.5 — The end-to-end acceptance test. It MUST fail.

**The property:** the master limiter's `envelope` stays at unity across a hostile patch — all
maxima, modulation live, transport running, the operator's real repro. This is the only end-to-end
proof that the per-stage headroom architecture works in the binary rather than on paper.

**Where:** `app/FroggersAudioRoutingTests.cpp`. The accessor already exists —
`rig.Application().TestOutputLimiter()` returns a live reference and `envelope` is a public field,
already read per-block at `:567,587`.

- [ ] **Step 1: Resolve the collision with the existing test — decide BEFORE writing.**
      `limiter_engages_on_overdriven_patch_and_stays_bounded` (`:555-599`) ends on
      `REQUIRE_TRUE(minEnvelopeSeen < 0.999f)` — *"gain reduction genuinely engaged, not a
      no-op."* **That is the exact opposite of B7.5's property on a similar patch.** Both cannot
      hold once B7.1 lands. Decide now and record the decision here:
      the existing test's real subject is *the master still works as a backstop*, so retarget it to
      a patch that defeats the per-stage ceilings deliberately (e.g. reverb Hold at max, which
      `proposal.md` records as parked just under self-oscillation by design), and leave B7.5 to own
      the ordinary-hostile-patch case. **Do not delete it, and do not discover this collision when
      the suite goes red after F2.**

- [ ] **Step 2: Write the failing test.** Use the operator's real repro, not a synthetic sweep
      (M3): Filter bank Crispy at max, modulation live, transport running.

```cpp
// B7.5 (proposal.md, "the acceptance criterion that governs everything"): the
// master limiter is the BACKSTOP, not the gain-staging mechanism. With every
// stage bounded to C, a hostile patch must not engage it at all. This test is
// the only end-to-end proof of that; every other limiter test in this file
// measures one stage under synthetic input (M3).
TEST_CASE(master_limiter_stays_at_unity_across_hostile_patch) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("b7_5_hostile"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    // The operator's actual repro: Filter bank Crispy at max scrambles all 8
    // bits of every Filter parameter per read, so this reaches the comb
    // feedback / LP maxima continuously rather than as an edge case.
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 14).SceneCenter(0) = 1.0f;
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;
    // (B7.5.0's ruling governs whether a ComputeAllParameters() pass is
    // required here for these writes to reach the DSP. Follow it.)

    rig.StartAt(0);
    auto& limiter = rig.Application().TestOutputLimiter();

    float minEnvelopeSeen = 1.0f;
    for (int block = 0; block < 256; ++block) {
        rig.RunBlocks(1);
        minEnvelopeSeen = std::min(minEnvelopeSeen, limiter.envelope);
    }

    REQUIRE_TRUE(!rig.SawNaN());
    RequireFiniteStereo(rig.Output());
    // The property. Unity means the master never had to do anything.
    REQUIRE_TRUE(minEnvelopeSeen > 0.999f);
}
```

- [ ] **Step 3: Run it and confirm it FAILS**, and record the actual `minEnvelopeSeen` in this
      file. A number, not a paragraph (M7).

```bash
cd app && nice make -j2 test 2>&1 | tail -30
```

Expected: FAIL on `master_limiter_stays_at_unity_across_hostile_patch`, with `minEnvelopeSeen`
well below 0.999 — the proposal's trace predicts continuous engagement.

- [ ] **Step 4:** If it PASSES, **stop and report.** A passing B7.5 before B7.1 means the trace in
      `proposal.md` is wrong and the whole plan needs re-deriving. Do not "fix" it by hardening the
      patch until it fails — that is M3 in reverse.
- [ ] **Step 5: Commit** the red test, marked expected-red in the commit message.

---

## F0 — Preflight remediation (2026-08-06 omni audit)

Five defects in shipped work. All mechanical. F0.3 blocks F1.

- [ ] **F0.1 — Hoist the three remaining short-circuit sites (OMNI §1, §8).**
      The Group A review's F4 finding was fixed at `app/FroggersAppCore.hpp:490-496` only. The
      identical pattern survives at `app/FroggersModulation.hpp:1037`, `:1093`, `:1139`, and two of
      those are **inside loops** — the obvious "cleanup" swap to `partial || Call(...)` would
      short-circuit and skip randomizing every remaining bank or depth. Hoist each call's result
      into a named local first, so evaluation is unconditional and ordering stops being
      load-bearing. Mirror the comment already at `FroggersAppCore.hpp:482-488`.

```cpp
// Before (FroggersModulation.hpp:1093) -- correct ONLY because the call is on the left:
partial = detail::RandomizeBankLevel1Depths(manager, bank) || partial;

// After:
const bool bankPartial = detail::RandomizeBankLevel1Depths(manager, bank);
partial = partial || bankPartial;
```

- [ ] **F0.2 — Two files named `*Tests.cpp` run zero tests.**
      `app/FroggersCrunchyBlowupReproTests.cpp` and `app/FroggersRandomizeAllReproTests.cpp` contain
      zero `TEST_CASE` — they are `int main()` diagnostic harnesses, and neither is in
      `app/Makefile:225`'s `test:` target. The predecessor's W2.0 cites
      `FroggersRandomizeAllReproTests.cpp:107-113` as evidence for why "fixed" passed every test;
      that file never runs. **Rename both to `*Repro.cpp`** (they are legitimate diagnostics, not
      tests — do not force them into the suite) and update every citation to them in this
      directory. Then fix the stale second definition site inside one of them:
      `FroggersCrunchyBlowupReproTests.cpp:212` still hardcodes
      `dsp::ExpMapCompute(1.0f, 10.0f, knob(FroggersBankId::Filter, 2))`, predating
      `dsp::kMaxResonantBumpHeight = 2.0f`. Use the constant.

- [ ] **F0.3 — Retract the stale distribution directive (OMNI §8, M5). PREREQUISITE FOR F1.**
      `app/FroggersModulation.hpp:922-925` currently reads:
      *"Do NOT tune for the resulting mean (~3.1); A6 records that as a consequence, not a target,
      and rules out reshaping toward it. Do not break the deliberate 30/30 tie between n=2 and n=3
      toward a single peak."*
      The operator's current ruling is **mode 2**, which requires exactly breaking that tie toward
      a single peak. Replace the comment with the mode-2 rationale and a pointer to this change.
      **Do this before F1 dispatches**, or F1's implementer hits a forceful in-code prohibition
      against the thing they were told to build. This is M5 recurring inside the code.

- [ ] **F0.4 — Retract the superseded premise in `CapacityExhausted`.**
      `app/FroggersModulation.hpp:790-792` reasons from *"design D14's own bias table
      (P(k)=0.5^(k+1), mean 1.0)"* — that is Sheaf's **replaced** geometric loop, not the app-side
      distribution in force since E.1. The conclusion (most depths untouched is healthy) still
      holds; restate it from the actual current distribution.

- [ ] **F0.5: Run the suite, confirm ten binaries green, commit.**

---

## F3 — Stop does not stop. HIGHEST SEVERITY.

Operator: *"it has now been over a minute since i stopped audio, and it's still coming out. even
though the oscilloscope isn't moving. something is stuck in an infinite loop with extremely harsh
loud noise."*

**The decisive clue:** the scope displays post-gate VCO output. Scope still + audio present ⇒ the
VCOs are silent and something **downstream is self-sustaining with no input.**

### The traced root-cause candidate (2026-08-06 audit)

`FroggersAppCore.hpp:1368` `RecoverPoisonedUnitState` is the **single definition site** for "every
stateful stage" — thirteen units, each with `Reset()` and `StateFinite()`.

The Stop flush (`:656-657`, `:683-684`) clears **`delay_` and `reverb_` only**, and justifies it at
`:623-626`: *"the two feedback structures that self-sustain on their own; VCOs/filters/drive do
not."*

**That justification is false, by this project's own measurements:**
- `filterChain_.comb` is a recirculating delay line with feedback to ±0.95 and a **6.7 s T60** at
  the longest delay (W2.1-MATH's own table). It has `Reset()` (`dsp/FilterFx.hpp:428`).
- `driveBlendPhase_` is a recursive allpass measured at **50.5×** under periodic phase/content
  coincidence (§K.1). It has `Reset()` (`dsp/Drive.hpp:492`).
- Both are **upstream** of the two stages that do get cleared.
- **The flush is one-shot** — `delayReverbClearPending_ = false` once it fires. Anything still
  ringing upstream refills delay and reverb permanently after the single clear.

**Hypothesis 1 (`AllIdle()` never latching) was checked and is unlikely:** `setGate(false)` forces
every voice to `Stage::Release` (`dsp/VoiceEnvelope.hpp:93`), and `releaseStep` is floored above
zero by `kMinTimeSeconds`, so `m_level` decrements monotonically to Idle regardless of modulation.
Chase the flush, not the gate.

- [ ] **F3.1: Print which stage is non-zero after Stop** (M1 — the predecessor never took this
      one print). Instrument each of the thirteen units' output magnitude per block for 60 s after
      Stop, in a scratch harness, on a patch with comb feedback at max and reverb Hold at max.
      Report which are non-zero at t+5 s, t+30 s, t+60 s. **A table of numbers.**
- [ ] **F3.2: Write the failing test.** Post-Stop, from that patch, assert output magnitude is
      below a silence threshold after the ~50 ms stop fade plus a margin. It must FAIL.
- [ ] **F3.3: Fix — single-source the stage enumeration (OMNI §1, §8).** Do **not** add two more
      hand-written `Reset()` calls beside `delay_`/`reverb_`; that reproduces the defect one size
      larger. Extract the unit list `RecoverPoisonedUnitState` already walks into one place with
      two consumers — fault recovery and the Stop flush — so "every stateful stage" has exactly one
      definition site. Correct the false comment at `:623-626` in the same edit.
- [ ] **F3.4:** Confirm F3.2 passes and the full suite stays green. Confirm the reverb tail and
      delay repeats still ring normally **while the transport is running** — the flush must remain
      gated on the Stop edge and `AllIdle()`, not become an unconditional reset (that would cut
      musical tails, which is why the per-unit design exists).
- [ ] **F3.5: Commit.**

---

## F4 + F5 — Drilldown: stop ejecting, then go three deep. ONE SHARED EDIT FIRST.

**Operator rulings, both binding:**
1. **The SCOPE is correct as built** — *"this is desired functionality for randomize all."* Level-1
   Randomize All affects only the drilled parameter. **Do not change it.**
2. **Navigating out is a BUG** — *"randomize all in level 1 shouldn't navigate me out wtf."*

**The traced cause:** `RandomizeAll`'s `Level()==1` branch ends on a bare `drillIn.Back()`
(`app/FroggersModulation.hpp:1152`), which drops level 1 → 0. Step 2 also leaves the view
(`:1112`) purely to look up an encoder id in the parameter grid. The operator ends on the main bank
page because the operation put them there.

**Verified, and it is what makes this cheap:** `detail::RandomizeParameterModulationDepths`
**does not need the modulation view open.** It takes `Parameter&`, reads eligibility from
`group.GetModulators().Metadata()` (`:910`), and calls `EnsureModulationDepth` itself (`:952`).
Nothing in it reads view state. So the `PressEncoder`/`Back()` round trips accomplish nothing
except (a) triggering `Bank::OpenModulationView`'s eager materialization of all 15 connected
sub-depths when ~2 are wanted, and (b) driving the level counter up and down — which is the
ejection.

- [ ] **F4.1: Delete both round trips.** Step 3's `PressEncoder(modIx)` / `Back()` pair
      (`:1132`, `:1150`) and Step 2's `Back()` / `PressEncoder(originalEncoderId)` pair
      (`:1112`, `:1120`). Find the encoder id without leaving the view, or cache it. The whole
      operation must be **visually atomic**: press it, stay put, see the badges change on the page
      you are on.
- [ ] **F4.2: Test** that level is unchanged across a level-1 Randomize All, and that the focused
      parameter's depths carry non-neutral level-2 sub-depths afterward. Also assert
      `LastRandomizePartial()` is false on a fresh patch — silent allocation failure is its own
      defect.
- [ ] **F4.3: Re-measure allocation.** Materialization should fall from 15 per depth to ~2, i.e.
      level-2 cost from 15 + 15×15 = **240** to roughly 15 + 15×2 ≈ **45** per focused parameter.
      Record the measured number.
- [ ] **F4.4: Commit.** F4 may be the whole visible symptom — the randomization was likely working
      all along and simply never visible.

- [ ] **F5.1: Remove the §8 violation blocking level 3.** The cap is **two hardcoded `2`s with no
      named constant**: `PressEncoder`'s `if (level_ >= 2)` (`:676`) and `Back()`'s
      `const bool wasLevelTwo = (level_ == 2)` (`:723`). Same concept, two definition sites, magic
      number both times. Replace with the array design the operator approved:

```cpp
static constexpr std::size_t kMaxDrillLevel = 3;                  // ONE definition site
std::array<synth::PhysicalEncoderId, kMaxDrillLevel> levelEncoders_{};
```

  - `PressEncoder`: cap becomes `if (level_ >= kMaxDrillLevel)`; on a successful descent, record
    `levelEncoders_[level_]` before incrementing.
  - `Back()`: pop one level by `Deselect()` to 0, then re-press `levelEncoders_[0 .. level_-2]` —
    **a loop, not a special case.** This removes the `wasLevelTwo` branch entirely, which is the
    §5/§8 win: one mechanism at any depth instead of a hardcoded two-level special case.

- [ ] **F5.2:** Land the refactor **with `kMaxDrillLevel` still 2** and confirm the suite is green.
      The refactor is a strict improvement independent of the new maximum, and separating the two
      keeps any breakage attributable.
- [ ] **F5.3:** Raise `kMaxDrillLevel` to 3. One constant. Confirm allocation against
      `CanAllocate()` — sparse fan-out should be ~15 + 30 + 60 ≈ **105**, not the 3615 that eager
      materialization would have cost. Record the measured number.
- [ ] **F5.4: Commit.**

---

## F1 — Randomize count distribution: mode 2

**Blocked on F0.3.** Do not dispatch until the contradicting in-code comment is retracted.

**Operator's ruling:** *"mode 2, rarely above 4 is good enough"* — superseding the predecessor's
median-3 spec. **And the same distribution must apply at EVERY level, not just level 0** (explicit).

**Traced:** `app/FroggersModulation.hpp:926-942` implements 10/30/30/20 for counts 1–4, then a
geometric r=0.7 tail. **P(count ≥ 4) = 30%.** Across 16 visible parameters that is ~5 expected at
4+ per press. *"Many parameters with 4+ badges" is what this distribution produces by design* —
"median 3" means half of all draws are 3 or below, not that most are.

**The part that does not fit the spec is 7.** The geometric tail should put P(7) well under 1%.

- [ ] **F1.1: The decisive measurement, before touching the draw** (M1 — the cause was asserted
      wrongly twice and the operator caught it both times). From a genuinely fresh patch, press
      Randomize All once and histogram the per-parameter count three ways: **(a)** the count the
      helper CHOSE, **(b)** the number of depths with non-neutral `SceneCenter`, **(c)** the number
      where `HasNonZeroState()` is true. Compare (a) against 10/30/30/20 + tail.
      - (a) matches spec and (c) == (a) → the implementation is correct and the **spec** is what
        the operator dislikes. Proceed to F1.2 as a spec change.
      - (a) does not match spec → a real bug in the draw. Fix that first, then F1.2.
      - (c) > (b) → the badge criterion over-counts. `Parameter::HasNonZeroState()` is true if a
        depth's OWN sub-depths are non-neutral, and `ZeroExistingModulationDepths` does not recurse.
        Note this only applies if the loaded patch already carried level-2 state; a level-0
        Randomize All creates none.
- [ ] **F1.2: Re-derive the table for mode 2** — 4+ genuinely rare (single digits of percent, not
      30%), 7 essentially never, never zero, still distinct sources. Record the derivation as
      numbers.
- [ ] **F1.3: Pin it with tests at EVERY level.** A histogram test on the level-0 draw, plus a test
      that the **same** distribution governs the level-1 and level-2 draws. Assert on the resulting
      count distribution, not on call counts — the predecessor's E.1 tests pinned the wrong layer
      and that is on record as the sixth green-while-wrong guard.
- [ ] **F1.4: Commit.**

---

## F2 — Filter Crispy at max stops blowing out (B7.1)

**Gated on B7.5 existing and failing.** The fix is `proposal.md`'s traced conclusion: every
per-stage limiter currently ships `ceiling = kSharedCeiling = 1.0`, so every stage may legitimately
deliver more than the master's 0.9 threshold, and the master rides continuously by construction.

- [ ] **F2.1: Retarget every per-stage ceiling to `C = 0.80`.** Measured cost on the default patch
      is **−0.115 dB**. The per-stage `ceiling` constants at `dsp/FilterFx.hpp:188`,
      `dsp/Delay.hpp:106`, `dsp/Reverb.hpp:116` and `dsp/Drive.hpp:464` all currently read
      `kSharedCeiling`; introduce `kStageCeiling = 0.80f` beside it and leave `kSharedCeiling = 1.0f`
      as the **master's** ceiling. Update `dsp/Limiter.hpp:66-70`'s B7.1 note to describe what
      landed.
      **TRAP:** `headroom = ceiling − threshold`. A stage whose threshold equals `C` gives
      `headroom == 0` and `DesiredMagnitude` evaluates `0/0` → NaN for every sample above
      threshold. Each stage's threshold must stay **strictly below** 0.80; the delay and reverb
      thresholds are currently 0.9 and must come down.
- [ ] **F2.2: Make-up gain `1/C` = 1.25× (+1.9 dB)**, applied **AFTER** `outputLimiter_.Process(x)`
      and **BEFORE** the trailing clamp (`FroggersAppCore.hpp:1228` area). Not before the limiter —
      its threshold/headroom math is calibrated in absolute terms, so pre-scaling would silently
      retune every stage's budget. The trailing hard clamp stays; it is the residual-overshoot net.
- [ ] **F2.3: Run B7.5. It must now be GREEN.** If it is not, **stop and report the number** — do
      not start adding per-stage bound tests (M3). Report which stage still exceeds `C` using
      F3.1's instrumentation.
- [ ] **F2.4: Confirm the retargeted `limiter_engages_on_overdriven_patch_and_stays_bounded`**
      (B7.5 Step 1) still passes — the master must remain a working backstop for reverb Hold, which
      is deliberately parked just under self-oscillation.
- [ ] **F2.5: Commit.**

---

## F6 — Operator verification (closes the change)

**One build, one pass, after everything above has landed** (M4). Nothing here is
implementer-closable.

- [ ] **F6.1:** Stop silences the instrument immediately, from a patch with comb feedback and
      reverb Hold at max. **By ear.**
- [ ] **F6.2:** Randomize All inside a level-1 drilldown leaves the operator exactly where they
      were, with badges changing on that page. **Visually.**
- [ ] **F6.3:** Randomize All badge density reads as mode 2, at every level. **Visually.**
- [ ] **F6.4:** Filter Crispy at max no longer blows out; every filter parameter can sit at its
      maximum on a sustained tone without the output limiter audibly pumping — or the specific
      exceptions are recorded as operator-accepted character. **By ear.**
- [ ] **F6.5:** Drilldown reaches level 3 and `Back()` pops one level at a time from any depth.
- [ ] **F6.6:** C.2 walkthrough and G.3 saved-patch load from `~/Library/Sheaf/synth/sheaf-patch/`
      — confirm saved patches still load and are not rewritten by any default change.

---

## Deferred, carried untouched

§H mobile-web UI layer · §I VST layer · §J bank parameter expansion · D.4 publish pipeline ·
W4 second Sheaf pin bump `77a3019e` → `508d9d68` · W4.2 `kExternalAudioOptedIn` removal ·
G.2 blank-window-on-startup-failure decision · B7.3 filter composite · B7.4 per-stage guards.

Full derivations for all of these are in
`../archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md`. They are deliberately **not**
copied forward — carrying ~250 lines of deferred research would cost every future agent context for
no scoped benefit (OMNI §16).
