# Proposal — `frogg3rs-windows-standalone`

**Created 2026-08-26.** Supersedes `frogg3rs-desktop-reaches-downloaders`,
which delivered its hygiene, signing and release-documentation sections and
then stalled on Windows. This change finishes the Windows application.

## Why the predecessor stalled

Not a toolchain mystery. Three attempts, each finding one error, because the
sources were never audited before a new compiler was asked to build them —
CI was used as the compiler.

- **Attempt 1 (`32583714787`–`32585757125`, five runs).** MinGW g++ could not
  compile JUCE's own headers: `memcpy`/`memset` missing from `std::` in
  `juce_UMPFactory.h` and `juce_Memory.h`, and `pointer_sized_int` resolving
  to 32-bit `int` on a 64-bit target in `juce_FloatVectorOperations.cpp` and
  `juce_HashMap.h`. Real, and correctly diagnosed: that is vendored JUCE code
  under a compiler JUCE does not primarily test, which is why this change
  keeps the CMake/MSVC decision.
- **Attempt 2 (`32924518787`).** `cmake --build ... -- -j2` reached MSBuild,
  which spells parallelism `/m` and rejects `-j2` with `MSB1001`. The `-- -j2`
  idiom was copied from `app/vst`'s macOS build line. Everything after `--`
  is generator-specific; one working invocation was read as if it were the
  rule that produced it.
- **Attempt 3 (`32924746650`).** JUCE and Sheaf's `libsynth.a` compiled clean
  under MSVC — the decision is vindicated — and the build stopped on THIS
  repository's code: `app/FroggersMain.cpp:57,69` call
  `juce::MenuBarModel::setMacMainMenu`, which JUCE declares inside
  `#if JUCE_MAC` (`juce_MenuBarModel.h:151-182`). That file was read during
  the predecessor and the call was in the output; it was never audited for
  portability before a CMake target was written to compile it on Windows.

The predecessor then began editing `app/FroggersBundledDocs.hpp` inline to get
past it. That header is compiled into the shipping macOS VST3 and AU editors
(`app/vst/FroggersPluginEditor.{hpp,cpp}`), so the fix would have altered a
shipping macOS path — which the predecessor's own task 3.3 exists to prevent.
The work stopped there rather than continuing.

## The audit this change is built on

Performed before writing anything, against the tree at `de64f44`.

**Clean — no Windows work needed:**
- Sheaf's runtime shell (`Shell.hpp`, `Runtime.hpp`, `MainPane.hpp`,
  `AudioConfigPage.hpp`, `FilePage.hpp`, `LauncherWindow.hpp`,
  `JuceRuntimeMainServices.hpp`, `MidiConnectionManager.hpp`) contains no
  `JUCE_MAC`, no `__APPLE__`, no bundle-layout path, and no Objective-C
  symbol.
- `HostDataPaths.cpp` resolves the data root through
  `juce::File::userApplicationDataDirectory`, which JUCE maps per platform.
- JUCE modules and `libsynth.a` are confirmed compiling under MSVC by
  attempt 3, not by assumption.

**The complete set of gaps, all in this repository's own code:**
1. `app/FroggersMain.cpp:57,69` — `setMacMainMenu` is macOS-only. The
   cross-platform equivalent is `DocumentWindow::setMenuBar`
   (`juce_DocumentWindow.h:169`), which attaches the same `MenuBarModel` as
   an in-window menu bar.
2. `app/FroggersBundledDocs.hpp:24-26` — locates documents at
   `Contents/Resources`, a macOS bundle layout that exists on no other
   platform. On Windows `currentApplicationFile` is the executable itself.
3. `app/standalone/CMakeLists.txt` copies neither `MANUAL.md` nor
   `QUICK_DICT.md` anywhere, so even a working menu would open nothing.

## What Changes

- **`app/FroggersMain.cpp`**: the Help menu attaches through
  `setMacMainMenu` on macOS and `DocumentWindow::setMenuBar` elsewhere. The
  macOS branch is the existing call, unchanged.
- **`app/FroggersBundledDocs.hpp`**: the document is located per platform —
  `Contents/Resources` on macOS, beside the executable elsewhere. The macOS
  branch is the existing expression, unchanged, because this header is
  compiled into the shipping VST3 and AU editors and its macOS behaviour is
  not this change's to alter.
- **`app/standalone/CMakeLists.txt`**: copies both documents to where
  `OpenBundledDoc` looks on the platform being built, from the repository's
  single copy, so no second checked-in copy exists.
- **`.github/workflows/desktop-release.yml`**: the Windows artifact ships in
  the desktop release once it builds, and the release states both platforms.
- **`MANUAL.md`**: stops saying a Windows build is in progress.

## Non-goals

- Changing the macOS build, its bundle, or its signing. The predecessor's
  section 1 is delivered and this change must not disturb it.
- Changing the VST3/AU plugin's behaviour on any platform. The shared
  documentation header is edited, so this is a claim to VERIFY, not assume.
- A Windows VST3. The plugin builds `FORMATS VST3 AU` on a macOS runner and
  is out of scope here.

## Impact

- Affected specs: `frogg3rs-distribution` (the desktop release covers both
  platforms), `froggers-sheaf-runtime-app` (the documentation requirement
  names a mechanism that is macOS-shaped).
- The desktop release ships a Windows application for the first time.
- Carried forward from the predecessor and still open: the site header logo,
  and the closing gate sweep.
