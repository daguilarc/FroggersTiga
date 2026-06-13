## Why

Desktop sim never finished `desktop-host-corrections` §1: a host-level **External** gate zeros line input before the engine Schmidt gate runs, so the checkbox feels broken and the **input level bar** stays an empty black box. Users also report runaway clipping/feedback after sim-only FX (stereo delay → reverb) with no output headroom guard. Label **External** is vague; **Ext. In.** matches Quick Dict and the meter’s role.

## What Changes

- **Rename** transport toggle **External** → **Ext. In.** (tooltip explains ring-mod routing + Schmidt gate).
- **Routing model (finish host-corrections §1)** — **Ext. In. ON** + device has input channels → always copy `inputChannelData[0]` into `m_inBlock` while Play runs. **Ext. In. OFF** → zeros (VCO-only). Engine Schmidt gate (`hasExternal`) unchanged — no second host gate beyond the checkbox.
- **Default Ext. In. OFF** at launch (VCO-only until user opts in; matches web External: Off).
- **Input level meter** — bar beside **Ext. In.** shows **peak input level** (post-limit, pre-Schmidt) when Ext. In. on + Play; dim idle track when off or stopped; minimum visible tick so the box is never a dead black void.
- **Output headroom** — soft limiter on stereo bus out (desktop callback only); extend `SoftResetFxState` to clear **comb** delay line; stereo delay `softResetFx` already clears delay buffers.
- **Docs** — `QUICK_DICT.md` transport line: **Ext. In.** ; note Schmidt gate in tooltip/MANUAL_VERIFY.
- **Unchanged** — web **External: Off/On** mic toggle; firmware; stereo delay insert topology.

## Capabilities

### New Capabilities

- `desktop-external-input-routing`: Ext. In. checkbox semantics, default-off, Field-parity Schmidt gate only in engine.
- `desktop-input-level-meter`: Peak input level bar beside Ext. In. with visible idle/active states.
- `desktop-output-headroom`: Post-mix soft limiter + comb state in FX soft-reset.

### Modified Capabilities

- (none — `desktop-host-corrections` not yet in `openspec/specs/`)

## Impact

- `desktop/Source/AudioEngine.{h,cpp}` — routing, peak meter tap, output limiter, soft-reset comb
- `desktop/Source/MainComponent.cpp` — label **Ext. In.**, default toggle from device caps
- `desktop/Source/InputEnvelopeIndicator.cpp` — idle/active paint, min visible fill
- `src/core/FroggersEngine.hpp` — `SoftResetFxState` clears comb line
- `QUICK_DICT.md`, `MANUAL_VERIFY.md`
