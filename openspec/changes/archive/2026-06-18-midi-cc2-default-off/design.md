## Context

Enable flags live in `CvMidiBridge::m_inCcEnabled[2]`. Desktop `MidiSettingsComponent`, VST plugin, and mod rack grey state all read `isCcPairEnabled()` — no separate UI defaults.

Web WASM init sets both pairs false, then External MIDI enables pair 0 only — already correct.

## Goals / Non-Goals

**Goals:** Single-line core default change; VCV duplicate defaults aligned; spec/manual truth.

**Non-goals:** Persisted preset migration; changing CC numbers or channel defaults.

## Decisions

### D1 — Core default only

**Choice:** `bool m_inCcEnabled[2] = {true, false};` in `CvMidiBridge.hpp`.

**Why:** OMNI — one assignment propagates to desktop standalone, VST (same `DesktopHostIO`), and WASM host until web UI overrides.

### D2 — VCV explicit defaults

**Choice:** `configSwitch` default `0.f` for CC2; `m_ccPairEnabled[1] = false`; `params[CC2_ENABLE_PARAM].setValue(0.f)`.

**Why:** VCV module mirrors enable state locally before `syncCcPairEnables`; must match core on first frame.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Operators expecting CC 2 live on launch | Manual + MIDI Settings **On** toggle still enables it |
| Saved VST/VCV sessions | User-saved state overrides defaults on load |
