## MODIFIED Requirements

### Requirement: v2-mod-assignment-without-patch-cables
Desktop v2 SHALL assign modulation routes through lit source cells and compact mod lane pickers per knob row; patch-cable drag routing SHALL NOT appear in v2. Mod lane pickers SHALL use fixed-width cells from shared layout authority.

#### Scenario: Compact mod lane picker on module page
- **WHEN** a knob row renders on a module page at 1280×920
- **THEN** the mod lane picker uses the shared fixed mod cell width
- **THEN** the picker does not expand to the full module row width
