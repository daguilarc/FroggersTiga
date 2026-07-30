## Why

`FroggersTigaV2` dies during `MainComponent` construction on branch `froggerstiga-desktop-v2` (commit `4e3d0a3`). lldb reproduces `EXC_BAD_ACCESS` in `DesktopHostIO::SetGlobalCrunchy` on the JUCE message thread. v1 desktop is unaffected. The v2 stack is on the side fork but cannot be manually tested until boot succeeds.

## What Changes

- Fix **dangling `HostCallbackContext` capture** in `desktop_v2::wireCallbacks` (`[&ctx]` binds to a temporary destroyed when `wireCallbacks` returns).
- Store `HostCallbackContext` on each host component (`MainComponent`, `HostedMainComponentV2`); route carousel callbacks and `pushSelectPage` / `pushRandomizeMod` through that member — no braced temporaries in stored lambdas.
- Consolidate duplicate host-callback init across both host entry points via `desktop_v2::refreshAndWireHostCallbacks` (OMNI repetition: identical blocks in `MainComponent.cpp` and `HostedMainComponentV2.cpp`).
- Remove redundant `pushSelectPage(0)` + `setActivePage(0)` double-sync on boot: one host sync via `pushSelectPage(0)`, UI-only `selectPage(0, fireCallback=false)`.
- Add **boot smoke gate** and a **callback-lifetime unit test** so GUI launch and dangling-capture regressions are gated, not only control-core `ctest`.
- **Docs delta only:** add boot-outcome gloss and carousel arrow navigation to `QUICK_DICT.md` (Engine/Stop, Shift, gates already present); run `scripts/sync-help-docs.sh` — **no release version bump**.

## Capabilities

### New Capabilities

- `desktop-v2-boot-host-sync`: Host-bridge sync on startup and carousel callbacks use stable object lifetimes; app process survives initial window construction.

### Modified Capabilities

- `sim-operator-doc-parity`: Add Quick Dict entries for **boot outcome** and **carousel arrow navigation** only (Engine/Stop, Shift, gates, Rand already documented).

## Impact

- `desktop-v2/Source/DesktopV2HostCallbacks.cpp` / `.hpp` — stable capture, `refreshAndWireHostCallbacks`, boot wiring.
- `desktop-v2/Source/MainComponent.cpp` / `.hpp` — `m_hostCallbacks` member, constructor init order, duplicate page select.
- `desktop-v2/Source/HostedMainComponentV2.cpp` / `.hpp` — same callback pattern (verified duplicate).
- `desktop-v2/Source/ui/PageCarouselComponent.cpp` / `.hpp` — `selectPage(page, fireCallback)`.
- `desktop-v2/tests/` — `BootSmoke_test.cpp`, `CallbackLifetime_test.cpp`.
- `QUICK_DICT.md` + `scripts/sync-help-docs.sh` mirrors (`docs/`, `web/public/`).
- **Not in scope**: `SIM_MANUAL.md` edits (already cross-linked), VST v2 build (task I.3 from archived change), web e2e full suite, firmware.

## Evidence (verified 2026-07-01)

| Claim | Verification |
|-------|----------------|
| Crash in `SetGlobalCrunchy` | lldb: `EXC_BAD_ACCESS (code=2)` at `DesktopHostIO::SetGlobalCrunchy+80` (`str s0, [x8, x9]`) |
| Call stack | `MainComponent::MainComponent` → `pushSelectPage` → `syncToHost` → `SetGlobalCrunchy` |
| `m_globalCrunchy` offset | `offsetof(DesktopHostIO, m_globalCrunchy) == 175708` (0x2ae5c); matches instruction `movk x9, #0x2` in disassembly |
| Dangling capture | `wireCallbacks` line 27: `[&ctx]`; caller passes temporary at `MainComponent.cpp:87` |
| Second sync on boot | `MainComponent.cpp:76-77`: `pushSelectPage(0)` then `m_carousel.setActivePage(0)`; `setActivePage` → `onPageChanged` (`PageCarouselComponent.cpp:73-75`) |
| `onPageChanged` installed before crash path | `wireCallbacks()` at line 74 runs before `pushSelectPage(0)` at line 76 |
| Unit tests pass without GUI | Prior session: desktop-v2 `ctest` 3/3; sim 20/20 — boot not gated |
| v1 unaffected | Separate target `FroggersTiga` / `BUILD_DESKTOP_V2=OFF` path unchanged |
| QUICK_DICT partial coverage | Engine/Stop (L17–18), Shift (L51–58), gates (L96–100) already documented; boot outcome + carousel arrows missing |
| Duplicate host wiring | `MainComponent.cpp` L85–87 ≡ `HostedMainComponentV2.cpp` L36–38; boot sequence L74–77 ≡ L27–29 |

## OMNI rule audit (2026-07-01)

Audit scope: `desktop-v2-boot-sync-fix` planning artifacts + verified sources (`desktop-v2/Source/DesktopV2HostCallbacks.*`, `MainComponent.*`, `HostedMainComponentV2.*`, `PageCarouselComponent.*`, `QUICK_DICT.md`). Implementation **not started** — code still uses `[&ctx]` and double boot sync.

### Compliant (artifacts)

| Rule | Finding |
|------|---------|
| Data flow — root cause | `design.md` traces boot crash: temporary `HostCallbackContext` → dangling `onPageChanged` → invalid `DesktopHostIO&` in `SetGlobalCrunchy`. |
| Data flow — fixed boot path | `design.md` §Data flow diagrams single `pushSelectPage(0)` then UI-only carousel select. |
| Nesting | `wireCallbacks` lambdas and `pushSelectPage` body are flat (≤2 levels); boot fix adds no depth >3. |
| Plan language | Grep over `specs/**/*.md`: zero forbidden hedge terms. |
| Defensive code | `fireCallback` default `true` preserves user-click carousel behavior; `false` only at construction. |
| Verification gates | `tasks.md` §OMNI verification gates (OMNI.1–OMNI.6) added in this audit. |

### Gaps closed in this audit (artifact updates)

| Rule | Finding | Resolution |
|------|---------|------------|
| Repetition — host callback init | Identical `wireCallbacks({...})` + boot triple in `MainComponent` and `HostedMainComponentV2`. | Add `desktop_v2::refreshAndWireHostCallbacks(HostCallbackContext&, ...)`; tasks §1.5. |
| Data flow — braced temporaries | `pushSelectPage` / `pushRandomizeMod` on both hosts still build `{m_core, ...}` per call. | Route through `m_hostCallbacks` after refresh; tasks §1.6. |
| Artifact drift — doc scope | Proposal listed Shift/gates/MIDI as new doc work; `QUICK_DICT.md` already covers them. | Narrow `sim-operator-doc-parity` delta to boot outcome + carousel nav only. |
| Spec gap — VST host | `HostedMainComponentV2` shares bug but was optional in risks only. | Add explicit requirement + scenario in `desktop-v2-boot-host-sync` spec. |
| Test gap — lifetime vs smoke | Boot subprocess test does not catch reintroduced `[&ctx]` without GUI. | Add `CallbackLifetime_test.cpp` (mock carousel, wire + invoke after return); tasks §4.3. |
| Terminology drift | Data flow diagram said `selectPageUiOnly(0)`; tasks say `selectPage(0, false)`. | Align on `selectPage(page, fireCallback)` parameter. |
| OMNI section thin | `design.md` had 4-line OMNI stub. | Expand to full compliance map + helper extraction table. |

### Implementation guardrails (from audit)

| Rule | Directive |
|------|-----------|
| Data flow | Carousel `onPageChanged` / `onRandomize` / `onRandomizeMod` use `[ctxPtr = &ctx]`; callers pass `m_hostCallbacks` by reference — never `[&ctx]` on the parameter, never braced temporaries at call sites. |
| Repetition | Both host components call `refreshAndWireHostCallbacks(m_hostCallbacks, ...)` — no copy-pasted braced-init + `wireCallbacks` blocks. |
| Accumulate then apply | `pushSelectPage`: `processBus` then **one** `syncToHost` then `carousel.refresh` — do not split sync across boot paths. |
| Defensive code | Do not add same-page early-return in `selectPage` unless profiling shows redundant host work on user re-clicks; `fireCallback` is sufficient for boot. |
| One-time helpers | `refreshAndWireHostCallbacks` — extract (trigger ≥2: `MainComponent`, `HostedMainComponentV2`; boundary: host callback binding; contract: fills ctx + wires). |

## Implementation status (2026-07-01 OMNI audit)

| Capability | Status | Verified evidence |
|---|---|---|
| `desktop-v2-boot-host-sync` | **NOT STARTED** | `[&ctx]` at `DesktopV2HostCallbacks.cpp` L27; temp at `MainComponent.cpp` L87 |
| `sim-operator-doc-parity` (delta) | **PARTIAL** | Engine/Stop/Shift/gates in `QUICK_DICT.md`; boot outcome + carousel arrows open |
| Boot smoke test | **NOT STARTED** | No `BootSmoke_test.cpp` in `desktop-v2/tests/` |
| Callback lifetime test | **NOT STARTED** | No unit test for post-`wireCallbacks` invoke |

**OMNI violations to fix during implementation:**

| Violation | Type | Fix | Task |
|---|---|---|---|
| `[&ctx]` on parameter temporary | Data flow / lifetime | `m_hostCallbacks` + stable capture | 1.1–1.3 |
| Duplicate `wireCallbacks` blocks | Repetition | `refreshAndWireHostCallbacks` | 1.5 |
| Braced-init in `pushSelectPage` / `pushRandomizeMod` | Data flow | Use `m_hostCallbacks` member | 1.6 |
| Boot double `syncToHost` | Data flow | `selectPage(0, false)` after `pushSelectPage(0)` | 2.1–2.2 |
| `HostedMainComponentV2` unaudited in spec | Artifact drift | Parity requirement in spec | spec |
| GUI-only verification | Verification | Boot smoke + callback lifetime tests | 4.1–4.3, OMNI.4–OMNI.5 |

## Artifact compliance status (post-audit)

Planning artifacts for this change are **OMNI-compliant** as of 2026-07-01. Remaining violations live in **source code** (not yet implemented). Exit explore mode and apply `tasks.md` §1–6 to fix code.
