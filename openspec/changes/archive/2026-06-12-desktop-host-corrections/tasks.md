# Desktop host corrections — tasks

Supersedes `desktop-sim-ux-polish` §2 (ring-mod toggle/meter-as-control). Retains polish §1, §3–§5.

## 1. External audio routing (remove host gate)

- [x] 1.1 Remove `ExternalInputMode`, `setExternalInputMode`, ring-mod toggle and meter from `MainComponent`
- [x] 1.2 `AudioEngine` always copies input channel 0 to `m_inBlock` when `numInputChannels > 0`
- [x] 1.3 Optional: passive read-only **In** envelope indicator (no toggle)
- [x] 1.4 Update `sim-hosts-multi-ui` delta archived via `specs/desktop-simulator/spec.md` in this change

## 2. Audio thread safety + transport

- [x] 2.1 Add morph command queue on `DesktopHostIO`; drain in `tickControls()` before `ProcessBlock`
- [x] 2.2 Route `CycleVcoMorph`, `RandomizeVcoMorphs`, global **Rand waves** through queue (no direct `m_vcoMorph` writes from UI thread) — completed in `desktop-vco-morph-fix` (`CycleMorph` + idle drain)
- [x] 2.3 Guard `ModulatedMorph` / morph path with `std::isfinite`; fallback `0.f`
- [x] 2.4 Implement `audioDeviceStopped()` → `m_audioRunning = false` + UI callback
- [x] 2.5 Implement `audioDeviceError()` → log + clear running state + UI callback
- [x] 2.6 `MainComponent::updateTransportUi()` on transport callback
- [x] 2.7 On Stop: soft-reset reverb/delay state if last block had non-finite peak (minimal reset helper on engine or host)

## 3. Mod rack labels + LFO clarity

- [x] 3.1 Rename mod rack box **VCO feat** → **VCO level** in `ModRackPanel.cpp`
- [x] 3.2 Web `INTERNAL_MOD_LABELS` and dropdown strings → **VCO level**
- [x] 3.3 Marbles box tooltips: manual step via **Marbles** button; not LFOs
- [x] 3.4 No LFO jacks (document only — no code unless tooltip)

## 4. Wave icon controls + VCO display names

- [x] 4.1 Wave buttons show **SIN** / **SAW** / **SQR** text (28×28 px; text fallback per user preference)
- [x] 4.2 Set wave label in ctor from `getVcoMorph`; refresh updates label
- [x] 4.3 Remove `waveGlyph()` ASCII `~^n`; band thresholds 0.33 / 0.66 match web
- [x] 4.4 Widen `kWaveButtonWidth` to 28; label column 72 px for VCO1/2/3 + button
- [x] 4.5 Tooltip on wave control: "Cycle waveform: sine → saw → square"
- [x] 4.6 `DesktopPanelBackend::getRowName` page 0 rows 0–2 → `VCO1` / `VCO2` / `VCO3`
- [x] 4.7 Web: `waveLabel` SIN/SAW/SQR in `main.ts`

## 5. Polish carryover verification (from `desktop-sim-ux-polish`)

- [x] 5.1 Confirm 44100 Hz forced (already implemented)
- [x] 5.2 Confirm thick random-hue cables + per-knob mod-in jacks (already implemented)
- [x] 5.3 Confirm rotary knobs + layout (already implemented)
- [x] 5.4 Mark superseded polish tasks §2.1–2.3 in `desktop-sim-ux-polish/tasks.md` with pointer to this change

## 6. Manual verification

- [ ] 6.1 Play with line in — ring mod works with no extra toggle; unplug → VCO path
- [ ] 6.2 Click wave buttons and **Randomize waves** during Play — audio never dies permanently
- [ ] 6.3 Stop then Play after wave spam — audio returns
- [ ] 6.4 Mod rack shows **VCO level**; Marbles step only on **Marbles** press
- [ ] 6.5 Audio panel shows VCO1/2/3 and SIN/SAW/SQR wave buttons (never `...`)

## 7. Sign-off

- [x] 7.1 OMNI review on touched files
- [x] 7.2 No firmware / WASM page-count changes
