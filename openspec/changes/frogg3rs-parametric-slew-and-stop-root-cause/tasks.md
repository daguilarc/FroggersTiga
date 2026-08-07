# Tasks — `frogg3rs-parametric-slew-and-stop-root-cause`

> **Read `SUPERSESSION-RECORD.md` first, all of §0 and §2.** §0 lists six lead errors from the
> session that produced this change, three of which the operator caught rather than the lead.
> §2 carries the measured F3 root cause. **Every claim in this directory that is not marked
> MEASURED should be re-read against the code before you rely on it** — that is the specific
> failure mode this project keeps repeating.

**Goal:** make the instrument stop when Stop is pressed, by fixing the mechanism that keeps it
running — a feedback loop whose gain is modulated at audio rate — rather than by resetting more
state. Then verify the whole list with the operator.

**Suite as inherited: 10 binaries, 183 tests, 0 failures, 0 warnings.** There are no
expected-red tests. **Any red is a regression.** `External/Sheaf` pinned at `77a3019e`.

## §0 Standing constraints (binding, carried forward unchanged)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **Builds emit nothing for ~70 s and look identical to a hang.** Run them in the background with
  a visible progress tick, not foreground-blocking. The operator stopped two runs this session
  believing they had stalled, and was right to.
- **A dispatched subagent that ends its turn with a build still running stalls the queue.** One
  did. Run the build in your own turn so you can act on the result.
- **`External/Sheaf` is pinned and unpatchable.** Needs go to `/UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose spec requires operator eyes or ears.**
- **Cite by SYMBOL, not by line.** Every line number in this directory is stale the moment
  anything above it changes.
- **A negative result requires a positive control** (OMNI §9.1, added 2026-08-07): before
  recording "X did not happen," print the number proving the setup could have produced X. Two
  measurements in the predecessor were void for exactly this and one was written up as a
  refutation.

---

## S1 — F3: the modulated feedback loop self-oscillates. ROOT CAUSE MEASURED.

**See `SUPERSESSION-RECORD.md` §2 for the `F3DIAG` capture.** Summary: the Stop flush fires
correctly (`allIdle=1`, `clearPending=0` from block 6), all 14 units are reset, and the output
then climbs from ~0.42 to the clamp in ~0.4 s and stays at 0.999999 for the remaining ~1495
blocks, never once dropping below 0.1.

**This is not a Stop bug. It is a stability bug that Stop merely exposes**, because Stop is the
only time you notice a self-sustaining loop with no player input. The same instability is the
leading explanation for F2's blowout during play.

- [ ] **S1.1 — Find the seed. §1 trace, no fix yet.**
      A zeroed loop with a genuinely zero input stays zero, so something in the chain emits
      nonzero from silence with the gate closed. **UNVERIFIED lead, do not treat as fact:**
      `drive_.digitalReorganizer`'s bit-level `SetFlip`/`SetHash` have no reason to map `0.0f` to
      `0.0f` — scrambling the bits of a zero float yields a nonzero float. Trace every stage
      between the gate and the delay input and report which ones are non-silent on a zero input.
      Report as a table of stage → output magnitude for a zero input, measured, not reasoned.
- [ ] **S1.2 — Reproduce it in the harness.** The harness has never reproduced F3 (F3.2c and
      F3.2d both measured exactly 0) because the rig's post-Stop chain supplies no seed. Once
      S1.1 names the seed, `app/FroggersStopFlushRepro.cpp` should reproduce the climb-to-clamp.
      **A failing harness case is the gate for any fix** — the predecessor shipped five green
      per-stage bound tests that moved no symptom.
      **Liveness is mandatory:** assert the loop coefficient actually sweeps (the first F3.2c run
      was void for exactly this and its refutation had to be retracted).
- [ ] **S1.3 — Fix via S2's slew, not via more resetting.** Do not add another unit to the Stop
      flush; the flush already resets everything and it did not help. Land S2 first, then re-run
      S1.2 and record the number.

---

## S1a — TRANSPORT-GATING THE MODULATION. Operator question, DECISION NOT TAKEN.

**Added 2026-08-07 after the operator asked "so the modulation was never transport gated, does
this plan and handoff address how to fix that?" — and it did not. That was an omission in the
first draft of this plan**, which went straight from the measured root cause to S2's slew without
ever considering the most direct lever. Recorded here rather than silently decided.

**The fact, verified:** `modulation_.Step(...)` and `parameters_.ProcessSample(...)` are both
called unconditionally in the per-sample loop (`grep -n "modulation_.Step" app/FroggersAppCore.hpp`).
Transport state is passed *into* `Step` as an argument; it does not gate the call. So after Stop
the ASR gate silences the VCOs while every modulation source keeps sweeping every modulated
parameter at audio rate — which is precisely the energy source the `F3DIAG` capture shows
re-pumping a fully reset chain to full scale.

**The two calls are NOT equivalent and must not be gated together:**

- **`parameters_.ProcessSample()` must NOT be gated.** It is what applies patch changes at all —
  B7.5.0 established that a `SceneCenter` write only reaches the DSP through this call's periodic
  smoothed `Compute()`. Gating it would freeze patch application whenever the transport is
  stopped, so knob edits and Randomize All would appear to do nothing until Play. That is a worse
  bug than the one being fixed.
- **`modulation_.Step()` is the gateable one** — but it also drives the UI visualizers and the
  modulation-source displays, so freezing it may simply make the instrument look dead while
  stopped. **Trace what reads modulation state for display before gating it.**

**Why this is a MASKING fix, and must be labelled as one:** a feedback loop whose gain is swept
at audio rate is unstable *whether or not the transport is running*. Gating removes the symptom
after Stop and **leaves F2's blowout during play completely untouched** — where the operator
actually hears it. This is structurally the same move as F3.3, which reset all 14 units, made the
Stop symptom vanish in the harness, and left the cause intact. **This project has already paid
for that mistake once; do not describe transport-gating as fixing F3.**

- [x] **S1a.1 — OPERATOR DECISION TAKEN 2026-08-07: modulation must NOT free-run while stopped.**
      Verbatim: *"no, modulation should not free-run while stopped lol. come on."* Not open;
      implement it.

- [ ] **S1a.2 — Gate `modulation_.Step()` on the transport. NOT YET IMPLEMENTED.**
      **Groundwork already traced by the lead — verified by reading, reuse it rather than
      re-deriving:**
      - The call site is `modulation_.Step(vcoDrive(0), …, transportQuarterNotes)` in
        `FroggersAppCore::ProcessBlock`'s per-sample loop
        (`grep -n "modulation_.Step" app/FroggersAppCore.hpp`). Transport state is passed *in* as
        an argument today; it does not gate the call. Gating is `if (transportQuarterNotes.has_value())`
        around it — the same predicate the ASR gate above already uses.
      - **`Step()`'s own clock-driven lanes ALREADY do this.** `StepClockDrivenLanes` returns early
        on `!transportQuarterNotes.has_value()` — its comment reads *"transport not running / no
        clock plan: no tick, gate-closed-equivalent."* What kept running was the rest: the random
        S&H lanes and the VCO audio sources — i.e. exactly what feeds `kModSlotVco1Audio`, the
        audio-rate source that pumps the loops. **This change makes the rest of `Step()`
        consistent with what its own clock lanes already did.**
      - **The UI concern is settled, it does not block:** `modulation_.PublishUiState()` is called
        once per block **independently of `Step()`**, so gating does not stop UI publication. The
        visualizers show a held state rather than going dark.
      - **`parameters_.ProcessSample()` must stay ungated** (B7.5.0: a `SceneCenter` write only
        reaches the DSP through its periodic smoothed `Compute()`, so gating it would freeze knob
        edits and Randomize All until Play). Only the SOURCES stop.
      - Sources will hold their last value rather than resetting to neutral. That is what "not
        free-running" means, and it is sufficient for F3: a held coefficient leaves the loop
        STATIC, and a static `fbk <= 0.98` decays by construction.

      **Still label it accurately in the commit: this removes the post-Stop symptom and leaves
      F2's blowout DURING PLAY untouched**, because a loop whose gain is swept at audio rate is
      unstable whether or not the transport runs. Same shape as F3.3, which reset all 14 units and
      cured nothing. The causal fix is S2.

      **Verification must be the `F3DIAG` capture, not the suite.** The harness has never
      reproduced F3, so a green suite proves nothing here. Re-run with `FROGG3RS_STOP_DIAG=1` and
      the operator's reproducing patch; the peak must decay after the flush instead of climbing to
      0.999999 and pinning there. Compare against
      `F3DIAG-capture-2026-08-07.txt` in this directory.

---

## S2 — Narrow per-parameter slew on recursive-loop coefficients

**Operator's design ruling, agreed 2026-08-07, not yet implemented.** Parametric pumping happens
because a *coefficient changes at audio rate*; per-stage limiters only bound the level after the
pumping has already happened. Slew-limit the coefficient and the mechanism itself goes away —
for F2's blowout and F3's sustain both.

**Operator, verbatim, and both halves bind:**
> *"why don't you just apply the saturation/limiter/smoothing/whatever to each parameter rather
> than each page/bank bus"* … *"the limiter has to be applied in that recursive loop! duh!
> obviously we are not throwing out audio modulation baby with the slew bathwater."*

- [ ] **S2.1 — NARROW scope only.** Slew ONLY coefficients that set a gain inside a recursive
      loop: comb feedback, delay Feedback and Send, peak Q, reverb Hold. **VCO pitch / shape /
      PM are explicitly excluded** — audio-rate modulation there is a feature, and a blanket
      slew would delete it. Enumerate the loop coefficients by reading the DSP units, not from
      this list; this list is a starting point and is UNVERIFIED as complete.
- [ ] **S2.2 — One definition site.** `RouteAudioSample`'s `knob()` lambda
      (`grep -n "auto knob = " app/FroggersAppCore.hpp`) is the single point where every bank
      parameter is read post-modulation, and every stage reads through it. Put the slew there;
      do not add per-stage smoothers (that is the §8 duplication this project keeps re-creating).
      **No Sheaf change is needed** — `Parameter::GetRaw` applies the modulation term raw per
      sample with no smoothing, which is the actual gap, but it can be handled app-side.
- [ ] **S2.3 — Gate it on F2's measurable symptom.** F3's symptom is not yet reproducible in the
      harness but **F2's is**: post-`RequestRandomizeAll()` + Filter Crispy max measures duty
      cycle **1.000** (the master never returns to unity), min 0.9667, mean 0.9784, range 0.0245
      (`app/FroggersLimiterPumpingRepro.cpp`). If the slew is the right mechanism that duty cycle
      must fall. Report it before and after. **If it does not move, stop and report** rather than
      tuning the slew until it does.
- [ ] **S2.4 — Operator confirms by ear.** Not implementer-closable: the slew trades modulation
      liveliness for stability, and only the operator can judge that trade.

---

## S3 — C2: three-VCO sequential duplication (deferred on purpose)

- [ ] **S3.1** `app/FroggersAppCore.hpp` processes `audioVco1_/2_/3_` in three structurally
      identical statements differing only by index (`0,3,6` / `1,4,7` / `2,5,8`), and the same
      `(i, +3, +6)` grouping appears a fourth time in `ProcessBlock`'s `vcoDrive` lambda.
      `std::array<dsp::Vco, 3>` collapses all of it to a loop and would collapse F3.3's three
      `visit(audioVcoN_, …)` lines too. **Check first whether `dsp::Vco` is copy/movable.**
      **Deferred past operator testing deliberately** — it is the only real refactor left and it
      touches the audio path; landing it into a build under evaluation would put a structural
      change inside the listening pass.

---

## S4 — F6: operator verification (closes the change)

**One build, one pass, after everything above lands.** Nothing here is implementer-closable.

- [ ] **S4.1** Stop silences the instrument immediately, from a patch with delay feedback/send
      modulated and reverb Hold at max. **By ear.** This is the one that has failed twice.
- [ ] **S4.2** Filter Crispy at max no longer blows out. **By ear.**
- [ ] **S4.3** Randomize All inside a level-1 drilldown leaves the operator where they were, with
      badges changing on that page. **Visually.**
- [ ] **S4.4** Badge density reads as mode 2 at every level, and a drilled parameter shows only
      sources that are actually modulating. **Visually.**
- [ ] **S4.5** Drilldown reaches level 3, `Back()` pops one level at a time, and each level shows
      its `Modulation Level N` header. **Visually.**
- [ ] **S4.6** Sustain at minimum is quiet but audible, and audio-rate modulation of an envelope
      parameter no longer gates a voice to silence. **By ear** — `kMinSustainLevel = 0.05` was
      chosen by argument, not by measurement, and the operator has never heard it.
- [ ] **S4.7** Saved patches still load from `~/Library/Sheaf/synth/sheaf-patch/` and are not
      rewritten by any default change.
