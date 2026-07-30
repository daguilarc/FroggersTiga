## Why

Froggers v2 and Sheaf overlap in four user-visible structures: a page model where each synth parameter can open a focused modulation page, MIDI/controller mapping to every parameter, runtime configuration pages for File/Audio/MIDI, and clocked sequencer state for parameter locks and snapshots. Today those structures are split across Froggers v2, the Sheaf synth miniapp, and a separate column-layout repair plan, which creates duplicate parameter, MIDI assignment, modulation routing, runtime page, and layout decisions.

This change converges Froggers v2 with the Sheaf pieces that reduce duplicate state while preserving the Froggers synth contract. Froggers remains the DSP/product authority: three cross-coupled VCOs that can ring-modulate external input or run continuously, module pages for Audio/VCO, Envelope, Filter, Distortion, Random/Marbles, Reverb, and Delay, per-parameter modulation/randomization/scene behavior, sequencer-owned parameter locks, desktop standalone, and VST/AU behavior.

## What Changes

- Add a **Froggers v2 manifest family**: a shared product manifest plus explicit desktop, VST/AU, and reserved VCV overlay fields for module pages, rows, ranges, defaults, stable host parameter IDs, display names, mod eligibility, fixed 16-step sequencer snapshot fields, scene participation, sequencer lock participation, MIDI assignment targets, clock sync input, and runtime-page projections.
- Introduce a Sheaf-compatible Froggers v2 app-core facade around the existing `FroggersV2ControlCore` / `FroggersV2HostBridge` before replacing internals, so current behavior remains the invariant during migration.
- Adopt Sheaf-style runtime access for **File**, **Audio**, and **MIDI/Controllers** via always-visible right-side buttons, while the carousel arrows continue to move through synth modules and the Audio/VCO page remains the default launch page.
- Improve Sheaf's Controllers page model before adoption: every setup field is labeled, selected input is named, mapping rows expose explicit event fields, mappings target manifest IDs, one physical input can intentionally fan out to multiple targets, saved mappings report persistence, and target readback uses the display format available in that context.
- Improve Sheaf's Audio page model before adoption: every setup field is labeled, output device/channels/sample rate/block size are shown, input device/channels are shown when external input is enabled or unavailable, host bus layout is shown in hosted builds, and signal shape remains visible through one global top-chrome oscilloscope rather than through a duplicate Audio-page oscilloscope.
- Unify MIDI/controller assignment through one controller configuration model. Desktop standalone maps MIDI note/CC/gate/pitch into Froggers semantic targets; VST/AU maps DAW MIDI to exposed host parameters without a duplicate private MIDI routing table.
- Unify modulation assignment under Sheaf-style bank/depth semantics with a Froggers-owned 4x4 parameter-detail grid: 15 permanent source-depth encoders plus one dedicated Crispy/target encoder. The source rack contains three audio-rate VCO pair buses (VCO 1+2, VCO 2+3, VCO 1+3), five VCO envelope followers, three first-class LFO module outputs, two Random/Marbles sources, External Audio audio-rate, and External Audio envelope follower. Depth defaults to zero/off, VCO-owned targets avoid self-feedback by using only audio-rate pair buses that do not contain the target VCO, external-audio lanes remain visible but unavailable when no external input source is active, MIDI remains controller mapping rather than a modulation source, and the no-patch-cable v2 UX is preserved.
- Remove the held-gesture model entirely. Randomize All, Randomize Mod, Crunchy, Crispy, and related controls act as explicit commands or parameter affordances, while locked parameter values live in the clocked sequencer snapshot/lock model instead of in press-and-hold MIDI/controller gestures.
- Add explicit global randomization scope controls below the global Randomize All and Randomize Mod buttons: `All Scenes` / `Current Scene` and `All Steps` / `Current Step`. `Current Scene` means the active scene edit-target endpoint, not the blended scene result; scene randomization writes scene parameter/modulation contents only and never randomizes which scenes are selected for the left or right side of the scene slider. `Current Step` means the playhead step while playing and the edit step while stopped.
- Fix the sequencer at exactly 16 steps with no user-editable length. Add a two-row icon strip above the sequencer: direction icons `<`, `>`, and `RND` with `>` selected by default, and speed icons `/2`, `/1.5`, `1`, `x1.5`, and `x2` with `1` selected by default.
- Let the user clear any sequencer step directly with a device-neutral long press on that step. Mouse press-and-hold, touch press-and-hold, and holding a mapped MIDI/controller step control all emit the same step-local clear command after the long-press threshold; no second click, menu selection, or controller confirm is required. Cleared steps remain visible as unwritten slots but are skipped during playback; clearing all 16 steps puts the sequencer into a clocked no-op state that leaves audio running from the current live synth state, matching playback before any steps have been recorded.
- Update the Froggers product shape: add an Envelope page after Audio/VCO containing A/R pairs for the three VCOs; add continuous waveform morph controls for each VCO that can be modulated, randomized, Crispy/Crunchy affected, and scene shifted; change the Audio/VCO page from one cross-coupler to two cross-couplers, VCO 1/2 and VCO 2/3.
- Add a miniapp-inspired global oscilloscope in the transport/signal band of the top chrome stack next to Play/Stop. Its default view shows three color-coded VCO traces in one scope, and it can apply Sheaf-style multi-signal visualization rules when displayed sources receive audio-rate modulation; external-audio source lanes report unavailable/off state instead of showing stale active traces when no external input is active.
- Fold `desktop-v2-module-column-layout` into this change: exclusive module columns, `ModuleRowColumnLayout`, center-global cluster containment, mod-column container coordinates, scroll policy, performance-band label fixes, and layout bounds regression gates become part of the convergence roadmap.
- Preserve the Froggers desktop v2 and VST/AU v2 product contracts: ADSR page, sequencer, global Crunchy, per-page Crispy, stereo/default bus behavior, hosted editor parity, full host parameter inventory, and release-channel rules remain in force.

## OMNI Rule Audit (2026-07-06)

Audit scope: this change's proposal, design, tasks, and delta specs, checked against `openspec/specs/froggers-host-master/spec.md`, `AGENTS.md` desktop release rules, the archived `omni-repository-harmonization` audit, and the folded `desktop-v2-module-column-layout` change. `openspec validate froggers-v2-sheaf-runtime-convergence --strict` passes after this audit.

| Rule | Finding | Artifact resolution |
|------|---------|---------------------|
| Product-first scope | The change correctly makes Froggers DSP, v2 chrome, sequencer, ADSR/Envelope behavior, Crunchy/Crispy, host inventory, and release policy binding before Sheaf adoption | Proposal and design now name `froggers-host-master` as the canonical host boundary; tasks add a host-master conformance gate |
| Single authority / data flow | Manifest family, `ModuleRowColumnLayout`, controller model, host parameter projection, mod eligibility, sequencer fields, and runtime pages are intended single authorities | Existing specs cover the authorities; tasks add duplicate-authority gates for row labels, stable IDs, MIDI targets, mod eligibility, sequencer fields, and layout offsets |
| Repetition / helper extraction | The folded column-layout work already captures repeated layout math; runtime/controller/audio convergence still needs negative checks to avoid new side tables | Tasks add explicit no-duplicate-table and no-independent-layout checks before archive |
| Host-native integration | Desktop standalone maps hardware MIDI through the controller model; VST/AU v2 maps DAW MIDI through host parameters; raw private CC routing remains disallowed | `vst-v2-midi-modulation` and controller specs already encode this; tasks add a host-master check for v2 mod catalog and MIDI semantics |
| Scope containment | The proposal excludes v1 desktop, v1 web/WASM, v1 VST/AU, VCV runtime behavior, Daisy firmware, release tags, package versions, and GitHub release workflow changes | Design and tasks now add explicit path/release no-change verification instead of relying on prose non-goals |
| Release channel integrity | Release-channel rules were mentioned but not audited as a gate | Tasks now require verifying no tag names matching `desktop-v*` are created, pushed, or documented as release channels; no `releases/latest`; no version bump; and unchanged `froggerstiga-v1` release policy |
| Sheaf local ownership | Existing specs reject an external Sheaf runtime dependency, but task gates did not yet forbid network/package-install drift | Tasks now require an adoption inventory and a no-fetch/no-new-dependency check for Sheaf-derived code |
| Realtime and local reasoning | The global oscilloscope and runtime facade introduce callback allocation and cross-cutting complexity risks unless checked | Tasks now require fixed-capacity oscilloscope/facade scans, no audio-thread allocation in new realtime paths, and nesting/local-reasoning review |
| Verification ownership | Original OMNI tasks were broad and did not record exact evidence for negative boundaries or successor choices | Tasks now require strict validation, hedge grep, manifest validators, duplicate-authority scans, host-master conformance, path no-change review, and option-decision checkpoints |
| Supersession honesty | `desktop-v2-module-column-layout` is folded in, but the proposal needs a closeout crosswalk before superseding it | Tasks now require a requirement crosswalk and supersession note after copied requirements and gates are verified |
| Folded layout behavioral parity | The folded column-layout change required migrated global controls to keep the same `GlobalStripV2` host/control-core mutations, but this convergence copy only carried the geometry constraint | `desktop-v2-center-global-cluster` now records that the global-command band replaces the center-cluster projection and must preserve mutation parity; tasks add a parity test/audit gate |
| Projection consistency | The folded page-carousel spec still read as if every module body must stay in legacy label/encoder/center/mod columns, while this convergence change also requires compact module grids and a 4x4 parameter-detail grid | `desktop-v2-page-carousel` now makes the carousel own the body projection and requires both legacy column and compact grid projections to derive from shared layout helpers |
| Archived capability carryover | Pre-closure hygiene reported duplicate active ownership where this convergence change overlapped `desktop-v2-module-column-layout` and remaining `desktop-v2-chrome-sequencer-ux` deltas | The old changes are archived locally under `openspec/changes/archive/2026-07-06-*`; this proposal now owns still-relevant unfinished work and keeps crosswalk/verification tasks before implementation closes |

## Archived Change Carryover

This change is the successor owner for still-relevant unfinished work copied or expanded from older OpenSpec changes. The older changes were archived locally without syncing their deltas into baseline specs.

| Archived change | Overlap | Disposition |
|---------------|---------|-------------|
| `archive/2026-07-06-desktop-v2-module-column-layout` | `desktop-v2-module-column-layout`, `desktop-v2-grid-layout`, `desktop-v2-center-global-cluster`, `desktop-v2-page-carousel`, `desktop-v2-mod-source-grid`, `desktop-v2-performance-band-chrome` | Fully folded here. Keep the crosswalk task so implementation can prove every requirement and gate remains represented. |
| `archive/2026-07-06-desktop-v2-chrome-sequencer-ux` | `desktop-v2-control-core`, `desktop-v2-sequencing`, `desktop-v2-performance-band-chrome`, `desktop-v2-scope-visualization` | Treat completed Play/Record audio/Write Seq. fixes as existing baseline behavior. This change owns the later manifest, fixed-16 sequencer, global oscilloscope, top-chrome convergence work, and remaining operator QA evidence. |
| `archive/2026-07-06-desktop-v2-boot-artefact-gate` | Boot/release-path hardening for desktop v2 | Folded into this change as boot-path hardening tasks. This does not authorize release workflow, release tag, package version, or web download URL changes. |

The `sim-operator-doc-parity` duplicate ownership between other active changes is outside this change's artifact set and is not resolved by this proposal.

## Capabilities

### New Capabilities

- `froggers-v2-product-contract`: Binding Froggers v2 synth behavior, module set, cross-couplers, Envelope page, waveform morph controls, modulation/randomization/scene participation, clocked sequencer locks, and no held gestures.
- `froggers-v2-app-manifest`: Manifest family for Froggers v2 modules, controls, stable IDs, mod eligibility, MIDI targets, sequencer snapshot fields, and host projections.
- `froggers-v2-sheaf-runtime`: Sheaf-compatible Froggers app-core facade plus right-side File/Audio/MIDI runtime access for desktop standalone and explicit VST/AU projections.
- `froggers-v2-controller-configuration`: Labeled controller assignment and MIDI configuration model for desktop standalone plus DAW parameter mapping semantics in hosted builds.
- `froggers-v2-runtime-audio-configuration`: Labeled runtime audio configuration model with standalone device fields and hosted bus/state projection.
- `froggers-v2-review-options`: Bounded option sets and recorded decisions for implementation. Manifest storage, Sheaf parameter/modulation management adoption, formatted controller target readback, hosted status UI, VST/AU host-state-only File/Patch behavior, controller multi-target behavior, global oscilloscope source mode, and top-chrome layout density are recorded decisions.

### Modified Capabilities

- `desktop-v2-control-core`: Control core becomes manifest-driven at the structural boundary while preserving message-bus, scene, clocked sequencer, parameter-lock, and UI-state behavior.
- `desktop-v2-sequencing`: Sequencer behavior is fixed to 16 steps, with direction and speed controlled by a compact two-row icon strip above the step grid rather than by a pattern-length control.
- `desktop-v2-midi-cv-input`: MIDI CV assignment moves under the controller configuration model and removes ad hoc target/config duplication.
- `vst-v2-midi-modulation`: VST/AU v2 MIDI behavior is clarified as DAW-to-host-parameter mapping with no raw duplicate modulation path.
- `desktop-v2-mod-source-grid`: Mod source assignment migrates to the manifest-owned 15-lane source rack while deriving route eligibility, availability, and depth targets from the app manifest.
- `desktop-v2-audio-io`: Audio configuration gains runtime-page visibility for standalone and hosted bus projection without changing stereo/default processing behavior.
- `desktop-v2-module-column-layout`: The existing column-layout proposal is folded into this convergence change and remains a required layout contract.
- `desktop-v2-center-global-cluster`: The top chrome stack's global-command band replaces center-cluster global-control placement; any legacy center-cluster transition component stays hidden or non-overlapping until removed.
- `desktop-v2-grid-layout`: Module row geometry uses the shared `ModuleRowColumnLayout` authority.
- `desktop-v2-page-carousel`: Carousel owns the column split and hosted/standalone projections.
- `desktop-v2-performance-band-chrome`: Readability fixes remain required at 1280px.
- `desktop-v2-scope-visualization`: A single global oscilloscope is added to the top chrome stack's transport/signal band and remains visible across pages.

## Impact

- `desktop-v2/Source/control/*`, `desktop-v2/Source/HostParameterInventoryV2.hpp`, `HostParameterRegistryV2.*`, `HostParameterRoutingV2.hpp`, and sequencer snapshot code gain manifest validation hooks and generated reviewer reports.
- `desktop-v2/Source/ui/*` consumes manifest projections and the folded column-layout contracts.
- `desktop-v2/Source/MidiCvSettingsComponent.*`, `MidiCvAssignmentTable.*`, `AudioSettingsComponent.*`, and related runtime page code converge with Sheaf’s Controllers/Audio page model.
- `desktop-v2/Source/PluginProcessorV2.*` and `PluginEditorV2.*` preserve hosted behavior while consuming the same app manifest and runtime projections.
- New scripts/tests validate manifest completeness, host parameter inventory coverage, MIDI target coverage, mod-source eligibility, layout bounds, and no duplicate row/source/ID tables.
- The existing `desktop-v2-module-column-layout` change is superseded by this change after its requirements are copied here; implementation SHALL happen from this larger proposal.
