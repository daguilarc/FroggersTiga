# Phase 2 release notes

## Binary (built, pending flash if device not in DFU)

- Path: `src/FroggersTiga/build/FroggersTiga.bin`
- Size: 87,260 B (66.57% of 128 KB)
- MD5: `81765ed607363b09fe13cde47e02e769`

## Changes from Phase 1 diag build

- Normal parameter OLED UI (no diag screen)
- Fast control poll every loop iteration
- OLED refresh throttled (~30 FPS) + dirty flag on page/randomize
- B2/B4 (Rand All) queued via `FieldMutationQueue`; drained one per loop
- B1/B3 remain immediate
- `FieldSwitchGuard` retained (boot pin audit + SW1 suppression)

## Flash

```sh
cd src/FroggersTiga
make program-dfu GCC_PATH=/Applications/ArmGNUToolchain/14.3.rel1/bin
```

## Expected behavior on this unit

- **SW1:** still hardware-stuck per `phase1-findings.md`; suppression may block page nav
- **SW2 / B-keys:** should feel more responsive under audio load vs Phase 1 diag loop

## User bench (task 6.2)

≥5 rapid SW2 taps in 2 s with audio playing; page title should track presses.
