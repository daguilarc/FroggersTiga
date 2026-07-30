## ADDED Requirements

### Requirement: bundled-non-helvetica-sans

Desktop v2 and VST v2 SHALL use bundled **IBM Plex Sans** (Regular + SemiBold) via application `DesktopV2LookAndFeel`. The app SHALL NOT rely on Helvetica or Helvetica Neue as the primary UI font.

#### Scenario: Font family is bundled

- **WHEN** desktop v2 or VST v2 renders parameter labels
- **THEN** glyphs come from the bundled font assets registered in `DesktopV2LookAndFeel`
- **THEN** `Helvetica` does not appear as the primary `Font` family name in runtime label rendering
- **THEN** body text fits the **1u×1u** grid cell at 11pt per `desktop-v2-grid-layout`
