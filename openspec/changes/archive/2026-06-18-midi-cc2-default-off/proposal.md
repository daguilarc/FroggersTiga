> **Superseded by `omni-repository-harmonization`.** CC-default deltas are absorbed into omni host-display, web CC1-only, VCV MIDI removal, and VST host-parameter routing. Archive with `--skip-specs`; do not mutate baseline specs from this change.

## Why

MIDI CC 2 is a secondary mod input. Most sessions only need CC 1; CC 2 active by default greys-in a mod column and accepts DAW CC traffic the operator did not opt into. Web already enables CC 1 only when External MIDI turns on; desktop/VST/VCV should match that intent at cold start (CC 1 on, CC 2 off).

## What Changes

- **`CvMidiBridge`:** `m_inCcEnabled[2]` default `{true, false}` (pair 0 = CC 1, pair 1 = CC 2)
- **VCV plugin:** CC 2 enable switch default Off; local `m_ccPairEnabled[1] = false`
- **Specs:** update `midi-cc-mod-gating`, `vcv-cc-mod-gating`, `juce-vst-cc-mod-gating` default scenarios
- **Manuals:** replace “defaults both CC on” with CC 1 on / CC 2 off

**Non-goals:** Web External MIDI flow (already CC 1 only when on); snapshot version bump (enable flags not in v1 snapshot body)

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `midi-cc-mod-gating`: Desktop default — CC 1 enabled, CC 2 disabled
- `vcv-cc-mod-gating`: CC 2 toggle default Off
- `juce-vst-cc-mod-gating`: CC 2 toggle default Off

## Impact

- `src/core/CvMidiBridge.hpp`
- `vcv/src/plugin.cpp`
- `openspec/specs/midi-cc-mod-gating/spec.md` (via change delta)
- `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md`
