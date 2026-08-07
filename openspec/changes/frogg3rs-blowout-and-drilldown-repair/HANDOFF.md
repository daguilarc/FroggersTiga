# Handoff — `frogg3rs-blowout-and-drilldown-repair`

**Written 2026-08-07 at session end. Nothing has gone to the operator yet** (M4: one build, one
listening pass, after the whole list lands).

**Read `proposal.md`'s Method constraints M1–M7 before your first dispatch.** They are not style
guidance; each is a specific way the predecessor failed while measuring everything green.

---

## 0. HOW THIS SESSION FAILED — behavioural lessons first

The predecessor's own handoff led with these because they outrank every technical finding. This
session earned its own set, and they are all one shape.

**0.1 — The lead held CODE claims to M1 and did not hold its own SPEC TEXT to the same standard.**
Five spec errors, each written into a brief from memory of the code rather than from a read, each
caught only downstream by a compiler or a measurement:

| error | caught by |
|---|---|
| `PeakMagnitude()` — function does not exist, it is `PeakAbs()` | compiler |
| `rig.Application().Context().parameterManager` — `context_` is private, no such accessor | compiler |
| `PageParameter(Filter, 14)` — throws; `pageParameters_` is 9 wide, Crispy is a separate array | runtime |
| `kModSlotNoise` cited against §K.1's **50.5× periodic** figure — noise is random, can only reach 1.002 | measurement |
| "do not relocate the timers" — an instruction the structure could not satisfy | compiler + review |

**The omni rule's §1 trace clause has been refined with this lesson** (2026-08-07): the plan's own
text is a set of claims and gets the same standard. Every identifier, signature, snippet, index and
API route is verified by reading at write time or explicitly marked UNVERIFIED. **A structural
instruction is itself a claim** — that the structure admits a good solution under that constraint.

**0.2 — Executors should challenge an instruction that forces a worse structure.** F3.3's first
attempt honoured "do not relocate the timers" and produced a second ten-entry table keyed by
`const void*`, linear-scanned per unit per block — reintroducing the exact §8 duplication the task
existed to delete. **That was the instruction's fault, not the implementer's.** When honouring an
instruction requires adding a definition site, a lookup, or a branch, stop and challenge it.

**0.3 — The one thing that worked: implementers treating spec-vs-code contradictions as stop
conditions.** Every one of the five errors above was caught that way, before landing. Keep it.
Ask every dispatch to report contradictions, and mean it.

**0.4 — Do not fish, in either direction.** B7.5 passed when it had to fail; the implementer
halted instead of hardening the patch until it went red. That is as important as not weakening a
test to make it pass. Both are the same defect.

**0.5 — Line citations rot.** Every citation in the F4 block drifted during this session and two
pointed at unrelated code. **Cite by symbol** (`grep -n "drillIn.Back()" …`); if you must use a
line number, pin it to a commit.

---

## 1. STATE — what is true right now

**Suite: ten binaries, all green except two deliberately-red acceptance-gate tests.** Verified
directly by the lead after `1c37657`, not taken from a report.

### Done and verified
| | evidence |
|---|---|
| **B7.5.0** patch-application anomaly — CLOSED, no defect | Writes reach the DSP through a smoothed periodic `Compute()`; ~81 % applied after one block, converged after ~30. Explains the predecessor's bit-identical P2/P4 anomaly with no defect. **The 85 `SceneCenter(0) =` sites are valid.** |
| **B7.5** acceptance gate — two end-to-end tests, both RED by design | `master_limiter_stays_at_unity_across_hostile_patch` and `…_under_live_modulation`, failing on `minEnvelopeSeen > 0.999f` with `PeakAbs` liveness passing |
| **F0** preflight remediation, 6/6 | Short-circuit sites hoisted; two zero-`TEST_CASE` files renamed `*Repro.cpp`; stale peak ceiling fixed; two stale comments retracted; settling sweep found 0 of 85 needing change |
| **F3.1** Stop measurement | **Refuted the traced hypothesis** — see §2 |
| **F3.3** enumeration fix (`1c37657`) | Hierarchical `ForEachStatefulUnit`, compile-time tier tags, timers relocated into units, new `dsp/RecoveryTier.hpp`. **NOT a Stop fix** |
| **F8** omni sweep | Code findings C1–C3; F4 citations corrected |

### Red by design — do not "fix"
Both acceptance-gate tests fail on `minEnvelopeSeen > 0.999f`. **They are supposed to.** They go
green when F2.1's ceiling retarget lands, and that is the only proof the architecture works in the
binary rather than on paper. Report them separately from any real regression.

### Open
F4+F5 · F1 · F3.2c · F2.0 → F2.1+ · C1–C3 · F7 · F6.

---

## 2. THE TWO HARD BUGS — root causes, honestly stated

### F3 — Stop does not stop

**My traced hypothesis was REFUTED by measurement.** The Stop flush clears 2 of 14 stateful units
and is one-shot; I predicted the comb and `DriveBlendPhase` would re-excite delay/reverb past it.
F3.1 measured otherwise:

| | pre-Stop | t+0.1 s | t+1 s | t+30 s |
|---|---|---|---|---|
| output peak | **0.840133** | 1.28e-05 | 1.64e-10 | **0** |
| `filterChain.comb` | — | 0.995679 | 0.003619 | 3.74e-16 |

Loud before, silent after, everything decays. **On a static patch, Stop stops.**

I also misread `driveBlendPhase_`'s flat **0.98** as a stuck resonator. It is
`coeffSmoother.output`, seeded to `-0.98f` by `Reset()` — **a coefficient, not signal energy.**

**LEADING HYPOTHESIS (untested): parametric oscillation.** `modulation_.Step(...)` and
`parameters_.ProcessSample(...)` are called **unconditionally** in the per-sample loop
(`FroggersAppCore.hpp:717,720`) — transport state is passed *into* `Step`, it does not gate the
call. So after Stop the ASR gate silences the VCOs (**scope flat**) while every modulation source
keeps sweeping every modulated parameter at audio rate, comb feedback included. A feedback loop
whose gain is *modulated* is time-varying and can self-sustain even when every instantaneous
`|fb| < 1`. §K.1 already measured this phenomenon in another stage (`DriveBlendPhase`: 1.002 free
random, **50.5× periodic**) without naming it.

It accounts for every element of the report — still scope, continuing audio, harshness, duration,
and why it only follows a randomize (that is what puts depths on Filter params).

**→ F3.2c is the decisive measurement. Read the trap box first: F3.3 may MASK this** by zeroing
the comb at Stop, curing the symptom while leaving the cause — which then still drives F2.

### F2 — Filter Crispy at max blows out

**Operator evidence, 2026-08-07:** *"crispy blowout is specific to the filter bank. usually
rhythmic"* and *"it can get pretty rhythmic but still pretty darn loud the whole time."*

That is **not** pump-and-recover. It is continuous heavy gain reduction with periodic deeper dips —
the master never returns to unity, and the quarter-note ASR gate modulates how far below it sits.
Which is exactly what this plan's §1 note predicted and nobody had confirmed by ear.

**But the harness measures only 0.12 dB of reduction on the most hostile patch it can build**, and
that number does not move across three modulation regimes (static 0.985796, noise 0.985726,
VCO1-audio 0.985954). Inaudible. So the harness is missing the operator's condition — **F2.0 is
blocking**: measure duty cycle and range (not a minimum — pumping is *variation*), and drive the
rig through `RequestRandomizeAll()` rather than five static knobs.

**F2 and F3 may be the same bug**: the same parametrically-pumped comb would hold the level up
between gate pulses (the loud floor) with the gate riding on top (the rhythm), and it is
Filter-specific because only there does Crispy reach the comb/peak resonant maxima.

---

## 3. SEQUENCING TRAPS ALREADY PAID FOR

1. **F3.3 masks F3.** Resetting all 14 units at Stop zeroes the comb; a parametric oscillator with
   zero state has nothing to amplify. Symptom may vanish, cause intact, F2 left looking unrelated.
   **Never describe F3.3 as fixing F3.**
2. **Never measure against a half-applied refactor.** The lead dispatched F3.2c while F3.3 was
   mid-edit and killed the run. A confident number from a moving tree is worse than no number.
3. **Code changes are sequential (§0).** F4+F5 correctly refused to start while `app/` was dirty.
   Honour that; do not edit around uncommitted work.
4. **Agents that spawn a build subagent and end their turn stall the queue.** Three did.
   Run the build in your own turn so you can act on the result.

---

## 4. NEXT DISPATCHES, in order

1. **F4+F5** — delete both `PressEncoder`/`Back()` round trips (`:1134`/`:1142`, `:1154`/`:1176`,
   and the final bare `Back()` at `:1178` — **re-verify by symbol first**), then the
   `kMaxDrillLevel` + `levelEncoders_` array refactor at max=2, then raise to 3. Report measured
   allocation at each step (expect 240 → ~45 → ~105).
2. **F1** — measure the count histogram three ways before touching the draw; the cause was
   asserted wrongly twice and the operator caught it both times. Then re-derive for mode 2.
3. **F3.2c** — the parametric measurement, with its stated refutation condition.
4. **F2.0** → **F2.1+** — measure, then retarget ceilings to `C = 0.80`. **Trap:** a stage whose
   threshold equals `C` gives `headroom == 0` and `DesiredMagnitude` evaluates `0/0`.
5. **C1–C3** — sweep fixes. C1 needs a §12 origin trace first.
6. **F7**, then **F6** — one build, one listening pass.

**What the operator can do in parallel, and it outranks all of the above:** save a patch that
reproduces either bug and hand over the file from `~/Library/Sheaf/synth/sheaf-patch/`. Every dead
end this session came from constructing a patch we *think* resembles theirs. A real one turns both
open bugs from guesswork into measurement.
