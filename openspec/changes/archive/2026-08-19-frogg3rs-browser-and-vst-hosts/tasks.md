# Tasks — frogg3rs-browser-and-vst-hosts

Gate after every group: `cd app && nice make -j2 test` green (baseline
279/279 + new; NEVER above -j2 on this machine; `check_no_juce` must stay
green and unmodified throughout). One commit per numbered group, repo
commit style (plain imperative, no attribution lines). Subagent dispatch
per omni-rule: lightest capable model, sequential code changes, per-task
review with §14 postflight. Design anchors marked UNVERIFIED must be read
by the executing task before being relied on.

## 1. Old VST deletion (cheapest, unblocks nothing, goes first)

- [x] 1.1 Read then delete both option blocks in
      `desktop/CMakeLists.txt:136-296` (audit-corrected 2026-08-18:
      actual span was 136-313) (capture the `juce_add_plugin`
      idiom for the VST-skeleton group BEFORE deleting); delete tracked v2 plugin sources
      (`desktop-v2/Source/PluginProcessorV2.*`, `PluginEditorV2.*`,
      `HostParameterInventoryV2.hpp`, `HostParameterProcessorV2*` test)
      and their CTest wiring; grep-gate that nothing else references the
      deleted names.
      TASK-TEXT CORRECTION (audit 2026-08-19, matches what the 0be9ab0
      commit message already recorded -- only this task's own text above
      had not been trued up): only `PluginProcessorV2.*` and its CTest
      wiring were actually deleted. `PluginEditorV2.h/.cpp`,
      `HostParameterInventoryV2.hpp`, `HostedMainComponentV2.*`, and
      `desktop-v2/tests/HostParameterProcessorV2_test.cpp` (all listed
      above as deletion targets) were KEPT instead: they turned out to be
      either load-bearing for the live desktop-v2 app or referenced (by
      file content, as text fixtures) from live desktop-v2
      projection-validator tests, not corpses of the deleted plugin
      blocks -- deleting them would have broken the live app. All four
      are still on disk today.
      KNOWN DANGLING INCLUDE (left deliberately, not an oversight):
      `desktop-v2/Source/PluginEditorV2.h:6` still `#include`s the deleted
      `PluginProcessorV2.h`. It compiles nowhere (no target in
      `desktop-v2/CMakeLists.txt` references `PluginEditorV2.h/.cpp`) --
      it is read only as a TEXT fixture, by path and content, from
      `desktop-v2/tests/FroggersV2ProjectionValidators_test.cpp:443-444`
      (`readTextFile("desktop-v2/Source/PluginEditorV2.h")` etc.), which
      never compiles or includes it either. desktop-v2 is frozen -- fixing
      the dangling include is out of scope for this change.
- [x] 1.2 Docs: remove `desktop/PACKAGING.md:115-127`,
      `desktop-v2/PACKAGING.md:68-97`, the `docs/CI.md:89` VST clause, and
      any README mention (enumerate by grep; report found vs changed).
- [x] 1.3 Verify the default desktop configure/build still succeeds
      (options were default-OFF; prove it, don't assume) and the app suite
      is green.
- [x] 1.4 Audit residue (found 2026-08-18 omni-rule audit): delete the
      guarded `BUILD_VST_V2` block in
      `scripts/verify_clean_rebuild.sh:147-161` — its `-LAH` option probe
      can never pass again (option and capability removed for good), so
      the block is permanently-skipping dead code, missed by 1.1's
      grep-gate. NOT residue (classified, KEEP): `desktop/PACKAGING.md:116`
      mentions `BUILD_VST` only in prose recording the removal itself.

## 2. Browser build of the app (design Part A1)

- [x] 2.1 `app/browser/` manifest (appId `frogg3rs`, cppType
      `synth_froggers::FroggersApp`, includeDirs → `app/`; schema per
      `app-build-manifest.mjs:5-9`) + build script invoking Sheaf's
      pipeline per the fixture precedent (`browser/Makefile:40-41`):
      tsc build, then `build-browser-apps.mjs --manifest ...
      --allowed-source-root <repo>/app --output-root app/browser/dist`.
      First emscripten compile of the core is the probe for std-only
      portability — report any core-side issue as BLOCKED, do not patch
      the core silently.
- [x] 2.2 Build-level test (CI-runnable): script exits 0 and emits
      `frogg3rs.js` + `frogg3rs.wasm` + emissions report; wire into the
      app suite or a dedicated make target per existing conventions.

## 3. Rename completion (repo renamed to `frogg3rs` 2026-08-18; MUST precede publication)

- [x] 3.1 Tracked-name sweep: `git grep FroggersTiga` excluding
      `openspec/changes/archive`. Survey re-measured at the second audit
      (2026-08-18, HEAD `a0f2f6c`, method pinned:
      `git grep -c FroggersTiga -- ':!openspec/changes/archive'`, matching
      lines summed per top-level path — the proposal-time numbers were
      method-inconsistent and are superseded; drift vs the `eb989a8`
      measurement is +3, all inside this change's own audit-amended
      artifacts): total 756 — src 518 (513
      of these in checked-in Daisy build artifacts under
      `src/FroggersTiga/build/`: frozen tree, expect KEEP), desktop-v2 70,
      desktop 47, openspec 44 (incl. this change's own artifacts), docs
      17, README 13, web 12, DAISY_MANUAL.md 10, scripts 6, app 4,
      SIM_MANUAL.md 4, sim 3, MANUAL.md 2, .github 2,
      sheaf-audioconfig-labels.patch 1, publish 1, QUICK_DICT.md 1,
      FroggersTiga.code-workspace 1.
      Classify EVERY hit before changing any (§8 method): rename
      product-, doc-, and publication-facing mentions to `frogg3rs`;
      KEEP, with per-hit reasons, historical citations (archived
      changes stay byte-identical) and frozen-tree source identifiers
      (desktop/desktop-v2 are frozen: doc-line edits only where
      publication-facing; NO behavioral renames of frozen build targets
      or bundle names). Report found vs changed.
- [x] 3.2 Workspace file: `git mv FroggersTiga.code-workspace
      frogg3rs.code-workspace`, updating any internal folder references
      inside it.
- [x] 3.3 Branch naming (DECIDED, operator-approved 2026-08-18: finish on current branch; NEXT branch cut under a frogg3rs-* name): `froggerstiga-desktop-v2` carries the old name.
      Recommendation: finish this change on the current branch (renaming
      a branch mid-change churns remotes for zero content), and cut the
      NEXT branch under a `frogg3rs-*` name; record the decision here.
      Operator may override to rename now.
- [x] 3.4 MOVED, not done (operator decision 2026-08-19): the
      machine-local folder rename `~/Desktop/FroggersTiga` ->
      `~/Desktop/frogg3rs` is carried by
      `frogg3rs-host-state-and-visibility` task 5b.1 so it survives
      this change's archive. The repo, remote, and every published URL
      are already on the new name; only the local working directory
      still carries the old one.

## 4. Package + catalog + site swap (design A2, A3)

- [x] 4.1 Package artifacts + self-hosted catalog JSON per
      `froggers-browser-package` (identity `spec.md:27-49`, no Sheaf-side
      slot `spec.md:75-81`; CORS/media-type constraints per `sbac-7`).
      Local smoke: Sheaf launcher + localhost catalog (`sbac-10`).
      RENAMED-ORIGIN GATE (froggers-web-host:34-45, repo renamed
      2026-08-18): every URL minted into the catalog, package artifacts,
      and site config uses `frogg3rs`; a CI-runnable check greps the
      published outputs and FAILS on any `FroggersTiga` string — the
      redirect GitHub provides is not compliance, absence of the old name
      is.
      GATE POLICY DETAIL (recorded here 2026-08-19; previously living only
      in `check-renamed-origin.sh`'s own header comment): the gate is not
      a flat FAIL-on-any-match. It is three-tiered — (1) any
      `FroggersTiga` in a TEXT file: FAIL; (2) an old-origin URL form
      (`github.com/daguilarc/FroggersTiga`,
      `daguilarc.github.io/FroggersTiga`, `/daguilarc/FroggersTiga`) in
      ANY file, binaries included: FAIL; (3) a bare `FroggersTiga` string
      inside a BINARY file that is not one of the URL forms above: WARN
      only, non-fatal. Tier 3 exists because `frogg3rs.wasm` embeds the
      *local build machine's* absolute source path via DWARF/compiler
      diagnostics (this dev machine's checkout is still literally under
      `~/Desktop/FroggersTiga/...` — a separate, operator-coordinated
      machine-local rename, tracked as task 3.4), which is not an
      app-identity defect and would false-fail every local build
      regardless of correctness; it is expected to be absent from a CI
      build (whose checkout path never contains `FroggersTiga`) and to
      clear entirely once the pending local folder rename lands. See
      `check-renamed-origin.sh`'s own header comment for the full per-tier
      reasoning.
- [x] 4.2 `pages.yml`: replace the legacy `wasm/`+`web/` steps
      (`pages.yml:41-59`) with the group-2 build + this group's packaging;
      CI asserts `web/` and `wasm/` stay byte-identical (git-clean
      check). GATE THE DEPLOY STEP ON MAIN (design A3 audit addition —
      as written the deploy at `:69-71` is unconditional and a branch
      dispatch could publish the unreleased site): branch
      `workflow_dispatch` = build + e2e only, which is the pre-main dry
      run; prove it from the working branch and confirm the live site
      is untouched. The public cutover/rename gate remains the
      operator's per `froggers-web-host:34-45` — the workflow lands
      ready without flipping anything the spec gates on the rename.
- [x] 4.3 Site shell (design A4; operator decisions 2026-08-18, ADDED
      delta on `froggers-web-host`): mobile-width viewports stack around
      a full-width sixteen-slot encoder grid, everything else above or
      below (mechanism is a design UNVERIFIED item — trace Sheaf's
      browser sizing path and the grid geometry first; surface-side
      layout work is in scope if the trace demands it, report it);
      legacy link roles carried forward (`web/index.html:39-62` is the
      enumeration; manual → `MANUAL.md`, release links → the release
      being published). Playwright e2e for the NEW site, own harness
      (e.g. `app/browser/e2e/` — the legacy `web/` suite stays dormant
      and byte-identical; copy its idiom, not its files):
      mobile-emulated stacking + desktop layout + link roles with no
      old-name target, no audio start, wired into the workflow before
      the deploy step. Trace first whether Sheaf's renderer exposes DOM
      geometry or draws to canvas (design UNVERIFIED item — JS-probe or
      screenshot fallback; cite the trace).
- [x] 4.4 Browser smoke DEFERRED (operator instruction 2026-08-18): the
      operator tests nothing until every group lands — the browser smoke
      runs together with the DAW smoke in the combined gate at 9.2.
      This group is complete when its automated checks (4.1's URL gate,
      4.2's branch-dispatch dry run + byte-identity, 4.3's Playwright
      suite) are green.

## 5. VST skeleton (design B1)

- [x] 5.1 `app/vst/` CMake + JUCE `AudioProcessor` (VST3+AU, IS_SYNTH)
      wrapping the core: `processBlock` → `ProcessFrame`, `prepareToPlay`
      → the core's prepare path (enumerate the launcher session's init
      calls first — design UNVERIFIED item; cite what you traced).
      All JUCE in the host layer only. Bus/MIDI posture per design B1:
      stereo output, no audio input bus, MIDI buffer accepted and
      ignored (no note wiring — the core has no note-input seam).
- [x] 5.2 Tests: plugin target builds; core suite + `check_no_juce`
      untouched and green; basic processBlock smoke (nonzero output on a
      running transport state injected directly).

## 6. DAW-external transport (design B2)

- [x] 6.1 Playhead edge-trigger: host play-state transitions → exactly one
      `MessageIn::Start`/`Stop` + `SetDesiredTransportRunning` per
      transition (the standalone producer at
      `app/FroggersUiSurface.hpp:1826-1900` is the semantic reference).
- [x] 6.2 Editor transport suppression: Play, Stop, AND Record not
      rendered in plugin mode — the row is Play | Stop | Freeze | Record
      (`app/FroggersUiSurface.hpp:126`); recording is the DAW's job
      (mechanism per design UNVERIFIED item — decide against the
      FroggersCellMap row-table conventions, cite). Freeze exposed as an
      automatable parameter preserving `SetFreezeLatched` semantics; its
      button stays and gains a "FREEZE" text label beside it in the
      freed row space (operator instruction 2026-08-18; design B2 cites
      the nearest labeling precedent).
- [x] 6.3 Host tempo (design B2 audit addition): host tempo reaches the
      master clock via the core's existing external-clock slaving; BPM
      slider behaves exactly as when MIDI-clock-slaved (display-only,
      requests suppressed). Mechanism is a design UNVERIFIED item —
      read the slave-engage path in MasterClock first, cite the trace.
- [x] 6.4 Tests: transport edge tests (run→stop→run: one message each, no
      repeats while state holds); freeze-parameter latch semantics test
      (mirror the stop-isolation T6/T7 assertions from the app suite);
      tempo-follow test (host tempo change → `DisplayTempoBpm()`
      follows, `TempoExternallyClocked()` true, user tempo request
      rejected while slaved); editor-row assertions (Play/Stop/Record
      absent, Freeze present with its label — ties to 8.2).

## 7. Stable-ID parameter surface (design B3)

- [x] 7.1 Inventory over the full six-bank model incl. Crispy/Crunchy:
      host parameters with flat stableId + grouped displayName, bridged
      both directions to the parameter authority via the message bus
      (enumerate the exact set-value message first — design UNVERIFIED
      item).
- [x] 7.2 Tests: automation round-trip (write→core moves→readback
      matches); stableId stability (two construction runs produce
      identical id lists); count matches the parameter model (assert
      against the model's own enumeration, not a hardcoded number).

## 8. Plugin editor hosts the portable surface (design B4)

- [x] 8.1 Editor renders `FroggersUiSurface` via `PortableJuceBackend`
      without the runtime shell (trace the render-host seam in
      `app/FroggersMain.cpp` + Sheaf `runtime/LauncherWindow.hpp` first).
      If the renderer is NOT separable from the runtime session, STOP and
      report BLOCKED with the coupling trace — the fallback (generic
      parameter view now, editor as follow-up change) is the operator's
      decision, not a silent descope.
- [x] 8.2 Tests: editor constructs/destructs cleanly headless where the
      harness allows; surface tree renders with Play/Stop/Record absent
      and Freeze present with its "FREEZE" label, BPM display-only
      (ties to 6.4's assertions).

## 9. Whole-change gate and operator acceptance (user-gated finish)

- [x] 9.1 Full suite green; browser build green; plugin builds VST3+AU;
      counts reported.
- [x] 9.2 OPERATOR GATE — BROWSER, PASSED 2026-08-19: the operator
      loaded the site and confirmed it working (surface renders at
      desktop width, encoders labeled before Play, slider values
      present, mobile stack, links).
      DAW CONFIRMATION MOVED OUT (operator decision 2026-08-19): the
      real-DAW smoke — host transport, host tempo, parameter
      automation, DAW-side MIDI mapping, and the editor's suppressed
      transport row — is no longer a gate on THIS change and is
      carried by `frogg3rs-host-state-and-visibility` (its group 6
      operator gate). This change therefore archives on automated
      evidence for the plugin (app suite 279/279, app/vst ctest 3/3
      covering transport edges, tempo slaving, the 92-parameter
      round-trip and stable ids, and the editor's plugin-mode surface;
      both formats building in CI) plus the browser confirmation above
      — NOT on a real-DAW run, which has not happened. Recorded so the
      archived change never reads as if one had.
- [x] 9.3 On both gates: archive with spec sync (ADDED froggers-vst-host;
      froggers-web-host ADDED delta synced; REMOVED capabilities synced
      from their per-capability delta files and spec dirs deleted;
      browser-package spec implemented as written, no delta).
