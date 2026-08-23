# Tasks — `frogg3rs-desktop-reaches-downloaders`

Gates: `cd app && nice make -j2 test` (301/301); plugin targets from
`app/vst/build` — `FroggersVstHostTests` 46/46, smoke 1/1, editor 3/3; Sheaf
`nice make -C projects/synth -j2 test` (923 passed / 2 failed, the known
braid-4 96kHz deadline tests — that recipe aborts there, so run anything after
it directly); `nice make -C projects/synth/apps/miniapp -j2 test`, the only
target that builds the JUCE runtime shell; and the browser e2e suite, whose
`pages` project runs against a headerless origin. Never above `-j2`, always
`nice`.

Section 0 edits `sim/`, which no CI job and none of the gates above cover, so
its ctest suite is a gate for this change: configure and build `sim/`, run
`ctest`, and report the count before and after.

## 0. Hygiene

- [ ] 0.1 Sweep the packaging surface: `app/build-launcher.sh`,
      `app/vst/CMakeLists.txt` and the three release workflows. Report dead
      steps, stale paths, and anything naming a tree the last change retired.
      Two things are already known to be in it and are fixed here:
      `app/build-launcher.sh`'s comments cite line numbers that no longer point
      at what they claim — `:5` cites the sheaf-patch `Makefile:47-48` for
      header prerequisites, which are handled at `:21`, `:26` and `:50`; `:25`
      cites `juce_build.mk:154` for the verbatim plist copy, which is in the
      `$(APP_BUNDLE)` rule further down; `:44-45` cites `:152-154` for that
      same rule. Cite the rule or variable by name instead of by line: these
      point into a submodule, so every line number drifts when the pin moves.
- [ ] 0.2 Retire the simulator surface: `vcv/`, `Rack-SDK/`, `sim/`, and the
      `src/core` headers that only the simulator compiles.
      The narrower version of this task claimed "the rest of `sim/` simulates
      the Daisy firmware and is not touched". That is false, and the same grep
      that scoped the task disproved it: `src/FroggersTiga/FroggersTiga.cpp`
      resolves to 32 files, 24 of them in `src/core`, and none of them in
      `sim/`. Nothing tracked builds `sim/`, no CI job runs its ctest suite,
      and `app/check_no_frozen_includes.sh` forbids `app/**` from including it
      at all. `vcv/` and `Rack-SDK/` have zero tracked files, so the one
      consumer that did reach into `sim/` reaches nobody who clones this
      repository.
      **0.2a PREFLIGHT — state what is dropped before dropping it.** This is a
      57-file tracked deletion plus two untracked trees, and a deletion whose
      losses are discovered afterwards is not a sweep.
      (a) Enumerate the `src/core` headers the firmware does NOT reach. There
      are 30 headers there and the firmware reaches 24; name all six, not the
      two this text already knows
      (`DesktopHostIO.hpp`, `PagedHostIO.hpp`). Each one that no surviving
      consumer reaches goes with `sim/`; each one that something else reaches
      stays, and the trace says which.
      (b) Diff the assertions. `sim/`'s 25 test targets and
      `app/FroggersDspParityTests.cpp` (5320 lines) both assert DSP parity
      against the firmware. Report, per sim test target, whether its assertion
      is already made in the app suite, made differently, or made nowhere else.
      What survives only in `sim/` is what this deletion costs, and it is
      stated here rather than found later.
      (c) Enumerate the inbound half by bare name across all tracked text, not
      by include: every comment, spec, doc, workflow, makefile variable and
      task list that mentions `vcv`, `sim/`, `Rack-SDK`, or any deleted
      filename. Those are not invocations and no invocation search finds them,
      and each becomes a reference to a path that no longer resolves.
      Report (a), (b) and (c) before any file is deleted.
      **0.2a RESULT (recorded 2026-08-22, verified against the tree).**
      (a) Seven orphans, not two. `src/core/CvPresence.hpp` and
      `V2EngineSetup.hpp` are reached only by the two shims;
      `DesktopHostIO.hpp` and `PagedHostIO.hpp` by nothing outside `sim/`;
      `ExportFormat.hpp` by nothing at all; `EQ.hpp` only by
      `src/common/EQ.hpp`, which has no includers of its own and dies with it.
      (b) 25 registered targets. Twenty assert a retired host — VCV, the wasm
      and desktop sim hosts, or the shims themselves — and cost nothing.
      THREE assert code that still ships, and they are the only automated
      coverage the Daisy firmware has anywhere in this repository:
      `HookIdentity_test`, `ModMgr_test` and `PairArEnvelope_test` (its
      `ExpParam::Compute` half), all of which compile without the shims.
      Two more looked like coverage and are not. `V2IndependentPm_test`'s
      flag-off golden-pin check needs the shims to compile at all.
      `Fuegoize_test` compares the simulator's free-function fuegoize against
      the firmware's inline one and asserts they agree — `src/core` has no
      fuegoize header of its own, so once the simulator copy goes the
      comparison has nothing on its other side. It reads as firmware coverage
      because it includes `Parameter.hpp`; its actual subject is the copy being
      deleted. Relocating it would mean copying `sim/Fuegoize.hpp` into a
      surviving tree to keep it compiling, which preserves exactly what this
      retirement removes — and that header is the one carrying the
      divide-by-zero, which the test's own tuples reach at full fuego.
      `app/FroggersDspParityTests.cpp` does not cover them: it asserts the
      app's own DSP copy under `app/dsp/`, not `src/core`.
      Separately, `sim/PageBootNav_test.cpp` and
      `sim/SwitchDebounce_replica_test.cpp` are on disk and in no target —
      27 test files, 25 targets — so they are already dead and leave as such.
      (c) The mention map is in the preflight report: 249 `vcv` hits across 41
      files, 96 `sim/` hits across 29. Most of the `sim/` hits are provenance
      comments in `app/` naming the file a DSP port was copied FROM, which
      stay accurate as history only if reworded; read each.
      **0.2a DECISION.** The three shim-free firmware assertions are relocated,
      not dropped. Their subject outlives the deletion — `FroggersEngine`,
      `ModMgr`, `Parameter` and `PairArEnvelope` all ship in the firmware — and
      that is the verified condition under which restoration is correct rather
      than the reflex §13.0 warns about. They move to a test target that puts
      only `src/core` on the include path, so nothing carries a dependency on
      the tree being removed. A test that needs a file copied out of `sim/` to
      compile is not relocatable — it is a test of `sim/`, and the copy is the
      tell. `V2IndependentPm_test` is not relocated: the half
      that asserts live behavior needs the shim to compile, and preserving the
      shim to keep one assertion alive is exactly the import this task exists
      to undo. Say so where the target is defined rather than leaving it
      unexplained.
      **0.2b DELETE.** `vcv/` and `Rack-SDK/` are working directories with
      nothing tracked — remove them from disk. `sim/` and the `src/core`
      headers (a) identified go as tracked deletions. Then the inbound half
      from (c): four capability specs die with the tree, verified by reading
      each. `vcv-cc-mod-gating`, `vcv-panel-silkscreen` and
      `vcv-section-expander-architecture` are wholly about VCV.
      `sim-pm3-knob-parity` names desktop standalone, web WASM and VCV
      `PagedHostIO` as its hosts — all retired — and binds
      `ParamDisplayNames::forHostPageRow` and `SetSimDedicatedPm3Knob`, both
      in the tree being deleted.
      `pair-ar-vcv-time-range` STAYS despite its name: VCV is the reference for
      its endpoints, not its subject, and its requirement binds
      `PairArEnvelope::kMinTimeSec` and `PhaseUtils::ExpParam::Compute` — src/core
      code the relocated test now covers. Specs that merely carry a VCV row or
      a sim aside keep, minus that row. Read each before editing —
      some describe a parity the firmware still holds, and only the attribution
      to a retired host goes.
      Guards whose sole target is being deleted go with it rather than being
      repaired: `app/check_no_frozen_includes.sh`'s deny pattern is
      `src|sim`, and with `sim/` gone half of it guards nothing. Narrow it to
      what still exists; do not leave it asserting against a missing tree.
      **0.2c POSTFLIGHT.** Not implementation-versus-plan — run §8 against the
      diff itself. A deletion this size retroactively changes what other code
      means: report what the removal made dead that was not dead before
      (a helper whose last caller went, a CMake function with no remaining
      call, a define nothing sets), and confirm by search that no surviving
      script, workflow, spec, manifest or comment names a path that no longer
      resolves. Report found versus changed for every category.
      `sim/Fuegoize.hpp`'s divide-by-zero at full fuego is not repaired: it
      leaves with the tree that carried it.
      **0.2d POSTFLIGHT FIX — what the removal made dead.** The postflight
      found live `src/core` code whose last callers were the deleted shims, and
      §13.0 fixes what the sweep finds inside the change rather than listing it.
      Verified against the tree:
      `Page::ConfigureV2Fuego` (`src/core/Page.hpp:133`) has no callers; the
      only other mention is a comment at `:18` describing it.
      `AudioPairArState::setV2FuegoConfig` (`AudioPairArState.hpp:159`) has no
      callers, and it is the only writer of `m_hostKind` (`:162`), so that field
      is now permanently its `SimHostKind::Desktop` default (`:194`). Therefore
      `AudioPairArState.hpp:179`'s `UsesV2Fuego(m_hostKind)` is always false and
      the V2 fuego path behind it is unreachable.
      That reaches further, and the trace decides how far rather than this text:
      `UsesV2Fuego` is true only for `DesktopV2`, `VstV2` and `Web`
      (`SimModSource.hpp:31,36`), and every surviving caller of the
      `SimHostKind` predicates passes `Desktop` literally (`:55`, `:65`,
      `Page.hpp:432`). Establish whether any reachable path can still carry a
      non-`Desktop` kind. If none can, the parameter, the enum's other values,
      and the branches on them at `SimModSource.hpp:82,135` are unreachable too,
      and they go — a branch nothing can enter is the dead-gate rot this sweep
      exists to remove, and it still passes review by looking like logic.
      Do not delete beyond what the trace proves unreachable, and report found
      versus changed. `test/firmware` compiles `Page.hpp`, `AudioPairArState.hpp`
      and `SimModSource.hpp` through `FroggersEngine`, so it is the gate: 3/3
      before and after.

- [ ] 0.3 `openspec/specs/froggers-host-master/spec.md`'s verification section
      cannot survive 0.2: it names `OwnedAllocation_test` and
      `WasmSimHostMalloc_test`, both of which leave with `sim/`. The rest of
      that block is already stale independently: the block runs `cd web && npm run ...`
      against a `web/` directory that is not in the repository and a root
      `package.json` that declares no scripts, and it names
      `HostParameterProcessor_test`, a ctest target that exists nowhere in the
      tree. Fix the whole block or delete it; do not leave a verification
      section that cannot be run.
- [ ] 0.4 `README.md:6-9` lists `desktop/`, `desktop-v2/`, `sim/`, `src/`,
      `wasm/`, `vcv/`, `web/` as frozen trees "kept in the tree,
      byte-identical". Four of those seven — `desktop/`, `desktop-v2/`,
      `wasm/`, `web/` — no longer exist, deleted by the last change; `vcv/` has
      zero tracked files and reaches nobody who clones the repository, and
      `README.md:347` already says so. This is that change's own unswept
      inbound half. Name only what is there.
- [ ] 0.5 `scripts/` is 499 lines across seven `.mjs` files and nothing invokes
      any of them: no workflow, no Makefile, and the root `package.json` has no
      `scripts` key at all. Five of the seven read or write paths under `web/`,
      which is gone — `web/src/hostDisplay.generated.ts`,
      `web/public/froggers.wasm`, `web/src/froggers-processor.ts`. The only
      references anywhere are prose, in `README.md` and in two live specs.
      Establish the invocation for each by bare name as well as by path, then
      delete what has none, and edit the prose that named it in the same
      commit.
- [ ] 0.6 The spec set stops describing software that is not here. Of 61 live
      capability specs, 36 name a product deleted in `b9a8199` — "desktop v2",
      "web sim", "WASM sim", "desktop standalone". `openspec/specs/` is the
      current-truth set, so today it states requirements for trees the
      repository does not contain. This is the inbound half of the previous
      change's own retirement, and it lands here because this change is already
      editing that directory.
      One rule decides each spec, applied by reading it: strike every subject
      that no longer exists, then ask whether a requirement survives.
      - Nothing survives -> delete the spec directory. Roughly 26 are in this
        state, among them `desktop-v2-grid-layout`, `desktop-v2-global-controls`,
        `web-v2-parameter-subset` and `web-mobile-external-audio-routing`.
      - A requirement survives because it also binds something that ships ->
        keep the spec, strike the dead subject, leave the surviving requirement
        otherwise untouched. `desktop-v2-audio-io` reads "Desktop v2 and VST v2
        SHALL initialize audio output as stereo by default"; the VST ships, so
        the requirement stays and only its dead half goes. Do NOT rewrite what
        the surviving half requires — that is a claim about today's behavior and
        it is not this task's to make.
      - The behavior survives under a retired tree's NAME -> the spec is renamed
        to what delivers it now, its text retargeted no further than the rename
        demands.
      Where a spec's own directory name carries the dead product, the rename is
      part of the fix, not a cosmetic afterthought.
      Report found versus changed, and list every spec by the branch it took.
      Anything that does not fit the three branches is reported, not guessed.

## 1. Every shipped bundle carries a signature that matches itself

- [ ] 1.1 Sign the app bundle as the LAST step of `app/build-launcher.sh` —
      after `juce_build.mk`'s `$(APP_BUNDLE)` rule has copied in the binary and
      `Info.plist`, AND after `:62-71` copies `Icon.icns`, `MANUAL.md` and
      `QUICK_DICT.md` into `Contents/Resources/`. Measured on a copy of the
      shipped bundle: signing the assembled bundle passes
      `--verify --deep --strict`, and adding one file to `Contents/Resources/`
      afterwards fails it with `a sealed resource is missing or invalid`.
      Signing after the plist but before those copies swaps one broken verdict
      for another. Task 2.1 edits `MANUAL.md`, which is one of them.
- [ ] 1.2 Sign the AU bundle in `app/vst/CMakeLists.txt` after JUCE assembles
      it. Measured: `Frogg3rs.component` is `adhoc,linker-signed`,
      `Info.plist=not bound`, `Sealed Resources=none` and fails
      `codesign --verify`, while `Frogg3rs.vst3` beside it is sealed and
      passes. JUCE attaches `_juce_adhoc_sign` to its VST3 and LV2 targets and
      to the copy-plugin step, and to no AU target — read in
      `JUCEUtils.cmake:962-980,1001,1237,1364`, not inferred from the one
      build. Both bundles are release assets of `vst-plugin.yml`.
- [ ] 1.3 Every bundle a release ships passes
      `codesign --verify --deep --strict`, and `spctl --assess --type execute`
      returns a verdict rather than an error. Enumerate the bundles from the
      workflows that publish them, not from memory: `Frogg3rs.app`,
      `Frogg3rs.vst3`, `Frogg3rs.component`. Both commands are one line;
      neither was ever run on any of the three.
- [ ] 1.4 Gate it, at the build entry points rather than in the workflows, so a
      local build fails the same way CI does. A bundle whose signature does not
      match its contents fails the build rather than reaching a release.
      POSITIVE CONTROL required: show the gate failing on a deliberately
      unsigned or mis-assembled bundle, and separately on a bundle signed
      before its resources are copied in, since that is the failure this
      change's own fix could reintroduce.
- [ ] 1.5 Verify against a real download, not a local build. Apply
      `com.apple.quarantine` to the packaged `.dmg` and to the packaged plugin
      zips, and report what `spctl --assess` says for each. A locally built
      bundle carries no quarantine, which is precisely why this defect survived
      two releases. The dialog a person actually sees is 4.3's operator step;
      this task is the headless half and reports verdicts, not dialogs.

## 2. The release says what a downloader must do

- [ ] 2.1 `MANUAL.md` states what a downloader sees and the step that opens it,
      for as long as the builds are unnotarized. Plain present tense. It covers
      the plugin bundles as well as the application, since both are downloaded
      and both are unsigned by any identity the system recognises.
- [ ] 2.2 The release bodies carry the same, from one source.
      `desktop-release.yml` already extracts its body from `MANUAL.md`'s
      `## Release platforms` section with `awk`, so the desktop half is 2.1's
      edit and nothing more. `vst-plugin.yml` instead carries its body inline
      in `--notes`, so state where the plugin's copy comes from: extend the
      same extraction to it, or say why a second copy is correct. Do not write
      the sentence into a workflow twice.
- [ ] 2.3 Record notarization as the thing that removes the step: Developer ID
      signing plus notarization is what makes a download open normally. It
      needs an Apple Developer account and CI secrets that do not exist, so it
      is named here, not attempted.

## 3. Windows

- [ ] 3.1 Trace and report before writing anything, and before choosing a build
      system. Four things, all readable today:
      (a) what `juce_build.mk` assembles for the standalone — sources, defines,
      include paths, link flags — and what its existing Windows branches
      already cover: the Objective-C++/C++ unity-source switch and the
      MinGW-versus-MSVC link-flag selection;
      (b) what the three CI attempts actually failed on. All three failed in
      plumbing — submodule checkout, the build invocation, path spelling — so
      no compiler has reported on this Makefile's portability, and "does not
      build" is not yet a statement about the toolchain;
      (c) how `app/vst/CMakeLists.txt` obtains the synth core: it invokes
      Sheaf's own Makefile to build `libsynth.a` and links the archive, adding
      only `runtime/HostDataPaths.cpp`. It does not restate Sheaf's source
      list, and a new build must not become the third place that does;
      (d) the v1 precedent, which was deleted with `desktop/` and lives only in
      history — `git show b9a8199^:desktop/CMakeLists.txt` (a
      `juce_add_gui_app` target with an MSVC branch) and
      `git show b9a8199^:.github/workflows/desktop-release.yml`. Note that
      `app/vst/CMakeLists.txt` requests `FORMATS VST3 AU` and AU is Apple-only,
      so it is JUCE's CMake that is cross-platform, not that file as written.
- [ ] 3.2 On that trace, choose: finish the Makefile's Windows port, or give
      the standalone a CMake build. Report the reason. Either way the result
      produces the same application `build-launcher.sh` produces on macOS —
      Sheaf's runtime shell, not JUCE's `Standalone` plugin wrapper. That
      wrapper substitutes JUCE's own audio settings dialog for Sheaf's Audio
      page, so the two platforms would ship different applications and the
      no-input selector would not exist on Windows. And either way the synth
      core is reused, not re-listed: a second source list for the same binary
      is the duplication this change would be creating, not inheriting.
- [ ] 3.3 The macOS build path is unchanged by the Windows work. Not
      byte-identical — that is unavailable here and always was:
      `juce_build.mk` compiles JUCE's `juce_core_CompilationTime.cpp`, which is
      `__DATE__` and `__TIME__` captured into the binary, so two clean builds
      minutes apart differ; and task 1.1 changes the bundle's bytes on purpose.
      What is checkable, and what this task means: the macOS invocation, its
      source and flag set, and the bundle's file inventory are the same before
      and after, and every gate that was green stays green with counts
      reported. A port that quietly changes the macOS build is a regression
      wearing a feature's clothes.
- [ ] 3.4 The Windows job builds and the signature gate's platform equivalent
      runs there. Report what it takes; if something cannot run on a Windows
      runner, say which step and why rather than marking it unverified and
      moving on. `codesign` and `spctl` are macOS tools, so state what stands
      in for 1.3 on Windows, or that nothing does and why that is acceptable.
- [ ] 3.5 Only once Windows genuinely builds: the desktop release ships both
      platforms again, and `MANUAL.md` stops saying Windows is in progress.

## 3b. The site header carries the app's logo

The published site shows the title alone. The application has a logo and the
header does not use it.

- [ ] 3b.1 Source it from `app/Resources/Icon.png` (800x800, 313 KB) rather
      than adding a second logo file to maintain by hand. That is not the file
      the bundle carries: `build-launcher.sh:63` copies `Icon.icns`, its
      sibling, and nothing in the tree derives either from the other — no
      `iconutil` or `sips` step exists. Establish which is the source before
      calling them one image, or say plainly that they are two files kept in
      step by hand. Either way the header needs a web-sized copy, so say where
      the derivation happens: a build step keeps one source, a committed
      derivative is simpler but is a second copy that can drift. Pick one and
      record why.
- [ ] 3b.2 Placement is constrained, not free. `stageSiteShell`
      (`app/browser/package-catalog.mjs:157-163`) copies every FILE in
      `app/browser/site/` flat into the staged site, so an image placed there
      ships with no pipeline change — this is why the header can reference it
      relatively and still resolve.
- [ ] 3b.3 The logo renders INSIDE `.site-header`. `blank-frame.spec.mjs:33-36`
      takes that element's bounding box and samples only the band beneath it,
      deliberately excluding header chrome so the title's own colour cannot be
      mistaken for a rendering app surface. A logo positioned outside the
      header, or one that makes the header not enclose it, leaks colour into
      the sampled band and lets that guard pass over a blank app. Confirm the
      guard still fails on a blank surface after the change — it is the test
      that already missed one blank deployment.
- [ ] 3b.4 Style it against the existing header: `.site-header` is centred with
      `padding: 12px 16px 0`, `.site-title` is 15px/1.2 (`site.css:28-38`). The
      logo sits inline beside the title at a size that matches it, and the
      header stays legible at the mobile viewport the `mobile` e2e project
      uses.
- [ ] 3b.5 Assert it actually loads. A header `<img>` whose file did not ship
      renders as a broken-image icon and every existing test still passes —
      that is precisely how the retired site looked. The assertion checks the
      image resolved (natural dimensions non-zero), not merely that the element
      exists.

## 4. Close

- [ ] 4.1 All gates green, counts reported, including the miniapp target, the
      browser e2e `pages` project, and `sim/`'s ctest suite.
- [ ] 4.2 Postflight: implementation versus proposal, plus a duplication pass
      over the whole diff for every new named concept — the signing step and
      its gate exist at more than one entry point by design, so check that they
      are one definition and not three — and a check that no surviving script,
      workflow, spec or manifest names a path that no longer exists.
- [ ] 4.3 OPERATOR: download the published `.dmg` through a browser — not a
      local build — and confirm it opens, recording the dialog it shows on the
      way.
- [ ] 4.4 OPERATOR: retire the v1 release once that download opens, and not
      before. It is still the only desktop download anyone can open.
