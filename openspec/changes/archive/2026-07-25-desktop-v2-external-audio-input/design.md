## Context

Desktop v2's external-audio modulation system is fully wired downstream: `FroggersV2ControlCore::setExternalAudioAvailable(bool)` feeds `computeEffective`'s assignability gate and `FroggersV2HostBridge`'s lane-depth sync into the engine (both fixed this session to use `isModLaneAssignable`, not the rand-only `isModSourceEligibleForRow`). Both consumers read from `AudioEngine::isExternalInputEnabled()`. That boolean has exactly one writer, `AudioEngine::setExternalInputEnabled(bool)`, and it is never called anywhere in `desktop-v2/Source` — confirmed by full-source grep. The feature is unreachable in the running app.

**How this got dropped (confirmed via git history, not assumed):** commit `4e3d0a3` ("Add desktop v2 app, engine extensions, and web/sim parity") is where v2's `MainComponent`/`GlobalStripV2` were first written. That single commit copied `AudioEngine`'s engine-state plumbing (`m_externalInputEnabled`, `setExternalInputEnabled`, `isExternalInputEnabled`, the DSP-side gate) faithfully from v1 — but the 330 new lines in `MainComponent.cpp` and 160 in `GlobalStripV2.cpp` never included the UI control itself. It wasn't removed later by a refactor; it was simply never rebuilt when v2's chrome was written from scratch, even though the backing engine state was carried over intact.

**Revised placement (corrected after reviewing v2's actual navigation structure, not v1's layout):** v1 puts the toggle directly on the main top bar. v2 does not need a new button at all — `MainComponent.cpp:375-376` already has a runtime-page rail on the right (`bounds.removeFromRight(kRuntimePageRailW)`) with three buttons — `m_fileButton`, `m_audioButton`, `m_midiButton` — each opening a settings page. `m_audioButton` already opens `AudioRuntimePageComponent`, which already embeds `AudioSettingsComponent` (the existing device-picker "menu"). This is exactly the "button leading to a menu, right-hand side, same area as MIDI" pattern already established in this UI — the Ext. In. control belongs *inside* the already-open Audio page, not as a new standalone toggle elsewhere. No new button, no new rail entry: the entry point already exists and already sits next to `m_midiButton`.

The hosted (VST) shell does not instantiate the rail or `AudioRuntimePageComponent` at all (confirmed: zero references in `HostedMainComponentV2.{h,cpp}`) — there is no standalone device manager to configure when a host owns audio I/O. This makes the hosted-shell exclusion structural, not a visibility flag to add.

## Goals / Non-Goals

**Goals:**
- Add the "Ext. In." toggle + input-level meter + route-status hint to `AudioSettingsComponent` (reached via the existing `m_audioButton` → `AudioRuntimePageComponent` path), wired to `AudioEngine::setExternalInputEnabled`.
- Close the test gap flagged this session: a test proving depth on an external-audio lane reaches both `computeEffective`'s UI sum and the engine (`V2LaneDepthStore`) once this control enables it.

**Non-Goals:**
- No new button, rail entry, or `GlobalStripV2` changes — the entry point (`m_audioButton`) already exists in the same rail as `m_midiButton`.
- No hosted-shell visibility gating — `AudioRuntimePageComponent` is structurally standalone-only already; there is nothing to hide.
- No changes to `AudioEngine`'s C++ API — every method this needs (`setExternalInputEnabled`, `isExternalInputEnabled`, `getInputPeakLevel`, `getInputRouteStatus`, `getInputRouteMessage`) already exists in `desktop-v2/Source/AudioEngine.h`, confirmed present, matching v1 signature-for-signature. (`isPluginHosted()` is no longer needed by this design — see above.)
- No changes to `FroggersV2ControlCore`, `FroggersV2HostBridge`, `sim/`, or the manifest — all already correct and already reading the flag this change makes reachable.
- No change to `AudioSettingsComponent`'s existing input-device selector logic (`applyInputDevice`) — the new toggle is additive alongside it, not a replacement; device selection (which hardware) stays orthogonal to enabling (whether the routed signal reaches the engine).

## Decisions

**D1 (revised) — Toggle lives in `AudioSettingsComponent`, reached via the existing Audio-page button in the existing runtime-page rail; no new UI entry point.**
Originally scoped as a new control on `GlobalStripV2` (mirroring v1's top-bar placement literally). Corrected after reviewing v2's actual navigation: v2 already has a "button opens a menu" pattern for exactly this class of setting — the runtime-page rail (`m_fileButton`/`m_audioButton`/`m_midiButton`, `MainComponent.cpp:375-376`) — and Audio settings are already reached through it via `AudioRuntimePageComponent` → `AudioSettingsComponent`. Adding a second, separate entry point on `GlobalStripV2` would fragment where audio-related controls live (some in the rail's Audio page, one bare toggle on the top bar) for no benefit. Alternative considered: keep the `GlobalStripV2` toggle for at-a-glance visibility without opening the menu — rejected for this change; if quick visibility is wanted later, it can be layered on top of this without conflicting (e.g., a status LED on the rail's Audio button itself), but that's separate scope.

**D2 — Reuse `AudioEngine`'s existing methods verbatim; write zero new engine code.**
Confirmed present and matching v1: `getInputPeakLevel()`, `getInputRouteStatus()` (returns `InputRouteStatus`), `getInputRouteMessage()`. The UI layer is a straight port of v1's `onClick`/meter-refresh/hint-refresh logic onto these existing calls — no new engine surface. (`isPluginHosted()` is no longer needed — see D1's structural exclusion.)

**D3 — The periodic refresh already reads `isExternalInputEnabled()`; this change adds the write side, not a new read path.**
`MainComponent.cpp`'s existing refresh loop already calls `m_audio->isExternalInputEnabled() && running` to feed the oscilloscope and control core (Packet 15.3a). The new toggle's `onClick` writes `setExternalInputEnabled(...)`; the existing refresh loop picks it up on its next tick with no changes needed there.

## Risks / Trade-offs

- **[Risk] The new end-to-end test (external-audio lane reaches both UI and engine) needs `AudioEngine::setExternalInputEnabled` reachable from a test harness without real audio hardware** → Mitigation: `FroggersV2ControlCore::setExternalAudioAvailable(bool)` is already a plain public setter independent of `AudioEngine` — the control-core/engine-summation half of the test needs no `AudioEngine` or hardware at all (call the control-core setter directly, as already established this session). A separate, smaller test can verify the UI toggle correctly calls `AudioEngine::setExternalInputEnabled` without needing a real device (JUCE's `AudioDeviceManager` can run with no physical device attached for this purpose).
- **[Risk] `AudioSettingsComponent`'s existing layout (device pickers, sample-rate, export format) has no reserved space for a new control** → Mitigation: it already has an input-level meter (`InputLevelMeter`, reading `m_engine.isExternalInputEnabled()`) and a status label (`m_status`) — the new toggle is a natural addition alongside the existing `m_inLabel`/`m_inDevice`/`m_inHelp`/`m_inMeterLabel` row, not a new layout region.

## Migration Plan

1. Add the toggle member to `AudioSettingsComponent.h`, wire `onClick` to `m_engine.setExternalInputEnabled(...)` in `.cpp`, matching v1's tooltip text ("Ext. In.: route line/mic to engine (off = VCO-only)").
2. Confirm the existing `InputLevelMeter`/`m_status` logic already covers the meter/route-hint requirements (it references `isExternalInputEnabled()` already — verify during implementation whether it needs extension or already satisfies the spec scenarios).
3. Add the end-to-end test (control-core + engine reachability) and a UI-wiring test (toggle → `AudioEngine::setExternalInputEnabled`).
4. Full desktop-v2 suite + packet gates green before considering complete (per this session's own hardened verification contract, C1).

No rollback complexity — this is additive (new control inside an existing settings page, no existing behavior changed) and no data/schema migration is involved.

## Open Questions

- Whether `AudioSettingsComponent`'s existing `InputLevelMeter`/`m_status` already fully satisfy the spec's meter/route-hint scenarios, or need extension — resolve during implementation, not a blocking design decision.
