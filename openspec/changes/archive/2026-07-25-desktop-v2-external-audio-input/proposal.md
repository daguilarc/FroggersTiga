## Why

Desktop v2's entire external-audio modulation system (Packet 15.3a of the archived `desktop-v2-operator-truth-repair` change, plus this session's `isModLaneAssignable`/`computeEffective` assignability fix) is correctly wired end-to-end from `FroggersV2ControlCore::setExternalAudioAvailable(bool)` down through the engine sum — but that boolean is only ever fed from `AudioEngine::isExternalInputEnabled()`, and **nothing in desktop-v2 ever calls `AudioEngine::setExternalInputEnabled(true)`**. Confirmed by full-source grep: zero call sites outside the function's own definition, and zero UI references to an enable control anywhere in `desktop-v2/Source/ui`. External-audio modulation lanes are therefore permanently unreachable in the running v2 app today, regardless of whether real hardware is plugged in.

v1 (`desktop/Source/MainComponent.{h,cpp}`) already has the working reference implementation: a `juce::ToggleButton m_externalInput{"Ext. In."}` on the main top bar, wired to `setExternalInputEnabled`, plus an input-level envelope meter and a route-status hint label. v2's `AudioEngine.{h,cpp}` already carries the identical `setExternalInputEnabled`/`isExternalInputEnabled`/`m_externalInputEnabled` plumbing and the identical DSP-side gate (`AudioEngine.cpp:759`) — it was ported from v1 along with everything else, just never given a caller. v2's own `AudioSettingsComponent` status text already says "Enable Ext. In. on the main bar to route audio," describing a control that was never built.

This is a bounded port of working, reference-complete v1 functionality onto already-correct v2 downstream wiring — not new design work.

## What Changes

- Add an "Ext. In." toggle, input-level meter, and route-status hint to `AudioSettingsComponent` — reached via the *existing* runtime-page rail's Audio button (`m_audioButton`, `MainComponent.cpp:375-376`), the same rail that holds the MIDI/Controllers button. No new button or entry point: v2 already has a "button opens a menu" pattern for this class of setting, and Audio settings are already reached through it.
- No engine/control-core changes: `FroggersV2ControlCore::setExternalAudioAvailable`, the `computeEffective`/`isModLaneAssignable` assignability gating, and the `FroggersV2HostBridge` lane-depth sync are already correct and already wired to read `AudioEngine::isExternalInputEnabled()` — this change only makes that boolean reachable.

## Capabilities

### New Capabilities
- `desktop-v2-external-audio-enable`: a user-facing control (and its AudioEngine wiring) that enables/disables routing real external audio input into the engine, mirroring v1's "Ext. In." toggle. Covers the toggle itself, the input-level meter, and the route-status hint.

### Modified Capabilities
(none — `froggers-v2-runtime-audio-configuration`'s existing requirement that the runtime Audio page derive external-input state "from the same audio input path used by Froggers v1/v2 external audio processing" already anticipates this path existing; this change makes that path reachable without changing that spec's requirement text.)

## Impact

- `desktop-v2/Source/AudioSettingsComponent.h`, `.cpp` — new toggle wired to `setExternalInputEnabled`; confirmed the existing `InputLevelMeter`/`m_status` already reference `isExternalInputEnabled()` (verify during implementation whether they already satisfy the meter/hint requirements or need extension).
- `desktop-v2/Source/AudioEngine.h`/`.cpp` — no changes needed; confirmed `getInputRouteStatus`/`getInputRouteMessage`/`getInputPeakLevel`/`setExternalInputEnabled`/`isExternalInputEnabled` all already exist on v2's `AudioEngine`, matching v1 signature-for-signature.
- No `GlobalStripV2`, `MainComponent.cpp`, or `HostedMainComponentV2.cpp` changes — the existing `m_audioButton` → `AudioRuntimePageComponent` → `AudioSettingsComponent` path already reaches this component; confirmed the hosted (VST) shell never instantiates this path at all (zero references in `HostedMainComponentV2.{h,cpp}`), so there is no hosted-shell visibility gate to add.
- Tests: a new end-to-end test proving `setExternalAudioAvailable` flips through this control (closing the gap flagged this session — no test currently locks in the external-audio-lane-reaches-engine scenario), plus a UI-wiring test for the toggle itself.
- No changes to `FroggersV2ControlCore`, `FroggersV2HostBridge`, `sim/`, `DesktopV2ChromeLayout`, or the manifest.
