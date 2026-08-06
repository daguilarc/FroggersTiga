# Supersession record — `frogg3rs-blowout-and-drilldown-repair`

**Created 2026-08-05. Supersedes `frogg3rs-modulation-truth-and-voicing`**, archived *superseded,
FAILED* — a stronger verdict than its own predecessor earned, and the distinction matters.

## Why superseded, and why "failed" rather than "not done"

The predecessor did not run out of time. **It shipped, claimed success, and did not work.** The
operator ran its final consolidated build and reported four failures, every one of which that plan
records as fixed with green tests attached. 176 tests pass; four reported symptoms remain.

**Its complete failure analysis is `../frogg3rs-modulation-truth-and-voicing/FAILURE-REPORT-AND-HANDOFF.md`.
Read that before this file's task list — especially §0 (behavioural lessons) and §1 (the systemic
error).** Nothing in this change should begin until §0.1 is understood, because the failure was
not primarily technical.

## Carried forward as this change's scope

| ID | Item | First move |
|---|---|---|
| **F1** | Randomize count distribution → **mode 2, 4+ rare, 7 essentially never; same distribution at EVERY level** | Histogram chosen count vs non-neutral `SceneCenter` vs `HasNonZeroState()` from a fresh patch. Cause asserted wrongly twice — measure, do not reason |
| **F2** | Filter Crispy at max still blows out | Write the end-to-end failing test FIRST (see below). Then B7.1's `C = 0.80` ceiling retarget, which was specified and never built |
| **F3** | **Stop does not stop** — audio > 1 minute after Stop, scope still | Instrument per-stage output magnitude after Stop; print which stage is non-zero. Scope still + audio present ⇒ a downstream stage self-oscillating with no input |
| **F4** | Randomize All at level 1 ejects the operator to the main page | Delete the `PressEncoder`/`Back()` round trips in the level-1 branch — verified unnecessary; the randomize helper does not need the view open |
| **F5** | Raise drill-in maximum to level 3 (base-3 theme) | Same deletion as F4 makes it cheap (fan-out 3615 → ~105). Then `kMaxDrillLevel` + `levelEncoders_[]` array, replacing two hardcoded `2`s and the `wasLevelTwo` special case |

**F4 and F5 share one edit.** Removing the level-1 round trips fixes the ejection, cuts level-2
materialization from 240 to ~45, and makes level 3 feasible. Do it first — it is the highest
value-per-line change on the list.

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
