# frogg3rs-web-mobile-ux Specification

## Purpose
TBD - created by archiving change frogg3rs-windows-and-mobile. Update Purpose after archive.
## Requirements
### Requirement: Encoder drags work on mobile

`Draw` nodes in the browser surface SHALL declare `touch-action: none` so that touch drags on encoder knobs are not reinterpreted as browser scroll or pan gestures. Container rows, buttons, and non-canvas areas SHALL keep the default `touch-action` so the page remains scrollable by dragging there.

#### Scenario: A touch drag on an encoder reaches pointerup

- **WHEN** a finger touches an encoder knob and drags
- **THEN** the surface receives `pointerdown`, `pointermove`, and `pointerup`
- **AND** `pointercancel` does not fire, and `lostpointercapture` does not fire before `pointerup`

#### Scenario: Page scrolling still works on non-canvas areas

- **WHEN** a finger drags on the site header, footer, or gaps between controls
- **THEN** the page scrolls normally

### Requirement: The surface owns its own mobile topology

A mobile-only difference in what the surface emits SHALL be expressed as a
row table consumed by the surface's existing emission code, selected by a flag
that defaults to false, rather than as a shell-side CSS or DOM rearrangement.
A shell that moves, clips, or duplicates emitted controls makes the rendered
tree disagree with the surface that produced it, and the surface's own tests
can no longer tell what a viewer sees.

The flag SHALL be set only by the browser host. No desktop, standalone, or
plugin path SHALL set it.

#### Scenario: No duplicate controls at any width

- **WHEN** the surface renders at any viewport width
- **THEN** exactly one Randomize row and one Reset row exist in the node tree

#### Scenario: Other hosts are unaffected

- **WHEN** the standalone, VST, or AU host builds the surface
- **THEN** the narrow-viewport flag is false and the desktop row order is emitted

### Requirement: Playwright regression coverage

The browser e2e suite SHALL include a mobile-emulated test asserting both the drag behavior and the Randomize/Reset placement without starting audio.

#### Scenario: CI mobile UX test

- **WHEN** the browser e2e suite runs with mobile emulation
- **THEN** the mobile UX spec passes

