# Daisy Field Diagnostics

Persistent hardware/firmware diagnostic log for the Daisy Field running FroggersTiga.
This document outlives individual OpenSpec changes — keep diagnostic findings here.

---

## SW1 stuck-input (2026-06-26)

**Verdict: hardware fault on SW1 / Seed pin `D30` for this unit.** Firmware is correct; SW2 proves the input path, mapping, and build are healthy.

### How it was diagnosed

A diagnostic firmware (`FroggersTiga` Phase 1 build, md5 `9e739163341728ec095a41f98cd65e22`) exposed per-switch state on the OLED:

```
SW1 r? p? n? s?     r=RawState (pre-debounce GPIO), p=Pressed (debounced),
SW2 r? p? n? s?     n=press count, s=stuck-suppression flag
cfg def|swap|norm|swpi   pin-audit result
page N name
```

The build also runs a **boot pin audit** (tries default / swapped pins / normal polarity) and a **stuck-switch suppression** guard (`src/common/FieldSwitchGuard.hpp`) that blocks page edges from a switch already pressed at boot.

### On-device readings (user-verified)

| Switch | r (RawState) | p (Pressed) | n (count) | s (suppress) | Behavior |
|--------|--------------|-------------|-----------|--------------|----------|
| SW1    | 1            | 1           | 1         | 1            | Never responds to deliberate press |
| SW2    | 0            | 0           | counts up on press | 0   | Healthy |

- Pin audit result: **`cfg def`** (default `D30`=SW1 / `D29`=SW2, inverted, pull-up). SW2 validates this mapping.
- Boot page stable at `page 0 V1VO` — suppression prevented runaway page flips.

### Interpretation

1. SW1 `r=1` at rest = GPIO `seed::D30` reads pressed **before** debounce → electrical, not timing/firmware.
2. Pin audit exhausted (`def`/`swap`/`norm`/`swpi`); none cleared SW1 raw → software cannot fix.
3. SW1 unresponsive is expected once stuck: no `FallingEdge()` to clear suppression.
4. SW2 healthy on identical code → isolates the fault to the SW1 switch / D30 line on this board.

### Corroborating evidence

- On-chip app read back via DFU matched the known proto build (md5 `37da0d1cb7f89fd837de5aade6817450`) — identical firmware bytes cannot explain SW1≠SW2, so the difference is hardware.
- Not related to libDaisy SRAM-bootloader issue [#534](https://github.com/electro-smith/libDaisy/issues/534): this firmware is `BOOT_NONE` (internal flash `0x08000000`), no Daisy bootloader at runtime.

### Hardware next steps

1. Inspect/resolder the SW1 tactile switch on the Field carrier board.
2. Check continuity SW1 → Seed pin `D30`.
3. Compare against another Field/Seed if available.

### Firmware references

- `src/common/FieldSwitchGuard.hpp` — boot pin audit + stuck-switch suppression
- `src/common/DaisyIO.hpp` — switch handling, diagnostic screen (Phase 1 diag build)

> Source: archived OpenSpec change `field-button-input-latency` (Phase 1). Raw worksheet preserved in that change's `phase1-test-notes.md`.
