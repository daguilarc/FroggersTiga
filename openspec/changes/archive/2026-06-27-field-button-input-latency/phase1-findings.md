# Phase 1 findings — SW1 stuck-input test (2026-06-26)

Firmware: `FroggersTiga` Phase 1 diag build  
MD5: `9e739163341728ec095a41f98cd65e22`  
Flash: 87,928 B @ `0x08000000` (`BOOT_NONE`, no bootloader)

## Summary

Phase 1 isolated **SW1 hardware / D30 electrical fault** on this Field unit. Firmware pin audit and stuck-switch suppression behaved as designed. **Phase 2 latency work will not repair SW1** on this hardware.

## On-device readings (user verified)

| Switch | r (RawState) | p (Pressed) | n (press count) | s (suppress) | Behavior |
|--------|--------------|-------------|-----------------|--------------|----------|
| SW1    | 1            | 1           | 1               | 1            | Never responds to deliberate press |
| SW2    | 0            | 0           | 0 → increments on press | 0 | Healthy |

- Boot page: **page 0 V1VO** (no runaway page flips — suppression worked)
- Pin audit: **cfg def** (default `D30`/`D29`, inverted, pull-up — SW2 validates this mapping)

## Interpretation

1. **`r=1` at rest on SW1** — GPIO `seed::D30` reads pressed *before* debounce. Not a poll-rate or OLED-blocking issue.
2. **Pin audit exhausted** — firmware tried `def`, `swap`, `norm`, `swpi`; none cleared SW1 raw at rest.
3. **`s=1` suppression** — boot stuck detection fired; page edges blocked correctly (stable page 0 at boot).
4. **SW1 unresponsive while `s=1` + stuck `r=1`** — no `FallingEdge()` to clear suppression; expected firmware behavior.
5. **SW2 healthy** — `D29` + libDaisy default config correct for this unit.
6. **`n=1` on SW1 at boot** — likely debounce settle during init, not a user press.

## Prior investigation (same unit)

- On-chip app matched known proto build md5 `37da0d1…` (DFU read-back) — identical bytes cannot explain SW1 vs SW2 difference.
- "Worked before" on same unit/binary is **unconfirmed**; Phase 1 data supersedes that claim for SW1.

## Phase 1 → Phase 2 decision

| Outcome | Action |
|---------|--------|
| SW1 `r=1`, audit did not fix | **Hardware repair** on SW1 / D30 / Field switch circuit |
| Phase 2 | Proceed for **SW2 / B-key latency** only; does not fix SW1 |

## Hardware next steps

1. Inspect/resolder SW1 tactile switch on Field carrier
2. Check continuity SW1 → Seed pin D30
3. Compare with another Field/Seed if available

## Related artifacts

- Raw test worksheet: `phase1-test-notes.md`
- Implementation: `src/common/FieldSwitchGuard.hpp`, `src/common/DaisyIO.hpp` (Phase 1 diag loop)
