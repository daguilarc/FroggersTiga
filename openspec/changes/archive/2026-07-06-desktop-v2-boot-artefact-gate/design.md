## Context

JUCE `juce_add_gui_app` with `CMAKE_BUILD_TYPE=Release` on macOS emits:

```
desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app
```

An **older** bundle remains at:

```
desktop-v2/build/FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app   ← Jul 1, pre boot-sync-fix
```

Manual QA, prior agent handoffs, and `PACKAGING.md` pointed at the stale path. The boot-sync fix (`m_hostCallbacks`, `selectPage(0, false)`) lives in source and the Release binary; launching the stale app reproduces the archived crash (`MainComponent` → `setActivePage` → dangling/stale `onPageChanged` → `SetGlobalCrunchy` SIGBUS).

`BootSmoke_test` already prefers the Release path in `kCandidates[]` but does not prevent human error, and its liveness check is insufficient.

## Goals / Non-Goals

**Goals:**

- One canonical macOS launch path documented and scripted.
- Stale root-level `.app` cannot be launched accidentally after a Release rebuild.
- Boot smoke test fails when the child exits early (including SIGBUS 138).
- Operator docs state Release path and launch script.

**Non-Goals:**

- Re-landing boot-sync source changes (already merged).
- Multi-config IDE project generation changes.
- Windows stale-path issue (Windows table already uses `Release/` subfolder).

## Decisions

### D1. Canonical path is always `Release/` on macOS

**Decision:** All docs, scripts, and test defaults reference `FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app`.

**Alternatives rejected:**

- Symlink root `.app` → `Release/` — acceptable as **implementation** detail in tasks, but docs still cite Release explicitly to match CMake output.

### D2. Remove stale root bundle on build (CMake POST_BUILD)

**Decision:** Add a `POST_BUILD` custom command on `FroggersTigaDesktopV2` that deletes `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` when it is not the same inode as the Release bundle (or unconditionally remove if directory exists and differs from `$<TARGET_BUNDLE_DIR:…>`).

**Rationale:** Prevents Finder history and muscle memory from opening Jul 1 binary.

### D3. BootSmoke uses `waitpid(WNOHANG)` loop

**Decision:** Replace `kill(child, 0)` liveness with a poll loop (e.g. 50 ms × 40 = 2 s):

```cpp
int status = 0;
const pid_t r = waitpid(child, &status, WNOHANG);
if (r == child && (WIFEXITED(status) || WIFSIGNALED(status))) { /* FAIL */ }
```

After delay, one final `waitpid(WNOHANG)`; if exited, FAIL; else SIGTERM + `waitpid`.

**Rationale:** Zombies still have `kill(pid,0)==0`; `waitpid` observes termination status.

### D4. Launch helper script

**Decision:** `scripts/open-desktop-v2.sh` resolves repo root, checks Release `.app` exists, runs `open` on it, exits non-zero with message if missing.

### D5. Shared path resolution (OMNI repetition — 2 callers)

**Decision:** Extract `desktop_v2::test::resolveV2BinaryPath()` in a small header used by `BootSmoke_test.cpp` and optional `ArtefactPath_test.cpp`. Trigger ≥2 met (BootSmoke + ArtefactPath).

| Helper | Trigger | Boundary | Complexity | Contract |
|--------|---------|----------|------------|----------|
| `resolveV2BinaryPath()` | 2 test TUs | Test binary discovery | env + candidate loop | Returns executable path or nullptr |

Review enforcement: Trigger ≥2 **Yes** | Domain **Yes** | Complexity **Yes** | Contract **Yes** | Side effects **None** | Local **Yes** (`desktop-v2/tests/`).

## Data flow (fixed launch)

```
Developer builds Release
  → POST_BUILD removes stale root FroggersTigaV2.app
  → scripts/open-desktop-v2.sh → open Release/FroggersTigaV2.app
  → process alive ≥3s (boot-sync fix in linked binary)
  → BootSmoke_test: fork → waitpid poll → PASS or FAIL (no zombie pass)
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Developer uses Debug config | Document Debug path in PACKAGING.md; BootSmoke candidates include Debug |
| POST_BUILD delete surprises someone relying on root bundle | Delete only known stale path; Release remains |
| BootSmoke still skips when binary absent | Keep explicit SKIP message; CI job builds desktop-v2 first |
| Freshness test flaky on fast incremental builds | Compare mtime only when object newer than binary by >1s threshold |

## Migration Plan

1. Merge artefact-gate change.
2. One-time: `rm -rf desktop-v2/build/FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` locally.
3. Rebuild Release; run `ctest -R BootSmoke`.
4. Update operator habit: `./scripts/open-desktop-v2.sh` or Release path in PACKAGING.md.

## Open Questions

None — root cause and fix path verified 2026-07-04.
