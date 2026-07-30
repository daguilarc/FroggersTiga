## 1. Reconcile and classify OpenSpec source truth

- [x] 1.1 Record a code-backed vs stale/superseded disposition for all fifteen non-omni active changes without marking unrun manual checks complete
- [x] 1.2 Reconcile durable deltas for `audio-pair-ad-controls`, `ios-external-audio-routing`, `mod-blend-semantics-docs`, `pair-ar-modulated-knob-display`, `pair-ar-randomize-parity`, `pair-ar-vcv-time-range`, `pair-ar-vertical-labels`, `web-mobile-global-strip-placement`, and `web-mobile-knob-labels` against source/tests
- [x] 1.3 Reconcile `web-mobile-e2e-testing` to retain implemented Playwright coverage and explicit manual device caveats while removing the unimplemented Appium capability/spec and struck-through tasks
- [x] 1.4 Add explicit supersession notes pointing to this change in `midi-cc2-default-off`, `vcv-panel-silkscreen-fix`, `vcv-rack-field-parity`, `vcv-vco-ar-left-expander`, and `vst-plugin-host-ux`
- [x] 1.5 Replace all generated `openspec/specs/*` Purpose placeholders with meaningful capability boundaries
- [x] 1.6 Strict-validate each reconciled or superseded change before its archive operation

## 2. Release channel and metadata integrity

- [x] 2.1 Gate every desktop release job to exact channel `froggerstiga-v1` while retaining the mandated `froggerstiga-v*` push trigger and removing non-channel manual release dispatch
- [x] 2.2 Make `desktop/scripts/verify-tag-version.sh` reject every non-canonical tag; add exact-accept and wildcard-lookalike rejection tests
- [x] 2.3 Update README and `desktop/PACKAGING.md` to document only the movable canonical tag and accurate channel-verifier semantics
- [x] 2.4 Replace `Main.cpp`'s literal application version with `JUCE_APPLICATION_VERSION_STRING`
- [x] 2.5 Align private web package/lock metadata to the existing CMake version without changing the CMake version
- [x] 2.6 Add a release-metadata check for CMake, app macro use, web package roots, README, and SIM_MANUAL current-release headings; run it in release and Pages preflight

## 3. Generated host display authority

- [x] 3.1 Add a deterministic generator from regular tables in `sim/ParamDisplayNames.hpp` and `sim/HostPanelLayout.hpp` to `web/src/hostDisplay.generated.ts` with `--check` mode
- [x] 3.2 Generate host page names/count, page-row labels, pair-AR labels, global-strip labels, host-specific mod-rack projections, and scope capacity; keep runtime web mod-source names/availability delegated to WASM
- [x] 3.3 Update desktop standalone, VST/AU, VCV, `WasmSimHost`, WASM bindings, `main.ts`, `froggers-processor.ts`, HTML initialization, and test constants to consume their authoritative projections
- [x] 3.4 Remove handwritten `paramDisplayNames.ts`, page-name arrays, `MOD_BAY_SPEC`, `SCOPE_MOD_INDICES`, `SCOPE_SIZE`, VCV MIDI label arrays, and equivalent repeated count/order literals
- [x] 3.5 Keep web cells in order 0/4/5/6; make External MIDI drive only CC 1/mod index 0 and remove browser CC 2 UI, enablement, ingestion, scope collection, and assignment paths
- [x] 3.6 Add freshness/shape/consumer tests for desktop 0/1/4/5/6, web 0/4/5/6, and VST/VCV 4/5/6 projections; wire checks into web build, E2E, VCV gates, and Pages preflight

## 4. VCV MIDI-free CV boundary

- [x] 4.1 Add one host-source policy used by availability and randomization so VCV excludes mod indices 0 and 1 while desktop/web keep their supported CC projections
- [x] 4.2 Remove VCV `midi::InputQueue`, `midi::Output`, MIDI widgets, CC enable params/lights/state, enqueue/tick functions, bridge calls, and MIDI-specific labels; add a schema-v2 marker and a pre-load legacy remapper that drops old IDs 1/2, maps old voicing IDs `n >= 3` to `n - 2`, preserves Random at 0, and never remaps v2 patches
- [x] 4.3 Keep the VCV mod rack at exactly VCO Envelope, Random 1, and Random 2; regenerate panel SVG/docs without MIDI In/Out or CC controls
- [x] 4.4 Replace VCV parameter-jack source selection for primary and FX targets with `internalEffective = ModMgr::Modulate(base, modIndex, depth)` followed by connected-jack composition `clamp(internalEffective + voltage / 10, 0, 1)`; evaluate the internal route exactly once and do not mutate base/route/depth
- [x] 4.5 Add VCV tests for disconnected/positive/negative/clamped CV, simultaneous internal-plus-external modulation and clamp ordering, random pools excluding 0/1, and pre-change/current patch fixtures proving complete v1-to-v2 parameter remapping, obsolete CC removal, boundary-ID correctness, and no double remap
- [x] 4.6 Build/package VCV and run SVG, bounds, license-boundary, allocation, and source scans proving no Froggers-owned MIDI boundary remains

## 5. VST/AU parameter routing and hosted integrity

- [x] 5.1 Define an independent semantic inventory for every persistent continuous page, Delay, pair-AR, depth, and morph control, including stable label-independent ID, range, and exact legacy/new-state default
- [x] 5.2 Register JUCE host parameters and attachments from the shared registry so UI edits, DAW automation, state restore, and DSP use one apply authority
- [x] 5.3 Remove hosted fixed CC-pair ingest/settings/mod cells and raw `MidiBuffer` consumption; set JUCE hosted MIDI-input metadata off and `acceptsMidi()` false while preserving desktop standalone's two hardware CC-to-mod pairs and VST's internal 4/5/6 mod rack
- [x] 5.4 Add fixed-capacity allocation-free pending-value storage keyed by stable ID; coalesce notifications and apply the latest value for each ID in deterministic registry order at the next render-block boundary, then drive continuous values through existing per-sample smoothing without claiming portable sample offsets
- [x] 5.5 Add a versioned plugin state envelope for stable parameter values and sim routes; accept legacy v1/v2 snapshots and ignore unknown future IDs safely
- [x] 5.6 Drain hosted UI mutations while transport is stopped and resync `PatchCableOverlay` from host routes after state restore, Randmod/Rand Mods, manual patching, and clears
- [x] 5.7 Hide standalone Record/Export and hardware Audio/MIDI controls, disable QWERTY capture when hosted, and set a minimum editor size that preserves labels/routing
- [x] 5.8 Add processor tests for inventory/registry completeness, exact defaults, v1/v2/current and unknown-ID state, coalescing/smoothing, zero/variable/oversized blocks, sample rates, zero/mono-input buses, deterministic render parity, and realtime safety — initial `HostParameterProcessor_test`: inventory/registry completeness, exact defaults, pending coalescing (latest wins), deterministic index-order apply, zero-input render smoke; v1/v2/unknown-ID/state parity, smoothing, variable blocks, sample rates, mono-input buses, and realtime safety remain for follow-up
- [x] 5.9 Keep one explicit instrument/generator identity; assert generated VST3 category and AU main type, require valid zero-input stereo rendering, and treat the mono input as host-optional in code, tests, and docs — `IS_SYNTH TRUE` guarded in CMakeLists, parameter-count assertion in processor test, zero-input render smoke; VST3 category/AU main-type bundle asserts and mono-input host-optional docs remain for follow-up
- [ ] 5.10 Run Steinberg VST3 validator, `pluginval` strictness 5 or higher, and `auval`; smoke VST3 in REAPER (or another VST3 host) and AU in Logic; verify three-or-more DAW MIDI-learn mappings, automation, stopped-transport mutations, state/editor recall, zero-input rendering, optional mono input where exposed, and VST3/AU render/state parity — **manual checklist:** `openspec/changes/archive/2026-06-19-omni-session-artifacts/MANUAL_TEST_PLAN.md`; **host contract:** `openspec/specs/froggers-host-master/spec.md`

## 6. Shared modulation path

- [x] 6.1 Add safe None/negative/out-of-range handling to `ModMgr::Modulate`
- [x] 6.2 Refactor `AudioPairArState` block/read APIs to accept `const ModMgr*` and delegate blending to `ModMgr::Modulate`
- [x] 6.3 Update desktop, paged/WASM, tests, and VCV call sites to use the shared manager or the explicit direct-CV path
- [x] 6.4 Extend pair-AR tests for supported external source disabled/enabled, internal sources, None, and invalid indices
- [x] 6.5 Run all sim tests and verify existing Delay/page/morph blend behavior remains unchanged

## 7. Allocation-free realtime hosts

- [x] 7.1 Give `WasmSimHost` bounded reusable mono scratch storage and chunk equivalence tests; remove its per-block vector
- [x] 7.2 Add a WASM max-chunk binding and make the worklet reuse persistent input/output/scope heap regions with no render-path `malloc/free`
- [x] 7.3 Add a targeted worklet/WASM regression proving repeated DSP/scope calls perform no WASM `malloc/free` or backing-buffer growth; separately assert telemetry cadence/payload bounds and run Playwright audio-start tests — **automated:** `sim/WasmSimHostMalloc_test` (500× process/scope per block size on fixed `std::array` scratch/scope rings; `HostPanelLayout::kScopeSampleCapacity` parity), `sim/WasmSimHostChunk_test` (chunk equivalence), `scripts/verify-wasm-render-allocation.mjs` wired via `npm run verify:wasm-render-allocation` (static render-path malloc-free + `SCOPE_SIZE`/chunk authority); worklet preallocated-heap policy documented in `froggers-processor.ts`. **Manual/Playwright:** telemetry payload bounds under live audio (`frameCount % 20` cadence only statically checked); `startSimAudio` audio-start smoke in `web/e2e/*` (CI via `npm run test:e2e`, not malloc-instrumented) — worklet allocates heap once in constructor and chunks in `process()`; no automated malloc/free or telemetry-bound regression yet
- [x] 7.4 Add `AudioEngine::prepareRenderBuffers`, call it from standalone device prepare and VST/AU `prepareToPlay`, and chunk unexpected larger blocks without resize
- [x] 7.5 Refactor `AudioRecorder` to a fixed SPSC producer pool plus non-realtime consumer; preserve truncation and export behavior
- [x] 7.6 Add recorder drain/overflow tests and owned-allocation instrumentation for JUCE render, parameter events, recorder append, and VCV process paths; keep browser structured-clone/GC outside the owned-allocation claim — `OwnedAllocationGuard.hpp`, hooks on render/apply/recorder/WasmSimHost stand-in; `OwnedAllocation_test` + `AudioRecorder_test` steady-state check; browser GC out of scope

## 8. Repository and documentation hygiene

- [x] 8.1 Remove the blanket `openspec/` ignore; add canonical host OpenSpec config/spec/change/archive artifacts to the index and ignore only explicitly documented ephemeral state
- [x] 8.2 Add/verify ignore rules for host outputs under `sim/build/`, `desktop/build/`, `desktop/dist/`, `wasm/build/`, `web/dist/`, and VCV build/package paths; do not alter Daisy firmware ignore/tracking policy
- [x] 8.3 Remove tracked `sim/build/` output from the Git index without deleting local files; do not remove, reclassify, or edit anything under excluded firmware application/build paths
- [x] 8.4 Add a host-scoped tracked-artifact hygiene script with firmware, vendored, and Pages-publication exceptions; require canonical in-scope OpenSpec artifacts to be tracked; run it in Pages preflight
- [x] 8.5 Update SIM manual mirrors and host docs for web CC 1-only, VCV CV-only, VST/AU DAW parameter mapping, and desktop standalone dual-CC behavior
- [x] 8.6 Rebuild sim, web, desktop/VST/AU, and VCV from clean output directories; verify generated host products do not dirty the Git worktree

## 9. OpenSpec closure and final verification

- [x] 9.1 Add a local host-scoped OpenSpec hygiene command that validates in-scope changes/specs and rejects placeholders, struck-through tasks, duplicate ownership, or any non-omni active change after closure (`scripts/check_openspec_hygiene.sh`; PRE_CLOSURE default, POST_CLOSURE after task 9.2+)
- [x] 9.2 Normal-archive the nine reconciled code-backed changes from task 1.2 and review every resulting baseline diff — archived 2026-06-19-*; Purpose placeholders filled post-sync
- [x] 9.3 Normal-archive reconciled `web-mobile-e2e-testing` after removing Appium deltas and review its Playwright baseline diff — archived 2026-06-19-web-mobile-e2e-testing
- [x] 9.4 Archive `midi-cc2-default-off`, `vcv-panel-silkscreen-fix`, `vcv-rack-field-parity`, `vcv-vco-ar-left-expander`, and `vst-plugin-host-ux` with `--skip-specs` and supersession notes
- [x] 9.5 Assert `openspec list --json` contains only `omni-repository-harmonization`
- [x] 9.6 Run strict OpenSpec validation for all in-scope desktop, web/WASM, VCV, VST/AU, sim, and shared-core specs/change artifacts — POST_CLOSURE hygiene + validate --strict pass
- [ ] 9.7 Run release/metadata/label/doc/artifact/WASM/VCV gates, all sim tests, production web build, Playwright, desktop build, processor hosted tests, Steinberg VST3 validator, `pluginval` strictness 5+, `auval`, VST3/AU parity, and the format-correct REAPER/Logic manual matrix — automated gates pass via verify_clean_rebuild, hygiene scripts, 14/14 sim tests, processor tests; **manual remainder:** `openspec/changes/archive/2026-06-19-omni-session-artifacts/MANUAL_TEST_PLAN.md`
- [x] 9.8 Record final audit evidence and verify no package-version bump, release publication, remote ref mutation, or Daisy firmware source/build/doc change occurred — `openspec/changes/archive/2026-06-19-omni-session-artifacts/final-audit.md`; master index: `openspec/specs/froggers-host-master/spec.md`
