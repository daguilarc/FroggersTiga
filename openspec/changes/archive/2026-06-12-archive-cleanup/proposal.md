## Why

FroggersTiga accumulated overlapping planning dirs (naming sweeps, web bootstrap attempts, umbrella multi-ui). **Canonical requirements now live in `openspec/specs/`** (`sim-parameter-display-names`, `filter-comb-offset`, `reverb-stereo-diffusion`, `quick-dict-format`). Active implementation workstreams:

1. **`web-sim-bootstrap-repair`** — WASM main-thread load (web dead on arrival)
2. **`filter-precomb-dispersion`** — Filter row 0 DSP + **Comb offset** label
3. **`desktop-header-hit-test`** — transport hit-test at startup
4. **`web-chrome-cohesion`** — mobile web polish

Archive everything else that is code-complete or superseded.

## What Changes

- **Archive 13 changes** via `openspec archive` (see design §2 table); move delta specs into `openspec/specs/` where applicable.
- **Keep 2 active changes** — `desktop-header-hit-test`, `web-chrome-cohesion` — as the only in-progress planning dirs until apply completes.
- **Fold open tails** into active changes before archive:
  - `desktop-chrome-cohesion` hit-test gap → already scoped in `desktop-header-hit-test`; archive chrome-cohesion after hit-test lands.
  - `web-sim-page-ux` §7 manual checks → append to `web-chrome-cohesion/tasks.md` §5.
  - `desktop-audio-export` §6 export verify → append to `desktop-header-hit-test/tasks.md` §5 (RECORD depends on clickable transport).
- **Spec supersession on archive** — `desktop-midi-input-clarity` velocity-only QWERTY spec superseded by `desktop-qwerty-midi-pitch-cv` pitch × velocity semantics.
- **One delta before MIDI archives** — add CC SPSC queue tasks to `desktop-qwerty-midi-pitch-cv` (mirror note queue; OMNI thread-safety parity).
- **`MANUAL_VERIFY.md`** at repo root — consolidated checklist from archived changes' open manual tasks (stereo delay §A–I, patch matrix, MIDI pitch steps, help menu).
- **Stale design footnotes** — `sim-hosts-multi-ui/design.md` and `desktop-compact-layout/design.md` get archive-time supersession notes (not re-opened as active work).
- **No application code** in this change — planning hygiene and `openspec archive` runs only.

## Capabilities

### New Capabilities

- `openspec-change-lifecycle`: Which changes stay active vs archived; archive order; tail-merge rules.
- `manual-verification-checklist`: Single repo-root manual test matrix superseding per-change open verify tasks.

### Modified Capabilities

- (none — no `openspec/specs/` baseline yet; archived change deltas populate main specs on first `openspec archive`)

## Impact

- `openspec/changes/` — 13 dirs archived to `openspec/changes/archive/` (or equivalent per CLI)
- `openspec/specs/` — created/populated by `openspec archive` from merged change specs
- `MANUAL_VERIFY.md` (new) — human sign-off checklist
- `desktop-header-hit-test/tasks.md`, `web-chrome-cohesion/tasks.md`, `desktop-qwerty-midi-pitch-cv/tasks.md` — tail tasks appended before their archive
- README — one line pointing to `MANUAL_VERIFY.md` and naming the two active changes
