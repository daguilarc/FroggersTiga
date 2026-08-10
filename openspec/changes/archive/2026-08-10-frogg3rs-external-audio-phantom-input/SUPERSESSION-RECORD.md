# Supersession record — `frogg3rs-external-audio-phantom-input`

**Created 2026-08-09. Supersedes `frogg3rs-parametric-slew-and-stop-root-cause`**, archived at
`../2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/` — **implementation COMPLETE**,
not a failure and not a partial delivery. This is a different relationship than either predecessor
in this change's lineage had to its own predecessor: nothing here is a correction of unfinished or
wrong work. It is a **new, narrowly-scoped fix** for the one item that directory carried forward,
unexecuted, across three change generations: `W4.2`.

## Why superseded rather than reopened

The archived change's own `tasks.md` records **ALL IMPLEMENTATION LANDED** as of 2026-08-08, with
S4.3–S4.8 as the only open items, all operator-only. Reopening that directory to add unrelated new
scope would misrepresent a closed, delivered change as still in flight.

`W4.2` was never part of that change's own task list either — it appears only in each generation's
"Deferred, untouched" footer, inherited without re-derivation since `frogg3rs-modulation-truth-and-
voicing` (2026-08-05). A footer item that outlives three change generations without ever being
re-examined is exactly the shape of finding this change exists to correct: **the specific plan
attached to it — bump the pin, then delete the flag — was carried forward four times and never once
re-verified against current fact. It was wrong the entire time.** See `proposal.md` §0 for the
trace.

## What the predecessor FINISHED — carried as done, not redone

| Scope | State carried |
|---|---|
| S1/S1.3 — F3 root cause (drive-stage DC seed maps silence to silence) | DONE, fixed at the source, operator-confirmed by ear (S4.1: *"STOP WORKS! :))))"*) |
| S1a.2 — modulation transport-gated while stopped | DONE, operator-ordered, landed |
| S2a.1 — reverb tank in-loop saturator | DONE, operator-ordered, landed |
| S3.1 — VCO `std::array` refactor | DONE, proven bit-identical (8192 samples × 2 channels, 0 differing bytes) |
| S5 — drilldown `Back()` pops one level; S6 — encoder ring/card collision | DONE, through several placement corrections; the drill-level header's current placement is STEP 1 (2026-08-09) |
| Sustain floor | Raised 0.05 → 0.10, operator-ordered |
| S0 — citation repair for that change's own predecessor references | DONE |
| Upstream asks | 15 filed; ledger in `UPSTREAM-SHEAF-ASK.md`, corrected 2026-08-09 (item 8) as part of this change |

**None of this is touched by the present change.** It is orthogonal — a different file region
(`FroggersApp::Config()` and `ProcessBlock`'s external-audio branch, not the drive stage, reverb,
VCO, or UI work above) and an entirely different mechanism.

## Carried OPEN into this change

Only one item transfers with a changed plan:

| Item | Was | Now |
|---|---|---|
| W4.2 — remove `kExternalAudioOptedIn`, gated on the W4.1 pin bump landing upstream ask #8 | deferred, untouched, four change generations | **This entire change** — re-scoped: `numAudioInputs = 0` is the actual fix; the flag is deleted as its consequence, not gated on any pin bump, because ask #8 does not supply what W4.2 assumed it would (`proposal.md` §0) |

Everything else — S4.3–S4.8, the fuegoization fixed point, the `GangedRandomLfoVisualizer`
background-fill item, W4.1 itself — carries forward **unchanged, untouched, not re-derived by this
document**. Full list and current wording: `tasks.md`'s "Carried forward as open scope" section.

## Thrown away

Nothing. The predecessor's artifacts remain in place as history, at
`../2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/`.

## Why this needed its own change rather than a silent footer edit

`W4.2`'s footer entry was never wrong about the *symptom* — a hardcoded flag masking a permanently-
open microphone channel is a real, correctly-identified hazard. It was wrong about the *fix's shape*
and its *dependency on the pin bump*, and the plan's own justifying source
(`UPSTREAM-SHEAF-ASK.md` item 8) was independently wrong in the same direction — "landed, can go" —
so nothing caught the error across four generations of carrying it forward as settled. That is worth
a change of its own, with its own record of having been wrong, rather than a quiet edit to a footer
line: the record is what stops a fifth generation from carrying the same mistake forward again, the
same reason this repo's other archive records keep their own `§0`-style sections instead of just
fixing and moving on.
