# web-mobile-audio-unlock Specification

## Purpose
TBD - created by archiving change web-sim-bootstrap-fix. Update Purpose after archive.
## Requirements
### Requirement: Touch unlock for suspended AudioContext

On mobile Safari and similar browsers, the web sim SHALL register a one-time `touchstart` listener that resumes the AudioContext if it exists and is suspended, before or during the first Play bootstrap.

#### Scenario: iOS first touch

- **WHEN** the user touches the page on iOS Safari before Play
- **THEN** a suspended AudioContext is resumed if one exists
- **AND** the listener removes itself after first fire

#### Scenario: Desktop unaffected

- **WHEN** the user interacts on desktop Chrome or Firefox
- **THEN** touch unlock is a no-op if no touch events occur
- **AND** Play click alone starts audio

