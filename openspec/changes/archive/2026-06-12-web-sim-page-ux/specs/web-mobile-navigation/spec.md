## ADDED Requirements

### Requirement: Bottom pill page navigation

The web simulator SHALL expose six labeled page pills as a secondary navigation control. Pill touch targets SHALL be at least 44×44 CSS pixels.

#### Scenario: Pill equals arrow navigation

- **WHEN** the user taps the **Drive** pill from **Reverb**
- **THEN** host page index matches the result of pressing next from Reverb once

### Requirement: Optional swipe paging

On touch devices, a horizontal swipe on the knob layout area MAY change host page when horizontal displacement exceeds 60 CSS pixels and no knob is being dragged.

#### Scenario: Swipe next page

- **WHEN** the user swipes left on the knob area while not dragging a slider
- **THEN** host page index increments by one clamped to 0–5

#### Scenario: Swipe ignored during knob drag

- **WHEN** a knob slider is active (`pointerdown` without `pointerup`)
- **THEN** swipe does not change host page

### Requirement: Collapsible mod bay on narrow viewports

On viewports at most 720 px wide, the mod source meter bay SHALL be collapsible via a header control.

#### Scenario: Collapse on mobile

- **WHEN** viewport width is 390 px and the user toggles mod bay closed
- **THEN** meter cells are hidden
- **AND** knob columns remain visible without horizontal scroll
