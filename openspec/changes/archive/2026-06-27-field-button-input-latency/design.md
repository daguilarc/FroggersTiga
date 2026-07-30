## Context

Daisy Field firmware (`src/common/DaisyIO.hpp` + `src/FroggersTiga/`) runs a single-threaded loop:

```text
while (true) { ProcessControls(); UpdateScreen(); }
```

`UpdateScreen()` performs a full SSD1306 redraw every iteration. `ProcessControls()` samples SW1/SW2, keyboard (B1–B4), and knobs once per lap. Audio runs in the ISR via `FroggersEngine::ProcessBlock`.

**Regression boundary** (git): commit `14e41eb` added 3-VCO `FroggersTiga` (~84 KB) while keeping the same loop structure. Proto `src/Froggers` (~81 KB, filter+drive only) felt responsive. SW1/SW2 were moved outside the `m_modIndex` gate in that commit, but user-verified behavior regressed — overload dominates, not the gate.

**Toolchain audit** (proto `e0ae431` vs HEAD): `APP_TYPE=BOOT_NONE`, `OPT_LEVEL=-Os`, `USE_LTO=1`, Arm GNU 14.3.rel1 — unchanged intent. Only `config.mk` path discovery widened (`bin` vs `arm-none-eabi/bin`). **No bootloader change.**

Desktop sim already solves heavy randomize via `DesktopHostIO` mutation queue + `HostRandomize` helpers. Field firmware has no `AudioPairArState` / `DelayState` — queue applies **PageManager-only** mutations.

## Goals / Non-Goals

**Goals:**

- SW1/SW2 page changes register within **≤20 ms** perceived latency under full audio load (user test).
- B1–B4 edges never dropped due to a single long `RandomizeAllPages` call blocking the poll loop.
- Poll `ProcessAllControls()` at **≥200 Hz** effective rate (target **≥500 Hz** when OLED idle).
- Reuse existing randomize implementations (`PageManager`, `HostRandomize` where applicable) — no duplicate RNG logic.
- Document toolchain parity; fix makefile only if flags drift.

**Non-Goals:**

- Bootloader / `BOOT_SRAM` / `BOOT_QSPI` migration.
- DSP changes, flash size reduction, or moving engine off internal flash.
- Sim/desktop/web host changes.
- Knob pickup (`> < =`) behavior.
- `-O0` debug builds on device.

## Decisions

### D1 — Decouple poll from OLED (primary fix)

**Choice:** Split `MainLoop` into:

1. **Fast path (every iteration):** `ProcessAllControls()` + enqueue/dequeue + LED transmit.
2. **Slow path (throttled):** `UpdateScreen()` only when `m_screenDirty` or **≥33 ms** since last frame (~30 FPS cap).

**Rationale:** Proto felt fine because the loop was short; VCO load stretched OLED+control together. Throttling OLED restores poll rate without removing visual feedback.

**Alternative rejected:** SysTick ISR for buttons only — more invasive; throttle is smaller diff.

### D2 — Mutation queue for heavy randomize (B2, B4)

**Choice:** On `KeyboardRisingEdge(1)` / `(3)`, enqueue `RandomizeAllPages` / `RandomizeAllPagesMod`. Drain **at most one mutation per fast-loop iteration** (or per N µs budget). Coalesce duplicate pending Rand-All like desktop (`enqueueMutation` dedup).

B1/B3 (single page) remain **immediate** — 8 params, cheap.

**Rationale:** Rand All is O(pages × 8) synchronous work; spreading it prevents multi-ms poll blackout.

**Alternative rejected:** Fixed 5 ms `Delay` — arbitrary; budget-based drain adapts to load.

### D3 — Page switches stay immediate

**Choice:** SW1/SW2 `RisingEdge` → `PagePrevious`/`PageNext` inline, set `m_screenDirty = true`.

**Rationale:** Page step is 16 param updates — acceptable on fast path; user priority is SW responsiveness.

### D4 — Optional audio-tick drain

**Choice:** If loop throttle insufficient, call `drainOneMutation()` from `App::Process` once per audio block (≤3 ms cadence at 48 kHz / 128 samples).

**Rationale:** Matches desktop `tickControls` pattern; keeps sound param updates soon after Rand All without blocking UI poll.

**Defer:** Implement D1+D2 first; add D4 only if bench still fails.

### D5 — Shared mutation types, not full DesktopHostIO

**Choice:** Extract minimal `FieldMutation` enum + ring buffer into `src/common/FieldMutationQueue.hpp` (or reuse `HostMutationType` subset + `applyFieldMutation(PageManager&)` helper). **Do not** link `DesktopHostIO` on firmware (pulls delay, atomics, sim-only paths).

**Rationale:** OMNI repetition rule — one apply path, field-specific subset.

### D6 — Toolchain parity gate

**Choice:** Add `sim/check_firmware_toolchain_parity.sh` comparing `config.mk` / `daisy.mk` flags against documented proto baseline; run in `host-preflight.yml` only if firmware paths change, or document as manual pre-flash step.

**Rationale:** User asked for compiler consistency check; audit shows no flag regression — script prevents future drift.

### D7 — Verification harness

**Choice:** Bench procedure in `tasks.md`: GPIO toggle or `SW` LED scope on SW1 rapid tap with audio full; optional `DWT_CYCCNT` around loop body in `DEBUG_FIELD_LOOP` compile flag (off in release).

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| OLED feels less smooth at 30 FPS | Dirty-only updates on knob move; full refresh on page change |
| Queue overflow on Rand All spam | Coalesce B2/B4; cap queue depth 8 |
| Audio/main PageManager race | Apply mutations only on main thread; audio reads params after block boundary (existing pattern) |
| D4 adds ISR work | Gate behind compile flag; measure cycles |

## Migration Plan

1. Implement in `src/common/DaisyIO.hpp` (+ small helper header).
2. `make clean && make` in `src/FroggersTiga`; flash via existing DFU flow (`MANUAL.md`).
3. A/B: rapid SW1 tap with dense audio — compare to `e0ae431` `src/Froggers` build if needed.
4. Update `MANUAL.md` only if acceptance criteria change wording.

**Rollback:** Revert DaisyIO loop changes; firmware binary only.

## Open Questions

- None blocking — D4 is optional follow-up within same change if D1+D2 fail bench.
