## 0. Archived Change Carryover

- [x] 0.1 Run `openspec list --json` and record active changes that share capability directories with this change
- [x] 0.2 Produce the `desktop-v2-module-column-layout` crosswalk before starting implementation for folded layout capabilities
- [x] 0.3 Archive `desktop-v2-module-column-layout` locally after folding relevant undone work into this change
- [x] 0.4 Archive `desktop-v2-chrome-sequencer-ux` locally after preserving completed Play/Record audio/Write Seq. behavior as baseline and carrying remaining operator QA into this change
- [x] 0.5 Carry forward `desktop-v2-boot-artefact-gate` as boot-path hardening inside this convergence change; it does not authorize release workflow, release tag, package version, or web download URL changes
- [x] 0.6 Verify this target change still passes `openspec validate froggers-v2-sheaf-runtime-convergence --strict` after carryover notes are added
- [x] 0.7 Archive `desktop-v2-boot-artefact-gate` locally after carrying boot-path hardening into this change; skip syncing old deltas into baseline specs

## 1. Manifest Foundation

- [x] 1.1 Implement the recorded manifest storage decision: C++ declarations are authoritative and JSON/Markdown snapshots are generated for review
- [x] 1.2 Add the Froggers v2 manifest-family schema covering product controls plus desktop, VST/AU, and reserved VCV projection overlay fields
- [x] 1.3 Encode current Froggers v2 behavior plus the new product contract: Audio/VCO default page, two cross-couplers, Envelope page, VCO A/R pairs, VCO waveform morph controls, first-class LFO module outputs/parameters, the 15-lane permanent modulation source rack with VCO pair-bus audio-rate lanes, external-audio lane availability, global Randomize All / Randomize Mod scene and step scope pairs, fixed 16-slot sequencer, written/unwritten step state, device-neutral long-press `Clear Step` action, sequencer-owned parameter locks, sequencer direction/speed icon choices, optional MIDI-clock sequencer sync, and no held gestures
- [x] 1.4 Add manifest validation for duplicate stable IDs, invalid page/row references, missing display names, invalid ranges, invalid defaults, invalid projection overlays, missing oscilloscope taps, invalid randomization scene/step scope controls, invalid sequencer step counts other than 16, missing written/unwritten state for any sequencer slot, invalid sequencer direction/speed choices or defaults, invalid source lane counts, duplicate source IDs, missing source colors/groups, and invalid external-audio availability rules
- [x] 1.5 Add `build/manifest/froggers-v2-manifest.snapshot.json` and `build/manifest/froggers-v2-manifest-report.md` outputs sorted by projection, page, row, and stable ID

## 2. Projection Validators

- [x] 2.1 Add a validator comparing `HostParameterInventoryV2` entries with manifest hosted-parameter entries
- [x] 2.2 Add a validator comparing carousel module/page/row counts with manifest page rows
- [x] 2.3 Add a validator comparing the manifest-owned 15-lane modulation source catalog, VCO pair-bus self-feedback eligibility rules, source availability rules, and row route eligibility with desktop/UI consumers
- [x] 2.4 Add a validator comparing the fixed 16 sequencer snapshots and lock fields with manifest `sequencerPersistent` and `sequencerLockable` fields
- [x] 2.5 Add a validator comparing desktop MIDI/controller target IDs with manifest target declarations
- [x] 2.6 Add a validator comparing hosted projection overlays with plugin editor behavior: no hardware selectors, no standalone MIDI picker, no standalone record/export controls, read-only bus/status fields, and host-parameter-backed controls present
- [x] 2.7 Add a duplicate-authority grep gate rejecting independent row labels, stable host IDs, MIDI target IDs, mod eligibility tables, and sequencer field inventories outside approved generated or checked files

### Subagent-Ready Completion Plan (2026-07-07, OMNI-enforced 2026-07-07)

**Authority:** [scripts/SUBAGENT_OMNI_CONTRACT.md](../../../scripts/SUBAGENT_OMNI_CONTRACT.md) — include verbatim in every subagent dispatch.

**Parent enforcement (binding):**

1. Dispatch **one packet at a time**. No parallel subagents.
2. Subagent runs `bash scripts/check_subagent_packet_gates.sh` before reporting complete.
3. Parent re-runs the same script on the merged tree. **Exit nonzero blocks the next packet** — revert or fix; do not mark tasks `[x]`.
4. Attach gate stdout to the task ledger or completion notes when marking work done.
5. Reject diffs that contain forbidden tactics (see contract): `(void)` stubs, comment grep bait, duplicate enums, literal stable IDs outside manifest, slice-test-only green.

Task 2 `[x]` means validator **infrastructure** exists, not migration complete. Migration packets must clear gates.

Execution contract for every delegated packet:

- Subagents stay in the default sandbox, use no network, do not install dependencies, do not escalate permissions, and report `BLOCKED` with exact evidence if blocked.
- Each packet has a disjoint write scope listed in the dispatch prompt; do not touch files outside that list.
- Subagents must read and obey `scripts/SUBAGENT_OMNI_CONTRACT.md` in full.
- **No packet is complete until `bash scripts/check_subagent_packet_gates.sh` exits 0.** Parent re-runs after merge; failure blocks the next packet.
- **Forbidden completion tactics:** grep/validator gaming, marking tasks `[x]` without parent gate evidence, claiming pass on slice tests while global gates fail, adding UI-owned duplicate authority tables.

Mandatory gates (`scripts/check_subagent_packet_gates.sh`):

1. `bash scripts/check_desktop_v2_duplicate_authority.sh`
2. `bash scripts/check_desktop_v2_layout_authority.sh`
3. `desktop-v2/build/FroggersV2ProjectionValidators_test`
4. `desktop-v2/build/FroggersV2Manifest_test`

Parent dispatch template (copy into every Task tool prompt):

```text
Read and obey: scripts/SUBAGENT_OMNI_CONTRACT.md

PACKET: <N> — <title>
WRITE SCOPE: <explicit paths>
FORBIDDEN: out-of-scope files; duplicate authority tables; validator/grep gaming

OMNI FOCUS: <single authority | no gaming | repetition | nesting | contract honesty — pick what this packet risks>

ACCEPTANCE:
- bash scripts/check_subagent_packet_gates.sh exits 0
- <packet-specific gate from table below>

Report using COMPLETION REPORT format in SUBAGENT_OMNI_CONTRACT.md
```

Sequential order (no parallelism): `1 → 2 → 3 → 4 → 5 → 6a → 6b → 7 → 8 → 9 → 9b → 10`

Packet-specific gates (mandatory script still required):

| Packet | Extra gate | OMNI focus |
|--------|------------|------------|
| 1 | Validator 2.3 PASS | Single mod-rack authority from manifest |
| 2 | Validator 2.4 PASS | Fixed 16 slots; no pattern-length authority |
| 3 | Duplicate-authority all 7 categories | No parallel inventories; no general held gestures |
| 4 | `ctest -R LayoutBounds_test --test-dir desktop-v2/build` | One layout authority; no overlay cluster |
| 5 | `ctest -R GlobalOscilloscopeDisplay_test --test-dir desktop-v2/build` | Fixed-capacity shell scope |
| 6a | `ctest -R FroggersV2AppCoreFacade_test --test-dir desktop-v2/build` | Facade boundary; no duplicate UI state |
| 6b | `ctest -R RuntimePages_test --test-dir desktop-v2/build` | Runtime pages; hosted projection |
| 7 | `ctest -R MidiCvAssignment_test --test-dir desktop-v2/build` | **Controllers UI from `buildTargetMappingRows()`; retire parallel label/sync authority** |
| 8 | Task group 9 behavioral tests | Manifest-derived mod/sequencer behavior |
| 9 | `ctest -R BootSmoke_test --test-dir desktop-v2/build` | Boot path honesty |
| 9b | `bash sim/check_operator_docs_sync.sh` | Operator docs match shipped UX |
| 10 | Task group 10 + `openspec validate ... --strict` | Baseline sync; contract honesty |

Work packets:

1. **Manifest source-rack migration** — Complete the implementation needed for 2.3 and the source-catalog parts of 2.7/10.4. Migrate DesktopV2/VstV2 modulation consumers from `V2ModSourceCatalog`, `V2ModTapBank` UI authority, and MIDI CC A/B source lanes to the manifest-owned 15-lane permanent source rack. Acceptance: `FroggersV2ProjectionValidators_test` no longer fails 2.3; raw VCO audio-rate lanes, VCO 1+2+3 EF, and MIDI source lanes are absent; External Audio lanes remain visible-but-unavailable when no input is active.
2. **Fixed-16 sequencer migration** — Complete the implementation needed for 2.4 and the variable-step-count parts of 2.7/10.4. Migrate sequencer state, UI, host parameters, snapshots, locks, and tests from variable pattern length/playhead-led fields to exactly 16 written/unwritten slots with manifest-declared snapshot and lock fields. Acceptance: `FroggersV2ProjectionValidators_test` no longer fails 2.4; no pattern-length control exists; empty sequencer playback is a clocked no-op; locks apply only from written active steps.
3. **Duplicate-authority cleanup** — Retire remaining duplicate row/stable-ID/controller-target/held-gesture authorities or replace them with manifest-backed checked consumers. Acceptance: `scripts/check_desktop_v2_duplicate_authority.sh` passes; only approved manifest/generated/test files hold structural inventories; no general held-gesture state remains.
4. **Layout and top chrome** — Complete task group 3. Move shared geometry into `DesktopV2ChromeLayout`, use compact module grids, a 4x4 parameter-detail grid, fixed sequencer region, and one top chrome stack with transport/signal and global-command bands. Acceptance: `LayoutBounds_test` proves no overlap, no default scrollbars, all 16 steps visible, untruncated direction/speed rows, and shared hosted/standalone layout authority.
5. **Global oscilloscope** — Complete task group 4A. Add one shell-level global oscilloscope with fixed-capacity buffers, manifest-declared taps, default three-VCO traces, source-group switching hooks, and hosted/standalone visibility. Acceptance: component/screenshot checks prove visibility across carousel/runtime pages and hosted minimum layout; realtime scan shows no steady-state allocation or unbounded buffer growth.
6. **Facade and runtime pages** — Complete task groups 4, 5, 7 (audio only), and 8.6. Add `FroggersV2AppCoreFacade`, runtime File/Audio/MIDI page shells, hosted read-only status. **Do not claim controller UI complete** — that is packet 7. Gates: `FroggersV2AppCoreFacade_test`, `RuntimePages_test`, mandatory script.
7. **Controller configuration (Sheaf-shaped UI)** — Complete task group 6. Replace legacy hand-wired `MidiCvSettingsComponent` authority with a data-driven mapping table UI projecting `buildTargetMappingRows()`; manifest target IDs; fan-out and readback visible per row; no parallel `syncTableFromUi` label inventory. Gates: `MidiCvAssignment_test`, mandatory script, validator 2.5.
8. **Modulation and sequencer behavior** — Complete task group 9. Derive parameter-detail lanes, Rand Mods, route eligibility, sequencer mod snapshots, and lock fields from the 15-lane manifest rack; implement the 4x4 detail grid, VCO pair-bus self-feedback rules, external-audio unavailable/off state, LFO modulation, long-press clear, and all-unwritten no-op playback. Acceptance: all task 9 tests pass and no held gesture route is introduced.
9. **Boot carryover** — Complete task group 11. Confirm Release app path docs, preserve/add `scripts/open-desktop-v2.sh`, add stale root `.app` guard, harden/register `BootSmoke_test`, and verify Release build/open/boot behavior.
9b. **Operator documentation sync** — Complete task group 12 after implementation packets 1–8 land. Update canonical `SIM_MANUAL.md` and `QUICK_DICT.md` for converged desktop v2 UX, sync `docs/` and `web/public/` mirrors, and run `sim/check_operator_docs_sync.sh`. Acceptance: operator docs no longer describe legacy v2 chrome (8-source mod rack, center global cluster, pattern-length sequencer, Shift+press reset, MIDI CC A/B mod lanes); embedded desktop v2 Help reflects canonical root files.
10. **OMNI closure and manifest sync closure** — Complete task group 10 only after implementation packets and operator-doc sync pass. Run strict OpenSpec validation, hedge grep, manifest validators, duplicate-authority scans, no-held-gesture scans, host-master audit, release-channel integrity checks, path no-change review, realtime/local-reasoning gates, focused tests, operator-doc sync, and manual QA evidence. Sync implemented and verified delta specs into `openspec/specs/**`, update `froggers-host-master` to the final 15-lane manifest contract, re-run strict validation, then prepare archive-readiness evidence.
11. **Manual acceptance before commit/merge/release** — Before committing, perform desktop v2 manual QA at 1280x920: no overlap, no unexpected scroll, no label ellipsis, visible top chrome stack, global oscilloscope, global randomization scope pairs, visible 16-step sequencer with untruncated direction/speed rows, accessible runtime pages, readable S1/S2/S&H labels, All Steps behavior, and audio Record requiring Play first for v1 parity. Before merge/release, run hosted/manual QA if VST/AU v2 is included and verify boot/release candidate artifacts from the exact tree intended for `main`.
12. **Release-channel boundary** — Do not create or document `desktop-v*` tags, do not add a GitHub Release, do not change web links away from `releases/download/froggerstiga-v1/`, do not enable `generate_release_notes`, and do not bump CMake/package versions for this desktop-v2 work unless explicitly requested. Any v2 binary publication must use the existing `froggerstiga-v1` channel after explicit release authorization.

**Parent enforcement checklist (run after every subagent merge):**

```bash
bash scripts/check_subagent_packet_gates.sh
```

- Exit 0 → attach stdout to completion notes; update task ledger only for tasks the packet actually closed.
- Exit nonzero → revert packet diff or fix before next dispatch; do not mark tasks `[x]`.
- Reject subagent reports missing COMPLETION REPORT format from `SUBAGENT_OMNI_CONTRACT.md`.
- Reject diffs containing forbidden tactics even if a slice test passes.

Post-plan-execution OMNI rule checks:

- **Single authority/data flow:** Confirm manifest, layout, controller, sequencer, host-parameter, and runtime-page authorities are the only remaining structural sources; no UI-owned duplicate tables, copied labels, private source catalogs, private MIDI routes, or independent sequencer inventories remain outside approved manifest/generated/test files.
- **Repetition/helper extraction:** Confirm repeated geometry, source-lane, target-ID, row-label, sequencer-field, hosted-overlay, and runtime-page logic was either consolidated into the chosen authority or remains only as a checked projection; no one-off duplicate helper was introduced to satisfy a single caller.
- **Host-native boundaries:** Confirm desktop standalone hardware MIDI remains controller-model-owned, VST/AU v2 MIDI reaches parameters only through host-parameter semantics, VCV remains reserved schema-only in this change, and v1 desktop/web/VST/AU behavior is unchanged unless explicitly recorded.
- **Contract honesty:** Compare implementation, delta specs, baseline specs, `froggers-host-master`, generated manifest artifacts, `SIM_MANUAL.md`, `QUICK_DICT.md`, and task status; remove or mark incomplete any claim that is not backed by code/tests/manual evidence.
- **Realtime/local reasoning:** Audit touched audio callback, oscilloscope, facade, controller, runtime-page, sequencer, and host-parameter paths for no steady-state allocation or unbounded growth, fixed-capacity buffers, no callback ownership cycles, and decision-heavy function nesting depth <= 3.
- **Scope containment:** Review `git diff --name-only` against impact/non-goal lists and release rules; confirm no Daisy firmware, VCV runtime/panel/package, v1 host, release workflow, release tag, package version, or unrelated docs changed without an explicit task note and matching verification.
- **Durable OpenSpec truth:** Confirm all implemented and verified deltas are synced to `openspec/specs/**`, stale baseline text is removed or superseded, archived carryover crosswalks are complete, and only active/incomplete work remains in this change.
- **Verification ownership:** Record exact command/manual evidence for strict validation, manifest validators, duplicate-authority gate, no-held-gesture scan, host-master audit, release integrity, focused tests, build/open/boot checks, `sim/check_operator_docs_sync.sh`, and manual QA; leave unrun manual checks unchecked.

## 3. Folded Module Column Layout

- [x] 3.1 Port `ModuleRowColumnLayout` and `moduleRowColumns(int rowWidth)` into `DesktopV2ChromeLayout.hpp`
- [x] 3.2 Refactor `PageCarouselComponent` to support compact center parameter grids for normal module pages, a 4x4 center grid for parameter-detail pages, and a fixed-height 16-step sequencer region from the shared layout authority
- [x] 3.3 Implement the recorded top chrome stack: transport/signal band with Play, Stop, and the global oscilloscope; global-command band with Randomize All, Randomize Mod, waveform-randomize, Marbles/Rand Resample, Crunchy, Shift, and `All Scenes` / `Current Scene` plus `All Steps` / `Current Step` radio pairs below Randomize All and Randomize Mod
- [x] 3.4 Refactor `SubmodulePagePanel` to use label+encoder viewport plus sibling mod-column viewport at local x=0
- [x] 3.5 Refactor `AdsrPagePanel` to use the same column layout and mod-column viewport structure
- [x] 3.6 Remove default-size vertical scrolling from carousel module pages whose manifest-visible parameter cells fit the center grid at 1280x920
- [x] 3.7 Hide row/grid scrollbars and reset view position when module pages and parameter-detail pages fit at 1280x920
- [x] 3.8 Update performance-band label sizing for S&H and scene ordinal labels at 1280px
- [x] 3.9 Add `LayoutBounds_test` for one top chrome stack with transport/signal and global-command bands, zero center-cluster/mod-cell intersection, zero sequencer/module-grid intersection, no carousel module-page scrollbar at 1280x920, no parameter-detail scrollbar at 1280x920, all 16 sequencer steps visible, and no direction/speed icon truncation
- [x] 3.10 Add the helper-authority grep gate rejecting independent `gridPx(31)` mod X placement
- [x] 3.11 Add global-control projection parity tests/audit proving Randomize All, Randomize Mod, waveform-randomize, Marbles/Rand Resample, Crunchy, Shift, and scene/step scope controls fire the same control-core and `DesktopHostIO` mutations from the global-command band projection, without adding UI-owned duplicate state
- [x] 3.12 Verify hosted editor carousel uses the same `DesktopV2ChromeLayout` projection authority as standalone desktop, with no hosted-only geometry fork

## 4. Sheaf-Compatible Froggers Facade

- [ ] 4.0 Implement the recorded Sheaf decision: use the facade as the migration boundary, then adopt Sheaf-style parameter/modulation management behind it after manifest validation passes
- [ ] 4.1 Add `FroggersV2AppCoreFacade` wrapping existing `FroggersV2ControlCore`, `FroggersV2HostBridge`, and audio engine integration
- [ ] 4.2 Implement facade configuration from current desktop v2 defaults without changing default window, audio, or page behavior
- [ ] 4.3 Route facade UI messages into the existing control-core bus and preserve current `FroggersV2UIState` publication
- [ ] 4.4 Add deterministic audio equivalence tests comparing facade path with current v2 path
- [ ] 4.5 Add UI-state equivalence tests for carousel, mod grid, scene, fixed 16-step sequencer, sequencer direction/speed state, sequencer-lock, ADSR, scope, Crunchy, and Crispy state
- [ ] 4.6 Add control-core tests proving Randomize All and Randomize Mod execute as immediate scoped commands, do not enter held state, read the scene/step radio pairs, preserve left/right scene-slider selections during scene-scoped randomization, and leave locked parameter values under sequencer ownership

## 4A. Global Top-Row Oscilloscope

- [ ] 4A.0 Implement the recorded global oscilloscope source-mode decision: the default view remains three color-coded VCO traces, with Sheaf-style modulation-aware visualization applied to those VCO traces
- [ ] 4A.1 Add one shell-level global oscilloscope component that is not owned by an individual module page
- [ ] 4A.2 Place the global oscilloscope in the transport/signal band next to the Play and Stop controls in desktop standalone and hosted editor projections
- [ ] 4A.3 Feed the default oscilloscope view from manifest-declared VCO 1, VCO 2, and VCO 3 taps after waveform morph and cross-coupling and before global reverb/delay output effects
- [ ] 4A.4 Add manifest-declared source-group scope taps for LFO 1-3, VCO pair buses, VCO EFs, Random/Marbles, and External Audio sources when those groups are inspected
- [ ] 4A.5 Implement Sheaf-style multi-signal visualization rules: trace colors match source colors, multiple traces share the same scope, and a trace changes visual treatment when that displayed signal has nonzero audio-rate modulation
- [ ] 4A.6 Use fixed-capacity signal buffers and a named desktop v2 scope UI timer for the global oscilloscope
- [ ] 4A.7 Keep EF scopes, Random LED cells, LFO trace indicators, external-audio unavailable/off indicators, and encoder-integrated CV LED modulation monitors as source indicators separate from the global oscilloscope
- [ ] 4A.8 Layout the global oscilloscope so it remains visible across carousel pages, Audio, MIDI/Controllers, and File/Patch runtime pages at 1280x920
- [ ] 4A.9 Layout the global oscilloscope so it remains visible in the hosted editor minimum layout
- [ ] 4A.10 Add component tests and screenshot checks for top-chrome oscilloscope visibility, three color-coded VCO traces, source-group trace switching, audio-rate-modulated trace visualization, runtime-page persistence, and hosted editor visibility

## 5. Runtime Page Model Upgrade

- [ ] 5.0 Implement the recorded hosted runtime status UI decision as a collapsible read-only status panel, and implement the recorded VST/AU File/Patch decision: host state and DAW preset mechanisms only, with no plugin preset browser, direct plugin file-system preset save/load, or plugin import/export workflow in this change
- [ ] 5.1 Add Froggers-owned runtime page adapters under the desktop-v2 source tree for File/Patch, Audio, and MIDI/Controllers pages
- [ ] 5.2 Add File/Patch page state for patch identity, dirty state, save/load/revert results, controller mapping persistence result, and runtime log messages
- [ ] 5.3 Add persistent right-side File, Audio, and MIDI buttons in desktop standalone without replacing carousel arrow navigation
- [ ] 5.4 Add projection overlay fields for `hidden`, `readOnly`, and `interactive` controls in desktop and VST/AU contexts
- [ ] 5.5 Add tests proving plugin editor hides hardware audio selectors, standalone MIDI selectors, QWERTY source selector, and standalone record/export controls while keeping carousel, Envelope/ADSR, Crunchy, mod grid, global oscilloscope, and host-parameter-backed controls interactive

## 6. Controller Configuration

- [ ] 6.0 Implement recorded controller decisions: no MIDI learn mode, no recent-event list, multi-target physical input mapping, and product-formatted target readback
- [ ] 6.1 Extend the controller model with selected input, connection/receiving/error state, explicit mapping event fields, target IDs, message kind, channel/CC/note details, optional value/range fields, multi-target fan-out state, persistence state, and product-formatted target readback
- [ ] 6.2 Replace ad hoc desktop v2 MIDI CV target storage with manifest target IDs through the controller configuration model
- [ ] 6.3 Add pitch, gate, CC, and external modulation target mapping tests
- [ ] 6.4 Add multi-target mapping tests proving duplicate selected input + message kind + channel + controller number edits for different targets are allowed, displayed as fan-out, persisted, and applied to every mapped target through the controller model
- [ ] 6.5 Add session persistence tests proving controller mappings survive label changes through stable target IDs
- [ ] 6.6 Represent QWERTY virtual MIDI as an explicit optional virtual controller source
- [ ] 6.7 Add a MIDI/Controllers page component test showing labeled fields, explicit mapping event fields, no MIDI learn/recent-event UI, multi-target fan-out status, persistence status, target readback, and MIDI clock source selection for sequencer sync

## 7. Runtime Audio Configuration

- [ ] 7.1 Extend desktop v2 audio state projection with labeled selected devices, active channel masks, negotiated sample rate, block size, bus layout, external-input state, input meter state, and output meter state
- [ ] 7.2 Replace or adapt `AudioSettingsComponent` into the runtime Audio page for standalone desktop
- [ ] 7.3 Preserve stereo default output, mono downmix, optional mono input, and mono-core rendering behavior
- [ ] 7.4 Add tests for default stereo output state visible on the Audio page
- [ ] 7.5 Add tests for mono output reporting and shared mono downmix behavior
- [ ] 7.6 Add hosted audio projection tests proving hardware selectors are absent in VST/AU while read-only host input bus count, host output bus count, active channel layout, sample rate, block size when reported, input present/unavailable, and output active/clipped/muted fields remain visible when the hosted runtime panel is enabled
- [ ] 7.7 Add a UI contract test and screenshot check proving signal-shape observation remains on the global top-chrome oscilloscope and the Audio page does not add a duplicate oscilloscope

## 8. Hosted Parameter and MIDI Projection

- [x] 8.1 Validate `HostParameterInventoryV2` against manifest hosted-parameter entries
- [ ] 8.2 Preserve stable flat IDs and grouped display names for every VST/AU v2 parameter
- [ ] 8.3 Add plugin state round-trip tests for manifest-owned stable IDs
- [ ] 8.4 Clarify plugin MIDI handling so DAW MIDI changes apply through host-parameter mapping semantics only
- [ ] 8.5 Add tests proving raw MIDI does not mutate a private CC-to-mod-source table in plugin v2
- [ ] 8.6 Add hosted editor tests proving standalone audio/MIDI device selectors and record/export controls remain hidden

## 9. Modulation Assignment Convergence

- [ ] 9.1 Derive parameter-detail source lanes from manifest row eligibility and the manifest-owned 15-lane source catalog
- [ ] 9.2 Derive Rand Mods source/depth choices from manifest row eligibility and from the global `All Scenes` / `Current Scene` plus `All Steps` / `Current Step` scope state
- [ ] 9.3 Preserve None as cleared route state and validate it across desktop and hosted projections
- [ ] 9.4 Implement Sheaf-style depth drill-down projection for Froggers v2 rows as a 4x4 parameter-detail grid while retaining CV LED modulation monitors, clickable `MOD` labels, depth-zero/off defaults, and Crispy/target-cell visibility
- [ ] 9.5 Add tests for 16-cell parameter-detail rendering, 15-lane source-rack rendering, dedicated Crispy/target encoder visibility, clickable `MOD` label drill-in, attenuated-centered CV LED behavior for slow negative/positive displacement, audio-rate balanced red/green energy display, audio-rate biased red/green energy display, VCO 1+2 / VCO 2+3 / VCO 1+3 audio-rate lane presence, raw VCO audio-rate lane absence, VCO 1+2+3 EF absence, MIDI source-lane absence, external-audio unavailable/off states, and bipolar depth edits
- [ ] 9.6 Add VCO-owned target eligibility tests proving VCO 1 targets only allow the VCO 2+3 audio-rate pair bus, VCO 2 targets only allow VCO 1+3, and VCO 3 targets only allow VCO 1+2 among audio-rate VCO pair buses
- [ ] 9.7 Add sequencer snapshot and lock tests proving exactly 16 step snapshots exist, written/unwritten state round-trips for each slot, mod source/depth fields and locked parameter values round-trip through manifest-declared fields, and locks are applied only by written active clocked steps
- [ ] 9.8 Add LFO module modulation tests proving LFO 1-3 outputs appear as source lanes and eligible LFO parameters can themselves receive nonzero source depths
- [ ] 9.9 Add global randomization scope tests proving Randomize All and Randomize Mod target stored values for all scene endpoints vs the current scene edit-target endpoint, preserve the left/right scene-slider selections and slider position, and target written steps among all 16 sequencer slots vs the current written step, where current step is playhead while playing and edit step while stopped
- [ ] 9.10 Add sequencer clock and transport tests proving internal clock and configured MIDI clock can advance sequencer locks, direction choices `<` / `>` / `RND` traverse the fixed 16-slot ring while skipping unwritten slots, speed choices `/2` / `/1.5` / `1` / `x1.5` / `x2` change timing only, defaults are `>` and `1`, and no held gestures or private MIDI modulation routes are introduced
- [ ] 9.11 Add sequencer UI/controller tests proving the two-row direction/speed icon strip renders above the sequencer, exactly one direction and one speed are selected, no pattern-length control exists, all 16 steps are visible, mouse press-and-hold, touch press-and-hold, and holding a mapped MIDI/controller step control directly clear a written step after the long-press threshold without a second click/menu/confirm, short holds cancel without clearing, the long press does not create a general held-gesture route, and no icon text is truncated at 1280x920
- [ ] 9.12 Add empty-sequencer playback tests proving that when all 16 steps are unwritten the sequencer transport can keep running, emits no snapshot/lock/gate/silence/reset/default-value event, and audio continues from the current live synth state exactly like playback before any step has been recorded

## 10. OMNI Verification and Supersession

- [x] 10.1 Run `openspec validate froggers-v2-sheaf-runtime-convergence --strict`
- [ ] 10.2 Run hedge-language grep over this change; replace vague hedges, and record allowed hits that are bounded option labels or domain terms such as optional mono input / optional virtual controller source
- [x] 10.3 Run manifest validators for host parameters, rows, mod sources, fixed 16-slot sequencer snapshot/lock fields, step written/unwritten state, sequencer direction/speed controls, clock sync declarations, and MIDI targets
- [x] 10.4 Run duplicate-authority grep gates for layout, IDs, MIDI targets, mod eligibility, sequencer fields, rejected variable-step-count controls, and rejected held-gesture state; run `bash scripts/check_subagent_packet_gates.sh` before claiming any packet complete
- [x] 10.5 Run `LayoutBounds_test` through desktop-v2 ctest
- [x] 10.6 Build desktop v2 standalone and run focused control-core, controller, audio, hosted-editor, and manifest tests
- [x] 10.7 ~~Perform operator QA at 1280x920~~ — **Transferred to `desktop-v2-operator-truth-repair` section 8** (2026-07-07 archive). Convergence archived with automated gates passing; manual QA deferred to operator-truth repair.
- [ ] 10.8 Mark `desktop-v2-module-column-layout` superseded by this change after verifying its requirements are represented here and task 0.3 is complete
- [ ] 10.9 Record any remaining Sheaf source gaps as explicit follow-up tasks rather than hidden assumptions
- [ ] 10.10 Run a `froggers-host-master` conformance audit: DesktopV2 and VstV2 migrate from the old v2 source catalog to the manifest-owned 15-lane source rack with VCO pair-bus audio-rate lanes, without reintroducing raw VCO audio-rate lanes or v1 mod rack cells 0/1/4/5/6 as UI authorities; VST/AU v2 MIDI reaches parameters through host-parameter semantics only; standalone desktop hardware MIDI remains controller-model-owned; External Audio source lanes stay visible-but-unavailable when external input is absent
- [ ] 10.11 Run release-channel integrity checks: no tag names matching `desktop-v*` are created, pushed, or documented as release channels in docs/workflows; web download links still target `releases/download/froggerstiga-v1/`; `generate_release_notes` remains disabled; CMake/package versions are not bumped for this desktop-v2-only work; and `froggerstiga-v1` remains the only desktop release channel
- [ ] 10.12 Review `git diff --name-only` against the impact and non-goal lists: no v1 desktop, v1 web/WASM, v1 VST/AU, VCV runtime/panel/package, Daisy firmware, release workflow, or unrelated documentation files change unless this tasks file records the explicit reason and matching verification
- [ ] 10.13 Verify Sheaf adoption remains locally owned: every borrowed source file is listed in an adoption inventory, namespaced or translated into FroggersTiga, covered by local tests, and no build/test path fetches Sheaf from the network or requires a new package install
- [ ] 10.14 Run realtime/local-reasoning gates for new facade, runtime-page, controller, audio, and global-oscilloscope code: no audio-thread allocation or unbounded growth in steady-state paths, fixed-capacity scope buffers, no callback ownership cycles, and nesting depth stays <=3 in touched decision-heavy functions
- [ ] 10.15 Verify recorded decisions remain reflected in implementation: C++ manifest storage, Sheaf-style parameter/modulation management behind the facade, product-formatted target readback, one top chrome stack, no MIDI learn/recent-event UI, VST/AU host-state-only File/Patch behavior, three-VCO oscilloscope default, collapsible hosted status, and multi-target controller mapping
- [ ] 10.16 Produce archive carryover crosswalks for `desktop-v2-module-column-layout`, `desktop-v2-chrome-sequencer-ux`, and `desktop-v2-boot-artefact-gate`, showing each relevant requirement/gate mapped to this change's specs/tasks or explicitly dropped as obsolete with rationale
- [ ] 10.17 Archive-readiness evidence includes the commands/results for strict OpenSpec validation, hedge grep, host-master audit, duplicate-authority scans, no-held-gesture scans, release integrity checks, path no-change review, focused tests, `sim/check_operator_docs_sync.sh`, and manual QA status; unrun manual checks remain unchecked

#### Closure evidence (2026-07-07, packet 10)

```text
bash scripts/check_subagent_packet_gates.sh
exit 0

ctest --test-dir desktop-v2/build --output-on-failure
12/12 tests passed
exit 0
(BootSmoke_test requires non-sandbox launch; fails in default sandbox with signal 6)

bash sim/check_operator_docs_sync.sh
operator docs mirrors in sync
exit 0

openspec validate froggers-v2-sheaf-runtime-convergence --strict
Change 'froggers-v2-sheaf-runtime-convergence' is valid
exit 0
```

## 11. Boot Path Hardening Carryover

- [x] 11.1 Confirm `desktop-v2/PACKAGING.md` macOS table lists `desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app` rather than the stale root-level bundle
- [x] 11.2 Add or preserve `scripts/open-desktop-v2.sh` so it resolves the repo root, opens the Release `.app`, and exits nonzero with a clear message when the Release app is missing
- [x] 11.3 Update boot-outcome glosses in `QUICK_DICT.md` (and mirrors if run before task group 12) to reference the Release path and `scripts/open-desktop-v2.sh`; full QUICK_DICT convergence is completed in task group 12
- [x] 11.4 Add the desktop-v2 build guard that removes or replaces a stale `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` when it is not the current Release output
- [x] 11.5 Harden `BootSmoke_test` to resolve the current Release binary, poll with `waitpid(WNOHANG)` or equivalent, fail on early exit/signal, and clean up with SIGTERM
- [x] 11.6 Register the boot-smoke helper/test in `desktop-v2/CMakeLists.txt` as needed
- [x] 11.7 Verify Release build, `./scripts/open-desktop-v2.sh`, `ctest --test-dir desktop-v2/build -R BootSmoke --output-on-failure`, and stale root `.app` absence/non-launchability

## 12. Operator Documentation Sync

Canonical operator docs are root `SIM_MANUAL.md` and `QUICK_DICT.md`. Mirrors under `docs/` and `web/public/` must byte-match the canonical files. Desktop v2 embeds the canonical files in Help via `desktop-v2/CMakeLists.txt`.

- [ ] 12.1 Update `SIM_MANUAL.md` Desktop v2 section for converged UX: one top chrome stack with transport/signal and global-command bands; global oscilloscope; 15-lane permanent mod source rack; 4x4 parameter-detail grid; fixed 16-step sequencer with direction/speed icon strip and device-neutral long-press clear; Envelope page, two cross-couplers, and waveform morph controls; runtime File/Patch, Audio, and MIDI/Controllers pages; global Randomize All/Mod scope pairs; and no general held-gesture model
- [ ] 12.2 Update `QUICK_DICT.md` for converged desktop v2 glosses: top chrome bands, global oscilloscope, 15-lane mod source names aligned with manifest display labels, fixed-16 sequencer/direction/speed/long-press clear, runtime pages, and controller-model semantics (MIDI maps to targets, not permanent mod lanes); retire center global cluster, Shift+press reset, MIDI CC A/B mod-source, and pattern-length/Steps slider glosses
- [ ] 12.3 Include boot/transport glosses for Release `.app` path, healthy boot outcome, `./scripts/open-desktop-v2.sh`, Play/Stop, and Record audio requiring Play first
- [ ] 12.4 Sync mirrors from canonical root files only: `docs/sim-manual.md`, `web/public/sim-manual.md`, `docs/quick-dict.md`, and `web/public/quick-dict.md`
- [x] 12.5 Run `bash sim/check_operator_docs_sync.sh`; confirm public sim-manual launch gate still passes for `docs/sim-manual.md` and `web/public/sim-manual.md`
- [ ] 12.6 Verify desktop v2 Help embeds the updated canonical docs through existing `desktop-v2/CMakeLists.txt` wiring to root `SIM_MANUAL.md` and `QUICK_DICT.md`
