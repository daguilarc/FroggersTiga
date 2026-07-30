# OpenSpec change disposition (task 1.1)

Recorded for `omni-repository-harmonization` apply phase. Manual-only verification items remain unchecked in source changes; this table does not mark unrun manual checks complete.

## Reconcile → normal archive (code-backed durable deltas)

| Change | Disposition | Evidence | Open gaps (not claimed complete) |
|--------|-------------|----------|----------------------------------|
| `audio-pair-ad-controls` | code-backed | `PairArEnvelope.hpp`, `AudioPairArState.hpp`, `AudioPairArLayout.hpp`, desktop/WASM/web UI, `PairArEnvelope_test.cpp` | Tasks 6.2–6.4 manual smokes |
| `ios-external-audio-routing` | code-backed | `web/src/main.ts` mobile audio session; `mobile-audio-routing.spec.ts`, `manual-mobile-routing.spec.ts` | Tasks 4.1–4.5 real-device routing (explicit manual) |
| `mod-blend-semantics-docs` | code-backed | `DelayState.hpp` + `ModMgr`, `DelayCrispyMod_test.cpp`, operator doc sync | Task 5.5 manual web smoke |
| `pair-ar-modulated-knob-display` | code-backed | `AudioPairArEffective_test.cpp`, `getEffectiveKnob` wired in hosts | Task 4.2 optional Playwright |
| `pair-ar-randomize-parity` | code-backed | `HostRandomize.hpp`, `HostRandomize_test.cpp`, host randomize wiring | Tasks 5.1–5.2 manual host smoke |
| `pair-ar-vcv-time-range` | code-backed | `PairArEnvelope.hpp` time range, `PairArEnvelope_test.cpp` | OpenSpec status: complete (10/10) |
| `pair-ar-vertical-labels` | code-backed | `AudioPairArLayout.hpp`, `PairArRotatedLabel.h`, `SubModulePanel.cpp` | Tasks 4.2–4.3 visual manual |
| `web-mobile-global-strip-placement` | code-backed | `web/index.html` global strip, `global-strip-placement.spec.ts` | OpenSpec status: complete (11/11) |
| `web-mobile-knob-labels` | code-backed | `paramDisplayNames.ts`, `mobile-knob-labels.spec.ts` | Task 4.2 optional iPhone; label table superseded by omni §3 generated authority |

## Reconcile → normal archive (partial plan, implemented scope retained)

| Change | Disposition | Evidence | Reconcile action |
|--------|-------------|----------|------------------|
| `web-mobile-e2e-testing` | partial (Playwright landed) | `web/e2e/*.spec.ts`, `.github/workflows/web-e2e.yml`, `web/TESTING.md`, `simSelectors.ts` | Retain Playwright deltas + manual device caveats; remove Appium capability/spec and struck-through tasks (omni 1.3) |

## Archive with `--skip-specs` (stale/superseded)

| Change | Disposition | Evidence | Supersession |
|--------|-------------|----------|--------------|
| `midi-cc2-default-off` | stale/superseded | Defaults implemented in `CvMidiBridge.hpp`, VCV plugin | Absorbed by omni §3.5 (web CC1-only), §4 (VCV MIDI removal), §5 (VST host parameters) |
| `vcv-panel-silkscreen-fix` | stale/superseded | `generate_panels.py`, SVG checks, partial silkscreen | Absorbed by omni §3 host display authority + §4 VCV panel regen |
| `vcv-rack-field-parity` | stale/superseded | 10/69 tasks; MIDI/CV miswiring remains in `plugin.cpp` | Absorbed by omni §3–§4; do not implement field-parity plan as written |
| `vcv-vco-ar-left-expander` | stale/superseded | 0/18 tasks; no VCO-AR expander code | Out of scope; superseded without implementation |
| `vst-plugin-host-ux` | stale/superseded | 0/26 tasks; hosted UX bugs still in `MainComponent.cpp` | Valid findings absorbed into omni §5.6–5.7 |
