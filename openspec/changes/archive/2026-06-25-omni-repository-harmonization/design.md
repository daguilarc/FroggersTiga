## Context

The audit covers governing instructions and the desktop, web/WASM, VCV, and VST/AU product surfaces, including their `sim/` support code and shared `src/core/` dependencies. It covers host release workflows/scripts/docs, host build paths, in-scope active/baseline OpenSpec artifacts, and the host executable verification suite.

The original Daisy Field firmware application is explicitly excluded: `src/FroggersTiga/`, hardware-support/demo applications under `src/`, `src/common/`, `src/mk/`, `External/libDaisy/`, firmware build outputs, and `MANUAL.md` are not audited or changed. Shared `src/core/` files remain in scope only where desktop/web/VCV/VST consume them; the change does not claim firmware verification or intentionally alter firmware behavior.

Current verification is healthy: six sim tests, fifteen Playwright tests, the production web build, label/doc parity, WASM exports, VCV SVG/bounds, and the VCV license-boundary check pass. The gaps are contracts those checks do not cover:

- release admission is wildcard-based even though only one movable channel is allowed;
- the desktop app reports a literal old version and private web metadata differs from the shared release version;
- C++ labels are copied into TypeScript, page names are copied twice more, and mod-rack index/kind/capacity tables are separately hardcoded in desktop, WASM, and two TypeScript files;
- the archived `web-midi-mod-rack` baseline incorrectly requires the desktop-style MIDI CC 2 input in the browser, while the intended web UI has four entries and supports only MIDI CC 1;
- VCV directly owns `midi::InputQueue`, `midi::Output`, two CC switches, two bridge latches, and Rack MIDI widgets even though its per-parameter CV jacks are the correct host-native boundary;
- those VCV per-parameter jacks currently quantize voltage into internal mod indices 4–6 instead of applying the incoming voltage to the connected parameter, so a separate MIDI-to-CV module cannot provide ordinary CV modulation semantics;
- VST/AU accepts DAW MIDI but funnels CC through two fixed bridge pairs, registers no complete stable host-parameter surface, and therefore cannot use ordinary DAW MIDI-to-parameter mapping at scale;
- `AudioPairArState` reimplements modulation and bypasses external-source presence gating;
- Web/WASM calls `malloc/free` every render quantum and for scope copies, JUCE buffers can resize in callbacks, and recording grows a vector on the audio thread;
- generated `sim/build/` products are tracked even though they are host test output;
- `.gitignore` excludes the entire `openspec/` tree, so baseline truth, active proposals, and archive history do not survive a clean clone;
- OpenSpec contains contradictory active artifacts, generated placeholder purposes, duplicate ownership, and fifteen non-omni active changes representing already-applied, superseded, or abandoned work.

The root `AGENTS.md` release policy is a hard constraint. In particular, this change does not create a tag, publish a release, add a release channel, or bump a package version.

## Goals / Non-Goals

**Goals:**

- Make single-source claims mechanically true rather than merely checked after duplication.
- Make release admission exact while preserving the mandated workflow trigger glob.
- Eliminate repository-controlled native/WASM allocation and dynamic buffer growth from steady-state realtime processing, and bound Web Audio telemetry work.
- Restore a host-source-only Git index with explicit Pages and firmware-scope exceptions.
- Put canonical in-scope host OpenSpec configuration/spec/change/history artifacts under version control while excluding only documented ephemeral state.
- Make active OpenSpec artifacts internally implementable and archive-ready state unambiguous.
- Make VCV a MIDI-free Rack citizen whose modulation boundary is voltage at its native CV jacks.
- Make VST/AU parameter routing DAW-native and free of the two-fixed-CC-pair limit.
- End with `omni-repository-harmonization` as the only active change.
- Add focused gates for every harmonized contract.

**Non-Goals:**

- Implement stale `vcv-rack-field-parity`, `vst-plugin-host-ux`, or VCO-AR plans as written; only the explicitly absorbed host fixes in this change survive.
- Perform pending physical iPhone, Rack, or DAW manual verification.
- Audit or modify the original Daisy Field firmware application, hardware-support/demo apps, firmware build system/output, firmware operator manual, or libDaisy integration.
- Treat a shared `src/core/` host fix as evidence that the Daisy firmware build or behavior was verified.
- Publish or delete remote Git refs/releases without separate user authorization.
- Eliminate browser-engine structured-clone/GC behavior, which is outside repository control; the enforceable web contract covers Froggers-owned WASM buffers/calls and bounded telemetry cadence.
- Change DSP sound, control ranges, unrelated UI layout, release version, or the root-authority plus two tracked published-help mirrors.
- Refactor vendored `.emsdk/` or `Rack-SDK/` code.

## OMNI Rule Audit

| Rule | Finding | Resolution in this change |
|------|---------|---------------------------|
| Single authority / data flow | Labels, page counts, host-specific mod topology, indicator kinds, and scope capacity are copied across C++ and TypeScript; the copies already disagree | Generate web metadata from regular shared C++ tables and make all host consumers use host projections from one specification |
| Repetition | `PAGE_NAMES`, `CORE_PAGE_NAMES`, `HOST_PAGE_NAMES`, `MOD_BAY_SPEC`, `SCOPE_MOD_INDICES`, literal counts, and repeated desktop box arrays encode the same concepts | Remove TypeScript authorities; use table-driven consumers and generated/runtime metadata |
| Contract honesty | The archived web baseline says five mod entries and the active gating delta says CC 2 can be enabled, but the intended browser product has one MIDI CC input | Correct the web baseline and gating delta to four cells with CC 1 only before archiving `midi-cc2-default-off` |
| Host responsibility | VCV implements MIDI device I/O and CC conversion that Rack already delegates to dedicated modules; its parameter jacks select internal sources instead of consuming CV | Remove Froggers-owned VCV MIDI and make each parameter jack apply normalized voltage directly to its target |
| Host-native integration | VST receives DAW MIDI through two private CC pairs and has no complete automatable parameter registry | Register stable host parameters and let the DAW own arbitrary channel/CC mapping |
| Defensive code | `ModMgr::Modulate` can index before validating None/negative/out-of-range inputs; render buffers grow on callbacks | Validate indices first and prepare bounded storage before callbacks |
| Realtime efficiency | The worklet and WASM perform explicit heap churn; recorder/JUCE vectors grow on the audio thread | Ban repository-controlled native/WASM allocation and growth in callbacks; allow only bounded, decimated browser telemetry |
| One-time helper extraction | Label generation, metadata verification, artifact hygiene, and OpenSpec hygiene each enforce multiple repeated contracts | Keep these focused scripts; do not introduce wrappers for one-off call sites |
| Nesting / local reasoning | A recorder worker and multi-host buffer preparation can become cross-cutting | Keep one producer/one consumer, explicit lifecycle states, and concern-local prepare APIs |
| Accumulate then apply | Index cleanup and archival can destroy or rewrite developer state if mixed with discovery | Complete artifact reconciliation first; use index-only cleanup and validate/review each archive delta |
| Durable source of truth | The complete OpenSpec planning home is ignored, so “baseline” and “archive” are local machine state | Track canonical OpenSpec artifacts and narrow ignores to named ephemeral files only |
| Scope containment | Daisy Field firmware, remote refs/releases, and physical device/DAW/Rack checks require different authority or hardware | Exclude firmware explicitly; retain host manual gates; do not silently mark excluded or unrun work complete |
| Verification ownership | “Zero allocations” cannot include browser internals, and duplicate/stale active capabilities obscure who closes a gate | Test owned calls/capacity changes, state the Web exception, and require no non-omni active changes after the closure sweep |

## Decisions

### D1 — Keep the required wildcard trigger; gate exact channel admission

`.github/workflows/desktop-release.yml` keeps `push.tags: ['froggerstiga-v*']` as required by repository policy. A first-class exact-channel condition gates all jobs, and `verify-tag-version.sh` rejects every value except `froggerstiga-v1`. The manual `workflow_dispatch` release path is removed because it is not the documented movable-tag flow and can run from a non-channel ref.

The existing script name may remain to minimize churn, but its output and packaging documentation will describe channel validation, not semver/tag matching. Release notes remain `SIM_MANUAL.md`-derived and `generate_release_notes` remains disabled.

**Alternative rejected:** change the workflow trigger to one exact literal. That is simpler technically but contradicts the checked-in instruction that the workflow trigger remains `froggerstiga-v*`.

### D2 — CMake is package-version authority; no version increment

`project(FroggersTigaDesktop VERSION ...)` remains the editable package version. `Main.cpp` returns `JUCE_APPLICATION_VERSION_STRING`. A metadata verifier compares the CMake version with the private web package/lock root version and the current release headings in README/SIM_MANUAL; historical changelog entries are excluded.

The apply phase aligns stale metadata to the already-declared current version but does not change the CMake version itself.

**Alternative rejected:** treat npm's private package version as independent. Its current visible drift has no useful semantic meaning and undermines the shared-release claim.

### D3 — Generate web display metadata and share mod-rack topology

A generator parses the deliberately regular display/layout tables in `ParamDisplayNames.hpp` and `HostPanelLayout.hpp` and emits `web/src/hostDisplay.generated.ts` containing:

- host page count/names;
- host row labels;
- pair-AR labels;
- global-strip labels;
- the ordered host-aware mod-rack specification: desktop standalone uses indices 0, 1, 4, 5, 6; web uses 0, 4, 5, 6; VST/AU and VCV use only internal sources 4, 5, 6;
- the scope sample capacity used by the web/WASM display protocol.

`main.ts`, `froggers-processor.ts`, desktop mod-rack iteration, VST hosted projection, VCV panel generation, WASM scope collection, and test constants consume the appropriate host projection from the shared specification. Mod-source names and enable availability continue to arrive from WASM where runtime state matters. Web renders four cells—MIDI CC 1, VCO Envelope, Random 1, and Random 2. The MIDI CC 1 cell remains visible but unavailable while External MIDI is Off. Desktop standalone retains two CC mod cells. VST/AU and VCV render no CC mod cells because MIDI maps to host parameters in the DAW and CV enters VCV through parameter jacks, respectively.

The generator supports `--check`; web build/e2e and Pages preflight run it. The current handwritten `paramDisplayNames.ts`, duplicated page arrays, `MOD_BAY_SPEC`, `SCOPE_MOD_INDICES`, and `SCOPE_SIZE` authorities are removed. Static HTML buttons may contain accessible fallback text, but startup code overwrites/checks them from the generated action table and preflight verifies any fallback.

**Alternative rejected:** runtime-only WASM labels. Labels must be useful before Play/audio-worklet initialization, so a generated build artifact is still needed.

### D4 — Pair-AR receives ModMgr, not a raw mod array

`AudioPairArState::beginBlock` and effective getters accept `const ModMgr*`. Its private blend delegates to `ModMgr::Modulate`. `ModMgr::Modulate` gains defensive index/None bounds handling before availability or array access. Existing host call sites pass the same manager already used by page and Delay paths.

Tests cover disabled/enabled external indices, internal sources, None, and out-of-range values. The delta extends the existing `mod-blend-semantics` capability instead of creating a second blend authority. Before this change is archived, `mod-blend-semantics-docs` must be validated and archived (including its pending web smoke) so this change layers new pair-AR requirements onto one baseline capability.

### D5 — Prepare bounded storage outside render callbacks

Web/WASM:

- `WasmSimHost` owns a bounded mono scratch buffer and processes larger requests in chunks.
- A binding reports the supported chunk size.
- The worklet allocates input, left, right, and scope WASM regions once after instantiation and reuses them; typed-array views are refreshed only if memory growth changes the backing buffer.
- Render calls larger than one chunk loop over the persistent region. No Froggers-owned WASM `malloc/free`, typed-array backing allocation, or growable collection occurs in the per-quantum DSP portion or scope copy.
- `postScreen` remains decimated to one of twenty quanta and bounded by fixed row/mod/scope counts. Its message object and browser structured clone are an explicit host-runtime exception; verification does not make an untestable claim about browser GC.

JUCE standalone/VST:

- `AudioEngine::prepareRenderBuffers(maxExpectedBlockSize)` sizes input/mono buffers off-thread.
- standalone calls it from `audioDeviceAboutToStart`; VST passes `samplesPerBlock` from `prepareToPlay`.
- an unexpectedly larger callback is processed in bounded chunks using prepared capacity, never resized in place.

Recording:

- the audio thread writes interleaved samples into a fixed SPSC chunk pool;
- a worker owns the growable capture vector or output stream;
- start allocates/starts, stop drains/joins, and overflow sets the existing truncation state without waiting.

VCV already processes scalar samples without allocation; a regression check preserves that property.

**Alternative rejected:** reserve the entire thirty-minute recording buffer. At the maximum configured rate, stereo float storage would require roughly 691 MB up front.

### D6 — Restore the index contract without deleting local outputs

The blanket `openspec/` ignore is removed. Canonical configuration, baseline specs, active changes, per-change schema metadata, and archive history for the in-scope host capabilities are added as source. Only specifically documented OpenSpec cache/session output may remain ignored; a catch-all planning-tree ignore is prohibited. This indexing change does not authorize edits to firmware-specific artifacts if any are introduced later.

`.gitignore` also gains/retains targeted patterns for host outputs such as `sim/build/`, `desktop/build/`, `desktop/dist/`, `wasm/build/`, `web/dist/`, and VCV build/package output. Apply uses index-only removal for the tracked `sim/build/` tree so local outputs are not destroyed. The hygiene script examines only the in-scope host/planning paths and rejects prohibited generated patterns, with explicit exclusions for the Daisy firmware surface, vendored content, and intentional `docs/` publication output.

The script is added to Pages preflight and documented for local use. Existing user modifications in generated trees are not treated as source changes and are not overwritten by this proposal phase.

### D7 — VCV is CV-only and owns no MIDI boundary

Froggers Tiga Rack removes `midi::InputQueue`, `midi::Output`, `MidiButton` widgets, CC enable controls/lights, CC bridge calls, CC mod-rack outputs, and MIDI labels/state. The primary module gains a serialized module-state schema marker and a version-aware parameter remapper that runs before ordinary Rack parameter assignment. A legacy patch without the marker is treated as schema v1: old CC parameter IDs 1 and 2 are discarded, old voicing-knob IDs `n >= 3` map explicitly to new ID `n - 2`, and ID 0 remains the Random control. Newly saved patches carry schema v2 and are never remapped again. A fixture verifies the complete old-to-new mapping, including boundary IDs. Its mod rack contains only VCO Envelope, Random 1, and Random 2 (indices 4, 5, 6). A host-source policy makes indices 0 and 1 unavailable to VCV assignment and randomization without forking the shared DSP enum.

Each VCV parameter input is real CV, not a source-selection bus. DSP first evaluates the persisted internal route through the shared modulation authority: `internalEffective = ModMgr::Modulate(base, modIndex, depth)`. If a per-parameter jack is connected, the final normalized control is `clamp(internalEffective + voltage / 10, 0, 1)`; if disconnected, it is `internalEffective`. The base knob and stored `modIndex`/depth remain unchanged. A transient per-target CV contribution is supplied to DSP without writing persisted knob or route state. Thus an internal source such as VCO Envelope can continue modulating the target while an external Rack MIDI-to-CV module, sequencer, or LFO adds its own control signal to the same target.

**Alternative rejected:** retain hidden/disabled `CvMidiBridge` state in the VCV UI. Hidden MIDI ownership still leaks into presets, randomization pools, and topology, and gives no benefit over Rack-native CV patching.

### D8 — VST/AU exposes parameters; the DAW owns MIDI mapping

A regular `HostParameterId` registry becomes the authority for every persistent continuous plugin control: page and Delay knobs/depths, pair-AR knobs/depths, and continuous morph controls. A separate compile-time inventory enumerates the semantic control axes, stable ID, range, and legacy/new-state default for every entry; completeness tests compare the registry against that inventory rather than allowing the registry to validate itself. JUCE `AudioProcessorParameter` objects and UI attachments are generated/constructed from the registry; UI edits, DAW automation, state restore, and DSP mutations converge on one bounded apply path.

Hosted mode removes the two `CvMidiBridge` CC pairs, MIDI Settings dialog, and CC mod-rack cells. Because DAW MIDI learn produces host-parameter automation rather than raw MIDI delivery to the plug-in, the hosted targets set `NEEDS_MIDI_INPUT FALSE`, return false from `acceptsMidi()`, and do not iterate `MidiBuffer`; desktop standalone keeps its existing two hardware CC-to-mod pairs. The DAW may map any of MIDI's 16 channels and 128 CC numbers to any exposed parameter; Froggers imposes no two-source limit and does not maintain a second private MIDI-learn table. VST/AU's mod rack contains only internal sources 4, 5, and 6.

The plugin state envelope stores stable parameter IDs/values and sim routing state, accepts existing v1/v2 snapshot bytes, and ignores unknown future IDs defensively. Legacy snapshots use the defaults declared by the independent parameter inventory for fields absent from v1/v2. Standard JUCE parameter notifications do not expose portable sample offsets to `AudioProcessor`; pending values are coalesced by stable ID and applied in deterministic registry order at the next render-block boundary through fixed-capacity, allocation-free storage. Continuous targets then feed the existing per-sample DSP smoothers so block-boundary target changes do not become discontinuous control jumps. The change makes no sample-accurate automation claim. UI/state mutations that must take effect while transport is stopped use the same value authority through a non-audio-thread drain.

The plug-in remains one instrument/generator identity (`IS_SYNTH TRUE`) for VST3 and AU rather than creating a second effect product in this harmonization. Its mono input bus is optional and host-dependent: processing must work with zero input channels, and documentation must not promise external audio in hosts that do not expose an input for an instrument. The VST3 category and AU main type are asserted from generated bundle metadata so an accidental category change fails verification.

Verification is layered rather than treating one DAW smoke test as conformance:

1. Processor tests compare the independent parameter inventory with the registry, exercise v1/v2/current state, unknown IDs, coalescing, smoothing, zero/variable/oversized blocks, supported sample rates and bus layouts, deterministic render equivalence, and realtime allocation/lock gates.
2. Bundle tests run Steinberg's VST3 validator, Tracktion `pluginval` at strictness 5 on the required local gate (strictness 10 as an extended/nightly gate), and Apple's `auval` for AU.
3. Host tests use the VST3 in a VST3 host such as REAPER and the AU in Logic Pro. They cover three-or-more DAW MIDI-learn mappings, automation record/playback, transport-stopped UI mutations, project/state recall, editor reopen/resize, zero-input rendering, and optional mono input where the host exposes it. Logic is never described as a VST3 host.
4. Cross-format parity loads equivalent state into VST3 and AU and compares exposed parameters, routing state, and deterministic rendered audio within a declared tolerance.

The valid non-MIDI findings from `vst-plugin-host-ux` are absorbed here: hosted mutations drain while transport is stopped, the patch overlay resyncs after bulk route/state changes, Record/Export and QWERTY capture are hidden/disabled when hosted, editor minimum size preserves the mod surface, and state recall notifies the derived overlay view.

**Alternatives rejected:**

- A plugin-private 16×128 MIDI mapping table duplicates DAW routing and persistence, creates a second parameter authority, and is less portable than standard VST/AU automation parameters.
- A VST3-only bridge to Steinberg `IParameterChanges` would recover sample offsets by leaving JUCE's portable `AudioProcessor` boundary, require a separate AU implementation, increase wrapper-upgrade risk, and make the two shipped formats behave differently. Block-boundary targets plus per-sample smoothing are the required contract unless later measured audio evidence justifies a separately proposed format-specific path.

### D9 — Close every non-omni active plan with an explicit disposition

The apply phase leaves `omni-repository-harmonization` as the only active change. Archival is not used to claim missing manual verification: each archived plan records whether it was merged as code-backed baseline truth or closed as stale/superseded.

- **Sync durable deltas, then archive:** `audio-pair-ad-controls`, `ios-external-audio-routing`, `mod-blend-semantics-docs`, `pair-ar-modulated-knob-display`, `pair-ar-randomize-parity`, `pair-ar-vcv-time-range`, `pair-ar-vertical-labels`, `web-mobile-global-strip-placement`, and `web-mobile-knob-labels`.
- **Reconcile, then archive:** `web-mobile-e2e-testing` keeps the implemented Playwright contract and manual physical-device caveat; its unimplemented Appium plan is removed before baseline sync.
- **Archive with `--skip-specs` as superseded/stale:** `midi-cc2-default-off`, `vcv-panel-silkscreen-fix`, `vcv-rack-field-parity`, `vcv-vco-ar-left-expander`, and `vst-plugin-host-ux`. Their surviving requirements are represented directly by this change; their obsolete deltas must not mutate baseline specs.

Before each normal archive, source/tests are compared with the delta and any unsupported claim is removed or explicitly labeled unverified. Before each `--skip-specs` archive, a supersession note points to this change. Replace all generated baseline Purpose placeholders as part of the sweep.

A local hygiene script runs strict validation for active changes and baseline specs governing desktop, web/WASM, VCV, VST/AU, and their shared sim/core contracts. It rejects placeholder purposes, unchecked struck-through tasks, unresolved duplicate ownership, and any active change other than this one. Firmware-only artifacts remain outside semantic pass/fail results.

## Risks / Trade-offs

- **[Risk] Generated TS parser is brittle against C++ formatting** → Keep the C++ tables deliberately regular, fail loudly, and test generation fixtures/shape counts.
- **[Risk] Host-specific topology drifts into duplicated tables** → Keep one authoritative table with explicit host inclusion metadata and test the desktop five-cell, web four-cell, and VST/VCV three-cell projections.
- **[Risk] VCV CV semantics change existing patches** → Treat removal of MIDI/state and source-selector voltage behavior as breaking, document the additive ±10 V migration, and add patch-level regression fixtures.
- **[Risk] Removing VCV CC parameters shifts Rack patch IDs** → Run an explicit schema-v1-to-v2 pre-load remap (`1/2` dropped, `n >= 3` mapped to `n - 2`), mark new saves as v2, and load an old fixture proving every later knob retains its value.
- **[Risk] Direct VCV CV destroys or replaces internal modulation** → Evaluate the stored route exactly once through `ModMgr::Modulate`, add the normalized jack voltage afterward, clamp once, and test simultaneous internal-plus-external modulation as well as disconnection.
- **[Risk] DAW parameter IDs break automation/session recall** → Freeze stable IDs, test legacy v1/v2 state reads, and never derive IDs from display labels.
- **[Risk] Registry self-tests omit a persistent control** → Compare it against an independent semantic inventory with declared ranges/defaults and fail on missing, duplicate, or extra IDs.
- **[Risk] Block-boundary automation produces zipper noise** → Apply targets only at the boundary but transition continuous DSP values through existing per-sample smoothing; test abrupt automation at multiple block sizes.
- **[Risk] Raw MIDI remains advertised after private CC ingest is removed** → Disable JUCE MIDI-input metadata, return false from `acceptsMidi`, and assert that hosted processing no longer consumes `MidiBuffer`.
- **[Risk] Instrument hosts omit the optional mono input** → Keep zero-input rendering valid, validate generated VST3/AU categories and bus layouts, test input where exposed, and document the host-dependent limitation rather than creating a second effect target here.
- **[Risk] A single validator gives false confidence** → Require Steinberg validator, `pluginval`, `auval`, format-correct DAW smokes, and VST3/AU state/render parity.
- **[Risk] Exact release gating silently skips an accidental tag** → Emit a clear skipped/admission result in the workflow and keep the local verifier documented.
- **[Risk] Chunked realtime processing changes block-boundary behavior** → Run equivalence tests comparing one large block with the same samples split into prepared-size chunks.
- **[Risk] Recorder worker introduces lifecycle races** → Use one producer/one consumer, explicit drain state, and stop/join tests including overflow.
- **[Risk] Browser telemetry still allocates internally** → Keep cadence and payload sizes fixed; verify no owned WASM allocation/growth and avoid claiming control over structured clone or GC.
- **[Risk] Index-only host-artifact cleanup leaves large local files** → Intentional; developers can clean locally, while no host output is destructively removed by harmonization.
- **[Risk] Broad hygiene patterns touch firmware** → Scope checks must reject proposed edits under the excluded Daisy paths; shared `src/core/` changes require host-facing justification and host verification only.
- **[Risk] First OpenSpec source commit is large** → Review it as a planning-only index addition, exclude ephemeral state explicitly, and require subsequent lifecycle changes to be ordinary reviewable diffs.
- **[Risk] Archiving stale changes mutates baseline specs** → Use normal archive only for reconciled code-backed deltas; use `--skip-specs` for superseded plans and review every baseline diff.

## Migration Plan

1. Record the disposition of all fifteen non-omni active changes and reconcile only code-backed deltas.
2. Add release/metadata/display generators and host projections; encode web CC 1-only plus VST/VCV internal-source-only racks.
3. Remove VCV MIDI ownership and correct direct per-parameter CV behavior.
4. Add the stable VST/AU parameter registry/attachments and hosted UX fixes; migrate legacy plugin state.
5. Refactor shared modulation and make owned Web/WASM, JUCE/recorder, and VCV realtime paths allocation-free.
6. Track canonical host OpenSpec source, add host-scoped ignore/hygiene rules, and remove tracked host products from the index only.
7. Sync and archive code-backed changes; archive superseded plans with `--skip-specs`; assert this is the only active change.
8. Run the full verification matrix and inspect `git status` after a clean rebuild.

Rollback is by concern: generated labels can temporarily retain the last emitted file; realtime changes can be reverted per host; index cleanup does not delete local outputs. Release publication or remote tag mutation is not part of apply.

## Open Questions

- None blocking. Remote cleanup of any legacy forbidden tag/release references requires separate explicit user authorization and is intentionally outside this change.
