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
- **A test whose property is "X did not happen" MUST assert that the conditions for X were
  present.** Otherwise it passes on silence, on a patch that never applied, on a code path never
  reached. B7.5's first version asserted "the master limiter never engaged" with no liveness
  check and passed at `minEnvelopeSeen` exactly 1.0 while proving nothing. `PeakAbs()`
  exists in `FroggersAudioRoutingTests.cpp` for precisely this; use it. **This is the single most
  common shape of the green-while-wrong guards on this project's record.**
- **The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`.**
- **Nothing goes to the operator until the whole list lands** (M4). One build, one listening pass.

## Execution order

| # | Task | Why here |
|---|---|---|
| — | ~~**B7.5.0** patch-application anomaly~~ | **CLOSED 2026-08-06, no defect.** Establishes the settling rule every test below depends on: a `SceneCenter` write is ~81 % applied after one block, converged after ~30 |
| 1 | **B7.5** end-to-end failing test | M3. Must be RED before any fix. The only acceptance criterion that matters |
| 2 | **F0** preflight remediation | Mechanical; F0.3 is a hard prerequisite for F1 |
| 3 | **F3** Stop does not stop | Highest severity, fully traced |
| 4 | **F4 + F5** drilldown | One shared edit; highest value per line |
| 5 | **F1** randomize distribution | Needs F0.3 |
| 6 | **F2** B7.1 ceiling retarget | Turns B7.5 green. Last, because it is the one B7.5 exists to judge |
| 7 | **F6** operator verification | Single pass on the complete build |

---

## B7.5.0 — Patch-application anomaly. ✅ CLOSED 2026-08-06, no defect.

- [x] Investigated and closed. **The test idiom is sound.** Read this section for the mechanism —
      it governs how every test in this plan sets up a patch — but there is no work left here.

**What was suspected:** the predecessor recorded, and never investigated, that a 4096-block limiter
diagnostic returned **bit-identical** `first_engage_block` (133), `min_envelope` (0.976694) and
`peak_output` (0.998264) for two structurally different patches — P2 delay-driven (Delay slot 2
Feedback = 1.0, slot 1 Send = 1.0) and P4 drive-only (Drive slot 0 = 1.0). The fear was that
`model.PageParameter(bank, slot).SceneCenter(0) = value` never reaches the DSP, which would have
made all 76 sites using that idiom green-while-wrong — and B7.5 worthless.

**It does reach the DSP. The write is smoothed, not inert.**

**Measured** (diagnostic subagent, written value `1.0f` in every case, one block run):

| Parameter | `ComputeAllParameters()`? | read back |
|---|---|---|
| `Drive, 0` | no | **0.80984** |
| `Delay, 2` | no | **0.80984** |
| `Drive, 0` | yes | **1.0** |
| `Delay, 2` | yes | **1.0** |

**The subagent's numbers are correct and its verdict ("IDIOM BROKEN", 76 invalid sites) is wrong.**
Verified by the lead reading Sheaf directly (M1/M2 — a subagent report is a lead, not a fact):

`Parameter::ProcessSamplePhase1` (`External/Sheaf/projects/synth/src/ParameterModulation.cpp:1486-1491`)
**does** advance `currentCenter_` from a `SceneCenter` write, on a periodic interval:

```cpp
if (sampleIndex % group_.Config().targetComputeIntervalSamples == 0) {
    Compute(group_.Manager().Scene());
}
```

That call takes the **smoothed** branch of `ComputeAtDepth` (`:2205-2209`),
`targetCenter_ += alpha*(rawCenter - targetCenter_)`, with
`kDefaultTargetCenterAlpha = 0.0994231307` and `kDefaultTargetComputeIntervalSamples = 16`
(`include/synth/ParameterModulation.hpp:171-172` — "about 50 Hz at 48 kHz").

**The arithmetic confirms it exactly.** One block is 16 computes:
`1 − (1 − 0.0994)^16 = 0.8127`, and `ProcessLitePhase1`'s second-stage `processLiteAlpha`
smoothing pulls that to the measured **0.80984**. The value is a one-pole 81 % of the way to the
commanded 1.0 — the signature of convergence, not of a write that never landed.

`ComputeAllParameters()` reads 1.0 because it passes `smoothTargetCenter = false`, taking the
`else` branch (`targetCenter_ = rawCenter`) — exact convergence in one call.

**The two parameters reading an identical 0.80984 is NOT "coincidence of the default patch"**
(the subagent's explanation, and it is wrong — `ApplyFroggersDefaultPatch` sets Drive slot 0 to
0.2, not 0.80984). It is the deterministic result of the same one-pole taking the same number of
steps toward the same target from the same starting state.

**This also explains the original W2.2-ANOMALY properly, and it is not a defect.** P2 and P4
produced bit-identical `first_engage_block` / `min_envelope` / `peak_output` because both were
sampled at the same early point on the same smoothing trajectory toward the same target of 1.0.
Nothing failed to reach the DSP. P1 vs P3 differed (101 vs 43) for the same reason it should —
different targets converge differently once enough blocks elapse.

**The settling rule this establishes — every task below depends on it:**

**A `SceneCenter` write reaches ~81 % of its target after one block and is effectively converged
after roughly 30.** So:

1. **The `SceneCenter(0) =` sites (76 when counted 2026-08-06; 85 today and still growing as tests are added) are NOT invalid.** Do not "fix" them. A test that runs
   hundreds of blocks (B7.5 runs 256, the existing limiter test runs 256) is fully converged long
   before it asserts.
2. **Any test that asserts within roughly the first 30 blocks is reading a partially-applied
   patch.** That is the real, narrow hazard — swept in **F0.5**, not by touching all 76.
3. **New tests call `ComputeAllParameters()` after their writes** — not because the idiom is
   broken, but so the patch is exact from block 0 and the test measures what it declares rather
   than a ramp into it. One shared helper, per F0.5.

**Nothing here blocks B7.5.** Proceed.

---

## B7.5 — The end-to-end acceptance test. It MUST fail.

**The property:** the master limiter's `envelope` stays at unity across a hostile patch — all
maxima, modulation live, transport running, the operator's real repro. This is the only end-to-end
proof that the per-stage headroom architecture works in the binary rather than on paper.

**Where:** `app/FroggersAudioRoutingTests.cpp`. The accessor already exists —
`rig.Application().TestOutputLimiter()` returns a live reference and `envelope` is a public field,
already read per-block at `:567,587`.

- [ ] **Step 1: Split the existing test. RULING TAKEN 2026-08-06 — implement it, do not re-decide.**

      `limiter_engages_on_overdriven_patch_and_stays_bounded`
      (`app/FroggersAudioRoutingTests.cpp:555-599`) bundles **two different properties**:
      1. *the master limiter functions* — it is wired in and does real work, not a silent no-op;
      2. *the chain gets hot enough to need it* — `REQUIRE_TRUE(minEnvelopeSeen < 0.999f)`.

      **Property 2 IS F2's symptom.** That assertion says the master sits in continuous gain
      reduction on an ordinary patch, which is precisely what B7.1 exists to remove. **The test
      pins the broken behaviour**, so it is supposed to fail when the bug is fixed. Property 1 is
      real and worth keeping; the test's NaN / finite / `≤ 1.0` assertions are independent and
      valid regardless.

      **Do all three of these:**
      - **Re-home property 1 to a direct unit test of `dsp::OutputLimiter`.** Feed it a signal
        above threshold and assert `envelope` drops below unity. Precedent exists in this same
        file — two tests already drive `TestOutputLimiter()` directly, "bypassing that hard bound
        entirely" (`:450`, `:500`). This makes the property permanently immune to gain-staging
        changes, which is why it belongs there rather than in a chain test.
      - **Strip the chain-level `REQUIRE_TRUE(minEnvelopeSeen < 0.999f)`**, and with it the
        now-unused `minEnvelopeSeen` accumulation. Leave a comment recording that it pinned the
        defect and was removed by B7.1's ruling, not by convenience.
      - **RENAME the stripped test** to describe what it now checks — it keeps its hostile patch
        (comb feedback max, comb/peak all-comb, reverb Hold max, fully wet, Drive max) and its
        boundedness assertions, so `overdriven_patch_stays_bounded` or similar. **A test whose
        name claims a property it no longer checks is its own trap** — that is exactly the class
        of defect this change is repairing.

      **Do NOT** retarget it to a "deliberately master-defeating" patch to preserve the engagement
      assertion. Whether ANY patch can still engage the master after B7.1 is an open empirical
      question, answered at **F2.4** by measurement, not predicted here. Two outcomes are both
      acceptable there: something can (pin it), or nothing can (record that the master is
      reachable only by fault, which `RecoverIfNonFinite` already covers, and write no test).

      **After this step the suite must still be fully green.** The split changes which layer each
      property is asserted at; it does not change any behaviour.

- [ ] **Step 2: Write the failing test — and the `ApplyPatchNow` helper it is the first consumer
      of.** Use the operator's real repro, not a synthetic sweep (M3): Filter bank Crispy at max,
      modulation live, transport running.

      Add the helper once, beside the `Rig` type in `app/FroggersAudioRoutingTests.cpp`, so the
      "apply the patch exactly" idiom has a single definition site (OMNI §8) rather than a
      `ComputeAllParameters()` call copied into every future test. F0.5 sweeps existing tests onto
      it later; **it is defined here, once.**

```cpp
// B7.5.0: SceneCenter writes are applied through a SMOOTHED periodic Compute
// (Parameter::ProcessSamplePhase1, alpha 0.0994 every 16 samples), so a patch
// is only ~81% applied one block after it is written. ComputeAllParameters()
// passes smoothTargetCenter=false and therefore converges exactly, in one
// call. Call this after the SceneCenter writes and before the first
// RunBlocks() whenever a test asserts on a patch rather than on a ramp.
inline void ApplyPatchNow(Rig& rig) {
    rig.Application().TestParameterManager().ComputeAllParameters();
}
```

      **Route VERIFIED 2026-08-06 and already landed.** `context_` is private in `FroggersAppCore`
      with no public accessor, so the lead's original `rig.Application().Context().parameterManager`
      snippet did not compile. The narrow test-only accessor `TestParameterManager()` was added
      beside `TestOutputLimiter()`, matching its precedent exactly. Both the accessor and
      `ApplyPatchNow` are in the tree — do not re-add them.

> ### ⚠ SPEC CORRECTED 2026-08-06 — the first version of this test was VACUOUS
>
> The original spec asserted only `!SawNaN`, `RequireFiniteStereo`, and
> `minEnvelopeSeen > 0.999f`. **All three pass on silence.** A test asserting *"the limiter never
> engages"* is trivially satisfied by an instrument that makes no sound at all — a green-while-wrong
> test of exactly the class this change exists to eliminate, written into the plan by the lead.
> It passed on first run with `minEnvelopeSeen` **exactly 1.0**.
>
> Two defects, both now fixed below:
> 1. **No liveness assertion.** This file already carries the antidote — `PeakAbs`, whose own
>    comment says it exists to "assert some output sample exceeds a small epsilon in magnitude,
>    which plain finiteness does not require." Use it. **Any test whose property is "X did not
>    happen" needs a companion assertion that the conditions for X were actually present.**
> 2. **The patch was not hostile.** Crispy + Drive alone, with every other parameter at default,
>    is not the operator's repro. The patch below is the one the pre-existing chain test measured
>    engaging the master at block 101 (min envelope 0.809) — comb feedback max, comb/peak all-comb,
>    reverb Hold max, fully wet, Drive max — **plus** Filter Crispy at max on top. That patch is
>    known-hostile by measurement, not by assumption.
>
> Also corrected: `PageParameter(Filter, 14)` **throws `std::out_of_range`**. `pageParameters_` is
> only 9 wide (`FroggersParameters.hpp:407-408`); Crispy lives in a separate `crispy_` array
> reached via `Crispy(bankId)` (`:413-414`). Found by the implementer, verified by the lead.

```cpp
// B7.5 (proposal.md, "the acceptance criterion that governs everything"): the
// master limiter is the BACKSTOP, not the gain-staging mechanism. With every
// stage bounded to C, a hostile patch must not engage it at all. This test is
// the only end-to-end proof of that; every other limiter test in this file
// measures one stage under synthetic input (M3).
TEST_CASE(master_limiter_stays_at_unity_across_hostile_patch) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("b7_5_hostile"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    // The SAME hostile patch `overdriven_patch_stays_bounded` uses -- measured
    // to engage the master at block 101, min envelope 0.809, so it is
    // known-hostile by measurement rather than by assumption.
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +0.95
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 1.0f;  // Comb/Peak -> all comb
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> ceiling
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // fully wet
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;   // maximum Drive
    // PLUS the operator's stated repro on top: Filter bank Crispy at max
    // scrambles all 8 bits of every Filter parameter per read. NOTE the
    // accessor -- Crispy is NOT in pageParameters_ (9 wide); it lives in its
    // own `crispy_` array (FroggersParameters.hpp:413-414), and
    // PageParameter(Filter, 14) throws std::out_of_range.
    model.Crispy(synth_froggers::FroggersBankId::Filter).SceneCenter(0) = 1.0f;
    // B7.5.0: a SceneCenter write is only ~81% applied after one block
    // (Parameter::ProcessSamplePhase1's periodic smoothed Compute, alpha
    // 0.0994 every 16 samples). This test must measure the patch it declares,
    // not a ramp into it, so apply it exactly before the first block.
    ApplyPatchNow(rig);

    rig.StartAt(0);
    auto& limiter = rig.Application().TestOutputLimiter();

    float minEnvelopeSeen = 1.0f;
    for (int block = 0; block < 256; ++block) {
        rig.RunBlocks(1);
        minEnvelopeSeen = std::min(minEnvelopeSeen, limiter.envelope);
    }

    REQUIRE_TRUE(!rig.SawNaN());
    RequireFiniteStereo(rig.Output());
    // LIVENESS -- without this the assertion below passes on silence, which is
    // how the first version of this test passed while proving nothing. The
    // instrument must actually be sounding for "the master never engaged" to
    // mean anything.
    REQUIRE_TRUE(PeakAbs(rig.Output()) > 0.1f);
    // The property. Unity means the master never had to do anything.
    REQUIRE_TRUE(minEnvelopeSeen > 0.999f);
}
```

- [ ] **Step 6 — B7.5 IS NOT COMPLETE WITHOUT LIVE MODULATION. Added 2026-08-06.**

      **Status: Steps 1-5 landed (`6195a41`). The test is red at `minEnvelopeSeen = 0.985796`,
      `PeakAbs = 0.991599`. But it exercises STATIC knobs only.**

      This change's own acceptance criterion, in `proposal.md` and `SUPERSESSION-RECORD.md`, is
      *"all maxima, **modulation live**, transport running, the operator's real Crispy repro."*
      The landed test has maxima, Crispy and transport. **It has no modulation at all** — no
      randomize, no depths set. That is the one ingredient most likely to produce the reported
      symptom, and it is missing from the gate that judges whether the symptom is fixed.

      **Why this is not "iterating the test until it is redder"** — a trap this plan explicitly
      forbids. Nothing here tunes a threshold or a block count to force an outcome. It implements
      a clause of the spec that was written down and then omitted. The distinction matters: the
      first is fishing, the second is finishing.

      **Why it is load-bearing:**
      - §K.1 measured `DriveBlendPhase` at **1.002** under free random phase but **4.15** under
        full-bank per-sample-random modulation and **50.5** under periodic phase/content
        coincidence. Its allpass coefficient is read fresh every sample from the Phase knob. **The
        single largest known blowout path in the instrument only appears under modulation** — and
        the current test does not exercise it at all.
      - `0.985796` is about **0.12 dB** of gain reduction. That is inaudible, and it does not
        match the operator's report of audible pumping. A gate that goes green on 0.12 dB while
        the real mechanism is untested would manufacture exactly the false confidence this change
        exists to destroy.
      - **An incomplete acceptance gate is worse than no gate.** If B7.5 passes after F2 while
        modulation-driven blowout persists, we ship a green suite and a broken instrument — the
        predecessor's failure, reproduced with more ceremony.

      **Add a SECOND end-to-end test, `master_limiter_stays_at_unity_under_live_modulation`**,
      alongside the existing one rather than replacing it. They discriminate different failures:
      static gain staging vs. modulation-driven transients. Same hostile patch, same liveness
      assertion, same unity assertion, plus deep audio-rate modulation on the two parameters the
      evidence names:
      - **Drive slot 8 (Phase)** — §K.1's 50× path.
      - **Filter slot 5 (Comb feedback)** — W2.2a's trim smoother was tuned against `rand()`
        sweeps, never against real modulation.

      > #### ⚠ SOURCE CORRECTED 2026-08-06 — use `kModSlotVco1Audio`, NOT `kModSlotNoise`
      >
      > The first version of this step specified `kModSlotNoise` while citing §K.1's **50.5×**
      > figure. **Those do not go together, and the implementer was right to stop.**
      >
      > §K.1 measured three distinct regimes: **1.002** under free random phase, **4.15** under
      > full-bank per-sample-random modulation, and **50.5** under *"periodic phase/content
      > coincidence — e.g. an LFO locked near the note's own period."* `kModSlotNoise` is
      > `random_.UniformOpen01()` per sample — it can only ever reach the first regime. Citing the
      > periodic number to justify a random source was a lead error, not a routing error.
      >
      > **Measured proof it was wrong**, from the noise-modulated attempt:
      >
      > | | minEnvelopeSeen | PeakAbs |
      > |---|---|---|
      > | static hostile patch | 0.985796 | 0.991599 |
      > | + noise modulation | 0.985726 | 0.991473 |
      >
      > Identical to seven decimal places of relative difference. Random modulation of the allpass
      > coefficient averages out; that is a real end-to-end confirmation of §K.1's own 1.002
      > free-random figure, not a failure.
      >
      > **The right source is the VCO's own audio output.** `vco1AudioSource_ =
      > NormalizeBipolarToUnit(vco1Raw)` (`FroggersModulation.hpp:384`) is registered as
      > `kModSlotVco1Audio` (slot 6, `connected = true` at `:535-536`). It is periodic at the note
      > frequency and **locked to the note's period by construction — it IS the note**, which is a
      > stronger form of §K.1's "LFO locked near the note's own period" than an LFO could be. Its
      > coincidence with the content passing through `DriveBlendPhase` is exact rather than
      > approximate.
      >
      > Use `kModSlotVco1Audio` on Drive slot 8. Keep `kModSlotNoise` nowhere in this test.

```cpp
// UNVERIFIED ROUTE (M1) -- read FroggersModulation.hpp and confirm before
// writing. `EnsureModulationDepth` returns nullptr at storage capacity, so
// check it. Depth centers are BIPOLAR: 0.5 is zero, 1.0 is full positive.
synth::Parameter* phaseDepth =
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 8)
         .EnsureModulationDepth(synth_froggers::kModSlotNoise);
REQUIRE_TRUE(phaseDepth != nullptr);
phaseDepth->SceneCenter(0) = 1.0f;
```

      **Report both numbers for the new test too.** Expected: red, with a LARGER gain reduction
      than the static test's 0.0142. **If the modulated test is no worse than the static one,
      stop and report** — that would contradict §K.1's measurement and means the modulation is
      not reaching `DriveBlendPhase`, which is its own defect.

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

- [x] **F0.1 — Hoist the three remaining short-circuit sites (OMNI §1, §8).**
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

- [x] **F0.2 — Two files named `*Tests.cpp` run zero tests.**
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

- [x] **F0.3 — Retract the stale distribution directive (OMNI §8, M5). PREREQUISITE FOR F1.**
      `app/FroggersModulation.hpp:922-925` currently reads:
      *"Do NOT tune for the resulting mean (~3.1); A6 records that as a consequence, not a target,
      and rules out reshaping toward it. Do not break the deliberate 30/30 tie between n=2 and n=3
      toward a single peak."*
      The operator's current ruling is **mode 2**, which requires exactly breaking that tie toward
      a single peak. Replace the comment with the mode-2 rationale and a pointer to this change.
      **Do this before F1 dispatches**, or F1's implementer hits a forceful in-code prohibition
      against the thing they were told to build. This is M5 recurring inside the code.

- [x] **F0.4 — Retract the superseded premise in `CapacityExhausted`.**
      `app/FroggersModulation.hpp:790-792` reasons from *"design D14's own bias table
      (P(k)=0.5^(k+1), mean 1.0)"* — that is Sheaf's **replaced** geometric loop, not the app-side
      distribution in force since E.1. The conclusion (most depths untouched is healthy) still
      holds; restate it from the actual current distribution.

- [x] **F0.5 — Patch settling: sweep for the narrow hazard.**
      From B7.5.0: a `SceneCenter` write is ~81 % applied after one block and converged after ~30,
      because `Parameter::ProcessSamplePhase1` advances it through a smoothed `Compute()`
      (alpha 0.0994, every 16 samples). Long-running tests are fine; short ones are not.
  - **`ApplyPatchNow` already exists** — B7.5 Step 2 adds it, since B7.5 is its first consumer.
    Do not define a second one (OMNI §8).
  - **Sweep, do not mass-edit:** find tests that assert on patch-dependent behaviour within
    roughly the first 30 blocks (`grep -n "RunBlocks" app/*Tests.cpp`, cross-referenced against the
    the 76 `SceneCenter(0) =` sites). Fix only those, with the helper. **Report the count found
    and the count changed.** Most of the 76 are expected to need nothing.
  - A test that is *deliberately* measuring the ramp (if any exists) keeps its current behaviour;
    say so in the report rather than converting it.

- [x] **F0.6: Run the suite, confirm ten binaries green, commit.**

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

- [ ] **F2.0 — BLOCKING: B7.1 is not yet justified by measurement. Added 2026-08-06.**

      B7.5 is red and will discriminate B7.1 — that part works. **But the numbers say B7.1 alone
      probably will not fix what the operator hears, and F2 must not be built on the assumption
      that it will** (§0: no fix before the recorded root cause).

      | patch | minEnvelopeSeen | PeakAbs |
      |---|---|---|
      | static hostile | 0.985796 | 0.991599 |
      | + noise modulation | 0.985726 | 0.991473 |
      | + VCO1-audio modulation (periodic, note-locked, full depth, on Drive Phase) | 0.985954 | 0.991505 |

      **Three regimes within 0.03 % of each other.** The master's gain reduction on the most
      hostile patch available is about **0.12 dB** — inaudible. W2.0 diagnosed the symptom as the
      master ducking the whole mix; 0.12 dB cannot be that. Removing it cannot produce an audible
      improvement.

      **Two gaps to close before F2.1, both measurements, neither a fix:**

  - **F2.0a — measure the WRONG QUANTITY problem.** `minEnvelopeSeen` is a minimum. It cannot
    distinguish "dipped once for one block" from "rode at 0.9858 for all 256." **Pumping is
    audible *variation* in gain, not a low minimum** — a constant 0.12 dB reduction is inaudible,
    while a gain swinging 1.0 ↔ 0.85 at a few Hz is very audible. Report the envelope's **duty
    cycle** (fraction of blocks below 0.999) and its **min-to-max range**, not just its minimum.
    If the envelope is essentially constant, the master is not pumping and F2's root cause is
    elsewhere entirely.
  - **F2.0b — measure the operator's ACTUAL condition, which is post-Randomize-All.** Every test
    above hand-sets 5 parameters and modulates 2. After Randomize All the operator has **dozens**
    of parameters carrying modulation depths across all six banks — a categorically different
    state, and the one they were in when they reported the symptom. Drive the rig with
    `RequestRandomizeAll()`, let `ProcessFrame` drain it, then Crispy at max, then measure duty
    cycle and range as in F2.0a. **This is the closest thing to the real repro that exists in the
    harness.**

      **Report numbers, change nothing.** Then the lead decides whether B7.1 is still F2's fix, or
      whether §K.1's 50.5× — which does not surface end-to-end — means the archived per-stage
      evidence describes isolated harnesses rather than the instrument, exactly the systemic error
      §1 of the failure report identifies.

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

## Deferred design — bank expansion to full 16-slot occupancy

`BANK-EXPANSION-DESIGN.md` in this directory. **Deferred: nothing in it is built by this change.**
All six banks carry 9 named parameters + 5 empty + Crispy + Crunchy today; the document proposes
the 5 missing slots per bank (30 total), each with a continuous reading, a cost tier, and
`file:line` evidence for every Tier-1 claim. It supersedes the archived §J research, which it
corrects in five places.

**Three findings from it that are relevant to code, not just to future design:**
- **Delay's `Color` and `Halo` are not independent DSP.** `app/dsp/Delay.hpp:433-434` averages
  them into Detune and Mod depth (`ddet = 0.5*(ddet + Color)`, `dmod = 0.5*(dmod + Halo)`), a
  ported-firmware fold. Two shipped parameter names promise something the code does not do, and
  the Delay bank has 7 distinct controls rather than 9.
- **The ASR envelopes are not modulation sources.** No envelope output appears in
  `FroggersModulatorSlot` (`app/FroggersModulation.hpp:156-171`); the "VCO EF" entries are
  amplitude followers, a different signal. Open question 8 in the design document.
- Five §K items the archive lists as "in flight" have already landed.

## Deferred, carried untouched

§H mobile-web UI layer · §I VST layer · §J bank parameter expansion · D.4 publish pipeline ·
W4 second Sheaf pin bump `77a3019e` → `508d9d68` · W4.2 `kExternalAudioOptedIn` removal ·
G.2 blank-window-on-startup-failure decision · B7.3 filter composite · B7.4 per-stage guards.

Full derivations for all of these are in
`../archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md`. They are deliberately **not**
copied forward — carrying ~250 lines of deferred research would cost every future agent context for
no scoped benefit (OMNI §16).
