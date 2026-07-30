## MODIFIED Requirements

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
