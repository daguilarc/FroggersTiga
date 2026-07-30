## Context

```
Cross-host mod routing model (target)
─────────────────────────────────────

Desktop / VST (closed UI)          Web (mod bay)              VCV (open rack)
─────────────────────────          ─────────────              ───────────────
PatchCableOverlay                  Dropdown per row           User Rack cables
  mod rack OUT → knob mod IN         modIndex state             + Randmod spawns
Randmod → modIndex shuffle           same engine                  Cable objects
  → overlay repaint                no cable widget            for indices 4–6
MIDI CC via mod rack jacks         MIDI CC via bridge         MIDI In only (no CC outs)
  + MIDI settings                  + CC enable                external MIDI-CV → mod IN

Shared engine: PickSimRandomModIndex, RandomizePageMod, ModMgr blend
```

**Architecture today:** `FroggersTigaAudioProcessor` → `AudioEngine(true)` → `MainComponent(externalEngine)` → `PatchCableOverlay` + `DesktopHostIO`.

**VST is not a third UI** — it is desktop with hosted audio/MIDI I/O. Parity fixes are policy branches and overlay sync, not a new mod system.

## Goals / Non-Goals

**Goals:**

- Randmod / Rand Mods / manual patch behave identically in VST and standalone
- UI mutations apply when DAW transport is stopped
- DAW preset save/recall restores timbre **and** cable overlay matches `modIndex`
- Document VST as A/B reference for VCV Randmod + cable work

**Non-Goals:**

- VCV Rack `addCable` implementation ( `vcv-rack-field-parity` )
- Web mod bay changes
- Publishing VST to GitHub `main`
- Serializing cable hue cosmetics in preset (rederive on load)

## Decisions

### D1 — Hosted mutation drain (OMNI: single apply path)

**Choice:** `MainComponent::timerCallback` calls `DrainPendingMutations()` **always** when `isPluginHosted()`, not only when `!isAudioRunning()`.

**Why:** Hosted constructor sets `m_audioRunning = true` permanently. Current timer only drains when standalone transport is stopped — VST buttons enqueue mutations that never apply if DAW transport is stopped and `processBlock` is not called.

**Data flow:**

```
UI click → Enqueue* → mutation queue
                          ↓
              timerCallback (hosted: always)
              renderSimOutputBlock → tickControls (audio thread)
                          ↓
              drainMutationQueue → applyMutation
```

**Alternative rejected:** Drain only on audio thread — fails when DAW paused.

### D2 — Overlay resync after mod state changes

**Choice:** Add `PatchCableOverlay::syncRoutesFromHost()`:

- Iterate registered input ports
- For each `(page, row)`: read `modIndex` from host
- If `modIndex == 255`: `removeCableHue`
- Else if no hue assigned: `assignCableHue`
- `repaint()`

Call sites:

| Event | Caller |
|-------|--------|
| `setStateInformation` after `SimPresetSnapshot::read` | `PluginProcessorEditor` or processor → `MainComponent` |
| `applyMutation` for `RandomizePageMod`, `RandomizeAllMod`, `SetPageModSource`, clears | `DesktopHostIO` callback → overlay (same pattern as `m_onBeforeClearModRoutes`) |
| Timer (existing) | `repaint()` only — no per-frame hue rebuild |

**Why:** Cables are **derived views** of `modIndex`, not separate connection objects (unlike VCV Rack `Cable`). Preset load mutates host without touching `m_cableHues`.

**OMNI:** One read path (`getModSource` on overlay already mirrors host) — sync assigns hues once after bulk changes.

### D3 — VCV parity matrix (reference only)

| Behavior | VST / Desktop | VCV (planned) |
|----------|---------------|---------------|
| Randmod scope | mod rack sources → in-app knob mod inputs | same engine; cables only for indices 4–6 |
| MIDI CC mod | mod rack CC cells + `CvMidiBridge` | MIDI In + enables; no CC mod rack outs |
| Cable on Randmod | overlay redraw | `engine::Cable` + `CableWidget` |
| External CV to param | N/A (closed UI) | user patches foreign module to mod IN |

### D4 — Hosted chrome: hide Record/Export

**Choice:** `m_recordCluster.setVisible(false)` when `isPluginHosted()` in `initFromEngine`.

### D5 — QWERTY MIDI off when hosted

**Choice:** `shouldCaptureQwertyMidi()` returns false when `isPluginHosted()`.

### D6 — Preset snapshot v3

**Choice:** Bump `kVersion` to 3. Append after v2 `pairArRows`:

| Field | Source |
|-------|--------|
| `vcoMorph[3]` | `host.m_engine` morph indices |
| `ccEnabled[2]` | `CvMidiBridge` |
| `ccChannel[2]`, `ccNumber[2]` | `CvMidiBridge` |

`read()` branches: v1 → defaults pair-AR + v3 fields; v2 → pair-AR + v3 defaults; v3 → full.

**Note:** v2 already shipped in `SimPresetSnapshot.hpp` with `pairArRows` — v3 adds morph/CC without removing pair-AR.

### D7 — Editor minimum size

**Choice:** `setResizeLimits(1440, 720, 8192, 4320)`.

### D8 — Manual verification gate

Document in manual § VST plugin-hosted:

1. Load VST3 in Logic or Reaper
2. Press Randmod on Audio column — cables reroute on overlay
3. Save/reload DAW project — cables match routes
4. Randmod with DAW transport **stopped** — knobs/mods still change
5. A/B audio vs standalone with same snapshot bytes

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Double drain (timer + audio) | `drainMutationQueue` is idempotent empty-loop when caught up |
| Overlay sync on every mutation | Only mod-affecting mutation types; not per-sample |
| v3 breaks old sessions | v1/v2 read paths preserved |

## Migration Plan

1. Mutation drain + overlay sync (unblocks functional Randmod in DAW)
2. Hosted chrome + QWERTY + snapshot v3 + editor size
3. Manual DAW checklist
4. VCV field-parity uses VST Randmod behavior as reference
