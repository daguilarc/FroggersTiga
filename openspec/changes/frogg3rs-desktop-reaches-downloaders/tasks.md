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
- [ ] 0.2 Remove the retired host's code from `sim/`. Retiring `desktop/` broke
      `sim/OwnedAllocation_test.cpp`, and rather than letting the broken case
      go with the tree it tested, `HostParameterPendingStore.hpp`,
      `HostParameterInventory.hpp` and `HostParameterRouting.hpp` were copied
      out of `desktop/Source` into `sim/` to keep it compiling. That import is
      the thing to undo.
      Traced: those three headers are referenced only by each other and by
      `OwnedAllocation_test.cpp`. The firmware does not use them — the two
      matches in `src/core/PagedHostIO.hpp:68` and
      `src/core/DesktopHostIO.hpp:317` are comments naming a desktop-v2 class,
      not includes. `app/vst/`'s `BuildHostParameterInventory()` is a method of
      its own, not an include of the header. Everything else `sim/` includes
      resolves to `src/core` or to `sim/` itself.
      **The file is not the unit to delete.** `OwnedAllocation_test.cpp` holds
      two cases: `:43-62` `test_apply_pending_steady_state`, whose subject is
      the imported headers, and `:21-41`
      `test_wasm_process_block_steady_state`, whose subject is
      `sim/WasmSimHost.hpp:53` — still present, and outside the tree that was
      retired. Remove the first case and the three headers; keep the second,
      its target, and the instrumentation it runs on.
      **If the whole target goes instead, the orphans go with it.** Deleting
      `OwnedAllocation_test` alone strands `sim/OwnedAllocationHooks.cpp`
      (56 lines; its only consumer is `add_sim_instrument_test`,
      `sim/CMakeLists.txt:15-21`, used once at `:37`), the
      `add_sim_instrument_test` function itself, and
      `FROGGERS_OWNED_ALLOCATION_INSTRUMENT`, which nothing else defines. With
      the hooks unlinked, `sim/WasmSimHost.hpp:53`'s guard still compiles and
      can no longer count anything — a guard that passes while checking
      nothing, which is what this sweep exists to remove. Either keep the
      instrument whole or delete it whole; do not leave the halves.
      Re-verify the trace before deleting, then report the sim ctest count
      before and after.
- [ ] 0.3 The inbound half of 0.2. `openspec/specs/froggers-host-master/spec.md`
      names `OwnedAllocation_test` in its verification section, so that prose
      moves in the same commit as whatever 0.2 decides. The two lines around it
      are already stale independently: the block runs `cd web && npm run ...`
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
- [ ] 0.6 Fix `sim/Fuegoize.hpp:22-23`'s divide-by-zero at full fuego. The
      cast is applied to the ternary's already-selected result, so at knob
      >= 0.9375 the divisor is 256 truncated to `uint8_t` — zero — and the next
      expression is `row % 0`. The firmware's own form has the cast on the
      modulo's result instead (`src/core/Parameter.hpp:143`), and
      `app/dsp/Fuegoize.hpp` ports that one and documents the discrepancy;
      move the cast to match. Nothing drives that path today, so it needs a
      test at maximum fuego. POSITIVE CONTROL required: build that test against
      the current form under `-fsanitize=undefined` and show the
      division-by-zero diagnostic, or the test is one that has never been
      observed to catch anything.
      This is the one code task
      `openspec/changes/archive/2026-08-22-frogg3rs-main-cutover-and-releases`
      archived undone. It reaches no shipping surface, and it is one line in a
      tree this change is already editing.

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

- [ ] 3b.1 Source it from `app/Resources/Icon.png` — the same image
      `build-launcher.sh:63` bundles as the app icon — rather than adding a
      second logo file to maintain by hand. It is 800x800 and 313 KB, far
      heavier than a header needs, so derive a web-sized copy. Say where the
      derivation happens: a build step keeps one source, a committed
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
