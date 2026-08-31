# Postflight — `frogg3rs-verify-reset-and-default-state`

Executed 2026-08-30 on `main` after fast-forwarding it onto
`froggers-reset-reseed-and-default-state` (the four commits now sit on `main`;
the branch is deleted). Every build ran from a tree verified clean under
`app/` and `External/Sheaf` immediately beforehand (task 0), and every
rebuilt artifact was deleted first and checked for a fresh mtime afterward.

## Task outcomes

- **Task 1 — shipping app: BUILDS.** `./app/build-launcher.sh` exit 0,
  fresh `Frogg3rs.app`. The script itself caps at `nice make -j2`. First
  attempt died silently in a background shell — caught by the stale artifact
  mtime, not by the log, which contained no error; the foreground rerun
  succeeded. Four pre-existing `-Wswitch` warnings on
  `ParamSetAbsoluteOnBank` in Sheaf's MIDI config sources, no errors.
  The by-ear confirmation is the operator's, after push.
- **Task 2 — browser target: BUILDS, test OK.** Fresh
  `frogg3rs.wasm`/`frogg3rs.js`. The `test` target asserts only that both
  outputs exist non-empty; the real claim carried is that the two changed
  headers compile under the wasm flags.
- **Task 3 — 40-vs-321: RESOLVED, no contradiction existed.** 321 is the
  ten-binary suite total; the audio-routing binary's complete output is 45
  result lines, and a hand build with the Makefile's actual flags
  (`TEST_CPPFLAGS`, compiler `c++`) reproduces exactly 45/0. The archived
  40-45 readings were complete single-binary runs compared against the
  suite total. The audible comparisons are `TEST_CASE`s in that binary and
  passed under `make` in this execution. The archived docs never recorded
  the original hand invocations' flags; with the count reconciled, nothing
  rests on them.
- **Task 4a — all four commits STAND.** Both never-compiled targets build,
  and each commit's load-bearing evidence reprinted fresh from `make`:
  reset transient `differing=0` from +0 blocks; latch probe `9.39357e-13`
  against floor `1.0e-3` with Freeze control `0.509084`; three-way detents
  `0.51 0.51 0.49 0.49 0.51 0.51`; suite 321/0 (322/0 after task 5).
  `0f00e49`'s upstream dependency (Sheaf `HasRestoreStartupState`,
  jvictor0/Sheaf#10) remains a tracked dependency, not a defect.
- **Task 4b — record amended.** Proposal's contradiction section rewritten
  to the task 3 resolution; the spec delta's audible-output rationale and
  decay scenario no longer carry unconfirmed/undelivered markers. SHAs,
  measurements, and the `Runtime.hpp` claim verified (`grep` finds only
  `BrowserRuntime.hpp` in the gate's compile surface; task 1 exercised the
  real runtime shell).
- **Task 5 — drift proof DELIVERED for Reset.**
  `perturbed_reset_detent_fails_the_launch_equality_check` in
  `app/FroggersAudioRoutingTests.cpp`: captures launch detents, runs Reset
  All, perturbs one restored detent by 0.05 through the same `SceneCenter`
  write the neighbouring tests use, and asserts the element-wise comparison
  reports the difference. Proven able to fail: with the perturbation forced
  to zero the case FAILS (control run logged), restored it passes; suite
  322/0. Launch drift stays unproven as near-vacuous, per the task. The
  comparator is a local lambda mirroring the sibling test's, matching this
  file's per-case idiom; collapsing the file's local comparators into a
  shared helper was out of scope and is noted, not done.
- **Task 6 — held.** No shell harnesses were written; every check ran as a
  `TEST_CASE` under `make` or as a direct build of a shipping target.

## Gates

| gate | state |
|---|---|
| `make -C app test` | 322 pass / 0 fail, fresh, binary deleted first |
| `make -C app/browser build` + `test` | fresh wasm, OK |
| `./app/build-launcher.sh` | exit 0, fresh bundle |
| Sheaf synth gate | CARRIED FORWARD from the archived change (918 pass, 2 pre-existing braid4 deadline failures): `External/Sheaf` is clean at the same pin `3ef36f67`, so nothing it compiles has moved |

## Standing gate debt (upstream, not this change's)

Two known soft spots in the Sheaf gates, both pre-existing and both living in
the submodule where per-change sweeps here do not reach; noted on
jvictor0/Sheaf#10 for upstream:

1. The miniapp test binary segfaults before emitting results, so nine checks
   never run while the gate reads green — a dead instrument inside a passing
   gate.
2. The two braid4 96 kHz deadline tests fail deterministically on this
   8-core machine; local runs are read as "green except these two" by
   convention. They should skip by environment with a printed reason
   instead of being memorized around.
