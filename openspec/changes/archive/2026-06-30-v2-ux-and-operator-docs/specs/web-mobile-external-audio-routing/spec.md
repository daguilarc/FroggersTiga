## ADDED Requirements

### Requirement: mobile-external-meter-uses-dedicated-label

On mobile web, external input level and routing state SHALL be read from the dedicated external meter label (`#external-meter-label` per `web-transport-morph-meter`), not from `#status` or `#ios-external-hint`.

#### Scenario: iOS external on with meter waiting

- **WHEN** iPhone emulation has External Audio on before Play
- **THEN** `#external-meter-label` reads `Waiting for Play`
- **THEN** `#status` does not carry meter or earpiece guidance copy

#### Scenario: Mobile play-and-record with active meter

- **WHEN** mobile web has External on, Play active, and `navigator.audioSession.type` is `play-and-record`
- **THEN** the external meter label shows active monitoring
- **THEN** mobile Audio Session lifecycle requirements from the baseline spec remain satisfied
