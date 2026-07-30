# Final audit evidence (task 9.8)

> **Canonical host contract:** [`openspec/specs/froggers-host-master/spec.md`](../../specs/froggers-host-master/spec.md)

Recorded: 2026-06-19 (local apply session)

## Scope boundaries verified

| Check | Result |
|-------|--------|
| Package version bump | **No** — web `package.json` remains `1.0.4`; CMake `PROJECT_VERSION` unchanged from pre-omni baseline |
| Release publication | **No** — no tag push, no GitHub release workflow dispatch |
| Remote ref mutation | **No** — `git push` not executed in this session |
| Daisy firmware source/build/doc | **No** — no porcelain paths under Daisy application/build trees |

## OpenSpec closure

| Check | Result |
|-------|--------|
| Active changes | Only `omni-repository-harmonization` (`openspec list --json`) |
| Superseded archives (--skip-specs) | 5 changes under `openspec/changes/archive/2026-06-18-*` |
| Reconciled normal archives | 10 changes under `openspec/changes/archive/2026-06-19-*` |
| POST_CLOSURE hygiene | `scripts/check_openspec_hygiene.sh --post-closure` — **pass** |
| Baseline Purpose placeholders | 13 post-archive TBD lines replaced; re-validate **pass** |

## Automated verification matrix (9.7 subset)

| Gate | Command / evidence | Result |
|------|-------------------|--------|
| Host display generator | `node scripts/generate-host-display.mjs --check` | fresh |
| Host display shape | `npm run verify:host-display` (web) | pass |
| WASM render allocation | `npm run verify:wasm-render-allocation` | pass (scope=96, chunk=4096) |
| Host artifact hygiene | `scripts/check_host_artifact_hygiene.sh` | pass |
| Release metadata | `desktop/scripts/verify-release-metadata.sh` (Pages preflight) | wired |
| Sim tests | 14/14 ctest (includes WasmSimHostMalloc, OwnedAllocation) | pass |
| Desktop processor tests | HostParameterRegistry + HostParameterProcessor (107 params) | pass |
| AudioRecorder | AudioRecorder_test (drain/overflow/allocation) | pass |
| Clean rebuild | `scripts/verify_clean_rebuild.sh` | pass; worktree unchanged after builds |
| VCV MIDI boundary | `sim/check_vcv_midi_boundary.sh` | wired in Pages preflight |

## Manual-only evidence (tasks 5.10 and 9.7)

**Operator checklist:** see [`MANUAL_TEST_PLAN.md`](./MANUAL_TEST_PLAN.md) for step-by-step validator commands, REAPER/Logic smoke matrix, parity checks, and an evidence table to fill in.

Summary of what remains manual:

- Steinberg VST3 validator
- `pluginval` strictness ≥ 5
- `auval` on AU bundle
- REAPER (or alternate VST3 host) + Logic smoke: MIDI learn ×3, automation, stopped-transport UI mutations, state/editor recall, zero-input rendering, optional mono input
- Playwright full suite (`npm run test:e2e`) — optional locally; CI covers on PR
- VCV Rack build/package — optional; requires local SDK

## Worktree note

~119 modified/untracked paths remain locally (generator outputs, openspec artifacts, desktop/VST sources). No commit or push performed in this session per user workflow.
