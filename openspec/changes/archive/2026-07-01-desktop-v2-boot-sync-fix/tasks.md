## OMNI verification gates (run before merge)

- [x] OMNI.1 `rg '\[&ctx\]' desktop-v2/Source/` — zero matches in stored lambdas
- [x] OMNI.2 `rg 'wireCallbacks\(\{' desktop-v2/Source/` — zero braced temporaries at call sites (use `m_hostCallbacks` / `refreshAndWireHostCallbacks`)
- [x] OMNI.3 `rg 'setActivePage\(0\)' desktop-v2/Source/MainComponent.cpp desktop-v2/Source/HostedMainComponentV2.cpp` — zero matches in constructors (use `selectPage(0, false)`)
- [x] OMNI.4 `ctest` desktop-v2 — `CallbackLifetime_test` + `BootSmoke_test` pass or skip with message
- [x] OMNI.5 Manual: `open FroggersTigaV2.app` — `pgrep FroggersTigaV2` alive after 3s (user approved)
- [x] OMNI.6 `rg '\*\*Release v1\.0\.4\*\*' QUICK_DICT.md` — unchanged release line after doc edits (line absent before and after; not introduced)

## 1. Fix callback lifetime (blocking)

- [x] 1.1 Add `m_hostCallbacks` member to `MainComponent` and `HostedMainComponentV2` (`HostCallbackContext` with refs to core, bridge, host, carousel, modRoutesVersion)
- [x] 1.2 Change `wireCallbacks` lambdas to `[ctxPtr = &ctx]` (pointer by value), not `[&ctx]` on the parameter (`DesktopV2HostCallbacks.cpp`); callers pass `m_hostCallbacks` ref so `ctxPtr` addresses the member
- [x] 1.3 Replace per-host `wireCallbacks()` bodies with `refreshAndWireHostCallbacks(m_hostCallbacks, ...)` call
- [x] 1.4 Audit `HostedMainComponentV2.cpp` — same temporary-context pattern at L36–38 (verified duplicate)
- [x] 1.5 Implement `desktop_v2::refreshAndWireHostCallbacks` in `DesktopV2HostCallbacks.cpp` / `.hpp` (assign refs + call `wireCallbacks`)
- [x] 1.6 Route `pushSelectPage` and `pushRandomizeMod` on both hosts through `m_hostCallbacks` — no braced temporaries

## 2. Fix boot double-sync (blocking)

- [x] 2.1 Add `fireCallback` parameter to `PageCarouselComponent::selectPage` (default `true`); guard `onPageChanged` behind it
- [x] 2.2 `MainComponent` ctor: keep `pushSelectPage(0)`; call `selectPage(0, false)` instead of `setActivePage(0)` for UI-only sync
- [x] 2.3 Mirror boot sequence in `HostedMainComponentV2` ctor (L27–29)

## 3. Verification (blocking)

- [x] 3.1 Rebuild `desktop-v2` release binary
- [x] 3.2 Manual: `open FroggersTigaV2.app` — process alive after 3s (`pgrep FroggersTigaV2`) (user approved)
- [x] 3.3 lldb: launch once — no stop in `SetGlobalCrunchy` (user approved)
- [x] 3.4 `ctest` desktop-v2 — change-specific tests pass via OMNI.4 (`CallbackLifetime_test` + `BootSmoke_test`); full suite blocked by pre-existing `ControlCoreBridge_test` `SetPageParam` (API absent on `DesktopHostIO`) — out of scope for this change
- [x] 3.5 `ctest` sim — no regressions (20/20)
- [x] 3.6 v1 regression — N/A; `6e8fa27` touches `desktop-v2/` only; no v1 `desktop/` files changed

## 4. Automated regression tests (blocking)

- [x] 4.1 Add `desktop-v2/tests/BootSmoke_test.cpp` (subprocess launch + alive check, skip if binary missing)
- [x] 4.2 Register both new tests in `desktop-v2/CMakeLists.txt`
- [x] 4.3 Add `desktop-v2/tests/CallbackLifetime_test.cpp` — wire callbacks, exit wiring scope, invoke `onPageChanged`; assert no fault and valid host ref

## 5. Operator docs (delta only, no version bump)

- [x] 5.1 `QUICK_DICT.md`: add **Boot outcome** gloss (window stays open = healthy boot) under Transport or new §Desktop v2 boot
- [x] 5.2 `QUICK_DICT.md`: add **carousel left/right arrows** page navigation (cross-ref existing Rand / Rand mod lines in §Transport)
- [x] 5.3 Run `scripts/sync-help-docs.sh` (updates `docs/`, `web/public/` mirrors — no `SIM_MANUAL.md` edits in this change)
- [x] 5.4 Confirm `**Release v1.0.4**` unchanged

## 6. Fork hygiene

- [x] 6.1 Commit fix to `froggerstiga-desktop-v2` (not `main`) — `6e8fa27`
- [x] 6.2 Push after verification passes
