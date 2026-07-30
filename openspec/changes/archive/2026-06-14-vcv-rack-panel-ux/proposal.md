## Why

v2.3.0 VCV panels are unusable: no `setPanel()` SVG, broken `ui::Label` page titles (zero-size box), CC switches overlapping the gate jack at X=13.5, and tooltip-only port names. Prior artifacts over-specified panel art (per-row abbrev table, `ui::Label` fallback, decorative scope talk). User direction: **functional legibility with minimal visual design** — tiny unobtrusive “FroggersTiga” in a corner (Comic Sans, minimum readable size); everything else standard gray/black Rack widgets and **green LEDs** (Random 1 / Random 2, never “marbles”). No oscilloscopes on VCV.

## What Changes

- Add **minimal panel SVGs** (light + dark) per module: gray background, black silkscreen text from `ParamDisplayNames`, tiny corner “FroggersTiga” watermark (~6–8 pt Comic Sans). `setPanel(createPanel(...))` on all three models.
- Fix **primary de-overlap**: dedicated CC-enable row; gate/MIDI/CV on separate grid positions.
- **Green LED only** on mod rack and CC enables — replace `GreenRedLight` with `GreenLight`; threshold >55% matches desktop Random LED policy.
- **Voicing + FX topology** (48 HP + 36 HP) — replaces Expander A/B; stereo FX jacks duplicate mono path.
- Standard Rack widgets only: `RoundSmallBlackKnob`, `CKSS`, `TL1105`, `ThemedPJ301MPort` — no custom art beyond SVG silkscreen.
- **Remove** broken `addPageTitleLabel` pattern; **do not add** `VcvRowAbbrev.hpp` or oscilloscope widgets.
- Bounds CI updated; version **2.4.0**; `vcv/` stays local-only.

## Capabilities

### New Capabilities

- `vcv-rack-panel-ux`: Minimal panel SVG + silkscreen labels, Voicing/FX topology, primary de-overlap, green LED mod indicators, bounds CI.

### Modified Capabilities

- (none — VCV-only; desktop/web/VST scopes and naming unchanged in this change)

## Impact

- `vcv/res/*.svg` (3 models × light/dark)
- `vcv/src/plugin.cpp`, `vcv/src/widgets/FieldParityWidget.hpp`
- `sim/VcvPanelLayout.hpp`, `sim/check_vcv_panel_bounds.sh`
- `vcv/plugin.json` — **BREAKING:** Expander A/B → Voicing/FX slugs
- `vcv/DEVELOPMENT.md`
