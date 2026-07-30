## ADDED Requirements

### Requirement: v2-desktop-uses-carousel-not-six-columns
Desktop v2 SHALL NOT apply the six-column simultaneous submodule layout from `desktop-host-panel-column-order`. Page carousel navigation replaces horizontal column permutation.

#### Scenario: v1 column order unchanged
- **WHEN** v1 desktop renders submodule columns
- **THEN** Drive → Filter → Reverb column permutation remains per existing spec

#### Scenario: v2 shows one module column
- **WHEN** desktop v2 is active
- **THEN** only the selected page's submodule panel is visible
- **THEN** column-order permutation rules do not apply
