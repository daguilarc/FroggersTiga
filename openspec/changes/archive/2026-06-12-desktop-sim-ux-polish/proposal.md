## Why

The desktop sim shipped with broken or misleading UX relative to the `sim-hosts-multi-ui` contract: sample rate follows the OS device (often 48 kHz) instead of the mandated 44.1 kHz default; mod-rack meters are misread as external audio input; patch cables are effectively unusable at current hit targets and port sync; and six narrow panels truncate VCO/wave labels while wasting horizontal space on tall vertical sliders. These are not polish — they block basic use of the sim rack.

## What Changes

- **Force 44.1 kHz** for desktop sim audio: configure `AudioDeviceManager` preferred rate, call `SetSampleRate(44100)` on engine and delay at init and on device start; resample or reject non-44100 devices with a clear settings hint.
- ~~**Separate ring-mod input meter** with on/off toggle~~ **SUPERSEDED** by `desktop-host-corrections`: desktop uses engine `m_extGate` only; optional passive input meter; no host Off switch.
- **Mod rack meter semantics**: rename/labeled as **mod bus output** (MIDI, VCO feat, Marbles 1/2); meters only move while audio is running (document in UI tooltip or subtitle).
- **Patch cable fix**: enlarge jack hit radius; sync port bounds every timer tick (not only on resize); optional jack hover highlight; verify z-order and drag-from-output → drop-on-input on all six panels including Delay.
- **Compact rotary knobs** replace `LinearVertical` sliders in `SubModulePanel` (and Delay panel): ~36–40 px diameter, same mod-depth vs knob-value dual mode on refresh; when patched and not dragging, show **effective modulated parameter** (not just attenuator depth) so controls visibly track modulation.
- **Label layout fix**: full `V1VO` / `V2VO` / `V3VO` text plus wave glyph without ellipsis at six-panel width (~336 px per panel); re-row layout — label column wider, knob column fixed width, jack between label and knob.
- **Panel density**: reduce row height by ~40% so six columns fit without ridiculous vertical scroll feel.

## Capabilities

### New Capabilities

- `desktop-sim-sample-rate`: Desktop sim SHALL run DSP at 44.1 kHz regardless of OS default device rate.
- `desktop-ring-mod-meter`: Dedicated external ring-mod input level meter separate from mod-rack CV meters.
- `desktop-patchbay-ux`: Patch cable interaction SHALL be reliable at six-panel layout width.
- `desktop-panel-knobs`: Submodule panels use compact rotary knobs with live modulated-value display when patched.

### Modified Capabilities

- (none — prior requirements live in change-folder specs under `sim-hosts-multi-ui`; this change adds delta specs only)

## Impact

- `desktop/Source/AudioEngine.cpp` — device init, sample rate, expose envelope level for UI
- `desktop/Source/MainComponent.*` — ring-mod meter, port sync cadence
- `desktop/Source/SubModulePanel.*` — knob control, layout, effective-value refresh
- `desktop/Source/PatchCableOverlay.*` — hit testing, hover, port sync hooks
- `desktop/Source/ModModuleBox.*` — clearer labels / subtitles for mod bus meters
- `src/core/DesktopHostIO.hpp` or `PageManager` — query effective knob value after modulation (if not already exposed)
- No firmware, WASM page count, or Daisy changes
