# Archive record — `frogg3rs-modulation-truth-and-voicing`, archived 2026-08-06

**Status at archive: superseded, FAILED.** Not "ran out of time" — it shipped, claimed success,
and did not work. The operator ran its final consolidated build on 2026-08-05 and reported four
failures, every one of which this change records as fixed with green tests attached.

**176 tests pass. Four reported symptoms remain.**

Successor: `../../frogg3rs-blowout-and-drilldown-repair/`.

## Why this directory is still worth reading

Two sections of `FAILURE-REPORT-AND-HANDOFF.md` are the most valuable artifact this change
produced, and both have been **carried verbatim in substance** into the successor's
`proposal.md` (§ "Method constraints") so no future agent has to reach into an archive to find
them:

- **§0 — the behavioural lessons.** Seven named failure modes, each with the specific wrong call
  it produced. §0.1 (asserting from reasoning where a direct look was available) is the parent of
  every other one.
- **§1 — the systemic error.** Every measurement taken was on an isolated stage under synthetic
  adversarial input. Not one measured the real signal path end to end. Five stages each pass their
  own bound test while the operator still hears blowouts, because the composite was never measured.

Everything else here is history. The successor is self-contained and does not depend on it.

## What genuinely shipped and is carried forward as working

Operator-confirmed or independently re-verified during the 2026-08-06 audit:

- **Non-additive randomize.** Depths reset between presses (operator-confirmed).
- **Drill-in knob display.** `ComputeAllParameters()` reseed, once, at the end of the audio-thread
  drain (`FroggersAppCore.hpp` `ProcessFrame`). W1.0's root cause — a recursion-depth accident in
  Sheaf's smoothing branch — was correctly diagnosed and correctly fixed app-side with no upstream
  patch.
- **Scene-pair semantics (A3).** One source set per parameter, independent values in both scene
  poles. `kScenePoles` + `static_assert` against `kNumScenes` is a genuine single definition site.
- **Scene-2 mirrored VCO shape defaults** and light cross-coupling in both poles (C1).
- **The limiter consolidation.** `dsp/Limiter.hpp`'s `kSharedCeiling` / `kSharedReleaseSeconds`,
  with per-stage thresholds and attacks deliberately NOT shared because they are measured per
  stage. The file is honest about what did not land (its own B7.1 note).
- **Per-unit fault recovery.** `RecoverPoisonedUnitState` enumerates all thirteen stateful units,
  and every stage's nested limiter participates in its owner's `Reset()`/`StateFinite()`.
- Sheaf pinned at `77a3019e`, clean and unpatched. Frozen trees byte-identical throughout.
- All 15 upstream asks filed (email + `jvictor0/Sheaf` issues #1, #2, #3).

## What did NOT work, stated plainly

The four operator-reported failures carry forward as the successor's F1–F5. Their full technical
state, including two hypotheses that were asserted and then disproved, is in
`FAILURE-REPORT-AND-HANDOFF.md` §2. Do not re-derive it; do not trust it either — §0.1 applies to
reading this change's own conclusions.

**The two items that would have prevented the failure were both specified here and never built:**

- **B7.1** — retarget every stage limiter to the measured shared ceiling `C = 0.80`, with make-up
  gain after the master. `dsp/Limiter.hpp:66-70` records it as outstanding. Without it every stage
  legitimately runs at ~1.0 into a master whose threshold is 0.9, so the master rides continuously
  by construction. This alone could account for the entire F2 symptom.
- **B7.5** — the end-to-end test asserting the master limiter's envelope stays at unity across a
  hostile patch. **The only acceptance criterion that mattered, and it was never written.**

## Landed-but-SUSPECT — measured green in isolation, symptom unresolved

Comb trim `1/(1+|fb|)`, peak trim `1/height`, peak limiter, delay in-loop saturator, delay and
reverb wet limiters, `DriveBlendPhase` smoothing + limiter.

Each passes its own bound test. **The composite was never measured.** Do not assume any of them is
wrong; do not assume any is sufficient. B7.5 decides.

`DriveBlendPhase` at **50.5×** under periodic phase/content coincidence (§K.1) was the largest
single find of the change — an unsmoothed per-sample allpass coefficient — and fixing it still did
not move F2. That is the change's failure in one fact.

## Defects found by the 2026-08-06 omni audit, after this change was written

Recorded here because they were latent in the work this change shipped; all five are carried into
the successor as F0 preflight items or folded into the F-item they belong to.

1. **The Stop flush enumerates 2 of 13 stateful stages, and its comment asserts a false reason.**
   `FroggersAppCore.hpp:623` justifies clearing only `delay_`/`reverb_` on the grounds that
   "VCOs/filters/drive do not [self-sustain]". `filterChain_.comb` is a recirculating delay line
   with feedback to ±0.95 and a 6.7 s T60 by this change's own W2.1-MATH table; `driveBlendPhase_`
   is a recursive allpass measured at 50.5× by this change's own §K.1. Both have `Reset()`. Both
   are upstream of the two stages that DO get cleared, and the clear is one-shot. This is a
   concrete root-cause candidate for F3 that §2's hypothesis 2 gestured at but never pinned.
2. **A test asserts the opposite of B7.5's criterion.**
   `limiter_engages_on_overdriven_patch_and_stays_bounded` requires `minEnvelopeSeen < 0.999f`.
   B7.5 requires unity. Nothing in this change flags the collision.
3. **A stale in-code directive now contradicts a live operator ruling.**
   `FroggersModulation.hpp:922-925` forbids "breaking the deliberate 30/30 tie toward a single
   peak" — which is exactly what the F1 ADDENDUM's mode-2 ruling requires. This is §0.5 (an
   instruction's rationale died and its letter was preserved) recurring inside the code, in a
   change whose own report names §0.5 as a lesson.
4. **The Group A omni-review's F4 fix landed at 1 of 4 sites.** `FroggersAppCore.hpp:490-496`
   hoists into named locals; `FroggersModulation.hpp:1037,1093,1139` still read
   `partial = Call(...) || partial`, and two of those are inside loops. tasks.md records F1–F4 as
   "fixed together in a single dispatch".
5. **Two files named `*Tests.cpp` run zero tests.** `FroggersCrunchyBlowupReproTests.cpp` and
   `FroggersRandomizeAllReproTests.cpp` are `int main()` harnesses excluded from `Makefile`'s
   `test:` target. W2.0 cites the latter as evidence about "why 'fixed' passed every test" — it
   never runs. The former still carries the stale `ExpMapCompute(1.0f, 10.0f, …)` peak-height
   ceiling that W2.2c queued for repair (at `:212`; the plan cites `:193`, drifted).

   > **Both were fixed by the successor's F0.2 (`52c2730`) and no longer exist under these
   > names.** They are now `app/FroggersCrunchyBlowupRepro.cpp` and
   > `app/FroggersRandomizeAllRepro.cpp`, and the stale ceiling now reads
   > `dsp::kMaxResonantBumpHeight`. The old names are left in the text above because this is a
   > record of state as found on 2026-08-06; this note exists so the paths above are not followed
   > to files that are no longer there.

## Durable rulings extracted before archiving

These are operator decisions with lasting force, not scoped research. Carried into the successor's
`proposal.md`; their full derivations stay here.

- **Every parameter must be continuous** (§J.6). No switch-type parameters even though Sheaf
  supports them — a discrete parameter cannot be meaningfully modulated by the 15-source slate.
- **The bank-slot vs modulation-layer rule** (§J.3). The modulation layer expresses "this
  parameter's value varies over time"; anything reducible to that is redundant as a dedicated
  parameter. Bank slots are for operations on the *signal path* — multiplying, summing, phase
  resetting, bit-combining.
- **Any new bank parameter that can raise a stage's output level ships with its trim/limiter budget
  re-derived** (§K standing rule). Named prerequisites on record: comb saturator drive requires
  re-deriving the `1/(1+fb)` trim; series/parallel routing requires the peak trim.
- **The master limiter stays, unchanged** (W2.1-MATH-2). It is the only thing between reverb Hold
  at `fb ≈ 0.99998` and the output. No task may propose removing it.
- **Randomize All's level-1 scope is correct as built** — the drilled parameter only. Navigating
  out of the drilldown is the bug, not the scope.

## Deferred, untouched, still open

§H mobile-web UI layer · §I VST layer · §J bank parameter expansion (the 14-param delay design,
the per-bank Tier 1/2/3 tables, the ADSR ×3 decision) · D.4 publish pipeline · W4 second Sheaf pin
bump to `508d9d68` and the `kExternalAudioOptedIn` removal · G.2 blank-window-on-startup-failure.

§J's research is **not** carried into the successor's task list — it is deferred work and copying
it forward would cost every future agent context for no scoped benefit (OMNI §16). It stays here
and is readable. Only the binding rulings above travel.
