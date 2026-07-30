## Context

Froggers v2 and Sheaf converge where the user experiences a synth as pages, parameters, modulation assignments, MIDI mappings, files, and runtime setup.

Froggers v2 already has a JUCE desktop/VST product surface, `FroggersV2ControlCore`, host bridge, v2 mod source catalog, sequencer, ADSR page, global Crunchy, per-page Crispy rows, hosted plugin editor, and a growing set of layout and host-parameter specs. Its duplicate-authority risks are row inventories, host parameter inventories, MIDI target inventories, runtime setup state, and carousel/panel layout math.

Sheaf has reusable synth-app pieces: a JUCE-free app core, `AppContext`, parameter/modulation manager, bank/depth-view semantics, message buses, patch manager, `Runtime<App>`, and runtime page adapters for Audio/Controllers/File. Before Froggers adopts those pieces, the Controllers page must show labeled assignment fields, explicit mapping event fields, multi-target fan-out state, persistence, and context-appropriate target readback; the Audio page must show labeled audio setup fields and external-input state.

The convergence rule is product-first: Froggers functionality specs remain binding. Sheaf pieces are adopted only when they reduce Froggers duplicate state or make File/Audio/MIDI setup and parameter modulation easier to implement without losing Froggers DSP, v2 UI, host behavior, release policy, or VST/AU parameter contracts.

`desktop-v2-module-column-layout` is folded into this change. The layout fix is a concrete instance of the broader OMNI rule: one structural authority for a repeated synth-app concept, then host/page-specific projections from that authority.

Canonical OMNI inputs for this change:

- `openspec/specs/froggers-host-master/spec.md` remains the cross-host source of truth for host differences. If a delta spec is ambiguous, the host-master contract wins.
- `AGENTS.md` remains the desktop release authority: exactly one desktop release channel, `froggerstiga-v1`; no tag names matching `desktop-v*` are created, pushed, or documented as release channels; no desktop-only version bump unless explicitly requested.
- `openspec/changes/desktop-v2-module-column-layout` is superseded only after its requirements and gates are represented in this change and verified by crosswalk.
- Older overlapping changes are archived locally; this convergence change owns still-relevant copied layout/chrome/sequencer/boot-path work and must preserve crosswalk evidence before implementation closes.
- Sheaf-derived code must be local, inventoried, and covered by Froggers tests before product builds use it. This change does not permit live dependency fetches, package installs, or external runtime ownership.

## Goals / Non-Goals

**Goals:**

- Make Froggers v2 use the Sheaf page/model/runtime ideas that serve the Froggers desktop and VST/AU user stories.
- Add a Froggers v2 manifest family as the durable authority for shared product controls plus explicit desktop, VST/AU, and reserved VCV overlay fields.
- Introduce a Sheaf-compatible app-core facade around the existing Froggers v2 core before migrating internals.
- Upgrade Sheaf-style Controllers and Audio runtime pages so every setup field is labeled and every persisted mapping/device decision has a visible state.
- Keep desktop standalone hardware MIDI/audio configuration and hosted plugin DAW mapping distinct by projection, not by duplicate code paths.
- Fold the module column-layout proposal into this change and preserve its exclusive-column/bounds-test gates.
- Add validation gates that reject duplicate row/source/ID/MIDI-target authorities.

**Non-Goals:**

- Rewrite Froggers DSP or change sound/control ranges.
- Remove the Froggers v2 carousel, performance band, sequencer, ADSR page, global Crunchy, per-page Crispy, global oscilloscope, or hosted editor UX.
- Import Sheaf as an external dependency during this change. Any borrowed Sheaf code must be copied, namespaced or translated into FroggersTiga, and listed in an adoption inventory before product builds use it.
- Change v1 desktop, v1 web/WASM, v1 VST/AU, VCV, Daisy firmware, release tags, package version, or GitHub release workflow.
- Claim sample-accurate DAW MIDI automation beyond the current JUCE host-parameter contract.
- Ship an unfinished Sheaf miniapp UI as the Froggers UI.
- Change VCV runtime behavior, VCV panel topology, VCV MIDI/CV boundaries, or VCV release/package behavior. The `vcv` manifest overlay is reserved schema space only in this change.

## Option Inventory

The Sheaf structural options are:

- **Page model:** adopt. The desktop shell keeps always-visible right-side File, Audio, and MIDI buttons. The synth module carousel remains Froggers-style, with Audio/VCO as the default page and arrow navigation between synth modules. Clicking a parameter opens a focused page/detail view for that parameter's modulation encoders.
- **Parameter/modulation manager and bank/depth semantics:** adopt with Froggers vocabulary. Each eligible parameter can expose a permanent 15-lane source rack with depth editing, a CV LED modulation monitor inside the encoder, a clickable `MOD` label for drill-in, depth-zero/off defaults, no patch cables, Random sources, EF sources, audio-rate sources, first-class LFO module sources, external-audio source availability, Crispy/Crunchy participation, scene participation, and sequencer lock participation.
- **MIDI/controller model:** adopt after extension. Every eligible parameter gets a manifest target ID. Desktop standalone maps hardware MIDI/QWERTY/controller events to those targets. VST/AU maps DAW automation/MIDI through host parameter semantics rather than a hidden private MIDI table.
- **Held gesture model:** reject. Froggers does not have enough MIDI/controller routing to support press-and-hold gestures cleanly, so Randomize All, Randomize Mod, Crunchy, Crispy, and related controls remain explicit commands or parameter affordances rather than held gestures. Locked parameter values live in the clocked sequencer snapshot/lock model.
- **Global randomization scope controls:** adopt. Randomize All and Randomize Mod share two visible scope pairs below the global buttons: `All Scenes` / `Current Scene` and `All Steps` / `Current Step`. The scene pair controls whether the randomization writes parameter/modulation contents for all scene stores or only the active scene edit-target endpoint. It does not randomize which scene ordinals are selected on the left or right side of the scene slider. The step pair controls whether the randomization writes every sequencer step snapshot or only the current step, where current follows the existing v2 rule: playhead while playing, edit step while stopped.
- **Fixed sequencer length and transport controls:** adopt. The sequencer has exactly 16 slots and no user-facing length control. A compact two-row icon strip sits above the step grid: direction row `<`, `>`, `RND` with `>` selected by default, and speed row `/2`, `/1.5`, `1`, `x1.5`, `x2` with `1` selected by default. Each slot has a written/unwritten state; a device-neutral long press on a written step directly clears that step and marks it unwritten. Mouse press-and-hold, touch press-and-hold, and holding a mapped MIDI/controller step control all emit the same step-local clear command after the long-press threshold, with no second click, menu selection, or controller confirm. This is not the rejected held-gesture model because it resolves to one addressed step and does not create a modifier for later parameter targets. Playback skips unwritten slots, so odd effective sequence lengths come from the written-step mask rather than from resizing the sequencer.
- **Clock sync model:** adopt as sequencer input only. The clocked sequencer can run from the internal clock or an explicit MIDI clock source, but MIDI clock synchronizes step timing; it does not create gesture routing or a second parameter-control path.
- **Runtime lifecycle facade:** adopt as a wrapper first. Froggers exposes configuration, initialize, prepare, process, message ingress, UI-state publish, and shutdown hooks while delegating DSP and behavior to the existing v2 core during migration.
- **Message bus envelope:** adopt where it removes duplicate UI/control paths. Existing Froggers messages remain valid until an equivalence test covers the replacement.
- **Patch manager/File page:** adopt for saving and loading synth presets, because File/Patch must own patch identity, dirty state, save/load/revert results, and persisted controller mappings.
- **Visual miniapp layout:** reject as product UI. Froggers keeps its carousel/performance/sequencer visual language, with only the persistent right-side File/Audio/MIDI access and Sheaf-style multi-signal oscilloscope behavior borrowed from the miniapp behavior.
- **External Sheaf dependency:** reject for product builds. Any adopted code is copied, namespaced, or translated into FroggersTiga and tested locally.

## Archived Change Carryover

Current OpenSpec ownership shape after the local archive:

```text
archive/2026-07-06-desktop-v2-module-column-layout ─┐
archive/2026-07-06-desktop-v2-chrome-sequencer-ux ──┼─ carried forward ─▶ froggers-v2-sheaf-runtime-convergence
archive/2026-07-06-desktop-v2-boot-artefact-gate ───┘

active changes: froggers-v2-sheaf-runtime-convergence
```

The carryover rule is:

- `desktop-v2-module-column-layout` is fully folded into this change; the crosswalk must still prove every requirement and gate is represented here before implementation closes.
- Completed `desktop-v2-chrome-sequencer-ux` behavior becomes baseline: Play label, Record audio behavior, Write Seq. behavior, sequencer toolbar fixes, and scoped tests. This convergence change owns remaining operator QA evidence plus later manifest-driven control-core, fixed-16 sequencer, global oscilloscope, and top-chrome convergence behavior.
- `desktop-v2-boot-artefact-gate` is carried forward as boot-path hardening tasks. This change must not alter release workflows, release tags, package versions, or web download release URLs.

## Froggers Product Contract

The convergence must preserve and extend this Froggers v2 shape:

- Three VCOs are cross-coupled. Each VCO can run continuously or ring-modulate external input according to the Froggers audio input mode.
- The Audio/VCO page is the default launch module. It exposes two cross-coupler controls: VCO 1/2 and VCO 2/3.
- An Envelope page follows Audio/VCO and exposes attack/release pairs for VCO 1, VCO 2, and VCO 3.
- VCO 1, VCO 2, and VCO 3 each expose a continuous waveform morph control.
- Waveform morph controls, cross-couplers, envelope A/R controls, filter, distortion, Random/Marbles, reverb, delay, global Crunchy, and eligible modulation depths participate in modulation assignment, randomization, Crispy/Crunchy actions, scene shifts, sequencer snapshots, and sequencer parameter locks according to manifest eligibility.
- Runtime File, Audio, and MIDI pages configure the app and presets; they do not replace the synth module carousel.
- Global Randomize All and Randomize Mod commands are scoped by two always-visible scope pairs: `All Scenes` / `Current Scene` and `All Steps` / `Current Step`. `Current Scene` is the selected scene endpoint whose contents edits will write to, not the current crossfaded/blended audio state. `All Scenes` and `Current Scene` randomize stored scene parameter/modulation values only; they preserve the current left-scene selection, right-scene selection, and scene-slider position. `Current Step` is always defined: it is the playhead step during playback and the edit step when stopped.
- Locked parameter values are sequencer-owned. A lock is stored on one of exactly 16 written sequencer step/snapshot fields and is applied by the clocked sequencer when that written step is active; it is not a held UI gesture, MIDI gesture, hidden controller mapping, or variable-length pattern lane. If every step is unwritten, sequencer transport remains clocked but emits no snapshot/lock recalls; audio continues from the current live synth state just as it does before any sequencer steps have been recorded.

## Successor Review Options

These choices are split into recorded decisions and intentionally open successor questions. Recorded decisions are locked unless the product owner explicitly reopens them; open option sets are bounded so the review starts from concrete trade-offs instead of vague architecture language.

- **Manifest storage format**
  - Decision: C++ declarations are authoritative; JSON/Markdown snapshots are generated for review.
  - Rationale: the implementation is already C++/JUCE-heavy, compiler-checked declarations reduce parser/schema churn, and generated snapshots still make stable IDs, projection overlays, and archived review diffs inspectable.
- **Sheaf parameter/modulation management**
  - Decision: adopt Sheaf-style parameter/modulation manager semantics behind the Froggers facade after manifest validation passes.
  - Rationale: the product decision is to converge duplicate parameter, modulation, source-depth, controller target, and sequencer-lock state. The facade-only path is a migration step, not the final architecture.
  - Implementation sequence: land the facade first for equivalence tests, then move parameter/modulation management behind it in bounded slices validated by manifest and behavior tests.
- **Hosted runtime status UI**
  - Decision: VST/AU uses a collapsible read-only status panel.
  - Rationale: bus/mapping visibility remains available without crowding the hosted editor.
- **File/Patch in VST/AU**
  - Decision: rely only on host state and DAW preset mechanisms for VST/AU in this change. Do not add a plugin preset browser, direct plugin file-system preset save/load, or plugin import/export workflow.
  - Rationale: plugin-managed presets are too broad for this convergence change and risk colliding with host conventions; desktop standalone remains the place where File/Patch owns richer preset save/load/revert behavior.
- **MIDI/Controller assignment mode**
  - Decision: no MIDI learn mode and no recent-event list. Controller setup uses explicit mapping rows for message kind, channel, controller/note number, optional range/value fields, and manifest target IDs.
  - Rationale: mapping stays intentional, repeatable, and file-preset-friendly without turning the Controllers page into a transient diagnostics console.
- **Duplicate physical assignment behavior**
  - Decision: duplicate physical input mappings to different targets are allowed and encouraged as multi-target controller fan-out.
  - Rationale: this matches Sheaf's multi-mapping/controller-profile principle. The UI summarizes fan-out targets and must not block an intentional physical-input-to-many-targets mapping merely because the input address is reused.
- **Target readback formatting**
  - Decision: controller target readback uses the target parameter's product display formatter.
  - Rationale: operators should see musical/product values, not implementation-normalized values. Raw normalized values are reserved for diagnostics, generated reports, or tests, and are not part of the normal Controllers page.
- **Global oscilloscope source mode**
  - Decision: default to three color-coded VCO traces. Sheaf-style modulation visualization changes how those three VCO traces render when their signals are receiving modulation; the scope does not automatically stop being the three-VCO monitor just because a parameter detail view is open.
  - Rationale: the top-chrome oscilloscope explains the Froggers three-VCO instrument first, while borrowing Sheaf's modulation-aware visual language.
- **Top chrome layout**
  - Decision: use one top chrome stack, not two top strips. The first band is the transport/signal band with Play, Stop, and the global oscilloscope; the second band is the global-command band with Randomize All, Randomize Mod, waveform-randomize, Marbles/Rand Resample, Crunchy, Shift, and the scene/step randomization scope pairs.
  - Rationale: global commands stay always available while the center body remains available for compact module grids and the 4x4 parameter-detail grid, and the transport/scope row stays distinct from randomization controls.

## Decisions

### D1 — Froggers v2 uses a manifest family, not one monolithic table

Create a manifest family for Froggers v2:

- `product`: shared Froggers synth modules, controls, stable IDs, ranges, defaults, colors, labels, modulation eligibility, randomization eligibility, Crispy/Crunchy eligibility, scene participation, sequencer snapshot fields, and sequencer lock fields;
- `desktop`: standalone-only runtime controls, hardware audio device controls, hardware MIDI/QWERTY controller controls, file/preset controls, and desktop layout groups;
- `plugin`: VST/AU projection for host parameters, DAW automation names, bus/status readouts, hidden standalone controls, and hosted editor layout groups;
- `vcv`: reserved overlay for VCV-specific module IO, panel controls, and CV mapping when that target enters v2 scope.

The product manifest is the parent authority. A projection overlay varies by context only when it declares the variation and has a validator proving it still maps to product controls or intentionally hides a standalone-only control.

The `vcv` overlay is inert during this change. It defines reserved schema shape needed to prevent follow-on manifest churn, but it must not change current VCV runtime behavior, panel generation, Rack parameter IDs, MIDI boundary, CV semantics, or packaging. Any active VCV v2 projection needs a separate scoped change.

Generated-vs-checked rule:

- Generate artifacts when the output is a mechanical projection of the manifest with no platform behavior: sorted manifest JSON, reviewer Markdown, target ID lists, and static documentation tables.
- Check artifacts when the output is platform code with lifecycle or host behavior: `HostParameterInventoryV2.hpp`, plugin processor/editor behavior, desktop UI row rendering, audio device components, controller components, and VCV code. These files remain hand-authored only while validators compare them to the manifest family.

The required reviewer artifacts are a sorted machine snapshot and a human report: `build/manifest/froggers-v2-manifest.snapshot.json` and `build/manifest/froggers-v2-manifest-report.md`.

**Alternative rejected:** Keep separate C++ tables for UI rows, VST parameter IDs, MIDI targets, and sequencer snapshots. That preserves the current drift vector and violates OMNI single-authority rules.

### D2 — Add a facade before replacing internals

The first implementation step wraps existing `FroggersV2ControlCore` and `FroggersV2HostBridge` in a Sheaf-compatible app-core facade:

```text
Sheaf-style Runtime/App contract
        │
        ▼
FroggersV2AppCoreFacade
        │ delegates
        ▼
FroggersV2ControlCore + FroggersV2HostBridge + FroggersEngine
```

The facade supplies configuration, initialization, prepare, process, message ingress, UI-state publishing, and shutdown hooks. Those hooks are sufficient when File/Patch save/load, Audio setup, MIDI/Controllers mapping, parameter click-through, modulation depth editing, global randomization commands, and sequencer locks all pass through the facade without a second UI-owned state path. Compatibility means behavioral compatibility with the selected Sheaf concepts, not binary/source compatibility with every Sheaf interface.

**Alternative rejected:** Port Froggers directly into the Sheaf miniapp. That risks losing product behavior and makes the miniapp’s current limitations part of the product surface.

### D3 — Runtime pages are owned by structure, projected by host

Standalone desktop gets runtime pages through always-visible right-side buttons:

- **File/Patch:** patch identity, save/load/revert status, dirty state, saved controller mapping state, and result logging. This page owns preset file operations.
- **Audio:** hardware input/output selection, active channels, negotiated sample rate/block size, external input state, input meter state, output meter state, clip/mute/unavailable labels, and hosted bus/status projection. The global top-chrome oscilloscope remains the signal-observation surface.
- **MIDI/Controllers:** selected MIDI input, connection/receiving/error state, explicit mapping event fields, mapped targets, message kind, channel/CC/note data, multi-target fan-out summary, persistence status, and context-appropriate target readback.

VST/AU gets an explicit hosted projection:

- no hardware audio device selector;
- no standalone record/export controls;
- no standalone MIDI device picker or QWERTY MIDI source selector;
- no plugin preset browser, direct plugin file-system preset save/load, or plugin import/export workflow in this change; patch state round-trips through plugin state and DAW/host preset mechanisms;
- Audio runtime information is read-only and limited to host input bus count, host output bus count, active channel layout, sample rate, block size if reported by the host, input present/unavailable, and output active/clipped/muted status;
- MIDI/controller runtime information is read-only and limited to DAW parameter mapping summary, last host automation/MIDI event when exposed by the host path, and no hidden CC table;
- carousel, ADSR/Envelope, Crunchy, Crispy, mod grid, global oscilloscope, and host-parameter-backed controls remain interactive.

**Alternative rejected:** Keep Froggers-specific Audio/MIDI settings panels forever. That duplicates the runtime responsibilities Sheaf is explicitly meant to standardize.

### D4 — MIDI/controller mapping is one model with host projections

Desktop standalone maps physical MIDI note/CC/gate/pitch events into semantic Froggers targets through the controller configuration model. The model stores device refs, mapping rows, target IDs, message kind, channel, number/range, multi-target fan-out state, and persistence.

VST/AU maps DAW MIDI to exposed host parameters. Raw MIDI is not allowed to create a second hidden modulation path. If the plugin accepts MIDI, incoming MIDI must feed host-parameter mapping semantics or a documented host-owned route; it must not bypass the manifest/parameter inventory.

**Alternative rejected:** A plugin-private 16×128 mapping table in parallel with DAW automation. It duplicates host mapping, creates a second state authority, and breaks portability across VST3/AU hosts.

### D5 — Modulation assignment uses Sheaf semantics with a Froggers 4x4 parameter-detail grid

Froggers v2 exposes permanent modulation lanes instead of requiring the player to first create or assign a source. Every depth defaults to zero/off, so a lane can be present without affecting the target. The manifest-owned source rack contains exactly these lanes:

- VCO audio-rate pair buses: **VCO 1+2**, **VCO 2+3**, **VCO 1+3**;
- VCO envelope followers: **VCO 1 EF**, **VCO 2 EF**, **VCO 3 EF**, **VCO 1+2 EF**, **VCO 2+3 EF**;
- LFO module outputs: **LFO 1**, **LFO 2**, **LFO 3**;
- random sources: **Random/Marbles 1**, **Random/Marbles 2**;
- external input sources: **External Audio (audio rate)** and **External Audio (envelope follower)**.

The parameter-detail view presents these 15 source-depth lanes with a sixteenth dedicated **Crispy/target** encoder in a 4x4 grid. The Crispy/target cell keeps the target parameter's own tone-scramble/target affordance visible while the user is editing source depths, instead of making the player leave modulation detail to adjust or inspect the target's Crispy behavior.

The audio-rate VCO lanes are pair buses rather than raw VCO duplicates. This makes the modulation surface more Froggers-specific and lets the manifest avoid direct VCO self-feedback. VCO-owned targets use only the pair bus that excludes the target oscillator: VCO 1 targets use **VCO 2+3**, VCO 2 targets use **VCO 1+3**, and VCO 3 targets use **VCO 1+2** when they accept audio-rate VCO modulation. Non-VCO targets such as filter, distortion, reverb, delay, LFO parameters, random parameters, and Crispy/Crunchy targets can expose all three audio-rate pair buses unless the manifest narrows eligibility.

The raw **VCO 1**, **VCO 2**, and **VCO 3** audio-rate lanes are intentionally excluded because they duplicate the Audio/VCO page and invite direct self-modulation loops. The **VCO 1+2+3 EF** combined follower is also intentionally excluded because it collapses the oscillator cluster into a generic whole-synth energy signal and weakens the more musical adjacent-pair relationships. MIDI CC sources are excluded from this permanent rack; MIDI/controller input maps to parameter targets through the Controllers page and host-parameter projections, not through a second modulation-source metaphor.

Froggers keeps no patch cables, permanent source lanes, encoder CV LED monitors, `MOD` drill-in labels, sequencer snapshot mod fields, and depth editing behavior. The old v2 source catalog is treated as a migration input, not the final source universe. Sheaf contributes the structural semantics: bank slots, visible target cell, modulation-depth drill-down, bipolar depth values, and UI-state min/max/range arcs.

The manifest declares which rows can accept which sources, which source lanes are currently available, and which depths exist. External Audio lanes remain visible in parameter detail views, but are disabled/off and display an unavailable state when no external input source is active. `Rand Mods`, sequencer snapshots, host parameter inventory, and parameter-detail controls consume that same declaration.

**Alternative rejected:** Treat dropdown assignment, depth drill-down, sequencer snapshots, and VST host parameters as separate representations of modulation. They must all project from one mod eligibility/depth model.

**Alternative rejected:** Keep MIDI CC A/B as permanent modulation sources. MIDI already maps to everything through controller/host-parameter mappings; duplicating it as source lanes makes controller setup and sound-design modulation fight over the same job.

### D6 — Module and parameter-detail grids drive the next layout migration

Fold `desktop-v2-module-column-layout` into this change:

- `ModuleRowColumnLayout` remains the single geometry helper.
- `PageCarouselComponent`, `SubmodulePagePanel`, and `AdsrPagePanel` consume it.
- The shell reserves one top chrome stack above the carousel header and center body. It is a single layout region with two bands, not two independent top strips.
- The transport/signal band contains Play, Stop, and the global oscilloscope. It does not contain global randomization controls.
- The global-command band appears below the transport/signal band on normal module pages and parameter-detail pages. It contains Randomize All, Randomize Mod, waveform-randomize, Marbles/Rand Resample, Crunchy, Shift, and the shared scene/step scope radio pairs.
- Randomize All and Randomize Mod show their shared `All Scenes` / `Current Scene` and `All Steps` / `Current Step` scope pairs directly below the command buttons, so the player can see whether a random command will affect all scene endpoints/current scene and all sequencer steps/current step before pressing it.
- The carousel header sits below the top chrome stack, the center body remains reserved for compact module grids and the 4x4 modulation grid, and the fixed sequencer region stays at the bottom of the page. The persistent right-side File/Audio/MIDI runtime rail is separate from the top chrome stack and is not a second top strip.
- The sequencer reserves a fixed-height bottom/sequence region with a two-row direction/speed icon strip above the 16-step grid. The icon strip and all 16 steps must fit the default 1280x920 app screen without truncating labels or requiring scrolling.
- Any legacy `CenterGlobalClusterV2` transition code must be hidden or removed once the global-command band owns global controls, and it must never overlap encoder, module-grid, parameter-detail, or mod-column bounds during migration.
- Carousel module pages render actual module parameters as compact, logical center grids rather than tall row lists when the manifest page fits within the default 1280x920 body.
- Parameter-detail pages render the 4x4 modulation grid.
- If a carousel module would exceed 16 visible parameter cells, the manifest must split it into a named subpage/group rather than relying on default-size vertical scrolling.
- Layout bounds tests prove no center-cluster/mod-cell intersection, no module-page scrollbar, no parameter-detail scrollbar, no sequencer icon truncation, and no sequencer/parameter-grid overlap at 1280×920.

This layout work is implemented early because it creates the visual space needed for runtime/controller convergence, one-screen module pages, and one-screen parameter modulation. At the default 1280x920 standalone size, normal carousel pages are compact parameter grids and parameter detail is a 4x4 encoder grid, not a cramped diagnostics page.

**Alternative rejected:** Keep the column-layout proposal separate. The bigger convergence touches the same carousel/panel/runtime chrome boundary and would otherwise re-open the same layout decisions.

### D7 — Manifest validation is independent from generated outputs

Validation compares generated artifacts to the manifest and checks hand-written consumers for illegal duplicate authority:

- no independent `gridPx(31)` mod X placement;
- no ungenerated VST stable IDs;
- no standalone MIDI target table outside the controller model;
- no mod source catalog literals outside the manifest/catalog source;
- no sequencer snapshot fields omitted from manifest coverage;
- no hosted UI path showing standalone-only controls.
- no release metadata, release tag name, web download URL, package version, v1 host, web/WASM v1, VCV, Daisy firmware, or external Sheaf dependency drift outside the explicit impact list.

**Alternative rejected:** Let generated files validate themselves. A second source of truth or an independent expected inventory is required for meaningful tests.

### D8 — One global top-chrome oscilloscope is the shell-level signal monitor

Add one shell-level signal monitor inspired by the Sheaf miniapp. It lives in the top chrome stack's transport/signal band next to Play and Stop and remains visible across carousel pages and runtime pages.

- **Default view:** displays VCO 1, VCO 2, and VCO 3 as three color-coded traces in the same oscilloscope after waveform morph and cross-coupling and before global reverb/delay output effects.
- **Source-group view:** can display other manifest-declared groups, such as LFO 1-3, VCO pair buses, EFs, Random/Marbles, or External Audio, inside the same oscilloscope when parameter-detail context or a pinned source group asks for it.
- **Sheaf-style visualization rules:** each displayed trace can change its visual treatment when that signal is receiving nonzero audio-rate modulation, without requiring another scope. Color remains tied to the manifest source color so the trace, source lane, and encoder indicators agree.

This monitor is owned by the desktop/plugin shell projection, not by one module page. It remains visible across carousel pages, Audio, MIDI/Controllers, and File/Patch runtime pages at the default 1280x920 standalone size and in the hosted editor minimum layout. It uses fixed-capacity buffers sized by a named constant and repaints on the desktop v2 timer used for existing scope UI. The mod-source grid EF scopes, Random indicators, and encoder-integrated CV LED modulation monitors remain local source indicators; the top chrome stack oscilloscope is the global signal monitor.

**Alternative rejected:** Put signal shape in the runtime Audio page. Audio is for device/routing/status; signal observation belongs in persistent synth monitors.

## Risks / Trade-offs

- **[Risk] Manifest becomes a giant opaque table** → Keep schema sections by domain, add generated docs, and require focused validators per projection.
- **[Risk] Facade adds an adapter layer without reducing complexity** → Use it only as a migration boundary; tasks must retire duplicate tables after each projection moves to the manifest.
- **[Risk] Sheaf controller model lacks Froggers fields** → Extend the model before adoption; tests cover pitch, gate, CC, multi-target fan-out, persistence, and target readback.
- **[Risk] Hosted MIDI semantics remain ambiguous** → Encode the VST/AU rule in specs: DAW mapping targets host parameters; no raw duplicate mod route.
- **[Risk] Runtime pages crowd the desktop UI** → Use MainPane-style runtime pages, not permanent extra chrome; app carousel remains the first screen.
- **[Risk] Column-layout migration conflicts with current dirty desktop-v2 work** → Apply through the folded tasks and inspect existing local changes before editing files.
- **[Risk] Generated host parameter IDs break sessions** → Stable IDs are explicit manifest fields, never derived from labels; tests load legacy/current state.
- **[Risk] Audio page overclaims hosted device control** → Hosted projection shows bus/status only; device selection remains standalone-only.

## Migration Plan

1. Add manifest schema and a read-only manifest for current Froggers v2 behavior.
2. Add validators comparing manifest projections to current host inventory, UI row counts, mod source catalog, sequencer snapshot fields, and MIDI target lists.
3. Fold and implement column-layout repairs using `ModuleRowColumnLayout`.
4. Add the Sheaf-compatible Froggers app-core facade while delegating to existing core/bridge.
5. Upgrade Controllers and Audio runtime page models to Froggers requirements.
6. Switch desktop standalone Audio/MIDI/File access to Froggers-owned runtime adapters reached by persistent right-side buttons.
7. Generate/validate VST/AU parameter inventory and hosted projections from the manifest.
8. Move modulation assignment/depth projections to manifest-driven Sheaf semantics.
9. Run desktop standalone, VST/AU, layout, controller, audio, state, and OMNI duplicate-authority gates.

Rollback is staged by projection: checked platform files remain hand-authored until validators pass; facade can delegate to the old path; runtime pages can be hidden behind existing settings entry points until parity tests pass.

## Open Questions

- None for product behavior in this audit. Remaining choices are implementation sequencing and verification evidence.
