# desktop-host-panel-column-order Specification

## Purpose

Desktop standalone horizontal column order for core submodule panels (pages 0–4). Host page indices and DSP bindings stay fixed; only layout positions permute so output FX columns read Drive → Filter → Reverb (pages 4 → 3 → 2) to match `FroggersEngine` signal flow.
## Requirements
### Requirement: Filter column precedes Reverb column on desktop

Desktop standalone SHALL lay out the five core submodule columns left-to-right in host-page order `{0, 1, 4, 3, 2}` (Audio, Random S&H, Drive, Filter, Reverb). Each `SubModulePanel` SHALL retain its original `pageIndex` binding so page 2 controls remain Reverb parameters, page 3 controls remain Filter parameters, and page 4 controls remain Drive parameters.

#### Scenario: Column positions after layout

- **WHEN** the desktop main window is shown at default width
- **THEN** the Drive panel (page index 4) appears immediately left of the Filter panel (page index 3)
- **THEN** the Filter panel (page index 3) appears immediately left of the Reverb panel (page index 2)
- **THEN** Audio and Random S&H columns remain left of Drive; Delay (page index 5) remains the sixth column

#### Scenario: Knobs still target correct engine pages

- **WHEN** the operator adjusts Wet/dry on the Reverb panel after the layout swap
- **THEN** `FroggersEngine` reverb mix changes audibly
- **WHEN** the operator adjusts Comb feedback on the Filter panel
- **THEN** comb filter timbre changes audibly
- **WHEN** the operator adjusts GAIN on the Drive panel
- **THEN** polynomial drive amount changes audibly

#### Scenario: Patch overlay follows swapped bounds

- **WHEN** patch-cable mode is active on desktop
- **THEN** input jacks for pages 2, 3, and 4 align with their respective panel column positions after the swap

### Requirement: v2-desktop-uses-carousel-not-six-columns
Desktop v2 SHALL NOT apply the six-column simultaneous submodule layout from `desktop-host-panel-column-order`. Page carousel navigation replaces horizontal column permutation.

#### Scenario: v1 column order unchanged
- **WHEN** v1 desktop renders submodule columns
- **THEN** Drive → Filter → Reverb column permutation remains per existing spec

#### Scenario: v2 shows one module column
- **WHEN** desktop v2 is active
- **THEN** only the selected page's submodule panel is visible
- **THEN** column-order permutation rules do not apply

