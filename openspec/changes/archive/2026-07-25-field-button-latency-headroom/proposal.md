## Why

Archived `field-button-input-latency` Phase 2 decoupled OLED from the poll loop and queued B2/B4, but Daisy Field still feels slow or freezes under rapid randomization. The remaining cost is audio ISR headroom (`UpdateParams` every sample), full `RandomizeAllPages` inside a single drain, and LED I2C every poll — not reverb RAM. This change restores button latency without removing the reverb page.

## What Changes

- Move `UpdateParams()` from per-sample to once per audio block (after `ReadParamsBlock`) in `FroggersEngine`.
- Drain B2/B4 mutations **one page per** `FieldMutationQueue::DrainOne` (keep coalesce; B1/B3 stay immediate).
- Throttle `led_driver.SwapBuffersAndTransmit()` to dirty / ~30 Hz (compute LEDs every poll; push less often).
- Dry-reverb bypass with hysteresis (`enter 1e-4` / `exit 5e-4`) after `m_rvMix.Process()`; **keep** the reverb page, params, and delay buffers.
- Tighten `field-button-input-latency` acceptance for poll rate and Rand All spam behavior.
- Update `MANUAL.md` / `docs/daisy-field-diagnostics.md` so operator claims match (CPU headroom, not memory / reverb removal).

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `field-button-input-latency`: Add requirements for block-rate param apply, page-cursor Rand All drain, LED transmit throttle, dry-reverb early-out, and tighter acceptance under B2/B4 spam.
- `field-operator-doc-parity`: MANUAL / diagnostics wording must record that randomize freezes were CPU/poll headroom, reverb page retained.

## Impact

- `src/core/FroggersEngine.hpp` — `ProcessBlock` / `ProcessSample` / `ApplyOutputFx` / `ProcessReverb`
- `src/common/FieldMutationQueue.hpp` — page-cursor drain state machine
- `src/common/DaisyIO.hpp` — LED transmit throttle
- Hosts that share `FroggersEngine::ProcessBlock` (desktop/sim) — regression build check
- `MANUAL.md`, `docs/daisy-field-diagnostics.md`
- Out of scope: remove reverb page; bootloader; desktop-v2 Rand All stacking; SW1 hardware fault
