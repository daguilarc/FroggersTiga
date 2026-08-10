# Archive record — `frogg3rs-gui-and-dsp-robustness`, archived 2026-07-28

**Status at archive: superseded, not failed.** Superseded same-day by
`frogg3rs-audio-safety-and-ui-rework` (see that change's own `SUPERSESSION-RECORD.md`, carried
forward at `../2026-08-05-frogg3rs-audio-safety-and-ui-rework/SUPERSESSION-RECORD.md`, for the
original landed/superseded/still-open breakdown written at the time). This record does not repeat
that breakdown; it exists because this directory sat **live** (not archived) for eleven more days
after being superseded, and this archiving pass had to independently verify none of its own
unchecked tasks (28 of 51 boxes) represented real, un-carried scope before moving it — the operator's
instruction was "archive what is genuinely redundant," not "archive what looks old."

Successor chain from here: `frogg3rs-audio-safety-and-ui-rework` (archived
`../2026-08-05-frogg3rs-audio-safety-and-ui-rework/`) → `frogg3rs-modulation-truth-and-voicing`
(archived `../2026-08-06-frogg3rs-modulation-truth-and-voicing/`) →
`frogg3rs-blowout-and-drilldown-repair` (archived `../2026-08-07-frogg3rs-blowout-and-drilldown-
repair/`) → `frogg3rs-parametric-slew-and-stop-root-cause` (archived
`../2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/`) → `frogg3rs-external-audio-phantom-
input` (live at time of this archiving).

## Why archivable — traced, not assumed, 2026-08-09/10

Its own successor's `SUPERSESSION-RECORD.md` lists most of what landed and what carried into `§D`,
but this directory's 28 unchecked boxes were checked individually against that table, the code, and
git log before archiving — three items the successor's prose summary did not name explicitly:

- **2.2/2.3/2.4/2.5/2.7 (DSP recovery tiers, full-range sweep test)** — confirmed landed: the
  successor's "Landed and verified" table cites `app/FroggersAppCore.hpp`, `app/dsp/*.hpp`,
  `app/FroggersAudioRoutingTests.cpp` directly ("Tier 1 finiteness + Tier 2 magnitude recovery,
  ceiling 100.0 derived"; "Per-unit `Reset()` on eight DSP units"; "Full-range endpoint sweep test").
- **2.8 (hard clamp at 1.0)** — explicitly superseded, not silently dropped: "Task 2.8's hard clamp
  at 1.0 → replaced by the limiter (§A.3)."
- **2.1 (rebuild blowup repro against the real engine path)** — subsumed by the extensive real-
  signal-path testing that became central to every later generation (F3DIAG capture, the
  Crunchy/RandomizeAll repro harnesses, the "systemic error" lesson in the 2026-08-06 archive record
  about isolated-stage-only tests being insufficient). No standalone completion citation exists, but
  the concept did not survive as open scope anywhere downstream either.
- **6.7 (scope overhaul, two-band vs one-panel)** — resolved by operator decision, recorded in the
  successor's "Superseded — actively replaced" section ("the operator looked at the running app and
  confirmed there is **one** panel; the second band is withdrawn") and implemented as that
  successor's own `B.3` ("Post-gate scope tap", checked done).
- **6.8 (S&H dice-roll motion)** — resolved: successor's `D.1`, "CONFIRMED ALIVE 2026-07-29", refutes
  the original F5 suspicion by reading the code.
- **6.10 (disclose the encoder double-click limitation)** — mooted, not merely disclosed: commit
  `84f83e7` ("Delete the workarounds the Sheaf bump obsoleted (F.2)", 2026-08-03, in the successor)
  states plainly "Encoders take a plain click" — the limitation itself was removed, verified still
  true in the current `app/FroggersAppCore.hpp`/`app/FroggersUiSurface.hpp` click-dispatch code as of
  this archiving.
- **4.1–4.3 (voicing judgements: Crunchy chaos, Random S&H character, randomize reach)** — map
  directly onto the successor's `D.3` ("Voicing judgements — partially closed": Crunchy chaos and
  S&H character closed as-wanted; the randomize-reach question carried to `E.1`, later resolved by
  `frogg3rs-blowout-and-drilldown-repair`'s F1 mode-2 distribution fix, measured P(≥4) 8.8%,
  P(≥7) 0.2%).
- **5.1–5.12 (the whole publish pipeline and acceptance gate)** — real, large, and still open. Maps
  directly to the successor's `D.4` ("The whole publish pipeline and acceptance gate (predecessor
  §5)"), which is carried forward **by name, in every single successor generation's "deferred"
  footer**, through to the most recently archived change. Nothing here was lost — it was carried
  before this directory was ever touched today, and stays carried.
- **1.2 (`ScopeWriter` sizing)** — resolved: successor's `D.2`, "DECIDED AND RECORDED 2026-07-29.
  Verdict: keep the defaults, no change," with the derivation recorded at the construction site.

No item in this directory's task list represents scope that is open today and untracked elsewhere.

## Specs carried, not synced

`specs/frogg3rs-dsp-recovery/`, `specs/froggers-app-surface-layout/`, `specs/froggers-sheaf-runtime-
app/`, `specs/froggers-vco-topology/` are this change's delta specs, preserved here as history. None
were synced to the main `openspec/specs/` baseline at archive time — consistent with this repo's
established practice (the most recently archived change in the chain,
`2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause`, carries the same four capabilities'
current deltas and was likewise archived without a baseline sync; sync is a separate, deliberate
step in this repo's own workflow, not implied by archiving). Do not treat this directory's spec
deltas as current — read the most recently archived change's `specs/` for the latest state of any
of these four capabilities.

## Citation sweep performed at archive time

See `../2026-08-05-frogg3rs-audio-safety-and-ui-rework/ARCHIVE-RECORD.md` for the combined sweep
report covering both directories archived in this same pass. One self-referential path inside this
directory's own `HANDOFF-NEXT-AGENT.md` (§"Canonical documents") was corrected to point at this
archive location.
