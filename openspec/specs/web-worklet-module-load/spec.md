# web-worklet-module-load Specification

## Purpose
TBD - created by archiving change web-sim-bootstrap-fix. Update Purpose after archive.
## Requirements
### Requirement: AudioWorklet processor uses plain module URL

The web sim SHALL load the AudioWorklet processor via Vite `?url` import (or production bundle equivalent). It SHALL NOT use `?worker&url`, which injects Vite HMR client code incompatible with AudioWorkletGlobalScope.

#### Scenario: Dev addModule succeeds

- **WHEN** the developer runs `npm run dev` and clicks **Play**
- **THEN** `audioContext.audioWorklet.addModule(processorUrl)` completes without error
- **AND** the loaded module does not import `vite/dist/client/env.mjs`

#### Scenario: Production bundle unchanged

- **WHEN** `npm run build` completes
- **THEN** the built processor asset is referenced from the main bundle
- **AND** Play produces audible output

