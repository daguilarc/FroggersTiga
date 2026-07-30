## Context

```
Desktop / Web / VST (after midi-cc-mod-gating)
──────────────────────────────────────────────
host.m_midiBridge.m_inCcEnabled[0/1]
  → PushMidiCc / drainMidiIn gated
  → IsSimModSourceAvailable
  → SetMidiCcPairEnabled → ClearModRoutesForIndex

VCV Rack (today — gaps)
───────────────────────
drainVcvMidiIn → mods[cc-1] = value     // bypasses bridge; CC 3–4 hit unused mods[2/3]
host.ProcessBlock → tickControls
  → drainMidiIn overwrites mods[0/1]    // from empty bridge latch → CC ingest broken
module.midiBridge (separate)            // MIDI out only; config can drift from host.m_midiBridge
No CC enable UI on panel
```

VCV mod rack exposes VCO Envelope / Random output jacks only — no MIDI CC mod columns like desktop. CC modulation on VCV is invisible (`mods[0/1]` latched from MIDI in); expander mod jacks accept indices 4–6 only. Disabled-state UX on VCV is enable toggles plus dimmed panel indicators, not grey mod-rack columns.

## Goals / Non-Goals

**Goals:**

- VCV CC ingest uses `host.m_midiBridge.PushMidiCc` → queue → `drainMidiIn` inside existing `ProcessBlock` / `tickControls`.
- Single `CvMidiBridge` on `host.m_midiBridge` for in, out, and enable flags.
- VCV primary panel exposes CC1/CC2 enable toggles (default On) via pair-indexed loop.
- Disabled pair: latch zeroed, `mods[0/1]` zeroed on drain, routes cleared, `IsModSourceAvailable` false for assignment guards.
- VST exposes CC enable toggles when plugin-hosted; DAW CC ingest respects enable flags.

**Non-Goals:**

- Web External MIDI master gate on VST (DAW provides MIDI).
- Persisting enable flags across sessions.
- New mod source types.
- Desktop-style MIDI CC mod rack columns or scopes on VCV (LED-only mod rack per field-parity).
- MIDI CC output jacks on VCV mod rack (deferred; field-parity spec mentions index 0 jack — separate work).

## Decisions

### D1 — Replace direct `mods[]` write in VCV ingest

Rename `drainVcvMidiIn` to reflect queue-only responsibility. For each MIDI CC message on the Rack input port:

```cpp
host.m_midiBridge.PushMidiCc(msg.getChannel(), msg.getNote(), msg.getValue());
```

Do **not** write `mods[]` in ingest. `host.ProcessBlock` already calls `tickControls` → `drainMidiIn(m_mods, …)` once per block.

Remove the CC 3–4 branch (`mods[2]` / `mods[3]`). Core assignable indices are `{0, 1, 4, 5, 6}`; indices 2–3 are not mod sources.

### D2 — Unify bridge instances

Delete `FroggersTigaModule::midiBridge`. Route `tickMidiOut` through `host.m_midiBridge` with the same callback pattern as today.

**Rationale:** Two bridge objects duplicate channel/CC/out config and violate single-authority data flow.

### D3 — Pair-indexed enable UI on VCV primary panel

Add two `LEDButton` or `CKSS` toggles on row 1 near the MIDI in jack, labeled CC1 / CC2. Wire in a pair loop:

```cpp
for (uint8_t pair = 0; pair < 2; ++pair)
    host.SetMidiCcPairEnabled(pair, toggleState[pair]);
```

Dim toggle / adjacent label when off (~40% brightness). No separate CC1/CC2 code paths.

### D4 — VCV disabled-state semantics (no grey mod columns)

VCV has no MIDI CC mod rack columns. When a pair is disabled:

- `SetMidiCcPairEnabled` clears routes for `kCcModIndices[pair]` (core).
- `SetPageModSource` / `DelayState::setModSource` reject unavailable indices via existing `IsSimModSourceAvailable` guards on `PagedHostIO`.
- Panel toggles show off/dim state.

### D5 — VST CC enable via trimmed MidiSettingsComponent

Remove the `m_pluginHosted` early return in `AudioEngine::showMidiSettings`. Pass a `pluginHostedOnlyCc` flag into `MidiSettingsComponent`:

- **Show:** CC1/CC2 group labels, channel sliders, CC sliders, enable toggles.
- **Hide:** MIDI In device picker, MIDI Out section, refresh/ status rows tied to hardware devices.

Keep `MainComponent` MIDI Settings button visible and functional in plugin mode.

**Alternative rejected:** Inline toggles on main plugin chrome — duplicates layout already in `MidiSettingsComponent`.

### D6 — Route clearing on disable

On disable transition, `PagedHostIO::SetMidiCcPairEnabled` already calls `ClearModRoutesForIndex(kCcModIndices[pairIndex])`. VCV toggles call that API; no VCV-local route scan.

## Risks / Trade-offs

- **VCV ingest timing:** PushMidiCc runs before `ProcessBlock`; drainMidiIn runs inside `tickControls` at block start — matches wasm `processBlock` ordering (beginBlock pointer, then ProcessBlock updates mods in place).
- **Panel density:** CC enable toggles sit beside existing MIDI in jack on row 1; no channel/CC config sliders on VCV (defaults ch1/CC1, ch1/CC2 match desktop defaults).

## Migration Plan

1. Rewire VCV ingest through `host.m_midiBridge`; remove duplicate bridge member.
2. Add panel toggles + dim off state.
3. Unblock VST CC enable section in `MidiSettingsComponent`.
4. Manual verify: disable CC2 in VCV → mods[1] stays 0, routes cleared; disable CC2 in VST → mod rack greys CC2 column, DAW CC2 ignored.

## Open Questions

- None blocking.
