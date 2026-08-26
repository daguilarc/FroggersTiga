# Tasks — `frogg3rs-windows-standalone`

Supersedes `frogg3rs-desktop-reaches-downloaders`. That change's sections 0, 1
and 2 are delivered and committed; its Windows section is replaced by this
change, and its two unfinished remainders are carried in section 4 below.

Gates: `cd app && nice make -j2 test` (301/301 at `de64f44`);
`cmake -S test/firmware -B test/firmware/build && ctest` (3/3);
`app/vst` ctest (3/3) — this change edits a header the plugin compiles, so
the plugin suite is a gate here, not a formality; the Daisy firmware ARM
build (`nice make -C src/FroggersTiga -j2`, `dec=278412` unchanged); and the
Windows job itself. Never above `-j2`, always `nice`.

**Every fix below is written from the audit in the proposal, not discovered
by a CI round-trip.** If a Windows run fails on something the audit did not
name, that is the audit being wrong — record what it missed and why before
fixing it, so the next platform port does not repeat the same discovery.

## 0. Hygiene

- [ ] 0.1 Sweep `app/standalone/` and the Windows half of
      `desktop-release.yml`, both written by the predecessor and neither ever
      run to completion. Report anything the CMake target carries because
      `app/vst/CMakeLists.txt` carries it rather than because this target
      needs it — the `nice`/`-j2` wrapper in particular is a macOS
      machine-specific cap that has no meaning on a runner.
- [ ] 0.2 `app/FroggersMain.cpp:80,247` and `app/FroggersBundledDocs.hpp`'s
      header comment both describe the macOS main menu as the mechanism. Once
      it is one of two, those comments describe half the code. Fix them with
      the change, not after it.

## 1. The app builds and runs on Windows

- [ ] 1.1 Attach the Help menu per platform in `app/FroggersMain.cpp`:
      `setMacMainMenu` under `#if JUCE_MAC` (the existing call, byte for
      byte), `window_->setMenuBar(&helpMenu_)` otherwise
      (`juce_DocumentWindow.h:169`). Detach symmetrically in `shutdown()`,
      before `window_.reset()` — the existing ordering comment at `:247`
      explains why that order matters and stays true for both branches.
- [ ] 1.2 Locate the documents per platform in
      `app/FroggersBundledDocs.hpp`: `Contents/Resources` under
      `#if JUCE_MAC` (the existing expression unchanged), the executable's
      own directory otherwise.
      THIS HEADER IS COMPILED INTO THE SHIPPING macOS VST3 AND AU EDITORS
      (`app/vst/FroggersPluginEditor.{hpp,cpp}`). The macOS branch must be
      the same code it is today, and 3.1 verifies that against the plugin
      suite rather than assuming it.
- [ ] 1.3 Copy `MANUAL.md` and `QUICK_DICT.md` in
      `app/standalone/CMakeLists.txt` to wherever 1.2 looks on the platform
      being built, from the repository's single copy. No second checked-in
      copy. On macOS this target is not the shipping path, so the copy exists
      there to keep the two branches testable, not to ship.
- [ ] 1.4 The Windows job builds. Report what it takes. If it fails on
      something outside the audit's list, record the gap in this file before
      fixing it.

## 2. The Windows build is signed, or says why not

- [ ] 2.1 `codesign` and `spctl` are macOS tools and have no Windows
      equivalent this project can use: Authenticode signing needs a
      certificate that does not exist here, exactly as notarization does on
      macOS. State that plainly as the Windows counterpart of the
      predecessor's section 1 rather than leaving the asymmetry unexplained,
      and say what a Windows downloader sees instead — SmartScreen's
      unrecognised-app prompt — in the same `MANUAL.md` section that already
      describes the macOS step.
- [ ] 2.2 The macOS signing gate does not regress: `app/build-launcher.sh`
      and `app/vst/CMakeLists.txt` still sign and verify, and the
      `if(APPLE)` guard in `app/standalone/CMakeLists.txt` still means the
      Windows target never invokes a tool it does not have.

## 3. Nothing else moved

- [ ] 3.1 The plugin is unaffected by 1.2. Build `app/vst` and run its ctest
      (3/3), and confirm the macOS document path is byte-identical by reading
      the preprocessed macOS branch, not by reasoning about the `#if`.
      POSITIVE CONTROL: the assertion must be able to fail — show the check
      catching a deliberately wrong macOS path before trusting it green.
- [ ] 3.2 The macOS standalone is unaffected: `app/build-launcher.sh` builds,
      signs, verifies, and the bundle's file inventory is the same six paths
      as at `de64f44`. App suite 301/301, firmware target 3/3, Daisy ARM
      build `dec=278412`.

## 4. Ship, and the predecessor's remainders

- [ ] 4.1 The desktop release ships both platforms, and `MANUAL.md` stops
      saying the Windows build is in progress — the `frogg3rs-distribution`
      delta's own scenario.
- [ ] 4.2 CARRIED FORWARD: the site header logo
      (`frogg3rs-desktop-reaches-downloaders` section 3b, never started). Its
      brief stands as written there; it is unblocked and independent of
      everything above.
- [ ] 4.3 CARRIED FORWARD: the closing gate sweep — all gates green with
      counts, a duplication pass over the whole diff, and a check that no
      surviving script, workflow, spec or manifest names a path that no
      longer resolves.
- [ ] 4.4 OPERATOR: download the published Windows artifact on a real
      Windows machine and confirm it opens, recording what SmartScreen shows.
      No CI check substitutes for this, the same way 4.3 of the predecessor
      could not substitute for opening the `.dmg`.
