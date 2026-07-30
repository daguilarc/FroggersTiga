## MODIFIED Requirements

### Requirement: Host bridge publishes all pages to the audio engine
The Froggers v2 host bridge SHALL synchronize effective parameter values for every host page on each UI frame, not only the carousel-visible page. Carousel page selection SHALL affect UI slot visibility only.

#### Scenario: Filter applies without visiting Filter page
- **WHEN** the operator adjusts Filter parameters then returns to the Audio page
- **THEN** filter processing remains active in the audio engine
- **THEN** effective filter values match the control-core state

#### Scenario: Rand All on Audio page affects Drive
- **WHEN** the operator clicks Rand All while viewing the Audio page
- **THEN** Drive, Filter, and other module pages receive updated host parameters without visiting each page
