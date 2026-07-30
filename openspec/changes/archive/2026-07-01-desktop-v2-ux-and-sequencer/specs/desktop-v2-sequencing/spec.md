## MODIFIED Requirements

**Audit 2026-06-30:** Main spec scenario "Sequencer and scenes" (L29–31) claims blend and endpoints update on step fire — code applies only six global L/R floats via `m_sceneBlend` (`FroggersV2ControlCore.cpp` L226–245). This delta corrects recall semantics and adds edit-step toolbar requirements delegated in detail to `desktop-v2-sequencer-rand`.

### Requirement: v2-full-sequencer

Desktop v2 and VST v2 SHALL include a **full step sequencer** integrated with transport, scenes, and the control core.

#### Scenario: Sequencer UI visible

- **WHEN** desktop v2 or VST v2 editor is open
- **THEN** a sequencer panel shows step grid, pattern length, playhead position, edit-step toolbar (prev/next arrows, dice, Step/Pattern scope), and per-step **right-click** context menu (**Reset**, **Randomize**)
- **THEN** the panel sits below the performance band in the vertical chrome stack (per `desktop-v2-page-carousel`)

#### Scenario: Clock from transport

- **WHEN** **Start Sequence** is active and BPM is set
- **THEN** the sequencer advances steps on beat boundaries at the configured BPM
- **THEN** step changes publish sequencer clock to the control core (`FroggersV2HostBridge.cpp` L22–24)

#### Scenario: Per-step scene capture

- **WHEN** the user records into step N while sequencer record mode is armed
- **THEN** step N stores a full per-row scene-slot snapshot per `desktop-v2-sequencer-rand`
- **THEN** playback recalls that snapshot when the playhead enters step N

#### Scenario: Pattern length

- **WHEN** the user sets pattern length to 16
- **THEN** the playhead cycles steps 0–15
- **THEN** supported lengths include at least 4, 8, 16, 32, and 64 steps

#### Scenario: Sequencer and scenes

- **WHEN** a step fires during playback
- **THEN** per-row `sceneCenter[*]` and gesture weights update from the step snapshot
- **THEN** live scene blend and S1/S2/S3 endpoint ordinals are **not** overwritten by step recall
- **THEN** unstored metadata outside the step buffer (mod depths, mod routes) is unchanged

#### Scenario: VST sequencer host parameters

- **WHEN** FroggersTigaPluginV2 is hosted
- **THEN** BPM, pattern length, play/record arm, and current step are exposed as host parameters with flat stable IDs
- **THEN** grouped display names appear in DAW trees per `vst-v2-midi-modulation` dual-ID rules

#### Scenario: MIDI clock sync optional input

- **WHEN** external MIDI clock is enabled in v2 MIDI settings
- **THEN** sequencer step advance may follow incoming MIDI clock instead of internal BPM when sync mode is External

### Requirement: v2-default-internal-vco-audio

On v2 hosts, when audio processing is active (**Engine** on / DAW playing audio), output SHALL be driven by **internal VCOs** at the current knob and scene values by default. Operators SHALL hear knob edits without sending MIDI and without running the step sequencer — including when **Start Sequence** is off or the pattern is empty.

This is the normal operating state, not a separate mode. v2.0 wired `VcoAdsrState` with gate-default silence because sequencer/MIDI gates were the implementation hook (`AudioPairArState` disabled on v2) — not because AR must be gated or because MIDI must unlock sound.

While `SequencerState::m_playing == false`, per-VCO envelope gate SHALL be **open** (continuous internal VCO level). While `m_playing == true`, envelope gate SHALL follow performance inputs per `v2-step-gates-require-start-sequence`.

#### Scenario: Engine on without MIDI or sequencer

- **WHEN** audio processing is active, **Start Sequence** is off, and no live MIDI/QWERTY/gate CV is held
- **THEN** internal VCOs are audible at the current knob/scene values
- **THEN** operators can tweak encoder rings and hear changes immediately

#### Scenario: Empty pattern does not silence output

- **WHEN** audio processing is active and all step gates are off or the pattern is empty
- **THEN** internal VCOs still drive output while **Start Sequence** is off

#### Scenario: Stop Sequence returns to default VCO output

- **WHEN** **Start Sequence** stops (including mid-pattern on a lit step)
- **THEN** pattern gate contribution ends
- **THEN** envelope gate returns to **open** and internal VCOs drive output continuously

### Requirement: v2-step-gates-require-start-sequence

Step gate cells in the sequencer grid SHALL store per-step gate state as pattern data. They SHALL NOT drive per-VCO AR envelope gates unless **Start Sequence** is active (`SequencerState::m_playing == true`).

While **Start Sequence** is running, live gate input (MIDI note, QWERTY, gate CV via `DesktopHostIO::SetGate` / `m_gateHigh`) SHALL OR-combine with the active pattern gate at the playhead.

Implementation: `SequencerState::activeStepGate()` returns `m_playing && stepGate()`. `DesktopHostIO` SHALL resolve envelope gate at every call site (currently `tickControls` and post-`advanceOnSamples` in `ProcessBlock`):

```
gate = m_playing ? (m_gateHigh || activeStepGate()) : true
```

#### Scenario: Stopped sequencer ignores lit step gates

- **WHEN** audio processing is active, **Start Sequence** is off, and step 0 has `gate == true`
- **THEN** the stored step gate does not affect the envelope
- **THEN** internal VCOs remain audible (default open gate)

#### Scenario: Running sequencer applies playhead gate

- **WHEN** **Start Sequence** is on and the playhead is on a step with `gate == true`
- **THEN** envelope gate is **true** unless neither live nor pattern gate is high
- **THEN** step snapshot recall on playhead advance is unchanged

#### Scenario: Stop Sequence ends pattern gating

- **WHEN** **Start Sequence** stops while the playhead sits on a lit step
- **THEN** pattern gate contribution ends immediately
- **THEN** output returns to continuous internal VCO level unless live MIDI is actively holding `m_gateHigh`
- **THEN** lit gate cells in the step grid render dimmed per `v2-sequencer-gate-cell-stopped-dim`

#### Scenario: DAW transport maps to Start Sequence

- **WHEN** VST receives MIDI Start/Stop/Continue mapped to sequencer transport
- **THEN** step gate behavior follows `m_playing` the same as the **Start Sequence** button

#### Scenario: Web out of scope for step gates

- **WHEN** web/WASM host runs without desktop sequencer grid
- **THEN** this requirement applies only to desktop v2 and VST v2 hosts using `DesktopHostIO` step sequencer
- **THEN** web Audio pair-AR remains always-on smoothing via `AudioPairArState`

#### Scenario: VST step grid interaction matches standalone

- **WHEN** FroggersTigaPluginV2 editor is open
- **THEN** step grid single-click, double-click, and right-click behavior matches `v2-sequencer-ui-shared-component` in `desktop-v2-sequencer-rand`
