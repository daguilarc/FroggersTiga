## Why

The boot-sync fix from `2026-07-01-desktop-v2-boot-sync-fix` is implemented in source and linked into the **Release** binary, but manual QA keeps launching a **stale** `.app` at `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` (Jul 1) instead of `…/Release/FroggersTigaV2.app` (Jul 2). That stale bundle still crashes on boot (`SetGlobalCrunchy` SIGBUS), producing the Dock “frog hop” even though the fix shipped. `PACKAGING.md` documented the wrong macOS path. `BootSmoke_test` resolves the Release binary correctly but uses `kill(child, 0)` after `sleep(2)`, which returns success for **zombie** processes — the test passed while the app crashed at ~600 ms when invoked from `BootSmoke_test` (verified in crash report `parentProc: BootSmoke_test`).

## What Changes

- Correct **macOS Release artefact path** in `desktop-v2/PACKAGING.md`, `QUICK_DICT.md` boot-outcome gloss, and any scripts/docs that reference the stale root-level `.app`.
- **Remove or replace** the stale root-level `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` bundle so Finder/`open` cannot launch a pre-fix binary after rebuild.
- Harden **`BootSmoke_test.cpp`**: use `waitpid(child, &status, WNOHANG)` to detect early exit; fail on `WIFEXITED` / `WIFSIGNALED`; remove zombie false-positive; keep Release-first candidate order.
- Add **`ArtefactPath_test.cpp`** (or extend BootSmoke): assert the resolved binary mtime is newer than object files for `MainComponent.cpp` when both exist — catches “recompiled but not relinked / wrong bundle” locally.
- Add **`scripts/open-desktop-v2.sh`**: opens the Release `.app` from repo root; used in tasks and Quick Dict cross-ref.
- Update **`desktop-v2-boot-host-sync`** spec: boot gate SHALL resolve the **current** Release binary, not a stale sibling path; boot smoke SHALL fail on early exit, not pass on zombie.

## Capabilities

### New Capabilities

- `desktop-v2-build-artefacts`: Canonical Release output paths, stale-bundle prevention, and launch helper for FroggersTigaV2 on macOS.

### Modified Capabilities

- `desktop-v2-boot-host-sync`: Boot smoke verification requirements — correct binary resolution, no zombie false pass, optional freshness check.
- `sim-operator-doc-parity`: Boot-outcome gloss SHALL reference `scripts/open-desktop-v2.sh` and Release `.app` path on macOS.

## Impact

- `desktop-v2/PACKAGING.md` — macOS artefact table (Release subfolder).
- `desktop-v2/tests/BootSmoke_test.cpp` — waitpid gate.
- `desktop-v2/tests/` — optional `ArtefactPath_test.cpp`.
- `scripts/open-desktop-v2.sh` — new launch helper.
- `QUICK_DICT.md` + `scripts/sync-help-docs.sh` mirrors.
- **Not in scope**: Re-implementing boot-sync fix (already in source); VST packaging; CI matrix expansion beyond desktop-v2 `ctest`.

## Evidence (verified 2026-07-04)

| Claim | Verification |
|-------|----------------|
| Stale root `.app` crashes | `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` mtime Jul 1; exits status 138 in 2s |
| Release `.app` survives | `…/Release/FroggersTigaV2.app` mtime Jul 2; ALIVE after 3s |
| Source has boot fix | `MainComponent.cpp` L82–83: `pushSelectPage(0)` + `selectPage(0, false)` |
| Crash stack matches old bug | Latest `.ips`: `setActivePage` → `onPageChanged` → `SetGlobalCrunchy` SIGBUS — stale binary only |
| BootSmoke false pass | `kill(child,0)` succeeds for zombies; crash report parent `BootSmoke_test` |
| Wrong doc path | `PACKAGING.md` L49 listed root `.app` without `Release/` |

## OMNI rule audit (2026-07-04)

| Rule | Finding | Resolution |
|------|---------|------------|
| Data flow | Launch path → wrong binary → boot crash misattributed to new features | Canonical path + stale bundle removal |
| Verification | BootSmoke does not observe actual process liveness | `waitpid(WNOHANG)` gate |
| Repetition | Path strings duplicated across docs/tests | `open-desktop-v2.sh` + shared candidate list |
| Defensive code | No guard against stale sibling `.app` | Freshness test or post-build symlink policy |
| Plan language | — | No hedge terms in artifacts |
