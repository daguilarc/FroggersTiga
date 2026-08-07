# Supersession record — `frogg3rs-blowout-and-drilldown-repair`

**Created 2026-08-06. Supersedes `frogg3rs-modulation-truth-and-voicing`**, archived *superseded,
FAILED* — a stronger verdict than its own predecessor earned, and the distinction matters.

## Why superseded, and why "failed" rather than "not done"

The predecessor did not run out of time. **It shipped, claimed success, and did not work.** The
operator ran its final consolidated build and reported four failures, every one of which that plan
records as fixed with green tests attached. 176 tests pass; four reported symptoms remain.

**The binding content of that failure analysis now lives in this change's own `proposal.md`, under
"Method constraints (M1–M7)". Read that, not the archive.** Nothing here should begin until M1 is
understood, because the failure was not primarily technical.

The original document is preserved as history at
`../archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/FAILURE-REPORT-AND-HANDOFF.md`,
alongside an `ARCHIVE-RECORD.md` that states what shipped, what did not, and what the 2026-08-06
omni audit found afterward. **This change is self-contained; you do not need either file to
execute it.**

## Carried forward as this change's scope

| ID | Item | First move |
|---|---|---|
| **F1** | Randomize count distribution → **mode 2, 4+ rare, 7 essentially never; same distribution at EVERY level** | Histogram chosen count vs non-neutral `SceneCenter` vs `HasNonZeroState()` from a fresh patch. Cause asserted wrongly twice — measure, do not reason |
| **F2** | Filter Crispy at max still blows out | **Traced 2026-08-06** — every per-stage limiter ships `ceiling = 1.0` against a master threshold of 0.9, so the master engages by construction. B7.1's `C = 0.80` retarget is the fix; it was specified and never built. Write the end-to-end failing test first anyway (see below) |
| **F3** | **Stop does not stop** — audio > 1 minute after Stop, scope still | ~~Traced 2026-08-06 — the Stop flush clears 2 of the 13 stateful stages~~ **SUPERSEDED TWICE. (a) The count is 14, not 13** (corrected 2026-08-07; `proposal.md` carried the same miscount). **(b) F3.1 REFUTED this trace by measurement** — on a static patch Stop stops cleanly, and F3.3 (`1c37657`) has since made the flush reset all 14 anyway. The live hypothesis is **parametric oscillation in a modulated feedback path**; see `tasks.md` §F3.2 and the F3.2c measurement |
| **F4** | Randomize All at level 1 ejects the operator to the main page | Delete the `PressEncoder`/`Back()` round trips in the level-1 branch — verified unnecessary; the randomize helper does not need the view open |
| **F5** | Raise drill-in maximum to level 3 (base-3 theme) | Same deletion as F4 makes it cheap (fan-out 3615 → ~105). Then `kMaxDrillLevel` + `levelEncoders_[]` array, replacing two hardcoded `2`s and the `wasLevelTwo` special case |

**Two items were added by the 2026-08-06 omni audit and come before F1–F5** (both in `tasks.md`):

- **B7.5.0 — the patch-application anomaly. CLOSED 2026-08-06, no defect.** The predecessor
  recorded that two structurally different test patches produced *bit-identical* limiter
  trajectories and never investigated it. Investigated now: `SceneCenter` writes DO reach the DSP,
  through `Parameter::ProcessSamplePhase1`'s periodic **smoothed** `Compute()` (alpha 0.0994 every
  16 samples). A patch is ~81 % applied one block after it is written and converged after ~30, so
  the two patches read identically because both were sampled at the same early point on the same
  trajectory toward the same target. **The 76 sites using the idiom stand.** What this leaves is a
  narrow hazard — tests asserting inside the first ~30 blocks — swept in F0.5.
- **F0 — preflight remediation.** Five defects in shipped work: three surviving short-circuit
  sites, two `*Tests.cpp` files that run zero tests and are excluded from `make test`, a stale
  `ExpMapCompute(1.0f, 10.0f, …)` peak-height ceiling, and an in-code directive that forbids
  exactly what F1's mode-2 ruling requires. That last one is a hard prerequisite for F1.

**F4 and F5 share one edit.** Removing the level-1 round trips fixes the ejection, cuts level-2
materialization from 240 to ~45, and makes level 3 feasible. Do it first of the F-items — it is the
highest value-per-line change on the list.

## The acceptance criterion that governs everything

**`B7.5`: the master limiter's `envelope` stays at unity across a hostile patch** — all maxima,
modulation live, transport running, the operator's real Crispy repro. It was specified in the
predecessor and never written.

**Write it first. It must FAIL.** That failing test is the entry point for F2 and the only
end-to-end proof that the per-stage headroom architecture actually works in the binary rather than
on paper.

**Standing rule, inherited: do not add another per-stage bound test until an end-to-end test
reproduces the operator's actual repro and fails.** The predecessor added five and moved nothing.

## Carried over as GENUINELY working (operator-confirmed or independently verified)

- Non-additive randomize — depths reset between presses (operator-confirmed).
- Drill-in knob display — `ComputeAllParameters()` reseed on the audio thread.
- Scene-2 mirrored VCO shape defaults; light cross-coupling in both poles.
- Sheaf pinned at `77a3019e`, clean and unpatched; the 6×6 single-grid surface; direct launch.
- All 15 upstream asks filed (email + jvictor0/Sheaf issues #1, #2, #3).

## Carried over as SUSPECT — landed, measured green in isolation, symptom unresolved

Comb trim, peak trim, peak limiter, delay in-loop saturator, delay/reverb wet limiters,
`DriveBlendPhase` smoothing + limiter. **Each passes its own bound test. The composite was never
measured.** Do not assume any of them is wrong; do not assume any is sufficient. B7.5 decides.

## Deferred, untouched

§H mobile-web UI layer · §I VST layer · §J bank parameter expansion (delay 14-param design,
per-bank candidates, the signal-path-vs-modulation rule) · B7.1 shared ceiling · B7.3 filter
composite · B7.4 per-stage guards · W4 second pin bump to `508d9d68` and the external-audio
unblock · G.2 blank-window-on-startup-failure.
