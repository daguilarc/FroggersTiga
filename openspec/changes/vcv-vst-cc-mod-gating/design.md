## Context

```
Desktop / Web / VST (after midi-cc-mod-gating)
──────────────────────────────────────────────
CvMidiBridge.m_inCcEnabled[0/1]
  → PushMidiCc / drainMidiIn gated
  → IsSimModSourceAvailable
  → SetMidiCcPairEnabled → ClearModRoutesForIndex

VCV Rack (today — gap)
──────────────────────
drainVcvMidiIn → mods[cc-1] = value   // bypasses bridge + enable flags
Separate midiBridge for MIDI out only
No CC enable UI on panel
```

## Goals / Non-Goals

**Goals:**

- VCV CC ingest uses the same `CvMidiBridge` path as desktop (`PushMidiCc` → queue → `drainMidiIn`).
- VCV panel exposes CC1/CC2 enable toggles matching desktop semantics (default On).
- Disabled CC columns grey out; assignment blocked via core availability helpers.
- VST exposes CC enable control path (unblock `showMidiSettings` or inline toggles on plugin UI).

**Non-Goals:**

- Web External MIDI master gate on VST (DAW provides MIDI).
- Persisting enable flags across sessions.
- New mod source types.

## Decisions

### D1 — Replace direct `mods[]` write in VCV

Remove `drainVcvMidiIn` direct assignment. Call `host.m_midiBridge.PushMidiCc(ch, cc, value)` per queued MIDI message, then `host.m_midiBridge.drainMidiIn(host.m_pageManager.m_modMgr.m_mods, ModMgr::x_numMods)` once per block.

### D2 — Pair-indexed enable UI on VCV panel

Add two toggle widgets bound to `SetMidiCcPairEnabled(pairIndex, enabled)` on the host IO struct VCV uses (same pattern as `MidiSettingsComponent` desktop).

Loop over `{0, 1}` for paint/grey state — no CC1/CC2 copy-paste blocks.

### D3 — VST MIDI Settings visibility

Remove or narrow `AudioEngine::showMidiSettings` early return for `m_pluginHosted`. Keep audio device / transport hidden; show CC enable toggles.

### D4 — Route clearing on disable

On disable transition, call the same page + delay clear path as desktop (`ClearModRoutesForIndex` for mod indices 0 and 1).

## Risks / Trade-offs

- **VCV ingest timing:** Per-block drain must run before mod application — match existing `tickControls` order in desktop.
- **Panel layout:** 2-row panel already dense — place CC enable toggles beside existing CC channel/CC sliders.

## Migration Plan

1. Rewire VCV `drainVcvMidiIn` through bridge.
2. Add panel toggles + grey mod columns.
3. Unblock VST MIDI Settings CC section.
4. Manual verify: disable CC2 in VCV → grey column, no mod assignment, routes cleared.

## Open Questions

- None blocking.
