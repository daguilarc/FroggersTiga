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
- **Cite by SYMBOL, not by line.** Line numbers in a living plan are stale the moment anything
  above them changes, and this change rewrites the same files it cites. Prefer
  `grep -n "drillIn.Back()" app/FroggersModulation.hpp` over `:1152`; where a line number is
  genuinely useful, pin it to a commit ("as of `1c37657`"). **Every citation in the F4 block had
  drifted by 2026-08-07 and two pointed at unrelated code** — the next dispatch would have edited
  the wrong lines. This is the artifact-side form of the omni rule's refined §1 trace clause.
- **The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`.**
- **Nothing goes to the operator until the whole list lands** (M4). One build, one listening pass.

## Execution order

| # | Task | Why here |
|---|---|---|
| — | ~~**B7.5.0** patch-application anomaly~~ | **CLOSED 2026-08-06, no defect.** Establishes the settling rule every test below depends on: a `SceneCenter` write is ~81 % applied after one block, converged after ~30 |
| 1 | **B7.5** end-to-end failing test | M3. Must be RED before any fix. The only acceptance criterion that matters |
| 2 | **F0** preflight remediation | Mechanical; F0.3 is a hard prerequisite for F1 |
| 3 | **F3.3** enumeration fix | ✅ DONE (`1c37657`). **Not a Stop fix** — F3.1 refuted that |
| 4 | **F4 + F5** drilldown | One shared edit; highest value per line. **Citations re-verified — see F8.2** |
| 5 | **F1** randomize distribution | Needs F0.3 (done) |
| 6 | **F3.2c** parametric-oscillation measurement | After F3.3, which can MASK it — see the trap box |
| 7 | **F2.0** then **F2.1+** ceiling retarget | F2.0 measures duty cycle + the real post-randomize condition first |
| 8 | **C1–C3** omni sweep fixes | From F8.1. Mechanical; C1 needs a §12 origin trace before deleting anything |
| 9 | **F7** drilldown level headers | After F4+F5 so level 3 exists to display |
| 10 | **F6** operator verification | Single pass on the complete build |

**SESSION CLOSEOUT (operator directive, 2026-08-07):** once F3.3 lands, run **F8** (omni-rule
structural sweep of `app/` + this change's living artifacts), fix the artifacts from its findings,
and write the handoff. F4+F5, F1, F2.0/F2.1+, F3.2c, F7 and F6 then execute in a NEXT session
against a swept, honest plan. **Frozen trees and the Daisy field app stay segregated — the sweep
never reads or writes `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/` or `External/Sheaf`.**

## F8 — Omni-rule structural sweep + handoff (closes this session, not the change)

**Model exception (operator, 2026-08-07): F8's sweep subagents may run on Opus** — *"this can be
an opus subagent."* Explicit operator override of §0's Sonnet/Haiku rule, scoped to F8 only; every
other dispatch in this change stays under the standing rule.

- [x] **F8.1 — Code sweep** (read-only, parallelizable per §0): all of `app/` (17 files) against
      §5 structure depth (2-of-4), §6 helper rules, §8 repetition (2+ occurrences → loop or
      abstract; duplication is symmetric, find the sibling), §10 condition-check efficiency,
      §12 defensive code (trace origin before guarding), and §1 definition-site enumeration.
      Findings ranked, each with file:line and WHICH rule, severity by blast radius. Excludes:
      frozen trees, `External/Sheaf`, the two `*Repro.cpp` diagnostics (report-only there).
- [x] **F8.2 — Spec-drift sweep** (read-only): every `file:line` citation and code snippet in this
      change's living artifacts (`proposal.md`, `tasks.md`, `SUPERSESSION-RECORD.md`,
      `BANK-EXPANSION-DESIGN.md`) re-verified against the code as it stands after F3.3. Today's
      edits have moved lines under many of them. Also: every claim a task makes about what exists
      ("X is public", "Y is called from Z") re-read, M1 applied to the artifacts themselves —
      today's five lead errors were all spec text written from memory instead of from a read.
      Archive dirs are history: report drift there, never edit (F0.2's footnote precedent).
### F8.1 FINDINGS — code sweep, lead, 2026-08-07 (post-`1c37657`)

Swept all of `app/` against §1, §5, §6, §8, §10, §11, §12. Frozen trees, `External/Sheaf` and the
Daisy field app untouched. **Production code is in better shape than the artifacts** — zero
repeated 3-line sequences anywhere, and every helper passes §6's 2-of-4 (`ZeroExistingModulation
Depths` and `CapacityExhausted` are single-use but each names a domain concept without obscuring
data flow, which is the stated exception). Four findings, none blocking.

- [ ] **C1 (§8 + §12, highest blast radius) — the sample-rate fallback is duplicated six times,
      with TWO different values, and the source never validates.**
      `sampleRate > 0.0f ? sampleRate : 44100.0f` appears identically at `dsp/Delay.hpp:250`,
      `dsp/EnvelopeFollowers.hpp:35`, `:74`, `dsp/VoiceEnvelope.hpp:81`, plus bare `44100.0f`
      defaults at `dsp/Delay.hpp:421` and `dsp/VoiceEnvelope.hpp:177`. But `dsp/Limiter.hpp:121`
      guards the same condition as `std::max(1.0f, sampleRate)` — **a different fallback for the
      same defect**, and 1.0 vs 44100.0 produce wildly different coefficients.
      Meanwhile `FroggersAppCore::PrepareToPlay` (`:262`) casts the host's rate with **no
      validation at all**. So five units re-guard a condition the source never checks and disagree
      on the answer. **§12: trace the origin first** — if the host contract permits a non-positive
      rate, validate ONCE in `PrepareToPlay` and delete the five downstream guards; if it does not,
      delete all six. Either way one named constant, one site.

- [ ] **C2 (§8) — three-VCO sequential duplication.** `FroggersAppCore.hpp:956-968` processes
      `audioVco1_/2_/3_` in three structurally identical statements differing only by index
      (`0,3,6` / `1,4,7` / `2,5,8`). The `(i, +3, +6)` grouping is a real concept expressed three
      times as literals, and it appears a fourth time in `ProcessBlock`'s `vcoDrive` lambda
      (`:711-713`) — whose own comment acknowledges the duplication. Blocked only by the VCOs
      being individually-named members. **`std::array<dsp::Vco, 3>` collapses all three to a loop
      and makes the grouping explicit** — and would collapse F3.3's three `visit(audioVcoN_, …)`
      lines too. Check first whether `dsp::Vco` is copy/movable; the modulation slate's
      visualizers are individually named precisely because `synth::ui::Visualizer` is not.

- [ ] **C3 (§10/§11/§12) — a loop-invariant null check evaluated once per sample.**
      `if (block.outputs != nullptr)` sits inside the per-frame loop (`FroggersAppCore.hpp:594`
      opens it; the check is at `:739`), so a block-scoped pointer is re-tested 48,000×/second.
      Hoist it out of the loop. The inner `if (channel != nullptr)` needs a §12 trace: if the
      engine contract guarantees non-null channels, both branches are impossible and go.

- [x] **C4 (§8) — per-stage limiter thresholds scattered (0.7 peak, 0.7 drive, 0.9 delay/reverb/
      master). ALREADY TRACKED, not a new finding:** `dsp/Limiter.hpp:66-70` records it and ties
      the consolidation to B7.1, i.e. to **F2.1**. Confirmed still accurate. No separate work item.

### F8.2 FINDINGS — artifact drift, lead, 2026-08-07

**Every line citation in the F4 block had drifted, and the two load-bearing ones pointed at
unrelated code** — `:1112` at a Crispy comment, `:1152` at a `continue;`. F0.1's hoisting and
F0.3/F0.4's comment rewrites moved them. **The next F4 dispatch would have edited the wrong
lines.** Corrected in place against `1c37657`, with a verify-before-editing warning.

F5's citations (`:676`, `:723`, `:736`) were re-verified and are **accurate** — that code is
untouched.

- [x] **F8.2a — DONE: the cite-by-symbol rule is now in §0.**
      Line numbers in a living plan are stale the moment anything above them changes, and this
      change rewrites the same files it cites. Prefer
      `grep -n "drillIn.Back()" app/FroggersModulation.hpp` over `:1152`; where a line number is
      genuinely useful, pin it to a commit ("as of `1c37657`") so a reader knows what it was true
      of. This is the artifact-side form of the omni rule's refined trace clause.

- [x] **F8.3 — Lead fixes the artifacts** from F8.1/F8.2 findings. Code findings that are real
      defects become tasks (or fold into existing F-items); artifact drift is corrected in place.
      Nothing in F8 lands code changes beyond what F3.3 already did.
- [x] **F8.4 — Handoff document** (`HANDOFF.md`, written 2026-08-07) in this directory, leading with the behavioural lessons (the
      predecessor's failure-report format, which the operator kept): what is DONE and verified,
      what is RED by design, what is OPEN with root cause unknown vs traced, the sequencing traps
      already discovered (F3.3-masks-F3, measurement-during-refactor), and the exact next dispatch
      for each remaining F-item.

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

1. **The `SceneCenter(0) =` sites (76 on 2026-08-06; 85 later that day; **92** counted
   2026-08-07 by `grep -rn "SceneCenter(0) =" app/ | wc -l`, and still growing as tests are
   added — treat any figure written here as a timestamp, not a fact) are NOT invalid.** Do not
   "fix" them. A test that runs
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

- [x] **Step 1: Split the existing test. RULING TAKEN 2026-08-06 — implement it, do not re-decide.**
      **DONE, verified in the tree 2026-08-07** (`6195a41`): the split landed as
      `limiter_engages_and_envelope_drops_below_unity` (`:572`) and `overdriven_patch_stays_bounded`
      (`:610`), with the `minEnvelopeSeen < 0.999f` assertion stripped and its removal recorded in
      a comment at `:597`/`:623`.

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

- [x] **Step 2: Write the failing test — and the `ApplyPatchNow` helper it is the first consumer
      of.** **DONE (`6195a41`).** `ApplyPatchNow` at `:104-105`, `PeakAbs` at `:111`. Use the operator's real repro, not a synthetic sweep (M3): Filter bank Crispy at max,
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

- [x] **Step 6 — B7.5 IS NOT COMPLETE WITHOUT LIVE MODULATION. Added 2026-08-06. DONE.**
      **Landed and verified in the tree 2026-08-07:**
      `master_limiter_stays_at_unity_under_live_modulation` at `:733`, depths on Drive slot 8 and
      Filter slot 5, both from `kModSlotVco1Audio` (`:751`, `:757`). `kModSlotNoise` appears
      nowhere in either acceptance test — confirmed by grep, as the correction below requires.

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

**The snippet that stood here contradicted the correction directly above it** — it still passed
`kModSlotNoise`, the source that box exists to forbid. Removed 2026-08-07 rather than corrected:
the code has landed, so **read the tree, not a snippet** (`grep -n "EnsureModulationDepth"
app/FroggersAudioRoutingTests.cpp` → `:751`, `:757` as of `4cde39c`). Retaining a hand-copied
version of landed code is a second definition site of the same fact (OMNI §8) and is how this
snippet went stale in the first place.

      **Report both numbers for the new test too.** Expected: red, with a LARGER gain reduction
      than the static test's 0.0142. **If the modulated test is no worse than the static one,
      stop and report** — that would contradict §K.1's measurement and means the modulation is
      not reaching `DriveBlendPhase`, which is its own defect.

- [x] **Step 3: Run it and confirm it FAILS**, and record the actual `minEnvelopeSeen` in this
      file. A number, not a paragraph (M7). **DONE — red at `minEnvelopeSeen = 0.985796`,
      `PeakAbs = 0.991599` (static); `0.985954` / `0.991505` (live modulation).**

```bash
cd app && nice make -j2 test 2>&1 | tail -30
```

Expected: FAIL on `master_limiter_stays_at_unity_across_hostile_patch`, with `minEnvelopeSeen`
well below 0.999 — the proposal's trace predicts continuous engagement.

- [x] **Step 4:** If it PASSES, **stop and report.** A passing B7.5 before B7.1 means the trace in
      `proposal.md` is wrong and the whole plan needs re-deriving. Do not "fix" it by hardening the
      patch until it fails — that is M3 in reverse. **N/A — it failed, as required.**
- [x] **Step 5: Commit** the red test, marked expected-red in the commit message. **DONE
      (`6195a41`, "B7.5 Step 2-3: EXPECTED-RED end-to-end acceptance test").**

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

> **⚠ THIS WHOLE SUBSECTION IS HISTORY AS OF `1c37657`. Do not act on it.** It describes the
> pre-F3.3 flush, which cleared 2 of 14 units. **The flush now resets all 14**
> (`FroggersAppCore.hpp:670`, `:696`), so "anything still ringing upstream refills delay and
> reverb" no longer has anything ringing upstream to do it — every upstream unit is zeroed in the
> same pass. The flush is *still* structurally one-shot, and that remains correct: after the clear
> the gate is closed and every voice is Idle, so nothing can re-inject.
> **The one-shot property is therefore no longer a defect — it is the load-bearing assumption
> F3.2c tests.** If output re-grows after a clear that zeroed all 14 units with no input, the only
> remaining energy source is the modulation that keeps running, which is precisely the parametric
> hypothesis below. Kept here because F3.2c's refutation condition is stated against it.

**Hypothesis 1 (`AllIdle()` never latching) was checked and is unlikely:** `setGate(false)` forces
every voice to `Stage::Release` (`dsp/VoiceEnvelope.hpp:93`), and `releaseStep` is floored above
zero by `kMinTimeSeconds`, so `m_level` decrements monotonically to Idle regardless of modulation.
Chase the flush, not the gate.

- [x] **F3.1 — MEASURED 2026-08-07. THE TRACED HYPOTHESIS IS REFUTED: on this patch, Stop stops.**

      Fourteen units (not thirteen — the lead miscounted; it is ten `RecoverUnitIfNeeded` plus four
      `RecoverIfNonFinite`) instrumented after Stop, comb feedback and reverb Hold at max.

      | | pre-Stop | t+0.1 s | t+1 s | t+5 s | t+30 s | t+60 s |
      |---|---|---|---|---|---|---|
      | **OUTPUT peak** | **0.840133** | 1.28e-05 | 1.64e-10 | 2.84e-32 | **0** | **0** |
      | `filterChain.comb` | — | 0.995679 | 0.003619 | 2.36e-05 | 3.74e-16 | 5.20e-29 |
      | reverb tank | — | 1.26e-04 | 2.26e-04 | 1.26e-04 | 3.34e-06 | 4.27e-08 |
      | `delay_` line | — | 0 | 0 | 0 | 0 | 0 |
      | `outputLimiter_.envelope` | — | 1 | 1 | 1 | 1 | 1 |

      **The instrument was genuinely loud before Stop (0.84) and is silent after it.** The comb
      decays cleanly, the reverb tank decays, output is at 1e-10 within a second and exactly zero
      by 30 s. **The traced mechanism — comb and `driveBlendPhase_` re-exciting delay/reverb past
      the one-shot flush — does not reproduce.**

      **A lead error worth recording:** `driveBlendPhase_` reads a constant **0.98** at every
      timepoint, which looks like a stuck resonator. It is not. `DriveBlendPhase::StateMagnitude()`
      returns `max(|allpassX1|, |allpassY1|, |coeffSmoother.output|)`, and `coeffSmoother.output`
      is seeded to `-0.98f` by `Reset()`. **That is a coefficient, not signal energy.** Reading it
      as a leak is the same class of error as everything else on this project's record.

      **Consequence: the §1/§8 enumeration defect is REAL but is NOT F3's cause.** It is a
      code-quality fix (F3.3) and must not be described in any commit as fixing Stop.

### F3.2 LEAD HYPOTHESIS — PARAMETRIC OSCILLATION. Traced by the lead 2026-08-07.

**Modulation is not transport-gated.** `modulation_.Step(...)` and `parameters_.ProcessSample(...)`
are called unconditionally in the per-sample loop (`app/FroggersAppCore.hpp:717,720`) — the
transport state is passed *into* `Step` as an argument, it does not gate the call. So after Stop:

- the ASR gate closes, the VCOs are silenced, **the scope goes flat** — matching the operator's
  "the oscilloscope isn't moving";
- but every modulation source keeps free-running, and keeps sweeping every modulated parameter at
  audio rate — **including comb feedback, comb LP and peak Q.**

**A feedback loop whose gain is modulated is not the same system as one whose gain is fixed.**
`Comb::Process` is `out = in + fb·Saturate(lp(delayed))`. With `fb` static, that is a linear
decaying system — which is exactly what F3.1 measured, decaying cleanly to 5.2e-29. With `fb`
swept at audio rate it is a **time-varying** system, and time-varying feedback can pump energy in
and self-sustain even when every instantaneous `|fb| < 1`. That is parametric amplification.

**This project has already MEASURED this phenomenon and not named it.** §K.1 recorded
`DriveBlendPhase` at **1.002** under free random phase but **50.5×** under *"periodic phase/content
coincidence"* — a time-varying allpass coefficient pumping a recursive filter. Same mechanism,
different stage. Nobody connected it to the comb.

**It accounts for every element of the report, which no previous hypothesis did:**

| observation | explained by |
|---|---|
| scope still | VCOs gated silent; modulation is not gated |
| audio continues | comb self-sustaining, driven by modulated feedback, no input needed |
| "harsh loud noise" | oscillation at the comb's delay frequency through its in-loop saturator |
| "over a minute" | self-sustaining, not decaying — it has no reason to stop |
| only after randomizing | randomize is what assigns modulation depths to Filter parameters |
| F3.1 saw clean decay | F3.1 used STATIC parameters and no modulation — the one ingredient that matters |

**And it may make F2 and F3 the same bug.** During play, the same parametrically-pumped comb would
hold the level up continuously between gate pulses — the operator's *"pretty darn loud the whole
time"* floor — with the quarter-note gate riding on top as the *"rhythmic"* part. Filter-specific,
exactly as reported.

> ### ⚠ F3.3 CAN MASK F3'S SYMPTOM WITHOUT FIXING ITS CAUSE — sequencing trap, found 2026-08-07
>
> F3.3 changes the Stop flush to reset **all 14** stateful units instead of 2, which means the comb
> gets zeroed at Stop. **A parametric oscillator with zero state has nothing left to amplify**, so
> F3's audible symptom may simply disappear once F3.3 lands — while the underlying instability is
> completely untouched.
>
> That matters because the same instability is the leading explanation for **F2**, which happens
> during PLAY, where no flush ever runs. Curing Stop and declaring the mechanism understood would
> be a green-while-wrong at the design level rather than the test level: symptom gone, cause
> intact, and the harder bug left looking unrelated.
>
> **Therefore:**
> 1. **Never describe F3.3 as fixing F3.** It is an OMNI §1/§8 duplication fix. F3.1 already showed
>    the enumeration defect is not F3's cause.
> 2. **Run F3.2c AFTER F3.3 lands, against the new flush**, and read the result as follows:
>    - **still sustains** → the pumping re-grows from numerical noise; root cause is real, present,
>      and unfixed. F2 and F3 are the same bug.
>    - **goes silent** → the reset is sufficient for the Stop symptom, but says **nothing** about
>      play-time behaviour. F2 remains open and must be measured separately, during play.
> 3. A measurement taken while F3.3 was half-applied would have been worthless in a way that
>    produces a confident number — the lead dispatched exactly that by mistake and killed the run.

- [ ] **F3.2c — THE DECISIVE MEASUREMENT. Run this AFTER F3.3 lands (see the trap above).**
      Re-run F3.1's harness with ONE delta: a modulation depth on **Filter slot 5 (Comb feedback)**
      from an audio-rate source, so `fb` is swept while the transport is stopped. Everything else
      identical. Report output magnitude and `filterChain.comb` at t+1 s, t+5 s, t+30 s, t+60 s
      against F3.1's baseline.
      - **Output sustains instead of decaying → hypothesis confirmed**, F3's root cause is
        parametric oscillation in a modulated feedback path, and the fix is about bounding a
        time-varying loop, not about the flush enumeration.
      - **Output still decays → hypothesis refuted.** Say so plainly and report the numbers; do
        not adjust depth or source to chase it.
      Also report whether `driveBlendPhase_` behaves differently under the same condition, since
      §K.1's 50.5× is the same mechanism.

- [ ] **F3.2 — Earlier candidates, now secondary to F3.2c.**
  - **F3.2a — the patch is not the operator's condition.** F3.1 used 5 static parameters and no
    modulation. The report came after Randomize All, with dozens of parameters carrying depths
    across all six banks. **Same gap as F2.0b — measure both in one harness.**
  - **F3.2b — the harness may not exercise the Stop the operator presses.** The rig drives the
    transport directly. In the app, the Stop button pushes `MessageIn::Stop` from the UI thread
    through `desiredTransportRunning_` (`FroggersAppCore.hpp:348,362`). **If the defect is in that
    path, F3.1 cannot see it by construction — and every existing Stop test shares the blind
    spot.** Trace that path before writing another DSP-level test.

      Only after one of these reproduces does a failing test get written. **Liveness assertion is
      mandatory** (§0): assert the instrument was loud pre-Stop, as F3.1 did at 0.840133.

- [x] **F3.3 — Fix the enumeration defect on its own merits (OMNI §1, §8). Not a Stop fix.**
      **DONE (`1c37657`), independently re-verified in the tree 2026-08-07.** All four required
      properties confirmed by reading, not by report: `dsp/RecoveryTier.hpp` exists and holds the
      tags (`struct FiniteOnly {}` / `struct Magnitude {}`); the enumeration is hierarchical
      (`FroggersAppCore.hpp:1480` composes `drive_.ForEachStatefulUnit` at `dsp/Drive.hpp:309` and
      `filterChain_.ForEachStatefulUnit` at `dsp/FilterFx.hpp:611`); the tags are compile-time
      types guarded by `if constexpr`, not a runtime enum; every Tier-2 timer now lives **in its
      unit** (`unit.overCeilingSeconds` — `dsp/Vco.hpp:125`, `dsp/FilterFx.hpp:234`/`:391`,
      `dsp/Drive.hpp:131`/`:191`/`:459`), with **no** parallel array or `const void*` lookup left in
      the parent. The Stop flush calls `ForEachStatefulUnit([](auto& unit, auto) { unit.Reset(); })`
      at both edges (`FroggersAppCore.hpp:670`, `:696`) and the false
      "VCOs/filters/drive do not self-sustain" comment is gone.
      **Unit count is 14** (10 `Magnitude` + 4 `FiniteOnly`), confirming F3.1's count and refuting
      the "thirteen" that `proposal.md` carried.

      The concept "every stateful unit" has two definition sites: `RecoverPoisonedUnitState`
      (14 units, complete) and the Stop flush (2 units, truncated), the latter justified by a
      comment at `:623-626` asserting "VCOs/filters/drive do not [self-sustain]" — false by this
      project's own measurements, and asserted rather than traced.

      **Enumerate hierarchically: each struct lists its own members, directly beneath their
      declarations.** Not one 14-entry list far from the members.

```cpp
enum class Recovery { FiniteOnly, Magnitude };   // Tier 1 / Tier 2

// dsp::FilterFxChain, immediately under its member declarations
template <typename V> void ForEachStatefulUnit(V&& visit) {
    visit(comb, Recovery::Magnitude);
    visit(peak, Recovery::Magnitude);
    visit(scoopNotch, Recovery::Magnitude);
    visit(peakLimiter, Recovery::FiniteOnly);
}

// FroggersAppCore composes rather than re-listing
template <typename V> void ForEachStatefulUnit(V&& visit) {
    visit(audioVco1_, Recovery::Magnitude); /* …vco2, vco3, driveBlendPhase_… */
    drive_.ForEachStatefulUnit(visit);
    filterChain_.ForEachStatefulUnit(visit);
    visit(delay_, Recovery::FiniteOnly);
    visit(reverb_, Recovery::FiniteOnly);
    visit(outputLimiter_, Recovery::FiniteOnly);
}
```

      **The tier is DECLARED, never inferred from the interface.** `if constexpr (requires {
      unit.StateMagnitude(); })` is tempting and wrong: F3.1 added `StateMagnitude()` to
      `StereoDelay` and `Reverb` purely as a diagnostic, and their own comments require them to
      stay Tier-1-only (a BIBO-stable loop settles at a large-but-finite level and the Tier-2
      ceiling would misfire). Inferring tier would mean **adding a read-only diagnostic silently
      reclassifies a unit's fault recovery.**

      Both consumers then collapse to one line: the Stop flush ignores the tag and calls `Reset()`;
      recovery switches on it. Delete the false comment in the same edit.

> #### ⚠ F3.3 SPEC CORRECTED 2026-08-07 — the first attempt did not compile, and the lead caused it
>
> The original spec said: *"Do NOT relocate the ten parallel `…OverCeilingSeconds_` members — a
> Stop-scope fix and a state refactor must not land in one commit."* **That instruction was
> wrong.** With the enumeration hierarchical and the timers owned by the parent, there is no clean
> way to pair a nested unit with its timer, and the implementer was forced into:
>
> ```cpp
> const std::array<MagnitudeCounter, 10> magnitudeCounters{{ {&audioVco1_, &vco1OverCeilingSeconds_}, … }};
> for (const auto& counter : magnitudeCounters) if (counter.unit == &unit) { … }
> ```
>
> — **a second ten-entry list keyed by `const void*`, linear-scanned per unit per block**, which
> reintroduces precisely the §8 duplication this task exists to delete, adds type-unsafety, and
> has a silent fallthrough when a unit is absent. Worse than what it replaced.
>
> It also does not compile: a **runtime** `if (tier == Recovery::FiniteOnly)` does not prevent
> instantiation, so `RecoverUnitIfNeeded` is type-checked for `dsp::OutputLimiter`, which has no
> `StateMagnitude()` by design. Six of ten binaries fail identically.
>
> **Both defects have one cause and one fix. Do all three of the following:**
>
> 1. **Move each Tier-2 unit's `overCeilingSeconds` INTO the unit.** It is per-unit state and
>    always was; ten parallel members in the parent existed only because the enumeration lived
>    there too. This deletes the second §8 instance rather than deferring it. **No lookup is then
>    needed at all** — the visitor reads `unit.overCeilingSeconds`.
> 2. **Make the tier a COMPILE-TIME tag, not a runtime enum**, so `if constexpr` can guard the
>    branch and only the correct call is instantiated:
>
> ```cpp
> struct FiniteOnly {};   // Tier 1
> struct Magnitude {};    // Tier 2
>
> visit(comb, Magnitude{});
> visit(delay_, FiniteOnly{});
>
> ForEachStatefulUnit([&](auto& unit, auto tier) {
>     if constexpr (std::is_same_v<decltype(tier), Magnitude>)
>         RecoverUnitIfNeeded(unit, unit.overCeilingSeconds, blockFrames);
>     else
>         RecoverIfNonFinite(unit);
> });
> ```
>
> 3. **Put the two tag types in a small dedicated header, not in `dsp/FilterFx.hpp`.** That file's
>    own header declares it *"a **copy** (design D3) of the cited Froggers formulas"* — a port of
>    frozen firmware — and `dsp/Limiter.hpp` exists **because of exactly this question**, its
>    comment stating a general-purpose unit was extracted to keep it *"out of a file whose own
>    header comment scopes it to comb/peak/scoop routing."* Same argument. Follow that precedent.
>
> The Stop flush is unaffected by all of this — it ignores the tag entirely and calls `Reset()`.

- [ ] **F3.4:** Suite stays green (except the two known acceptance-gate reds). Confirm the reverb
      tail and delay repeats still ring normally **while the transport is running** — the flush
      must remain gated on the Stop edge and `AllIdle()`, never an unconditional reset. Confirm
      rapid Stop→Play still cancels the pending clear.
- [ ] **F3.5: Commit.**

---

## F4 + F5 — Drilldown: stop ejecting, then go three deep. ONE SHARED EDIT FIRST.

**Operator rulings, both binding:**
1. **The SCOPE is correct as built** — *"this is desired functionality for randomize all."* Level-1
   Randomize All affects only the drilled parameter. **Do not change it.**
2. **Navigating out is a BUG** — *"randomize all in level 1 shouldn't navigate me out wtf."*

**The traced cause:** `RandomizeAll`'s `Level()==1` branch ends on a bare `drillIn.Back()`
(`app/FroggersModulation.hpp:1178`), which drops level 1 → 0. Step 2 also leaves the view
(`:1134`) purely to look up an encoder id in the parameter grid. The operator ends on the main bank
page because the operation put them there.

**Verified, and it is what makes this cheap:** `detail::RandomizeParameterModulationDepths`
**does not need the modulation view open.** It takes `Parameter&`, reads eligibility from
`group.GetModulators().Metadata()` (`:914`), and calls `EnsureModulationDepth` itself (`:961`).
Nothing in it reads view state. So the `PressEncoder`/`Back()` round trips accomplish nothing
except (a) triggering `Bank::OpenModulationView`'s eager materialization of all 15 connected
sub-depths when ~2 are wanted, and (b) driving the level counter up and down — which is the
ejection.

> **Line citations re-verified 2026-08-07 (F8.2).** Every number in this F4 block had drifted —
> F0.1's hoisting and F0.3/F0.4's comment rewrites moved them, and the stale ones pointed at
> unrelated code (`:1112` landed on a Crispy comment, `:1152` on a `continue;`). An implementer
> following the old numbers would have edited the wrong lines. **Verify these before editing too;
> anything committed between this note and your dispatch moves them again.** Locate by symbol,
> not by line: `grep -n "drillIn.Back()\|drillIn.PressEncoder" app/FroggersModulation.hpp`.

- [ ] **F4.1: Delete both round trips.** Verified line numbers as of `1c37657`:
      - **Step 2's pair:** `drillIn.Back()` at **`:1134`**, then the encoder-id scan (`:1135-1141`),
        then `drillIn.PressEncoder(originalEncoderId)` at **`:1142`**.
      - **Step 3's pair:** `drillIn.PressEncoder(modIx)` at **`:1154`** and `drillIn.Back()` at
        **`:1176`**, inside the per-source loop.
      - **The final bare `drillIn.Back()` at `:1178`** — the one that actually drops level 1 → 0
        and ejects the operator.

      Find the encoder id without leaving the view, or cache it. The whole operation must be
      **visually atomic**: press it, stay put, see the badges change on the page you are on.
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

> **⚠ "7 needs a separate explanation" was FALSE — computed from the code 2026-08-07.** The
> previous text read *"The part that does not fit the spec is 7. The geometric tail should put
> P(7) well under 1%."* **It does not.** Read the draw
> (`grep -n "NextRandomCoin" app/FroggersModulation.hpp`, `:936-948` as of `4cde39c`): counts 5+
> enter with probability 0.10, then `while (count < eligible.size() && NextRandomCoin() < 0.7f)`
> — so each further increment succeeds with p = 0.7, and a count STOPS at exactly n with the
> complementary 0.3.
>
> | | probability |
> |---|---|
> | P(count = 7) | 0.10 × 0.7² × 0.3 = **1.47 %** |
> | P(count ≥ 7) | 0.10 × 0.7² = **4.9 %** |
> | expected params at 7+ across 16 visible | 16 × 0.049 = **0.78 per press** |
>
> **Roughly one parameter shows 7+ badges on any given press.** That is not an anomaly needing a
> separate cause — it is what this table produces, and it is the same finding as the 30 % at 4+,
> one rung further out. **F1 is a pure spec change; there is no second bug hiding behind the 7s.**
> Do not spend a dispatch hunting one.

**Also settled 2026-08-07 — "the same distribution at EVERY level" needs NO structural work.**
All four randomize entry points already call the one shared
`detail::RandomizeParameterModulationDepths` (single definition site,
`grep -n "inline bool RandomizeParameterModulationDepths" app/FroggersModulation.hpp` → `:893`;
call sites `:1053`, `:1085`, `:1126`, `:1164` as of `4cde39c`). The level-1 and level-2 draws are
the same code as level 0. **Retuning the table in one place changes every level by construction**
— this is OMNI §1's "reuse is only REAL when the runtime call graph is single-sourced," and here
it already is. F1.3 still tests all three levels, but as a regression pin, not as a fix.

- [ ] **F1.1: The decisive measurement, before touching the draw** (M1 — the cause was asserted
      wrongly twice and the operator caught it both times). From a genuinely fresh patch, press
      Randomize All once and histogram the per-parameter count three ways: **(a)** the count the
      helper CHOSE, **(b)** the number of depths with non-neutral `SceneCenter`, **(c)** the number
      where `HasNonZeroState()` is true. Compare (a) against 10/30/30/20 + tail.
      **Scope reduced 2026-08-07:** (a)-vs-spec is now computed above and matches, so this step's
      remaining job is the (b)/(c) badge-criterion comparison — i.e. whether what the operator SEES
      equals what the helper CHOSE. If (c) == (b) == (a), skip straight to F1.2.
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

### F2 — OPERATOR EVIDENCE, 2026-08-07. Read before F2.0.

Asked whether the blowout is rhythmic or constant, and whether Crispy misbehaves on other banks:

> **"crispy blowout is specific to the filter bank. usually rhythmic."**

**Both answers are load-bearing and they point the same way.**

**"Rhythmic" means the pumping diagnosis is RIGHT and the harness is wrong.** A constant harshness
would have killed the limiter theory outright. Rhythmic gain movement is a limiter with a release
constant, so W2.0's mechanism stands — and the 0.12 dB the harness measures is simply not the
operator's condition. **Do not weaken the pumping hypothesis on the strength of the harness
numbers; strengthen the harness.**

**REFINED by the operator moments later, and this is the more precise signature:**
> *"it can get pretty rhythmic but still pretty darn loud the whole time."*

**That is NOT pump-and-recover.** Classic pumping ducks audibly between pulses; this stays loud
throughout with rhythmic movement riding on top. The signature is **continuous heavy gain
reduction with periodic deeper dips** — i.e. the master never returns to unity at all, and the
quarter-note gate modulates how far below unity it sits.

**This is exactly what the plan's own §1 note predicted and nobody had confirmed by ear:**
*"'each stage ≤ 1.0' combined with 'master threshold 0.9' still means the master is engaged
essentially always."* The operator has now confirmed that audibly.

**The mechanism, complete:** the comb at high feedback rings with a **6.7 s T60**, so it fills the
gaps between gate pulses and holds the level up continuously — that is the "loud the whole time."
The gate then pulses on top of that floor, driving periodic deeper reduction — that is the
"rhythmic." Both halves of the description are accounted for, and both come from the same place.

**Consequence: B7.1 becomes MORE likely to be the right fix, not less.** Continuous engagement is
precisely the condition retargeting the ceiling to `C = 0.80` removes. F2.0's job is now to
measure *how far below unity* the envelope sits in the operator's real condition — the harness's
0.12 dB is the right shape and plainly the wrong magnitude.

**The likely rhythm source, traced:** the ASR gate is open for the first half of every quarter
note and closed for the second (`FroggersAppCore.hpp:607-612`, `gateOpen = phase < 0.5`). So the
instrument pulses at the transport's quarter-note rate **by design**. Each pulse slams the master,
whose release is **100 ms** (`kSharedReleaseSeconds`, `dsp/Limiter.hpp:64`) — long enough that at
ordinary tempi the envelope has not recovered before the next pulse arrives. **That is
tempo-locked pumping, and it is a property of the gate meeting the release, not of any one
stage.** F2.0a's duty-cycle measurement should therefore be read against the quarter-note period,
not against an arbitrary window.

**"Specific to the Filter bank" narrows the amplifier.** Crispy scrambles all 8 bits of its own
bank's parameters. On Drive that reaches parameters bounded by the sine fold; on Delay/Reverb,
time-effect parameters behind wet limiters. **Only on Filter does it reach the comb feedback / LP
/ peak-Q maxima continuously** — the resonant parameters W2.0 ranked as offenders 1-3. This is
direct operator confirmation of that ranking, and it means the treatment target is the filter's
resonant path, not Crispy.

**Operator's objection to the isolation test, recorded because it is correct:**
> *"i don't see why delay send and reverb wet aren't already clamped sufficiently by design"*

They are clamped — B6a/B6b put wet limiters on both. **The point is the level they clamp TO.**
Every per-stage limiter ships `ceiling = kSharedCeiling = 1.0` while the master's threshold is
**0.9**, so a correctly-clamped stage still delivers above the level at which the master begins
working. The stages are not unclamped; they are clamped just above the wrong line. That is the
entire headroom-budget defect, and it is why B7.1 retargets the **ceiling** rather than adding
more limiters.

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
      > **⚠ TRAP RESTATED 2026-08-07 — the previous statement of it was WRONG, and the real
      > failure mode is worse and silent.** Previous text: *"a stage whose threshold equals `C`
      > gives `headroom == 0` and `DesiredMagnitude` evaluates `0/0` → NaN for every sample above
      > threshold."* **Both halves are false.** Read
      > `dsp::OutputLimiter::DesiredMagnitude` (`grep -n "float DesiredMagnitude" app/dsp/Limiter.hpp`
      > — `:143-149` as of `4cde39c`):
      >
      > ```cpp
      > if (absX <= threshold) { return absX; }
      > return threshold + headroom * (1.0f - std::exp(-(absX - threshold) / headroom));
      > ```
      >
      > **`headroom == 0` does NOT produce NaN.** `absX == threshold` is caught by the early
      > return, so the division only ever runs with a strictly positive numerator: `+x/0.0f` is
      > `+inf`, `exp(-inf)` is `0.0f`, and `0.0f * (1.0f - 0.0f)` is `0.0f`. It returns exactly
      > `threshold` — a hard brickwall. Undesirable, but finite, quiet, and harmless.
      >
      > **The real trap is `headroom < 0`, i.e. leaving a threshold ABOVE the new ceiling.** That is
      > the default outcome of this task if the delay and reverb thresholds (both `0.9f`) are not
      > lowered when the ceiling drops to `0.80`: `headroom = 0.80 − 0.9 = −0.1`. The exponent's
      > sign flips and `DesiredMagnitude` stops being a limiter and becomes an **exponential
      > amplifier**:
      >
      > | `absX` | returns | effective gain |
      > |---|---|---|
      > | 1.0 | 1.072 | 1.07× |
      > | 1.5 | 41.1 | **27.4×** |
      > | 2.0 | 2203 | **1101×** |
      >
      > **Nothing in the suite catches this.** The values stay finite until `absX ≈ 9.8`, so
      > `REQUIRE_TRUE(!rig.SawNaN())` and `RequireFiniteStereo` both pass. It is a silent
      > catastrophic blowout — *the exact symptom F2 exists to remove*, introduced by F2's own fix,
      > and invisible to every guard this change has built. An implementer who tests for the NaN
      > the old text described would find none and conclude the retarget was safe.
      >
      > **`Configure()` does not validate the invariant** (`grep -n "void Configure" app/dsp/Limiter.hpp`,
      > `:113-124` as of `4cde39c`): it assigns `headroom = ceiling - threshold` with the sign
      > unchecked. This is a real edge case the moment F2.1 changes these constants, so OMNI §12
      > requires the guard rather than forbidding it. **F2.1a below adds it.**

- [ ] **F2.1a — Pin the `threshold < ceiling` invariant at COMPILE time, before F2.1's edit.**
      Every per-stage threshold/ceiling pair is a `constexpr` constant beside its limiter, so the
      invariant is checkable with zero runtime cost and cannot be forgotten by a later stage.
      Add one `static_assert` per pair, at the definition site of each pair:
      `dsp/FilterFx.hpp` (`kPeakLimiterThreshold`/`kPeakLimiterCeiling`), `dsp/Delay.hpp`
      (`kDelayWetLimiterThreshold`/`kDelayWetLimiterCeiling`), `dsp/Reverb.hpp`
      (`kReverbWetLimiterThreshold`/`kReverbWetLimiterCeiling`), `dsp/Drive.hpp`
      (`kOutputLimiterThreshold` against `kSharedCeiling`), and `dsp/Limiter.hpp`
      (`kDefaultThreshold`/`kDefaultCeiling`). Message names the failure mode, not the rule —
      e.g. *"threshold must stay strictly below ceiling; a negative headroom turns
      DesiredMagnitude into an exponential amplifier."*
      **Land F2.1a and confirm the suite is still green BEFORE F2.1 changes any constant**, so the
      guard is proven to compile against the current (valid) values first and any later red is
      attributable to the retarget alone.

- [ ] **F2.1b — Then retarget.** Each stage's threshold must stay **strictly below** `C = 0.80`.
      The delay and reverb thresholds are currently `0.9f` and **must come down in the same edit
      that lowers the ceiling** — F2.1a's `static_assert` now makes forgetting a build error
      rather than a silent 27× amplifier. Choose the new delay/reverb thresholds by preserving
      their current *relative* headroom fraction rather than by picking a round number, and record
      the two values chosen with their derivation.
- [ ] **F2.2: Make-up gain `1/C` = 1.25× (+1.9 dB)**, applied **AFTER** `outputLimiter_.Process(x)`
      and **BEFORE** the trailing clamp (`FroggersAppCore.hpp:1228` area). Not before the limiter —
      its threshold/headroom math is calibrated in absolute terms, so pre-scaling would silently
      retune every stage's budget. The trailing hard clamp stays; it is the residual-overshoot net.
- [ ] **F2.3: Run B7.5. It must now be GREEN.** If it is not, **stop and report the number** — do
      not start adding per-stage bound tests (M3). Report which stage still exceeds `C` using
      F3.1's instrumentation.
- [ ] **F2.4: Confirm both halves of B7.5 Step 1's split still pass.**
      **NAME CORRECTED 2026-08-07:** `limiter_engages_on_overdriven_patch_and_stays_bounded` **no
      longer exists** — Step 1 split it and renamed both halves, so this task cited a dead symbol.
      The two live tests (verified by `grep -n "TEST_CASE(" app/FroggersAudioRoutingTests.cpp`,
      `:572` and `:610` as of `4cde39c`) are:
      - `limiter_engages_and_envelope_drops_below_unity` — property 1, the master does real work,
        re-homed to drive `dsp::OutputLimiter` directly. **This one is the backstop proof** and
        must stay green: the master must remain functional for reverb Hold, which is deliberately
        parked just under self-oscillation. Because it drives the limiter directly it is immune to
        the gain-staging change, which is exactly why Step 1 moved it there.
      - `overdriven_patch_stays_bounded` — the chain-level boundedness/finiteness assertions, with
        the `minEnvelopeSeen < 0.999f` engagement assertion deliberately stripped.
- [ ] **F2.5: Commit.**

---

## F7 — Drilldown level headers (operator request, 2026-08-07)

Operator: *"when we are in modulation drilldown levels … it should be trivial to have headers when
we are in the drilldown levels, 'Modulation Level 1' then 2 then 3."*

- [ ] **F7.1 — Show the current drill level as a header while drilled in.**
      Level 0 (the parameter grid) shows no such header; levels 1+ show `Modulation Level N`.

      **This does NOT depend on F5's `levelEncoders_` array.** The operator's note assumed the
      array is what makes this cheap; in fact `FroggersModulationDrillIn::Level()` is already
      public and already returns the current level (`app/FroggersModulation.hpp:669`). The header
      needs the counter, not the array. F5 matters only in that it raises the maximum to 3, which
      the header should then display without a code change — **derive the label from `Level()`,
      never from a hardcoded set of level names**, or F5 will need a second edit (OMNI §8).

      **Sequence it AFTER F4+F5** so level 3 exists to display and the header is verified against
      all three levels in one pass, not two.

  - **Read the surface layer first** (`app/FroggersUiSurface.hpp`) and report where a header can
    be emitted without disturbing the single declared 6×6 grid — that grid is settled and
    operator-approved, and this must not perturb it. **If a header cannot be added without
    changing the grid, stop and report rather than reflowing the layout.**
  - Operator confirms visually at F6. Not implementer-closable.

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
