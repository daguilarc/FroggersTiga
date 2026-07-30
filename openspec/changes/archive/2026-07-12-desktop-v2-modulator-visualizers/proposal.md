## Why

Detail-grid modulation-depth encoders show no source activity. Sheaf portable modulator visualizers place a display-only waveform under each depth encoder. Desktop v2 already samples permanent mod CV via `GetCvOut` for the global oscilloscope; that path must become a single 15-lane history that both the global strip and detail underlays consume.

## What Changes

- Add `CvLaneHistoryStore`: fixed 15-lane UI-thread rings (`kBufferSize` = `CvScopeDisplay::kBufferSize`).
- Push `host.GetCvOut(0..14)` once per UI tick; `GlobalOscilloscopeDisplay::refreshTraces` reads the store (no second `GetCvOut` loop).
- Detail-grid lane encoders paint a non-owning underlay from the store; Target (Back) has no underlay; module-row MOD unchanged.
- Shared `bindDetailUnderlays` for Submodule + Adsr panels.
- Spec delta, tests, operator docs.

## Capabilities

### New Capabilities

- `desktop-v2-modulator-visualizers`: detail-grid underlay contract; 15-lane CV history authority.

### Modified Capabilities

- `desktop-v2-mod-source-grid`: detail cells show source activity underlay; interaction (`ParamTurn` / `ModDrillIn`) unchanged.

## Impact

desktop-v2 UI (store, EncoderRing, panels, GlobalOscilloscopeDisplay, shells), ModSourceGrid tests, SIM_MANUAL / QUICK_DICT mirrors. No DSP formula change. No module-row MOD LED polish. No Sheaf portable DrawCommand port.
