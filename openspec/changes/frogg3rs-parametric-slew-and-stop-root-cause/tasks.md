# Tasks — `frogg3rs-parametric-slew-and-stop-root-cause`

> **Read `proposal.md` first.** It is this change's only executable artifact (OMNI §3) and it
> carries the traced mechanism. `SUPERSESSION-RECORD.md` is history: read it for how the diagnosis
> moved, not for what is true.
>
> **Rewritten 2026-08-07 by an omni-rule audit.** The previous revision named audio-rate modulation
> as F3's confirmed root cause and built both fixes on it. The symptom reproduces end-to-end with
> **every coefficient held static and no modulation at all**, so that mechanism is an aggravator,
> not the cause. Tasks below are re-scoped accordingly. Claims marked MEASURED or VERIFIED were
> re-read or re-measured during the audit; everything else still needs checking before you rely
> on it.

**Goal:** make silence produce silence. Then stop modulation free-running while stopped (operator
ruling), then remove the parametric-pumping mechanism behind F2's blowout during play.

**Suite as inherited: 10 binaries, 183 tests, 0 failures, 0 warnings.**
**Suite as of S1a.2: 10 binaries, 185 tests, 0 failures, 0 warnings** (+2 from S1a.2's coverage).
`External/Sheaf` pinned at `77a3019e`. **Any red is a regression — with exactly one sanctioned
exception, S1.3's parity divergence.**

## Status as of 2026-08-08 — ALL IMPLEMENTATION LANDED

**Suite: 10 binaries, 189 tests, 0 failures, 0 warnings.**

**Operator-verified by ear 2026-08-07:** S4.1 Stop works, S4.2 blowout fixed.

**Landed:** S0.1, S0.2 (citations) · S1.1–S1.3 (**F3 root cause: the drive stage no longer
manufactures DC from silence**) · S1a.2 (modulation transport-gated) · S2a.1 (reverb in-loop
saturator, operator-ordered) · S5.1/S5.2/S5.3 (drilldown Back pops one level; level header legible)
· S6.1 (encoder card frame dropped, operator ruling) · S3.1 (VCO `std::array` refactor, proven
bit-identical: 8192 samples × 2 channels, identical MD5, 0 differing bytes, liveness control
rms 0.253 / peak 0.584) · **sustain floor raised 0.05 → 0.10** (operator: *"minimum sustain should
be higher, maybe .1"*).

**Dropped by operator:** S2 (the slew) — see its own section for the reasoning.

**Open:** S4.3–S4.8 (operator eyes/ears only).

**NEW, unscheduled — the fuegoization fixed point.** Operator observed 2026-08-08: a parameter at
its minimum position is not perturbed by local Crispy or global Crunchy, though every other position
is. **Cause, VERIFIED by reading `app/dsp/Fuegoize.hpp`:** every step of the scramble is
`lowerBits ^= (shift(lowerBits) & mask)` — self-referential XOR. At the minimum,
`inputInt = uint16_t(0.0f * 255.0f) = 0`, every shift of zero is zero, and `0 ^ 0 = 0`, so **zero is
a mathematical fixed point of the whole transform** for every mask, every row, every fuego amount.
Fuego is not being skipped; it runs and provably returns the input unchanged. Generalisation: the
scramble only touches the low bits under `mask`, so any value whose masked low bits are zero is
likewise invariant — at low fuego amounts (small mask) many "round" positions barely move.
**Note the symmetry with S1.3:** `DigitalReorganizer` XORs by a constant `flip`, which is exactly
why ITS zero mapped to nonzero; `Fuegoize` has no such constant, which is exactly why ITS zero maps
to zero. Same family, opposite failure. This is a verbatim port of the firmware's
`Parameter.hpp:143`, so the hardware behaves identically — **fixing it would be another deliberate
parity divergence and is an operator decision, not taken.** The fix shape would be to XOR in a
row-derived constant so zero is not a fixed point, preserving the permutation character.

## Status as of 2026-08-07 (superseded by the block above)

**LANDED:** S0.1, S0.2, S1.1, S1.2, S1.3, S1a.2 — **F3 is fixed at its root.** The S1.2 harness
case measures the post-flush peak at **exactly 0** (seeded run, and both the Flip=0 and Blend=0
controls). One parity case, `digital_reorganizer_process_matches_bit_scramble_formula`, was
re-asserted against the divergence; its expected value moved by exactly `-Mangle(0.0f, 51, 6)`,
which is the correction term itself. Pass-through cases did not move.

**Also retired:** the F3.2c/F3.2d probes in `app/FroggersStopFlushRepro.cpp` now print
**`VOID — premise eliminated`** rather than failing the binary. Their premise was "modulation keeps
sweeping a loop coefficient after Stop," which S1a.2 removed; the probe now measures the knob
frozen bit-exactly (`max-min = 0`). VOID, not refuted — OMNI §9.1.

**OPEN:** S2 (slew, re-scoped — take S2.3's baseline first), S2a (reverb saturator, operator's
call), S3 (deferred refactor), S4 (operator verification, including the new S4.8).

**Nothing is committed.** All of the above sits in the working tree.

## §0 Standing constraints (binding)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **Builds emit nothing for ~70 s and look identical to a hang.** Run them in the background with a
  visible progress tick. **A dispatched subagent that ends its turn with a build still running
  stalls the queue.** One did. Run the build in your own turn so you can act on the result.
- **`External/Sheaf` is pinned and unpatchable.** Needs go to `/UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`. In particular
  `src/core/PolynomialDrive.hpp` — the firmware original of the unit S2 fixes — **is not edited.**
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose spec requires operator eyes or ears.**
- **Cite by SYMBOL, not by line.** Every line number here is stale the moment anything above it
  changes.
- **A negative result requires a positive control** (OMNI §9.1): before recording "X did not
  happen," print the number proving the setup could have produced X.

---

## S0 — Citation repair (mechanical, do first)

- [x] **S0.1 (DONE) — Ten stale references to the predecessor's pre-archive path.**
      `SUPERSESSION-RECORD.md` claims "Verified: zero references to the old live path remain."
      **That claim is false.** Ten remain, every one **wrapped across two comment lines**, which is
      why a single-line grep found none — the §8 failure the record documents about itself one
      paragraph earlier. Each reads `openspec/changes/` at the end of one line and
      `frogg3rs-blowout-and-drilldown-repair/…` at the start of the next; the correct path inserts
      `archive/2026-08-07-`. Sites (symbol-anchored, since line numbers move):
      `app/FroggersStopFlushRepro.cpp` (file header), `app/FroggersLimiterPumpingRepro.cpp` (file
      header), `app/FroggersBadgeCriterionRepro.cpp` (file header), `app/dsp/RecoveryTier.hpp`
      (file header), `app/dsp/FilterFx.hpp` (the `F3.3 SPEC CORRECTED` note above the per-unit
      fault comment), `app/dsp/Vco.hpp` (the `F3.3 SPEC CORRECTED` note), `app/dsp/Delay.hpp` (the
      `C1` note above `setSampleRate`), `app/FroggersUiSurface.hpp` (the `F7 (operator request…)`
      note), `app/FroggersAppCore.hpp` ×2 (the `F3.3` note in `ProcessBlock`'s Stop-edge block, and
      the `F3.3 SPEC CORRECTED` note above `ForEachStatefulUnit`).
      **Verify with `grep -rn -B1` on the slug, not with a single-line grep** — that is the whole
      lesson of this task.
- [x] **S0.2 (DONE) — One stale value claim in a code comment.** `RouteAudioSample`'s peak-ceiling comment
      says Randomize All "pins the feedback near its **-1.1** extreme". This port deliberately
      diverged to **±0.95** (`dsp::Comb::GetFeedback`). Correct the number; leave the argument.

---

## S1 — F3: the chain manufactures signal from silence. ROOT CAUSE MEASURED AND TRACED.

**See `proposal.md` §2 for the full trace.** Summary: with the transport stopped, every voice
`Idle` and all 14 units `Reset()`, `chainIn` is exactly `0.0f` — and
`DigitalReorganizer::Process(0.0f)` returns `(scramble(128 ^ flip))/128 - 1`, which is **nonzero
for any `flip != 0`** and exactly **−1.0** at `flip == 128` (Drive slot 4 ≈ 0.502). That DC is
gated only by Drive slot 7 (Blend), then amplified by the comb (±0.95), the delay (0.98) and — the
one loop with **no in-loop saturator** — the reverb tank. A per-stage limiter asymptotes to
`kStageCeiling = 0.80`, `SanitizeOutputSample` multiplies by `kMakeUpGain = 1.25`, and
`0.7999999 × 1.25 = 0.9999999`: the exact number in `F3DIAG-capture-2026-08-07.txt`.

**This is not a Stop bug and it is not a modulation bug.** It is a memoryless nonlinearity whose
`f(0) != 0`, in a chain with no DC blocker and no AC coupling. Stop is simply the only time the
input is *exactly* zero long enough to notice.

- [x] **S1.1 — Find the seed. DONE, VERIFIED BY READING + MEASURED.** The predecessor's
      "UNVERIFIED lead" was right. `app/dsp/Drive.hpp`, `DigitalReorganizer::Process`. Every other
      stage between the gate and the output is provably silent on a zero input with reset state,
      each with a positive control (OMNI §9.1). Full table in `proposal.md` §2.
- [x] **S1.2 (DONE) — Reproduce it in the harness. WRITE THIS BEFORE THE FIX.**
      Extend `app/FroggersStopFlushRepro.cpp`: transport stopped, all voices `Idle`, all units
      `Reset()`, **Drive Flip ≈ 0.502 and Drive Blend > 0**, all coefficients static, no modulation
      whatsoever.
      **Assert the DESIRED behaviour — that the post-flush peak decays to and stays at ~0.** That
      assertion FAILS today (it will measure the climb to ~1.0) and PASSES after S1.3. Do not write
      it the other way round: a test asserting "the peak pins near 1.0" passes before the fix and
      fails after, which is a characterization test, not a gate. The predecessor shipped five green
      per-stage bound tests that moved no symptom; this one must be red first.
      **Positive/negative controls are mandatory (OMNI §9.1):** the same rig with Flip = 0, and
      again with Blend = 0, must measure **exactly 0.0** forever. Print all three numbers. Without
      them a passing test proves only that the rig runs.
      Why the harness never reproduced F3 before: its post-Stop patch leaves Flip or Blend at 0.
- [x] **S1.3 (DONE) — Fix it at the source: make the drive stage map silence to silence.**
      `app/dsp/Drive.hpp`, `DigitalReorganizer`. Factor the existing bit-mangling body into one
      static helper and return `Mangle(input) - Mangle(0.0f)`.
      - **One definition of the mangle math, two call sites.** Do not copy the expression.
      - **Compute the correction, do not cache it.** `flip`/`hashBits` are public fields; an offset
        updated only inside `SetFlip`/`SetHash` goes stale on any direct assignment. A handful of
        integer ops per sample buys determinism.
      - **Do not touch `src/core/PolynomialDrive.hpp`.** It is frozen. This is a documented
        divergence in the port, and it gets the same in-code divergence note the comb's
        `±1.1 → ±0.95` and the peak's `10× → 4× → 2×` carry.
      - **`app/FroggersDspParityTests.cpp` will go red at the `DigitalReorganizer` cases with
        nonzero flip/hash. That is the divergence, not a regression** — the one sanctioned
        exception to "any red is a regression". Re-assert those cases against the new behaviour in
        the same commit; never delete them. The pass-through configuration
        (`flip == 0, hashBits == 0`) is provably unaffected — the correction term is exactly 0
        there — so the documented `Process(1.0f) == 1.0f` property must stay green untouched. If it
        does not, stop: something else is wrong.
      - Re-run S1.2 and record the number.

---

## S1a — Transport-gating the modulation. OPERATOR-ORDERED, and NOT the F3 fix.

- [x] **S1a.1 — OPERATOR DECISION TAKEN 2026-08-07: modulation must NOT free-run while stopped.**
      Verbatim: *"no, modulation should not free-run while stopped lol. come on."*

- [x] **S1a.2 (DONE) — Gate `modulation_.Step()` on the transport.**
      **Corrected by the audit — the previous revision's supporting argument was wrong in a way
      that matters, and its conclusion still holds:**
      - The call site is `modulation_.Step(vcoDrive(0), …, transportQuarterNotes)` in
        `FroggersAppCore::ProcessBlock`'s per-sample loop. Gating is
        `if (transportQuarterNotes.has_value())` around it — **VERIFIED: that is exactly the
        predicate the ASR gate above already uses**, and `transportRunningNow` is already derived
        from it.
      - **WRONG in the previous revision:** *"what kept running was the random S&H lanes and the
        VCO audio sources."* `StepClockDrivenLanes` **is** what ticks `randomShLanes_[0..4]`; its
        `has_value()` guard already stops their `Increment()`. What it does not stop is
        `RandomShLane::Process()`, a one-pole glide that keeps sliding toward the now-fixed target
        for ~2–4 samples (fast lanes) to ~8–10 ms (lane 5) and is then bit-exact constant.
      - **The genuinely free-running set is 8 slots, VERIFIED:** ganged random LFO #6
        (`kModSlotRandomSh6`, tempo-proportional by design), `kModSlotVco1/2/3Audio`,
        `kModSlotVco1/2/3Ef`, `kModSlotNoise`. Random S&H 1–5 are transient-then-frozen. Cite this
        set, not the old one.
      - **Sources hold rather than reset — VERIFIED.** `RegisterSources()` hands Sheaf raw pointers
        to member variables and `Modulators::UpdateModValues()` dereferences them every sample via
        the ungated `parameters_.ProcessSample()`. There is no neutral-reset path. Held is what
        "not free-running" means.
      - **The UI concern is settled and does not block.** `modulation_.PublishUiState()` is
        unconditional, once per block, outside the per-sample loop; it republishes already-held
        state. Visualizers show a held value, not darkness.
      - **`parameters_.ProcessSample()` stays ungated** (B7.5.0: a `SceneCenter` write only reaches
        the DSP through its periodic smoothed `Compute()`, so gating it would freeze knob edits and
        Randomize All until Play). Only the sources stop.
      - The one non-source side effect of `Step()` is `SetExternalAudioConnected`, and
        `kExternalAudioOptedIn` is hardcoded `false`, so it is a standing no-op today. Harmless.

      **Label it accurately in the commit: this is operator-ordered behaviour, NOT the F3 fix.**
      A static DC seed is a pure function of frozen knob state; freezing modulation cannot remove
      it. F3 is fixed by S1.3.

      **Verification:** the suite plus the S1.2 rig. Do not claim the `F3DIAG` capture as the gate
      for this task — after S1.3 lands, the capture goes quiet for a reason that has nothing to do
      with this change.

---

## S2 — Narrow slew on recursive-loop coefficients — **DROPPED BY OPERATOR 2026-08-07**

> **OPERATOR DECISION: do not implement.** After S1.3 fixed the blowout, the operator weighed the
> slew's cost and withdrew it: *"oh right, audio rate. never mind, we good."*
>
> **The reason, so nobody re-proposes it:** a slew is a lowpass on the coefficient, so on every
> parameter it touches, audio-rate modulation stops producing sidebands and starts producing a
> smoothed contour. On comb/delay feedback and peak Q that character IS the parametric sound the
> instrument is for. The operator's earlier ruling — *"obviously we are not throwing out audio
> modulation baby with the slew bathwater"* — turns out to rule the slew itself out on the
> coefficients that matter, once the blowout is no longer severe enough to justify the trade.
>
> Two secondary costs, recorded because they were part of the decision: there is no single correct
> time constant (this app already carries three smoothers ~128× apart with an in-code warning
> against reasoning from one to another, so S2 was seven separate by-ear tunings, not one), and
> seven slew instances would have to join `ForEachStatefulUnit`'s enumeration or become un-reset
> state surviving Stop — the same shape as the bug S1.3 just removed.
>
> **Residual, NOT scheduled, recorded so it is not lost:** the operator still hears *"big resonance
> sweeps and clipping clicks."* Sweeps are a RANGE problem, not a RATE problem — bounding the
> resonance is the cheaper lever than slowing it. The clicks are a different mechanism again and a
> slew would not have fixed them: `SanitizeOutputSample` ends in
> `std::clamp(outputLimiter_.Process(x) * kMakeUpGain, -1.0f, 1.0f)` — VERIFIED by reading — so any
> transient faster than the limiter's attack is squared off by a hard clamp. That clamp is where to
> look first if the clicks ever become worth chasing.

*Original scope below, kept for derivation only. Do not execute it.*

> *"why don't you just apply the saturation/limiter/smoothing/whatever to each parameter rather
> than each page/bank bus"* … *"the limiter has to be applied in that recursive loop! duh!
> obviously we are not throwing out audio modulation baby with the slew bathwater."*

- [ ] **S2.1 — The corrected coefficient enumeration.** The previous revision's list was
      self-described as UNVERIFIED and was wrong in three ways. Audited result:
      - **Confirmed loop gains:** comb Feedback `(Filter,5)`; delay Feedback `(Delay,2)`.
      - **Confirmed, but named wrong:** "peak Q" is `SetWidth` = `(Filter,3)`, not `SetHeight`.
        Note `bumpWidth`/`bumpFreq` are **shared with `scoopNotch`** — one slewed value feeds two
        stages.
      - **Refuted:** delay **Send** `(Delay,1)` is not a loop gain. `inSignal = bumpIn * send`
        scales the input *into* the loop.
      - **Incomplete:** reverb **Hold** `(Reverb,8)` is only half of it —
        `fb = decayFb + (1−decayFb)·min(hold,0.999)`, so **Decay `(Reverb,2)` is co-equal** and
        dominant at Hold = 0.
      - **Missed entirely:** comb LP `(Filter,6)` (feeds the lowpass *inside* the comb loop), delay
        Width `(Delay,3)` and reverb Diffusion `(Reverb,6)` (both cross-feed coefficients inside
        their loops), and Drive Phase `(Drive,8)`, the allpass coefficient — a different bank the
        old list never considered.
      - **Cannot be classified as one thing:** `(Filter,8)` "Scoop" feeds both a loop-adjacent
        biquad coefficient and a pure output blend, through two separate reads.
      - **VCO pitch / shape / PM stay excluded.** Audio-rate modulation there is a feature.
- [ ] **S2.2 — Placement. The previous revision's placement was structurally unsound; do not use
      it.** It said to put the slew in `RouteAudioSample`'s `knob()` lambda as "one definition
      site." Audited: `knob()` is called **55 times per sample**, and `(Filter,8)` is read
      **twice** — a slew whose state advances per *call* would double-step exactly that parameter.
      `ProcessBlock`'s `vcoDrive` lambda is a second per-sample post-modulation reader outside
      `knob()` entirely, so "single point" is false as stated.
      **Put the slew at each coefficient's own computation site**, reusing the existing
      `dsp::OnePoleLowPass` type. The codebase already does exactly this three times
      (`combTrimSmoother`, `peakTrimSmoother`, `DriveBlendPhase::coeffSmoother`) with
      independently-measured time constants ~128× apart — which is affirmative evidence that one
      shared time constant at one shared site is wrong, not merely inconvenient. **One shared
      smoother type, per-site instances and per-site constants: that is single-sourced math (§8)
      without a false merge (§8's "classify every hit before changing any").**
- [ ] **S2.3 — Gate it on F2's measurable symptom.** Post-`RequestRandomizeAll()` + Filter Crispy
      max currently measures duty cycle **1.000** (the master never returns to unity), min 0.9667,
      mean 0.9784, range 0.0245 (`app/FroggersLimiterPumpingRepro.cpp`). **Re-measure this after
      S1.3 lands and before touching the slew** — part of that duty cycle may be the DC seed rather
      than parametric pumping, and attributing S1.3's improvement to S2 would repeat this project's
      signature error. Report before/after for both changes separately. **If the slew does not move
      it, stop and report** rather than tuning until it does.
- [ ] **S2.4 — Operator confirms by ear.** Not implementer-closable: the slew trades modulation
      liveliness for stability.

---

## S2a — Reverb tank has no in-loop saturator (latent, structural)

- [x] **S2a.1 (DONE 2026-08-07) — OPERATOR DECISION TAKEN 2026-08-07: implement it.** Verbatim: *"reverb tank should
      have in-loop saturator anyway, the same one, for omni rule purposes, but i dont think it will
      ever be necessary."* **"The same one"** is binding: reuse `dsp::PadeSaturator` in the same
      in-loop position the delay uses, so the two units read as one concept (OMNI §8). The operator
      expects it never to engage; that is fine — it is a structural guard, not a tone change being
      sought. No longer deferred, no longer needs ears before landing.
      *Original framing, kept for the record:* `dsp::StereoDelay` got an in-loop
      `PadeSaturator` (fix B2, per its own comment) precisely because an unsaturated recursive loop
      settles at `in/(1−fbk)`. **Its structural sibling `dsp::Reverb` never got one:**
      `aIn = preOut + aFb * fb` is written unsaturated straight into `lineA`, and the only limiter
      sits after both tanks and the dry/wet mix. With Hold maxed, `1/(1−fb) ≈ 50,000×`.
      Consequence: *any* sustained overdrive from *any* cause pins the output near 1.0 via the
      `0.80 × 1.25` arithmetic, invisibly to the `FiniteOnly` recovery tier.
      **Not required once S1.3 lands** — it removes the amplifier, not this change's seed. It is
      recorded because it is the same concept as B2 with one instance fixed and its sibling missed
      (OMNI §8: duplication is symmetric — go find the sibling). **Landing it changes long reverb
      tails, so it is the operator's call, not an implementer's.**

      **DONE.** `dsp/Reverb.hpp`'s `Process()` now writes
      `preOut + fb * PadeSaturator::Saturate(aFb)` / `...(bFb)` (mirrors B2's
      `fbk * PadeSaturator::Saturate(fbL)` exactly). 3 tests: bounded-vs-
      unbounded under sustained overdrive (WITH saturator maxes at exactly
      the analytic bound `|input|+fb`; WITHOUT, measured via a
      positive-control replica, reaches 86x that in the same run — OMNI
      §9.1), a quiet/ordinary-level tail comparison against the same
      unsaturated replica (retention within ~4% absolute, i.e. essentially
      untouched at ordinary levels), and a loud/Hold-pinned tail check.

      These landed in a dedicated `app/FroggersReverbSaturatorTests.cpp`
      with its own `REVERB_SATURATOR_TEST_BIN`, for one stated reason only:
      `FroggersDspParityTests.cpp` was other in-flight work and this task's
      brief forbade touching it. **That reason has since lapsed and the
      separation was retired** — all 3 tests and both replica fixtures now
      live in `FroggersDspParityTests.cpp`'s reverb section, the separate
      file and Makefile target are deleted, and a duplicated ~60-line
      harness plus a duplicated build recipe went with them (OMNI §8). The
      fold is verified behaviour-preserving: every number all 3 tests print
      is byte-identical before and after, including the tank-math
      single-sourcing described below.
      **§14 postflight (OMNI §8 by operands — `PadeSaturator`/`Saturate`/
      feedback writes into a delay line):** exactly 3 recursive
      delay-line-feedback loops exist in the whole tree — comb
      (`FilterFx.hpp`, pre-existing), delay (`Delay.hpp`, fix B2,
      pre-existing), reverb (this task). All 3 now carry the saturator; no
      4th instance found (`ResonantBump`'s peak/scoopNotch biquads are a
      different concept — RBJ pole coefficients unconditionally stable by
      construction, not a knob-driven gain pushed toward 1 — correctly out
      of scope, not a missed instance).

      **§14 postflight re-run against the fold's own diff (OMNI §8 by
      operands — `lineA`/`lineB`/`preLine`/`indexA` tank state):** 3 sites
      restate the reverb tank's recursion. `PreFixReverbReplica` was a
      verbatim copy of `UnsaturatedTankReplica`'s tank half plus the output
      stages, so it now *contains* an `UnsaturatedTankReplica` and consumes
      the `(valA, valB)` its `Step` returns — one definition site, and the
      reorder is sound because `dA`/`dB` clamp to ≥1, so a sample's writes
      can never disturb that same sample's reads. The 3rd site,
      `reverb_process_matches_manual_tank_replica_at_neutral_mod_and_hold`,
      is deliberately NOT merged (OMNI §8: classify before merging) — it
      re-derives the tank from `FroggersEngine.hpp`'s raw `ExpMapCompute`
      calls precisely so it does *not* route through `dsp::Reverb`'s own
      static helpers; its independence from them **is** its assertion, and
      a shared fixture would gut it. Found 3, merged 2, left 1 with cause.

      **IMPORTANT, found by measurement, reported not hidden — SINCE
      RESOLVED, see below:** this fix made `FroggersDspParityTests.cpp`'s own
      `reverb_hold_at_max_tail_still_sustains_after_wet_limiter_is_added`
      go red (`peakAfterSilenceRun` 0.379×`peakAfterExcite`, under that
      test's `>0.5×` bar) at its exact scenario (full-scale, 4000-sample
      sustained burst, Hold=Decay=max). Root cause, measured: PRE-fix at
      that scenario the OUTPUT sits pinned at the wetLimiter ceiling for
      the entire 10000-sample silence window (retention ≈1.000 — not a
      decaying tail, the unbounded-loop defect itself, reconstructed and
      printed by this task's own `PreFixReverbReplica`). POST-fix it is a
      genuine, gradually-decaying, still-clearly-audible tail
      (0.8 → 0.374 → 0.302 over the same window) — correct reverb behaviour
      replacing a pinned artifact, not a regression, but the pre-existing
      test's threshold was calibrated to the old regime and was stale.
      *It was left untouched at the time per this task's explicit brief
      ("leave `FroggersDspParityTests.cpp` alone") — flagged here instead
      of silently fixed.*

      **RESOLVED (2026-08-07), once that file was no longer in flight.** The
      stale test was not merely widened — widening it would still have left
      the bar pointing the wrong way. It is superseded by
      `reverb_hold_at_max_tail_stays_audible_and_decays_gradually_not_pinned`
      (same file, same scenario), which merges B6b's persistence guard and
      S2a.1's loud-tail guard into one TEST_CASE — they were the same
      measurement on the same signal, so keeping both would have been
      sequential duplication (OMNI §8). It asserts what a real reverb tail
      actually is, all thresholds measured with margin, none tuned to just
      barely pass: **(1)** still clearly audible long after the burst
      (`>0.1` absolute; measured 0.302); **(2)** declining *gradually* across
      two checkpoints rather than off a cliff (`>0.3×` the previous
      checkpoint each; measured 0.467 and 0.807) — a start/end check alone
      would pass an on/off gate; **(3)** genuinely *decaying* rather than
      pinned (`<0.8×` the post-burst peak; measured 0.377) — the direction
      the old bar had backwards, and the one that stops (1) and (2) from
      going vacuous if the tank ever regresses to riding the limiter.
      **(3) is kept honest by OMNI §9.1:** `PreFixReverbReplica` runs in
      lockstep and is *asserted* to exhibit the pinned regime
      (`>0.9×` retention; measured 1.000), not merely printed as the
      standalone version did — so the run demonstrably could have caught it.
      **Mutation-verified, not just reasoned about:** reverting the two
      `Saturate` calls in `dsp/Reverb.hpp` and rebuilding turns the suite red
      on assertion (3) specifically (and on the bounded-tank test), then
      restores byte-identically. Suite: **77/77**, up from 74/75.

---

## S5 — REGRESSIONS: two features recorded as LANDED that do not work

**Operator-reported 2026-08-07, testing the built app.** Both were listed in
`SUPERSESSION-RECORD.md` §1 as delivered, with commits attached. Neither works. **This is the
predecessor's exact failure mode — shipped, claimed success, did not work — and it survived into
the successor's own "what actually landed" table.** The lesson for this directory: a commit sha in
a landed-table is not evidence, and the audit that rewrote these artifacts did not re-verify the
inherited claims either. It should have.

- [x] **S5.1 (DONE) — Drilldown `Back()` pops ALL levels instead of one.** Operator: *"the drilldown back
      button still doesn't go one back, it goes all the way back."* Recorded as landed in `49ce9af`
      / `9d0802c` (`kMaxDrillLevel`; `wasLevelTwo`/`level1Encoder_` removed).
- [x] **S5.2 (DONE) — The `Modulation Level N` header is not visible.** Operator: *"i still don't see a
      header label counting the drilldown levels."* Recorded as landed in `3a9e8c5`.
- [x] **S5.3 (DONE) — For BOTH: check whether the claimed commit did what its message says**, and whether
      a later commit reverted or overwrote it. Report landed / partial / overwritten per commit.
      Fix at the real root cause; do not add a second mechanism beside a broken one.

## S6 — Encoder ring collides with the parameter card's border

- [x] **S6.1 (DONE)** Operator-reported 2026-08-07 with a screenshot: the card's rounded-rect outline
      passes through the red modulation ring drawn around the encoder. Operator's read: *"i doubt
      this is a sheaf issue, it is probably an app side omni rule violation."* Treat as a
      hypothesis, not a fact — but `External/Sheaf` is pinned and unpatchable, so if the geometry
      originates upstream the app compensates and the need is filed to `/UPSTREAM-SHEAF-ASK.md`.
      **Give the numeric collision** (ring outer edge vs card inner bound, from the real
      constants), not a qualitative "they overlap". **Must not disturb the 6×6 grid.**

## S3 — C2: three-VCO sequential duplication (deferred on purpose)

- [x] **S3.1 (DONE)** `app/FroggersAppCore.hpp` processes `audioVco1_/2_/3_` in three structurally
      identical statements differing only by index (`0,3,6` / `1,4,7` / `2,5,8`), and the same
      `(i, +3, +6)` grouping appears a fourth time in `ProcessBlock`'s `vcoDrive` lambda — **both
      VERIFIED by reading.** `std::array<dsp::Vco, 3>` collapses all of it to a loop.
      **Check first whether `dsp::Vco` is copy/movable.**
      **Deferred past operator testing deliberately** — it is the only real refactor left and it
      touches the audio path.

---

## S4 — F6: operator verification (closes the change)

**One build, one pass, after everything above lands.** Nothing here is implementer-closable.

- [x] **S4.1 — PASSED 2026-08-07, operator by ear + measured.** Operator: *"STOP WORKS! :))))"*
      Corroborated by an `FROGG3RS_STOP_DIAG=1` capture of that same session: of 7491 post-flush
      blocks, **7100 read exactly 0**, 317 fall in 0.001–0.1 (decay tails), and the 74 above 0.1
      are the first block or two after each Stop press — the legitimate tail of what was ringing.
      The first window peaks at 0.032 then holds exact zero. Compare the pre-fix capture in this
      directory: **1495 of 1495 blocks pinned at 0.999999, never once below 0.1.**
- [x] **S4.2 — PASSED 2026-08-07, operator by ear.** Operator: *"WE HAVE FIXED THE BLOWOUT ISSUE."*
      Note this passed **without S2's slew**, which was aimed at parametric pumping — further
      evidence the DC seed, not modulation, was the dominant mechanism in F2 as well as F3.
      **Re-scope S2 accordingly before implementing it** (see S2.3: take the baseline first).
- [ ] **S4.3** Randomize All inside a level-1 drilldown leaves the operator where they were, with
      badges changing on that page. **Visually.**
- [ ] **S4.4** Badge density reads as mode 2 at every level, and a drilled parameter shows only
      sources that are actually modulating. **Visually.**
- [ ] **S4.5 — NOTE THE PLACEMENT CHANGED 2026-08-08.** The level indicator is on the **Target/Back
      cell**, reading `BACK L<N>`, NOT on the VCO scope cell. It lived on the scope cell through two
      failed fixes: `kVcoScope` resolves to `{16, 16, 284.67, 181.33}` in the LEFT block while the
      modulation grid resolves to `{314.67, 16, 569.33, 600}` in `kRightBlock` — disjoint columns,
      14px apart. It rendered correctly the entire time, in a column the operator never looks at
      while reading the grid. **Do not move it back.** Original wording follows:
      Drilldown reaches level 3, `Back()` pops one level at a time, and each level shows
      its `Modulation Level N` header. **Visually.**
- [ ] **S4.6** Sustain at minimum is quiet but audible, and audio-rate modulation of an envelope
      parameter no longer gates a voice to silence. **By ear** — `kMinSustainLevel = 0.05` was
      chosen by argument, not by measurement.
- [ ] **S4.7** Saved patches still load from `~/Library/Sheaf/synth/sheaf-patch/` and are not
      rewritten by any default change.
- [ ] **S4.8 — NEW, and the one that most needs ears:** the Drive bank's **XOR (Flip)** and **Bit
      depth (Hash)** knobs still sound right after S1.3. They previously injected a DC offset that
      the delay and reverb amplified; that offset is gone. **This is a deliberate parity divergence
      from the frozen firmware** and the operator has never heard the corrected version.
