## MODIFIED Requirements

### Requirement: Signal path is always active across modules
Froggers v2 desktop standalone SHALL process all module pages in the audio engine regardless of carousel selection. Visiting a module page SHALL NOT be required to enable that module's signal processing.

#### Scenario: Drive does not require page focus
- **WHEN** Drive parameters are non-default and the carousel shows Audio
- **THEN** drive processing affects the audible output

### Requirement: Global randomization is not duplicated at module headers
The product contract SHALL expose global Randomize All and Randomize Mod commands only in the global-command band. Per-module Randomize/Randmod header buttons are retired from desktop v2.

#### Scenario: No module-header randomization
- **WHEN** the operator views any module page
- **THEN** Randomize and Randmod buttons do not appear in the module panel header row
