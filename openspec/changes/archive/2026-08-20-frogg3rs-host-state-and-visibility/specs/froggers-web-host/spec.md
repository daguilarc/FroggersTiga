# Delta — `froggers-web-host`

**Added 2026-08-19.** Two requirements arising from the operator's
2026-08-19 smoke of the `frogg3rs-browser-and-vst-hosts` site build.

## ADDED Requirements

### Requirement: Parameter controls are legible before audio starts
WHEN the published site loads, THE parameter controls SHALL show their
names and current values without requiring the visitor to press Play or
interact at all, matching the guarantee the predecessor site made
(`web-mobile-knob-labels`). Audio SHALL still not start without a user
gesture.

#### Scenario: Knobs are readable on arrival
- **WHEN** a visitor loads the site and does nothing
- **THEN** every encoder cell shows its parameter name and value
- **AND** no audio has started

### Requirement: Automated checks assert rendered visibility
THE site's automated checks SHALL assert that the surface is actually
VISIBLE — non-zero rendered extent and painted content — and SHALL NOT
rely on element geometry alone, which reports full bounding boxes for
content clipped to invisibility. Each such assertion SHALL be
demonstrated to fail against a build carrying the defect it guards.

#### Scenario: A blank page fails the suite
- **WHEN** a regression clips or blanks the rendered surface while
  leaving element geometry intact
- **THEN** the automated checks fail
