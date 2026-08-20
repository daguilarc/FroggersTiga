# froggers-host-master Specification

## Purpose

Canonical cross-host contract for Froggers Tiga **sim surfaces** (desktop standalone, web/WASM, VST/AU, VCV Rack) after `omni-repository-harmonization`. Use this document first for **what differs per host**; drill into linked baseline specs for scenario-level detail. Excludes Daisy Field firmware (`MANUAL.md`, `src/mk/`, libDaisy).
## Requirements
### Requirement: Four sim hosts with one shared engine

The repository SHALL treat desktop standalone, web/WASM, VST/AU, and VCV Rack as peers over shared `sim/` and `src/core/` code. Host-specific behavior SHALL be expressed through `SimHostKind`, `HostPanelLayout::kModRackCatalog`, host IO adapters, and generated display projections — not duplicated label/topology literals.

#### Scenario: Host kind selects mod availability

- **WHEN** `IsSimModSourceAvailable` is queried for mod index 0 or 1 with `SimHostKind::Vst` or `SimHostKind::Vcv`
- **THEN** the source is unavailable regardless of bridge state
- **WHEN** queried for mod index 4, 5, or 6 on any host
- **THEN** internal sources remain available per host UI projection

---

### Requirement: Mod rack topology differs by host
Mod rack cells SHALL come from `HostPanelLayout::kModRackCatalog` for hosts `Desktop`, `Web`, `Vst`, and `Vcv`. Hosts `DesktopV2` and `VstV2` SHALL use the manifest-owned permanent 15-lane modulation source catalog declared in `FroggersV2AppManifest.hpp` (`kPermanentModulationSources`), with UI projection defined in v2 specs; they SHALL NOT render v1 mod rack cells or the legacy eight-source `V2ModSourceCatalog` indices 7–14 as structural authority.

| Mod index | Source | Desktop | Web | VST/AU | VCV | DesktopV2 | VstV2 |
|-----------|--------|---------|-----|--------|-----|-----------|-------|
| 0 | MIDI CC 1 | yes | yes | no | no | no | no |
| 1 | MIDI CC 2 | yes | no | no | no | no | no |
| 4 | VCO Envelope (legacy sum) | yes | yes | yes | yes | no | no |
| 5 | Random S&H 1 | yes | yes | yes | yes | no | no |
| 6 | Random S&H 2 | yes | yes | yes | yes | no | no |

**DesktopV2 / VstV2 mod sources:** fifteen permanent manifest lanes (VCO pair buses 1+2 / 2+3 / 1+3, per-VCO and pair EFs, LFO 1–3, Random/Marbles 1–2, External Audio audio-rate and EF). MIDI CC A/B are controller targets only — not mod-rack lanes. Raw VCO audio-rate lanes and VCO 1+2+3 EF are absent.

**Cell counts:** desktop **5**; web **4**; VST/AU v1 **3**; VCV **3**; DesktopV2 **15-lane parameter-detail rack + local indicators**; VstV2 **same manifest rack projection**.

#### Scenario: v2 desktop excludes v1 CC scope cells
- **WHEN** desktop v2 renders mod sources
- **THEN** indices 0 and 1 do not appear as scope cells
- **THEN** external MIDI CV is configured through v2 MIDI/controller assignment UI (manifest target IDs)

#### Scenario: Cross-host LED curve parity on Random S&H v2
- **WHEN** Random/Marbles lane 1 is active on DesktopV2 with audio running
- **THEN** Random LED brightness follows the manifest-declared modulation visualization rules

### Requirement: External MIDI and parameter control differ by host
Each sim host SHALL expose external MIDI, continuous parameters, and mod-assignment UI according to its column in the matrix below.

| Host | External MIDI / CC | Continuous parameter surface | Mod assignment UI |
|------|-------------------|------------------------------|-------------------|
| **Desktop standalone** | Two hardware CC pairs via MIDI Settings (CC 1 default On, CC 2 default Off); QWERTY → CC 1 | On-panel knobs + patch cables | Patch cables from mod rack |
| **Web** | Web MIDI CC 1 when enabled | Expanded pages 1–5 + **global Crunchy**; v1 mod dropdowns | Dropdown per knob; v1 four-cell mod bay |
| **VST / AU v1** | **None** in plugin — `acceptsMidi()` false; DAW maps any MIDI/CC to **107** host parameters | DAW automation + plugin knobs | Patch cables; no hosted MIDI Settings |
| **VCV Rack** | **None** in module — use Rack MIDI-to-CV → per-parameter jacks | Section knobs + per-parameter CV inputs + global Crunchy CV; no selected page | Internal mod routes + CV jacks |
| **Desktop v2** | One assignable MIDI input for pitch/gate/CC targets | Carousel knobs + mod grid + control core | Lit cells + dropdown |
| **VST / AU v2** | DAW MIDI → any `HostParameterInventoryV2` parameter | Full v2 inventory + carousel UI | Lit cells + dropdown; DAW maps MIDI to parameters |

#### Scenario: VST v2 accepts MIDI for parameter modulation
- **WHEN** a DAW sends MIDI to FroggersTigaPluginV2
- **THEN** `acceptsMidi()` is true
- **THEN** parameter changes from MIDI arrive through JUCE host parameter mapping, not raw `ModMgr` CC slots 0/1

#### Scenario: VCV exposes no MIDI boundary
- **WHEN** a VCV main or extension module is placed
- **THEN** the module has no Froggers-owned MIDI port, MIDI queue, CC-enable control, or MIDI-specific saved state
- **THEN** external MIDI must arrive as ordinary Rack CV from another module

### Requirement: VCV host contract is section and expander based
VCV Rack SHALL be modeled as a section/expander host, not a paged host. The Rack-facing VCV code SHALL use named sections and extension snapshots for Audio, Random, Filter, Drive, Reverb, Delay, Global, and VCO AR controls. VCV SHALL NOT use `m_currentPage`, page navigation, or shared hardware/current-page knob positions to apply Rack controls.

#### Scenario: VCV control surface has no selected page
- **WHEN** a VCV Rack knob changes on any visible section
- **THEN** the host applies the value to named VCV section state
- **THEN** there is no selected-page transition or current-page replay

#### Scenario: Shared engine compatibility remains internal
- **WHEN** VCV section state is applied to the shared engine
- **THEN** any internal legacy page/bank mapping remains behind a VCV-safe adapter
- **THEN** desktop, web, and VST host behavior remains unchanged

### Requirement: VCV global Crunchy participates in host differences
VCV Rack SHALL expose global Crunchy as a main-module knob with a CV input. The effective value SHALL clamp the sum of knob value and normalized Rack CV. This global control SHALL coexist with section-local Crispy controls.

#### Scenario: VCV global Crunchy appears as global control
- **WHEN** a user places the VCV main module
- **THEN** global Crunchy knob and CV input are available on the main module
- **THEN** per-section Crispy rows remain available where those sections expose Crispy

### Requirement: VCV per-parameter CV combines with internal modulation

VCV SHALL compute `internalEffective = ModMgr::Modulate(base, modIndex, depth)` once per target, then for connected jacks `clamp(internalEffective + voltage / 10, 0, 1)`. Disconnected jacks SHALL use `internalEffective` only. Base, route, and depth SHALL NOT mutate during jack evaluation. VCV SHALL pass this combined value as temporary effective state and SHALL NOT write the combined value back into the stored base knob while the internal route remains active.

#### Scenario: Negative CV clamps

- **WHEN** a negative CV is patched to a parameter jack
- **THEN** the effective value clamps at 0 after addition

#### Scenario: Disconnected internal route is not doubled
- **WHEN** a VCV target has base `0.0`, internal route source `5`, depth `1.0`, source value `0.4`, and no jack connected
- **THEN** the effective value supplied to the engine is `0.4`
- **THEN** the stored base value remains `0.0`
- **THEN** the route source `5` is not applied a second time by a later engine read

---

### Requirement: Shared modulation blend on all hosts

All modulated targets (page rows, Delay sidecar, pair-AR) SHALL use crossfade blend via `ModMgr::Modulate`:

`effective = clamp(base × (1 − depth) + modSource × depth, 0, 1)`

Invalid or `None` mod indices SHALL be safe (no crash, predictable fallback).

#### Scenario: Pair-AR displays effective value when mod active

- **WHEN** a pair-AR parameter has mod depth > 0 and a live mod source on desktop or web with audio running
- **THEN** the rotary displays the effective blended value, not frozen base

---

### Requirement: Generated host display authority

Labels, page names, global-strip strings, mod-rack projections, and scope capacity SHALL originate from `sim/ParamDisplayNames.hpp` and `sim/HostPanelLayout.hpp`, with web projection generated to `web/src/hostDisplay.generated.ts` by `scripts/generate-host-display.mjs`. Hand-maintained duplicate tables in host UI source SHALL NOT exist.

#### Scenario: Generator check in CI

- **WHEN** `node scripts/generate-host-display.mjs --check` runs
- **THEN** `web/src/hostDisplay.generated.ts` matches the C++ tables

---

### Requirement: Hosted plugin UX (VST/AU only)

When running as a hosted plugin, the editor SHALL hide Record/Export and hardware Audio/MIDI device controls, SHALL disable QWERTY CC capture, SHALL enforce minimum editor size (`kHostedEditorMinWidth` × `kHostedEditorMinHeight`), and SHALL drain stopped-transport UI mutations before the next render while resyncing `PatchCableOverlay` after state restore, Rand Mods, manual patching, and clears.

#### Scenario: Hosted chrome hidden

- **WHEN** FroggersTiga loads as a VST3 or AU plugin in a DAW
- **THEN** Record, Export, Audio Settings, and MIDI Settings controls are not visible
- **THEN** QWERTY input does not drive CC 1

#### Scenario: Standalone retains hardware controls

- **WHEN** `FroggersTiga.app` runs as standalone
- **THEN** Record/Export, Audio Settings, and MIDI Settings remain available

---

### Requirement: Realtime allocation policy

Native/WASM steady-state audio paths SHALL NOT perform owned heap allocation per block. `WasmSimHost` SHALL use fixed scratch; web worklet SHALL preallocate heap sized by `froggers_max_process_chunk()`; `AudioEngine` SHALL chunk via `prepareRenderBuffers`; `AudioRecorder` SHALL use fixed SPSC pool. Browser structured-clone and GC are **outside** the owned-allocation claim.

#### Scenario: WasmSimHost repeated process is allocation-stable

- **WHEN** `WasmSimHostMalloc_test` runs hundreds of `processBlock` calls with varying block sizes
- **THEN** fixed scratch and scope storage do not grow and the test passes

#### Scenario: Worklet render path avoids malloc

- **WHEN** `scripts/verify-wasm-render-allocation.mjs` scans `froggers-processor.ts`
- **THEN** the render-path `process()` and `readScopeSamples()` contain no `malloc` or `free`

---

### Requirement: Release and version authority

Desktop release channel SHALL be exactly `froggerstiga-v1`. Version strings SHALL derive from CMake `PROJECT_VERSION` (currently aligned with web `package.json`). Daisy firmware versioning is out of scope.

#### Scenario: Non-channel tag rejected

- **WHEN** `desktop/scripts/verify-tag-version.sh` is invoked with a tag other than `froggerstiga-v1`
- **THEN** the script exits non-zero

#### Scenario: Release metadata roots agree

- **WHEN** `desktop/scripts/verify-release-metadata.sh` runs
- **THEN** CMake version, app macro, and web package roots report consistent values

---

### Requirement: SimHostKind includes v2 surfaces
The repository SHALL add `SimHostKind::DesktopV2` and `SimHostKind::VstV2` for the forked desktop and VST products. v1 kinds remain unchanged.

#### Scenario: v2 mod catalog query
- **WHEN** `IsSimModSourceAvailable` is queried for index 7 with `SimHostKind::DesktopV2`
- **THEN** the source is available
- **WHEN** queried for index 0 with `SimHostKind::VstV2`
- **THEN** the legacy CC1 scope cell is unavailable (v2 uses host-parameter MIDI only on VST)

### Requirement: v2 mod rack replaced by manifest permanent source rack
Desktop v2 and VST v2 SHALL NOT use `HostPanelLayout::kModRackCatalog` five-cell mod rack or legacy `V2ModSourceCatalog` as structural authority. They SHALL use the Froggers v2 app manifest permanent 15-lane source catalog for parameter-detail modulation, Rand Mods eligibility, and sequencer snapshot fields.

#### Scenario: v1 desktop mod rack unchanged
- **WHEN** `SimHostKind::Desktop` renders mod rack
- **THEN** five-cell patch-cable rack behavior is unchanged

#### Scenario: v2 uses fifteen manifest lanes
- **WHEN** `SimHostKind::DesktopV2` renders parameter-detail modulation
- **THEN** exactly fifteen permanent manifest source lanes are available per row eligibility
- **THEN** MIDI CC A/B are absent from the source rack

### Requirement: v2-host-page-count-and-adsr
v2 sim hosts SHALL use **seven** host pages (indices 0–6). Index 6 is the ADSR module. v2 hosts SHALL use `VcoAdsrState` instead of `AudioPairArState`.

#### Scenario: Host page count on v2
- **WHEN** `SimHostKind::DesktopV2` queries host page count
- **THEN** the count is 7 including ADSR at index 6

#### Scenario: Global Crunchy on v2
- **WHEN** v2 host applies global Crunchy
- **THEN** `Fuegoize` runs on every persisted row on every module page including all Crispy instances (`CrispyRowForPage`: row 7 on Audio; row 9 on expanded modules 1–5 and ADSR) and all musical rows
- **THEN** per-page Crispy rows remain present on every module page

#### Scenario: Per-page Crispy retained on v2
- **WHEN** Audio module (page 0) is visible
- **THEN** row 7 Crispy is present and applies page-local fuego to rows 0–6 after global Crunchy
- **WHEN** any expanded module page 1–5 is visible
- **THEN** row 9 Crispy is present and applies page-local fuego to rows 0–8 after global Crunchy

#### Scenario: pair-AR inactive on v2
- **WHEN** `SimHostKind::DesktopV2` processes audio
- **THEN** `AudioPairArState` is not ticked
- **THEN** `VcoAdsrState` supplies gated ADSR envelope shaping per VCO

### Requirement: v2-sequencer-on-v2-hosts
v2 sim hosts SHALL include `SequencerState` integrated with the control core and `VcoAdsrState` gate input.

#### Scenario: Sequencer gate drives ADSR
- **WHEN** sequencer playback is active on DesktopV2
- **THEN** per-step gate merges with MIDI gate before `VcoAdsrState` tick

## Host difference reference

Read top-to-bottom when implementing or reviewing a change. **Same** = behavior matches across all four sim hosts unless noted.

### Desktop standalone

| Topic | Behavior |
|-------|----------|
| Layout | Five visible submodule panels + Delay overlay; no page pills |
| Mod rack | **5 cells** — CC 1 scope, CC 2 scope, VCO Env scope, Random 1/2 LEDs |
| MIDI | MIDI Settings dialog: two CC→CV pairs + hardware MIDI out; CC 2 default Off |
| Keyboard | QWERTY → CC 1 when focused |
| Pair-AR UI | Horizontal band on Audio page; rotated labels; patch jacks |
| Patch cables | Full overlay on all assignable knobs including pair-AR |
| Parameters | Not exposed as DAW host parameters (standalone app) |
| Record/Export | Visible |

**Baseline specs:** `mod-rack-dual-midi-jacks`, `midi-cc-mod-gating`, `midi-cc-to-mod-cv`, `desktop-midi-cc-display`, `audio-pair-ar-desktop-ui`, `pair-ar-rotated-desktop-labels`, `global-strip-marbles-label`

---

### Web / WASM

| Topic | Behavior |
|-------|----------|
| Layout | Paged UI; one host page at a time; mobile ≤720px uses **3-column** knob grid |
| Mod bay | **4 cells** — CC 1 scope (greyed when External MIDI off), VCO Env, Random 1/2 |
| MIDI | External MIDI permission-gated; **CC 1 only**; no CC 2 UI or ingestion |
| Mod assignment | Dropdown below each knob (no patch cables) |
| Pair-AR UI | Third knob row on Audio page (cells 8–11) |
| Global strip | Below External MIDI, above mod bay (`Rand Resample` label) |
| Mobile audio | `navigator.audioSession` hooks on mobile only |
| WASM scope | 96 samples; chunk size 4096; worklet heap allocated once |

**Baseline specs:** `web-midi-mod-rack`, `midi-cc-mod-gating` (CC 1 path only), `web-mobile-knob-labels`, `web-mobile-global-strip-placement`, `web-mobile-external-audio-routing`, `audio-pair-ar-web-ui`, `web-playwright-e2e`

---

### VST / AU (local-only build)

| Topic | Behavior |
|-------|----------|
| Layout | Same six-panel desktop chrome with hosted chrome hidden |
| Mod rack | **3 cells** — indices 4/5/6 only |
| MIDI | **No** plugin MIDI ingest; **107** JUCE host parameters |
| DAW routing | Map any MIDI channel/CC to any exposed parameter in the host |
| State | Versioned envelope v3; legacy v1/v2 snapshots via `SimPresetSnapshot` magic |
| Transport | DAW owns audio; standalone transport/record hidden |
| Input bus | Mono input host-optional for Ext. In. |
| Identity | `IS_SYNTH TRUE`; instrument/generator |

**Manual verification:** Steinberg validator, pluginval ≥5, auval, REAPER + Logic smoke — see archived `MANUAL_TEST_PLAN.md` in `openspec/changes/archive/2026-06-19-omni-session-artifacts/`.

**Supersedes:** `juce-vst-cc-mod-gating` (historical fixed CC-pair model — do not re-implement)

---

### VCV Rack (local-only build)

| Topic | Behavior |
|-------|----------|
| Mod rack | **3 cells** — 4/5/6; Random LEDs only (no scopes on 5/6) |
| MIDI | **None** in module; patch Rack MIDI-to-CV externally |
| CV jacks | Per-parameter; voltage adds to internal route |
| Random pools | Exclude mod indices 0 and 1 |
| Patch state | Schema v2 marker; v1→v2 ID remap on load |
| Panel | Generated from shared catalog; silkscreen readable at 100% zoom |
| Pair-AR time | 1 ms – 10 s exponential (Fundamental ADSR parity) |

**Supersedes:** `vcv-cc-mod-gating` (historical CC enable/MIDI ingest — removed)

**Baseline specs:** `vcv-panel-silkscreen`, `pair-ar-vcv-time-range`

---

## Shared across all sim hosts

| Topic | Spec / code authority |
|-------|------------------------|
| Pair-AR engine (4 params, envelope) | `audio-pair-ar-engine`, `PairArEnvelope.hpp` |
| Pair-AR randomize parity | `pair-ar-randomize` |
| Modulated knob display (pair-AR) | `pair-ar-modulated-knob-display` |
| Mod blend semantics | `mod-blend-semantics`, `ModMgr::Modulate` |
| PM3 knob label parity | `sim-pm3-knob-parity` |
| Operator doc mirrors | `sim-operator-doc-parity`, `SIM_MANUAL.md` |
| Global strip labels | `global-strip-marbles-label` |
| CC→mod CV mapping (where MIDI exists) | `midi-cc-to-mod-cv` |

---

## Baseline spec index

| Spec | Applies to | Notes |
|------|------------|-------|
| `froggers-host-master` | All sim hosts | **This document — read first** |
| `audio-pair-ar-engine` | All | Shared engine |
| `audio-pair-ar-desktop-ui` | Desktop | Band + jacks |
| `audio-pair-ar-web-ui` | Web | Third row |
| `pair-ar-rotated-desktop-labels` | Desktop | 90° labels |
| `pair-ar-modulated-knob-display` | Desktop, Web | Effective knob display |
| `pair-ar-randomize` | All | Rand parity |
| `pair-ar-vcv-time-range` | All (VCV display ref) | 1 ms–10 s |
| `mod-blend-semantics` | All | Crossfade |
| `global-strip-marbles-label` | Desktop, Web | Rand Resample |
| `mod-led-level-meter` | Desktop, Web, VST, VCV | Random LED brightness |
| `midi-cc-mod-gating` | Desktop, Web | CC enable flags |
| `midi-cc-to-mod-cv` | Desktop, Web | Bridge mapping |
| `mod-rack-dual-midi-jacks` | Desktop | Two CC jacks |
| `desktop-midi-cc-display` | Desktop | Dialog layout |
| `web-midi-mod-rack` | Web | Four-entry bay |
| `web-mobile-knob-labels` | Web | 3-col mobile |
| `web-mobile-global-strip-placement` | Web | Strip order |
| `web-mobile-external-audio-routing` | Web mobile | audioSession |
| `web-playwright-e2e` | Web CI | Playwright |
| `sim-pm3-knob-parity` | All | Label parity |
| `sim-operator-doc-parity` | Docs | SIM_MANUAL sync |
| `vcv-panel-silkscreen` | VCV | Panel SVG |
| `vcv-cc-mod-gating` | — | **Historical only** — pre-omni VCV MIDI |
| `juce-vst-cc-mod-gating` | — | **Historical only** — pre-omni VST CC pairs |

---

## Operator documentation

Human-readable mirror of host differences: `SIM_MANUAL.md` § **Host input boundaries** and § **Host guide**. Release packaging: `desktop/PACKAGING.md`, `README.md`.

---

## Verification commands

```sh
node scripts/generate-host-display.mjs --check
cd web && npm run verify:host-display && npm run verify:wasm-render-allocation
```

Automated sim/desktop tests: 14+ ctest targets including `HostParameterProcessor_test`, `WasmSimHostMalloc_test`, `OwnedAllocation_test`.

---

## Archived session artifacts

Planning session files consolidated here and moved out of the active omni change folder:

| Artifact | Location | Contents |
|----------|----------|----------|
| Change disposition (task 1.1) | `openspec/changes/archive/2026-06-19-omni-session-artifacts/disposition.md` | Reconcile vs supersede table |
| Final audit (task 9.8) | `openspec/changes/archive/2026-06-19-omni-session-artifacts/final-audit.md` | Automated gate evidence |
| Manual test plan (5.10 / 9.7) | `openspec/changes/archive/2026-06-19-omni-session-artifacts/MANUAL_TEST_PLAN.md` | DAW/validator checklist |

OpenSpec change history: `openspec/changes/archive/2026-06-18-*` (superseded), `openspec/changes/archive/2026-06-19-*` (reconciled).
