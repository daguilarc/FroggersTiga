## MODIFIED Requirements

### Requirement: AudioWorklet processor uses plain module URL

The web sim SHALL load the AudioWorklet processor from `${import.meta.env.BASE_URL}froggers-processor.js`, a **transpiled ES module** produced by the worklet build script and copied from `web/public/` to `dist/` by Vite. It SHALL NOT use Vite `?url` or `?worker&url` imports for the processor (both are incompatible with production AudioWorklet deployment on static hosts).

#### Scenario: Dev addModule succeeds

- **WHEN** the developer runs `npm run dev` and clicks **Play**
- **THEN** `audioContext.audioWorklet.addModule(processorUrl)` completes without error
- **AND** the loaded module does not import `vite/dist/client/env.mjs`
- **AND** the loaded module parses as valid JavaScript (no TypeScript syntax)

#### Scenario: Production bundle produces audible output

- **WHEN** `npm run build` completes and the app is served from `dist/` or GitHub Pages
- **THEN** `froggers-processor.js` is reachable at `${BASE_URL}froggers-processor.js` with `Content-Type: application/javascript`
- **AND** Play produces audible output
- **AND** status does not show a parse error from `addModule`

#### Scenario: GitHub Pages MIME compatibility

- **WHEN** the sim is deployed to `https://<user>.github.io/<repo>/`
- **THEN** the worklet URL ends in `.js`
- **AND** the worklet URL is not under `assets/` with a content hash
- **AND** `addModule` does not fail due to `video/mp2t` or TypeScript parse errors
