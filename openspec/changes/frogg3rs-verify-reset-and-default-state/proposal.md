# Proposal — `frogg3rs-verify-reset-and-default-state`

**Created 2026-08-30. Supersedes and archives
`frogg3rs-reset-reseed-and-default-state`,** whose code is committed and pushed
and whose spec delta is carried forward here unchanged.

The code in that change is probably right. Its verification is not trustworthy,
and the reason is a repeated failure pattern in how it was verified. This
change hands the evidence to someone else. The analysis of what it means is
theirs, not the previous author's.

## What the next agent should reach by the end

A defensible answer to one question: **is the committed work safe to keep?**

The criterion is per-commit, not global: does evidence from a build that
PROVABLY RAN support the claim that commit makes? "Provably ran" means a full
result count -- 321 lines from `make -C app test`, 918 from the Sheaf gate. A
hand invocation emitting 40 does not qualify regardless of what it printed.

Two faults are possible and they take different remedies. **Revert on a build
failure; amend the record on an evidence failure.** A target that does not
compile means committed code is wrong. A measurement that cannot be trusted
means the proposal over-claimed while the code may be fine. Task 4a sets this
out per commit.

This does not require re-doing the investigation. It may conclude that some of
the committed work should be reverted.

## The failure record

Presented as observations rather than conclusions. Every item below was caught
by a control, an independent reviewer, or the operator — none by the author
noticing.

### Instruments that silently did nothing, read as success

| instrument | what it did |
|---|---|
| 48-block measurement window | long enough for the transport gate to reopen inside it, so every arm read "audible", including a pristine one |
| `make -C app app/build/...` | doubled path; nothing compiled; three probes reported `BUILD FAILED` including the restore control |
| `./"$BIN"` with `$BIN` absolute | resolved to `.//Users/...`; the binary never ran; empty grep read as "check passed" |
| `str.replace()` with no assertion | silent no-op if the pattern had drifted |
| `>100 PASS` as a liveness signal | misfired on a run where ~280 tests legitimately failed |

Each returned a result that looked like an answer. Four of the five were
written after the author had already identified this exact pattern in writing.

### Enumerations reported as complete that were partial

- Hygiene sweep: covered `:100-320` of one file, counted definitions as call
  sites, concluded "every helper has 3+ callers". Actual single-caller helpers
  existed.
- Depth walk: reached 62 of 153 parameters, skipping Crispy, Crunchy and nested
  depth children, and concluded no depth was stale.
- Parameter snapshot: captured 3 of 11 available public observables and
  concluded the two arms were identical.

The corrective in each case was reading the accessor or definition list rather
than choosing a plausible subset.

### Claims asserted then withdrawn

- "`ComputeAllParameters()` is the discriminator" — asserted on one run,
  withdrawn, then re-established by a controlled ten-run comparison. Both the
  assertion and the retraction were premature.
- "Randomize is not reproducible run to run" — asserted from two runs believed
  to share a binary, withdrawn after ten controlled runs showed zero variance.
  A stale comment asserting it survived in the test file until an independent
  reviewer found it.

### Process failures

- The Sheaf change was committed onto a pre-existing 44-commit branch that
  already had an open upstream PR, silently adding the commit to that PR —
  after the operator had asked three times for a NEW Sheaf PR.
- Extracting it via cherry-pick dragged 40 lines of an unrelated contributor's
  unmerged work into the PR branch. Caught only because the gate was re-run on
  the new base rather than carried forward.
- Four verification harnesses were written that appear in no task list.
  Unplanned work receives no preflight, which is why nothing checked whether
  those instruments could produce the results they claimed.

## What is committed

Branch `froggers-reset-reseed-and-default-state`:

| commit | content |
|---|---|
| `d12b979` | Grace/Curve knob mappings |
| `aed087b` | Defect A: reseed after Reset as well as Randomize |
| `94eeb7c` | Defect C probe: a fast sweep does not latch the instrument |
| `0f00e49` | Defect B: New restores the cross-VCO detents, + submodule pin |

Sheaf: `3ef36f67` on `fork/fix-out-of-tree-app-gaps` (the pinned commit), and
the same change standalone as [jvictor0/Sheaf#10](https://github.com/jvictor0/Sheaf/pull/10).

## Other active changes

Two are live, and neither plans work this change duplicates:

- `frogg3rs-microphone-path-delivery` — remaining items are operator checks.
  It owns the surfaces tasks 1 and 2 build (`app/browser`, the launcher), so
  evidence from those tasks states whether its uncommitted work was present in
  the tree that built (task 0).
- `frogg3rs-guitar-and-solo-variants` — complete in `src/` and
  `test/firmware/`, uncommitted pending an operator device test. It does not
  touch `app/`, which includes nothing from `src/`.

Superseded predecessors of both lines are in `openspec/changes/archive/`.

## Evidence produced by `make`, and therefore trustworthy

- Defect A mechanism: 84 parameters differing at the reset block, worst drift
  0.19, converging by 8 blocks at ~81% per block; 0 differing after the fix.
- Defect B: `fresh`, `after New`, `after Reset All` all `0.51 0.51 0.49 0.49
  0.51 0.51`, asserted equal element-wise.
- Defect C refuted: maximal sweep then restore decays to 9.39e-13 against a
  1.0e-3 floor, with a Freeze control at 0.509 proving the rig can report a
  hold.
- Gates: app 321/0; Sheaf 918 pass with 2 pre-existing `braid4` deadline
  failures on a wall-clock bound.

## Evidence NOT produced by `make`, and therefore suspect

The audible 12/12-holds vs 0/12-decays comparison, including the ten-run
controlled Grace/Curve comparison, came from hand invocations of `c++`.

Later, hand-compiled binaries emitted 40-45 result lines where `make` emits
321 — including one built from a restored, unmodified tree. Yet the earlier
hand-built runs printed output from `TEST_CASE`s near the end of the file,
which a binary stopping at test 45 could not reach.

RESOLVED by task 3: the two counts measure different units. 321 is the whole
ten-binary `make -C app test` suite; the audio-routing binary's own complete
output is 45 result lines, and a hand build using the Makefile's actual flags
reproduces exactly 45/0, matching `make`'s subset. The 40-45 readings were
complete runs of one binary compared against the suite's total; no run was
short, and there was never a contradiction. The audible comparisons are
`TEST_CASE`s in that binary and pass under `make` today. The criterion above
stands for suite-level claims, but a hand invocation emitting 45 was a full
run of the one binary it built.

## Never compiled

- `./app/build-launcher.sh` — the shipping app through Sheaf's runtime shell.
  No gate here compiles `Runtime.hpp`; every one links the headless rig.
- `make -C app/browser` — the wasm target, which compiles two changed headers
  under different flags.
