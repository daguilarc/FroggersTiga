## ADDED Requirements

### Requirement: carousel-arrows-flank-title

Module carousel previous and next buttons SHALL be positioned immediately left and right of the module title text as a single centered group, not at the far horizontal edges of the carousel panel.

#### Scenario: Arrows adjacent to Module Audio title

- **WHEN** desktop v2 or VST v2 renders the carousel header on Audio
- **THEN** layout order is `[←][Module: Audio][→]` as one centered cluster
- **THEN** each arrow is **2u×2u** per `desktop-v2-grid-layout`
- **THEN** neither arrow is flush against the carousel panel edge while the title sits in the center gap
