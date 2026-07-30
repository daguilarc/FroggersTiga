## 0. Phase 1 — SW1 stuck-input test firmware (first; flash and test before §1–§6)

- [x] 0.1 Add input diagnostic mode on OLED: SW1/SW2 `r` (RawState), `p` (Pressed), press count, suppression flag, pin-audit label
- [x] 0.2 Boot pin audit: verify libDaisy defaults (`D30`/`D29`, inverted, pull-up); if stuck at rest, try swapped pins and/or normal polarity; record chosen config on screen
- [x] 0.3 Boot-time stuck-switch detection: flag switches pressed at rest; suppress page edges until clean release
- [x] 0.4 Wire Phase 1 main loop (diag screen + audio); keep B1–B4/SW behavior otherwise unchanged except suppression
- [x] 0.5 `make clean && make` in `src/FroggersTiga`; confirm `.bin` fits 128 KB internal flash
- [x] 0.6 Flash via DFU (`make program-dfu`); do not change bootloader
- [x] 0.7 **User test:** at rest record SW1/SW2 `r`/`p`; tap SW1/SW2; confirm suppression stops spurious page changes; record Phase 1 → Phase 2 decision in change notes (see `phase1-findings.md`)

## 1. Toolchain parity audit

- [x] 1.1 Diff `src/mk/config.mk` and `src/mk/daisy.mk` against proto Froggers baseline (`e0ae431`); record `APP_TYPE`, `OPT_LEVEL`, `USE_LTO`, toolchain version in change notes or `docs/CI.md` if drift found — see `toolchain-audit.md`
- [x] 1.2 Add `sim/check_firmware_toolchain_parity.sh` that fails on flag drift (`BOOT_NONE`, `-Os`, `USE_LTO=1`); wire into `host-preflight.yml` only when `src/mk/**` or `src/FroggersTiga/**` changes — script added; no `host-preflight.yml` in repo (N/A)

## 2. Field mutation queue

- [x] 2.1 Add `src/common/FieldMutationQueue.hpp` with `FieldMutationType` (`RandAll`, `RandAllMod`), ring buffer depth 8, coalescing enqueue, `drainOne(PageManager&)`
- [x] 2.2 Implement apply using existing `PageManager::RandomizeAllPages` / `RandomizeAllPagesMod` — no DesktopHostIO or sim-only state on firmware

## 3. DaisyIO main-loop refactor

- [x] 3.1 Split `MainLoop`: fast path calls `ProcessAllControls()` every iteration; slow path calls `UpdateScreen()` only when `m_screenDirty` or ≥33 ms since last frame
- [x] 3.2 Set `m_screenDirty` on page change, param edit, and Rand All completion
- [x] 3.3 Route B2/B4 rising edges to `enqueueMutation`; call `drainOne` once per fast-loop iteration
- [x] 3.4 Keep B1/B3 immediate; keep SW1/SW2 outside `m_modIndex` gate with immediate `PagePrevious`/`PageNext`
- [x] 3.5 Verify nesting ≤3 in modified functions; no duplicate randomize blocks (OMNI repetition)

## 4. Optional audio-tick drain (only if bench fails 3.x)

- [ ] 4.1 If SW1 bench still fails after 3.x, call `drainOne` from `App::Process` once per audio block behind `FIELD_MUTATION_AUDIO_DRAIN` macro — deferred; SW1 is hardware (Phase 1); run after 6.2 if SW2/B latency still fails

## 5. Build and flash

- [x] 5.1 `make clean && make` in `src/FroggersTiga`; confirm `.bin` fits 128 KB internal flash — 87,260 B (66.5%)
- [x] 5.2 Flash via DFU (`make program-dfu` per `MANUAL.md`); do not change bootloader — flashed 2026-06-27, md5 `81765ed607363b09fe13cde47e02e769`

## 6. Documentation and verification

- [x] 6.1 Update `MANUAL.md` SW1/SW2 troubleshooting: fast poll path, B2/B4 async, bootloader not involved
- [x] 6.2 Run acceptance bench: ≥5 rapid SW1/SW2 taps in 2 s with audio full; 4/5 page title changes within one throttle frame — **PASS (user-verified 2026-06-27):** SW2 + B1–B4 responsive under audio load; boots to audio page. SW1 N/A (hardware fault per Phase 1).
- [ ] 6.3 Optional A/B: flash proto `e0ae431` `src/Froggers` build and compare SW feel if regression unclear
