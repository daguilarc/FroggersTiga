## Why

Sim hosts (web + desktop) benefit from a stereo delay that the Daisy Field does not need. Delay is **sim-only overlay**: sixth page/panel in the UI, host-owned params and DSP, **pre-reverb** insert in the FX chain. Firmware keeps five pages; WASM keeps `froggers_num_pages() === 5`. Core gets only a **null-safe sim hook** between resonant bump and reverb — not a sixth `PageManager` page.

## What Changes

- **Sim overlay page 6** (host index 5, label **Delay**): 7 knobs + **FUEG**, same chrome as Field panels. Params live in `DelayState` (host), not `PageManager`.
- **`StereoDelay` DSP** in `sim/` (linked into desktop + sim WASM): pre-reverb insert via core hook; **DTIM** 0–3 s exponential; **DSND**, **DFBK**, **DWID**, **DTON**, **DMOD**, **DMIX**, **FUEG**.
- **Pre-reverb routing**: `bump → StereoDelay → ProcessReverb (RPRE + tank) → rvMix`. **RPRE** and **DTIM** are independent controls.
- **FUEG v1**: shared `sim/Fuegoize.hpp` (extracted from core `Parameter` XOR path); applies to Delay params 0–6 — **functional on ship**, not a decorative knob.
- **Stereo output**: L/R when device has ≥2 channels; mono `(L+R)*0.5` fallback. Width from delay wet L/R applied at **host output bus** after core `ProcessBlock` (core reverb path stays mono).
- **Randomize parity**: per-panel and global **Randomize all / Randomize mod (all)** include Delay.
- **Patch cables**: Delay jacks → `DelayState` mod matrix; overlay branches `page >= 5` — never `SetPageModSource(5, …)`.
- **Web**: large ◀ ▶ arrows; host page 0–5; page 5 = host state + synthetic OLED.
- **Desktop**: six panels; default **1680×720** (was 2016×720; superseded by `desktop-compact-layout`).
- **Firmware**: unchanged — hook null, no Delay page.

### Parameter map (7 + FUEG)

| Knob | ID | Role |
|------|-----|------|
| 1 | **DTIM** | Delay time — 0–3 s, exponential |
| 2 | **DSND** | Send — bump bus into delay |
| 3 | **DFBK** | Feedback |
| 4 | **DWID** | Stereo width — L/R offset + ping-pong |
| 5 | **DTON** | Tone — LP in feedback path |
| 6 | **DMOD** | Modulation — LFO depth on delay time |
| 7 | **DMIX** | Wet mix — delay return level |
| 8 | **FUEG** | Fuegoizer — XOR scramble on params 0–6 via `Fuegoize.hpp` |

## Capabilities

### New Capabilities

- `stereo-delay`: `StereoDelay`, pre-reverb hook, `DelayState`, `Fuegoize.hpp`, stereo output bus.
- `host-ui-delay-page`: Sixth page/panel, arrows, patch cables, randomize parity.

### Modified Capabilities

- (none)

## Impact

- **`src/core/FroggersEngine.hpp`** — optional `SimFxInsertFn` after bump, before `ProcessReverb`; null on firmware; 4096-sample identity test.
- **`sim/`** — `StereoDelay.hpp`, `DelayState.hpp`, `Fuegoize.hpp` + parity test (no PageManager, no firmware link).
- **`desktop/`** — `IPanelBackend`, sixth panel, patch overlay branch, stereo bus, stop clearing output ch1.
- **`web/`** — `hostPage` 0–5, `froggers_select_page(host, 0..4)`, delay WASM exports, stereo worklet bus.
- **sim `wasm/`** — links `sim/`; `froggers_num_pages()` stays 5; new delay + select-page exports only.
- **`src/FroggersTiga/` firmware** — unchanged.
