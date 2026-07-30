## ADDED Requirements

### Requirement: web-parameter-expansion-subset
The web/WASM host (`SimHostKind::Web`) SHALL adopt **engine and UI parameters** from `desktop-v2-module-expansion` and global **Crunchy** fuego. It SHALL **NOT** adopt desktop v2 chrome: encoder rings, module carousel relabeling, scenes, gestures, shift, sequencer, ADSR module page, v2 mod-source grid, or `FroggersV2ControlCore`.

#### Scenario: Web gains expanded module rows
- **WHEN** the web sim renders module pages 1–5 (Random, Reverb, Filter, Drive, Delay)
- **THEN** each page shows ten knob rows per `desktop-v2-module-expansion` (rows 0–6 legacy, 7–8 new, row 9 Crispy)
- **THEN** labels come from shared `V2ParamDisplayNames` / generated `hostDisplay` bindings

#### Scenario: Web Audio page unchanged
- **WHEN** the web sim renders the Audio page
- **THEN** no v2 expansion rows are added
- **THEN** v1 pair-AR band remains on Audio (web does not get ADSR module page)

#### Scenario: Web Filter uses parallel topology
- **WHEN** `SimHostKind::Web` processes the filter stage
- **THEN** parallel Comb/Peak + Scoop behavior matches `desktop-v2-module-expansion` (shared engine path with DesktopV2)

#### Scenario: Web keeps v1 interaction model
- **WHEN** the user adjusts knobs on web
- **THEN** v1 web UX remains: page pills, rotary knobs, mod dropdowns per row, v1 mod bay (4 cells)
- **THEN** no encoder rings, scene strip, or gesture controls appear

### Requirement: web-global-crunchy-knob
The web global strip SHALL expose a **Crunchy** rotary control applying global fuego per v2 Crunchy/Crispy stacking rules in `design.md` §9.

#### Scenario: Crunchy visible on web
- **WHEN** the web app renders the global control strip
- **THEN** a **Crunchy** knob is visible alongside existing Play/Stop/External/Rand controls
- **THEN** Crunchy fuegoizes all persisted rows on all pages including every Crispy instance

#### Scenario: Per-page Crispy retained on web
- **WHEN** any module page is visible on web
- **THEN** per-page Crispy remains (row 9 on expanded pages 1–5; Audio retains its existing Crispy row index)
- **THEN** page Crispy stacks after global Crunchy on musical rows

#### Scenario: Web manual and quick-dict updated
- **WHEN** web help content is published
- **THEN** `web/public/sim-manual.md` and quick-dict document Crunchy and new module row labels

### Requirement: web-playwright-e2e-v2-subset
The web v2 parameter subset SHALL have Playwright e2e coverage using existing `web/e2e/` infrastructure and shared selectors in `web/test-shared/simSelectors.ts`.

#### Scenario: Expanded module pages show ten knobs
- **WHEN** Playwright navigates to each module page 1–5
- **THEN** exactly ten knob labels are visible including rows 7–8 expansion labels and row 9 Crispy per `V2ParamDisplayNames`

#### Scenario: Crunchy visible in global strip
- **WHEN** Playwright loads the web app
- **THEN** a **Crunchy** control is visible in the global strip alongside existing Play/Stop/External/Rand controls

#### Scenario: Filter expansion labels
- **WHEN** Playwright selects the Filter page
- **THEN** **Comb/Peak** (row 7) and **Scoop** (row 8) labels are visible

#### Scenario: v1 Playwright specs remain green
- **WHEN** `npm run test:e2e` runs on the feature branch after v2 web changes
- **THEN** all pre-existing Playwright specs pass without regression

### Requirement: web-out-of-scope-desktop-v2-chrome
The following SHALL NOT ship on web as part of this change:

- ADSR module carousel page and `VcoAdsrState` UI (pair-AR stays on Audio)
- Encoder rings, scene L/R, scene blend, gestures, shift
- Full step sequencer
- Eight-source v2 mod grid (web keeps v1 four-cell mod bay)
- Pink-noise LFO and EF scope grid (desktop v2 mod catalog)
- `BUILD_DESKTOP_V2` / `FroggersTigaPluginV2` surfaces

#### Scenario: Web UI excludes desktop v2 chrome
- **WHEN** the web sim renders any page
- **THEN** no encoder rings, scene strip, gesture controls, or ADSR module page appear
- **THEN** the mod bay remains the v1 four-cell layout

#### Scenario: Web DOM excludes desktop v2 chrome
- **WHEN** Playwright loads the web app on the v2 feature branch
- **THEN** no encoder rings, scene strip, gesture controls, or ADSR module page appear in the DOM
- **THEN** the mod bay remains the v1 four-cell layout
