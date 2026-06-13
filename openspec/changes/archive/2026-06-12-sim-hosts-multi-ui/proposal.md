## Why

FroggersTiga firmware is tied to Daisy Field hardware (8 knobs, page switches, OLED). We need browser and desktop simulators plus a future VCV module sharing one portable DSP core—but a single “Field clone” UI is wrong for desktop (screen space, simultaneous parameter access) and the current plan locks sample rate, conflates host UIs, and leaves `PageManager` knob routing incompatible with multi-panel desktop control.

## What Changes

- **Portable core** (`src/core/`): `FroggersEngine`, moved DSP headers, no `daisy` in include graph; firmware keeps thin shim.
- **Sample rate v1**: default **44.1 kHz** for sim hosts; `FroggersEngine::SetSampleRate(float)` replaces hardcoded `48000` literals; firmware remains 48 kHz (Daisy hardware).
- **Host adapters split by UX**, not one `HostIO` shape for all:
  - **PagedHostIO** — SW1/SW2 page switch, 8 shared knobs, OLED rows (web mobile, VCV Rack).
  - **DesktopHostIO** — five adjacent identical sub-panels (per-panel **Randomize** + **Randomize mod**, knobs 1–7 + FUEG, inline param labels + mod input jacks — no mini-OLED duplicate rows); no page switching; **mod rack** above panels (four module boxes + VCV-style patch cables); **shared strip** for global **Randomize all** / **Randomize mod** (all pages), **Marbles**, **XCPL −/+**, **Randomize waves**; desktop `Init()` forces all params to Tracking (no hardware pickup).
- **PageManager extensions**: `KnobUpdateOnPage(page, position, value)` so desktop updates any page without `SelectPage`; `RandomizePage(page)` and `RandomizeAllPagesIndependent()` so desktop B1/B2 read per-page stored values instead of stale `m_knobPositions`; paged hosts keep current `KnobUpdate` / `RandomizeAllPages` on `m_currentPage`.
- **Web sim**: Vite + TS + WASM AudioWorklet; Field-ish paged UI; **Mic** toggle (default **off** — VCO-only until user enables mic and grants permission); GitHub Pages via `docs/`.
- **Desktop sim**: JUCE native app; horizontal rack of five sub-modules + shared mod/button strip; links same core.
- **VCO waveform morph (sim hosts only — NOT Daisy firmware)**: Each VCO (1–3) gets a **continuous** sine → saw → square morph (smooth blend, not discrete steps), driven by a **linear 0–1** morph knob (`GetMorph` clamps modulated knob; no `ExpParam(0,1)`). Morph targets are **CV-modulatable** through the same ModMgr attenuator path as other parameters. UI shows a **wave icon** beside VCO row labels on the Audio panel — **no separate OLED row or slider bar**. Shared strip keeps **randomize-all-waves**; wave buttons cycle morph. Firmware keeps discrete A8/B8 button cycling and sine-only VCO3; **`DaisyIO.hpp` unchanged**. Queue + NaN fix: `desktop-vco-morph-fix`.
- **MIDI I/O (desktop + VCV only — NOT Daisy firmware)**: **MIDI in** (one channel + one CC → `m_mods[0]` in sim hosts) drains in `tickControls()` before `ProcessBlock`; **MIDI out** sends envelope CC after each block. VCV may retain four CV jacks (hardware parity); desktop/web sim present **one** external MIDI mod source. Web sim has no MIDI in v2.1.
- **Desktop external audio input**: User-selectable audio input device + channel for the **external ring-mod signal** fed to `ProcessSample` (separate from main output device / monitoring).
- **VCV (phase 2)**: GPL wrapper in `vcv/` links MIT `src/core`; one-page-at-a-time Rack module with Field-parity jacks (§6.3). Rack Library publication remains a separate review gate.
- **OMNI fixes** from audit: canonical `tickControls()` once per block; single C++ host logic (TS transport only); no copy/symlink fork for core headers.
- Pause or revert in-progress web/desktop work that assumed identical Field UI and 48 kHz lock.

### v2 UX (sim hosts — shipped scaffold)

Transport, human labels, VCO row controls, Play/Stop, global strip — implemented. Desktop **External Off/L/R** host switch was a **mistake** (web Mic model); corrected in `desktop-host-corrections` — engine gate only. Desktop mod dropdowns were a **mistake** (see v2.1).

### v2.1 UX (sim hosts — correct mod model)

**Sim hosts have no external CV.** Only:

| UI source | Core `m_modIndex` | Origin |
|-----------|-------------------|--------|
| **MIDI** (desktop only) | `0` | One MIDI in channel + one CC → `m_mods[0]` |
| **VCO level** (was "VCO feat") | `4` | Internal (`UpdateM5FromVco`) |
| **Marbles 1** | `5` | Internal |
| **Marbles 2** | `6` | Internal |

`m_mods[1..3]` are **unused in sim UI** (hardware Field CV only). Firmware `M1..M7` / CV jacks unchanged.

- **Desktop mod rack**: four **module boxes** (meter + output jack), not a text row. **VCV Rack-style patch cables** ([Getting Started](https://vcvrack.com/manual/GettingStarted)): **drag from output jack** → cable follows cursor → **drop on input jack** to connect; drop on empty space to cancel. One source → many destinations (Field semantics). **Not** two-click “arm then click”.
- **Web mod UI**: per-knob **dropdown only** (`None | VCO feat | Marbles 1 | Marbles 2`; no MIDI in browser v2.1). Same one-to-many assignment model in core.
- **Per-panel actions**: **Randomize** + **Randomize mod** (hardware B1 + B3), not global-only mod randomize.
- **Layout**: remove desktop per-knob mod dropdowns — that alone restores full `V1VO` labels at current panel width; **no** panel widening required. Web: mod dropdown **below** each knob column, not beside the name.
- **MIDI panel** (desktop): device picker, **one** in channel + in CC, envelope CC out — not four pseudo-CV lanes.

**Non-goals (v2.1):** Sim hosts do not expose M1–M4 as separate CV sources. VCV phase 2 keeps Field-parity CV1–4 jacks mapped to core indices 0–3.

## Capabilities

### New Capabilities

- `froggers-core`: Portable engine extraction, `ProcessBlock`/`ProcessSample`, sample-rate parameterization.
- `paged-host-io`: SW1/SW2, shared 8-knob model, OLED query API for web and VCV.
- `desktop-host-io`: Multi-panel knob routing, per-page FUEG, shared global buttons, MIDI I/O bridge, external audio input selection.
- `sim-midi-io`: Envelope→MIDI CC out; sim hosts: one MIDI CC in→`m_mods[0]`; VCV phase 2 adds Field-parity CV jacks.
- `web-simulator`: Browser WASM sim, paged UI, GitHub Pages deploy.
- `desktop-simulator`: JUCE app with adjacent sub-module layout.
- `vcv-module`: Rack 2 plugin (phase 2, license-gated).
- `sim-mod-patchbay`: Sim mod source rack (MIDI + 3 internal), patch cables (desktop), dropdown assign (web), one-to-many routing.

### Modified Capabilities

- (none — no existing `openspec/specs/` baseline in repo)

## Impact

- `src/core/`, `src/common/` re-exports, `src/FroggersTiga/` firmware shim
- New `wasm/`, `web/`, `desktop/`, `.github/workflows/pages.yml`
- `Page.hpp` / `PageManager` API additions (`KnobUpdateOnPage`, `RandomizePage`, `RandomizeAllPagesIndependent`)
- `FroggersEngine.hpp`: `m_sampleRate`; `SetSimWaveMorph(bool)` (default false — firmware never enables); sim-only `VcoWaveMorph[3]` (`EvalWaveMorph`, exp knob map, ModMgr-modulatable); firmware keeps `m_vco1Wave`/`m_vco2Wave` uint8 from A8/B8 and sine VCO3
- New `src/core/CvMidiBridge.hpp`, `src/core/CvPresence.hpp`: MIDI bridge + shared CV presence loop; JUCE `AudioDeviceSelector` for external input on desktop
- Partial implementation on disk (`HostIO.hpp`, `web/`) refactored into `PagedHostIO` / `DesktopHostIO`
- Plan file `.cursor/plans/desktop_sim_vcv_rack.plan.md` superseded by this change for host UX and sample-rate decisions
