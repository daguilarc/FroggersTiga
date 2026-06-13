## ADDED Requirements

### Requirement: Transport buttons meet 44px touch minimum

Play, Stop, and External buttons in `.controls-top` SHALL have a minimum touch height of **44 px** on all viewports.

#### Scenario: Mobile transport row

- **WHEN** the viewport width is 390 px
- **THEN** Play and Stop buttons are at least 44 px tall

### Requirement: Global strip buttons meet 44px touch minimum

All buttons in `.global-strip` SHALL have a minimum touch height of **44 px** and SHALL wrap to multiple rows on narrow viewports without clipping labels.

#### Scenario: Narrow global strip

- **WHEN** the viewport width is 390 px
- **THEN** each global strip button is at least 44 px tall
- **AND** button labels remain fully visible (wrap or stack allowed)
