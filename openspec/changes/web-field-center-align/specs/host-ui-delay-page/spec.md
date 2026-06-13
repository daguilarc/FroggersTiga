## MODIFIED Requirements

### Requirement: Web mobile page arrows

The web simulator SHALL provide large previous/next controls flanking the knob area on viewports at most 720px wide. Touch targets SHALL be at least 44×44 CSS pixels. On viewports wider than 720px, visible flanking arrows are optional; page pills and keyboard `[` / `]` navigation SHALL remain available.

#### Scenario: Mobile layout

- **WHEN** viewport width is at most 720 px
- **THEN** prev/next arrows SHALL remain beside the knob column without horizontal scroll of the knobs

#### Scenario: Desktop layout without flanking arrows

- **WHEN** viewport width is greater than 720 px
- **THEN** the knob grid MAY span the full `#app` content width without visible flanking arrows
- **AND** page navigation via pills and keyboard SHALL still work
