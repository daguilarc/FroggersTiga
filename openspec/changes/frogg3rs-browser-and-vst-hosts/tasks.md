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
      `desktop/CMakeLists.txt:136-296` (capture the `juce_add_plugin`
      idiom for group 4 BEFORE deleting); delete tracked v2 plugin sources
      (`desktop-v2/Source/PluginProcessorV2.*`, `PluginEditorV2.*`,
      `HostParameterInventoryV2.hpp`, `HostParameterProcessorV2*` test)
      and their CTest wiring; grep-gate that nothing else references the
      deleted names.
- [x] 1.2 Docs: remove `desktop/PACKAGING.md:115-127`,
      `desktop-v2/PACKAGING.md:68-97`, the `docs/CI.md:89` VST clause, and
      any README mention (enumerate by grep; report found vs changed).
- [x] 1.3 Verify the default desktop configure/build still succeeds
      (options were default-OFF; prove it, don't assume) and the app suite
      is green.

## 2. Browser build of the app (design Part A1)

- [ ] 2.1 `app/browser/` manifest (appId `frogg3rs`, cppType
      `synth_froggers::FroggersApp`, includeDirs → `app/`; schema per
      `app-build-manifest.mjs:5-9`) + build script invoking Sheaf's
      pipeline per the fixture precedent (`browser/Makefile:40-41`):
      tsc build, then `build-browser-apps.mjs --manifest ...
      --allowed-source-root <repo>/app --output-root app/browser/dist`.
      First emscripten compile of the core is the probe for std-only
      portability — report any core-side issue as BLOCKED, do not patch
      the core silently.
- [ ] 2.2 Build-level test (CI-runnable): script exits 0 and emits
      `frogg3rs.js` + `frogg3rs.wasm` + emissions report; wire into the
      app suite or a dedicated make target per existing conventions.

## 3. Package + catalog + site swap (design A2, A3)

- [ ] 3.1 Package artifacts + self-hosted catalog JSON per
      `froggers-browser-package` (identity `spec.md:27-49`, no Sheaf-side
      slot `spec.md:75-81`; CORS/media-type constraints per `sbac-7`).
      Local smoke: Sheaf launcher + localhost catalog (`sbac-10`).
      RENAMED-ORIGIN GATE (froggers-web-host:34-45, repo renamed
      2026-08-18): every URL minted into the catalog, package artifacts,
      and site config uses `frogg3rs`; a CI-runnable check greps the
      published outputs and FAILS on any `FroggersTiga` string — the
      redirect GitHub provides is not compliance, absence of the old name
      is.
- [ ] 3.2 `pages.yml`: replace the legacy `wasm/`+`web/` steps
      (`pages.yml:41-59`) with the group-2/3 build + publication;
      CI asserts `web/` and `wasm/` stay byte-identical (git-clean
      check). The public cutover/rename gate remains the operator's per
      `froggers-web-host:34-45` — the workflow lands ready, dry-run
      proven (workflow_dispatch), without flipping anything the spec
      gates on the rename.
- [ ] 3.3 OPERATOR GATE: browser smoke — the operator loads the app via
      the launcher/catalog locally and confirms the surface (incl.
      carousel arrows) works in the browser.

## 4. VST skeleton (design B1)

- [ ] 4.1 `app/vst/` CMake + JUCE `AudioProcessor` (VST3+AU, IS_SYNTH)
      wrapping the core: `processBlock` → `ProcessFrame`, `prepareToPlay`
      → the core's prepare path (enumerate the launcher session's init
      calls first — design UNVERIFIED item; cite what you traced).
      All JUCE in the host layer only.
- [ ] 4.2 Tests: plugin target builds; core suite + `check_no_juce`
      untouched and green; basic processBlock smoke (nonzero output on a
      running transport state injected directly).

## 5. DAW-external transport (design B2)

- [ ] 5.1 Playhead edge-trigger: host play-state transitions → exactly one
      `MessageIn::Start`/`Stop` + `SetDesiredTransportRunning` per
      transition (the standalone producer at
      `app/FroggersUiSurface.hpp:1826-1900` is the semantic reference).
- [ ] 5.2 Editor transport suppression: internal transport controls not
      rendered in plugin mode (mechanism per design UNVERIFIED item —
      decide against the FroggersCellMap row-table conventions, cite).
      Freeze exposed as an automatable parameter preserving
      `SetFreezeLatched` semantics.
- [ ] 5.3 Tests: transport edge tests (run→stop→run: one message each, no
      repeats while state holds); freeze-parameter latch semantics test
      (mirror the stop-isolation T6/T7 assertions from the app suite).

## 6. Stable-ID parameter surface (design B3)

- [ ] 6.1 Inventory over the full six-bank model incl. Crispy/Crunchy:
      host parameters with flat stableId + grouped displayName, bridged
      both directions to the parameter authority via the message bus
      (enumerate the exact set-value message first — design UNVERIFIED
      item).
- [ ] 6.2 Tests: automation round-trip (write→core moves→readback
      matches); stableId stability (two construction runs produce
      identical id lists); count matches the parameter model (assert
      against the model's own enumeration, not a hardcoded number).

## 7. Plugin editor hosts the portable surface (design B4)

- [ ] 7.1 Editor renders `FroggersUiSurface` via `PortableJuceBackend`
      without the runtime shell (trace the render-host seam in
      `app/FroggersMain.cpp` + Sheaf `runtime/LauncherWindow.hpp` first).
      If the renderer is NOT separable from the runtime session, STOP and
      report BLOCKED with the coupling trace — the fallback (generic
      parameter view now, editor as follow-up change) is the operator's
      decision, not a silent descope.
- [ ] 7.2 Tests: editor constructs/destructs cleanly headless where the
      harness allows; surface tree renders with transport row suppressed
      (ties to 5.2's assertions).

## 8. Whole-change gate and operator acceptance (user-gated finish)

- [ ] 8.1 Full suite green; browser build green; plugin builds VST3+AU;
      counts reported.
- [ ] 8.2 OPERATOR GATE: DAW smoke — load the VST in a real DAW: host
      transport drives it, a parameter automates, a DAW-side MIDI mapping
      moves a parameter, the editor shows the surface without transport
      controls.
- [ ] 8.3 On both gates: archive with spec sync (ADDED froggers-vst-host;
      REMOVED spec dirs deleted; browser/web-host specs implemented as
      written, no delta).
