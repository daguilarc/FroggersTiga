## ADDED Requirements

### Requirement: v2-desktop-unified-midi-cv
Desktop v2 SHALL replace the v1 two CC-pair `CvMidiBridge` settings UI with a unified MIDI CV assignment table on the single primary MIDI input.

#### Scenario: v1 desktop CC bridge unchanged
- **WHEN** `SimHostKind::Desktop` opens MIDI Settings
- **THEN** two CC pairs with enable toggles remain as today

#### Scenario: v2 assigns CC to control-core external slot
- **WHEN** desktop v2 maps hardware CC 1 to external mod slot A
- **THEN** CC 1 level feeds the control-core external modulator, not hard-wired `ModMgr` index 0
