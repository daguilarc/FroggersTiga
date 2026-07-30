## ADDED Requirements

### Requirement: Filter column precedes Reverb column on desktop

Desktop standalone SHALL lay out the five core submodule columns left-to-right in host-page order `{0, 1, 3, 2, 4}` (Audio, Random S&H, Filter, Reverb, Drive). Each `SubModulePanel` SHALL retain its original `pageIndex` binding so page 2 controls remain Reverb parameters and page 3 controls remain Filter parameters.

#### Scenario: Column positions after layout

- **WHEN** the desktop main window is shown at default width
- **THEN** the Filter panel (page index 3) appears immediately left of the Reverb panel (page index 2)
- **THEN** Audio and Random S&H columns remain left of Filter; Drive remains right of Reverb

#### Scenario: Knobs still target correct engine pages

- **WHEN** the operator adjusts Wet/dry on the Reverb panel after the layout swap
- **THEN** `FroggersEngine` reverb mix changes audibly
- **WHEN** the operator adjusts Comb feedback on the Filter panel
- **THEN** comb filter timbre changes audibly

#### Scenario: Patch overlay follows swapped bounds

- **WHEN** patch-cable mode is active on desktop
- **THEN** input jacks for page 2 and page 3 align with their respective panel column positions after the swap
