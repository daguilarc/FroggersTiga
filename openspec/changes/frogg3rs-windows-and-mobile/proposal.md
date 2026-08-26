# Proposal — `frogg3rs-windows-and-mobile`

**Created 2026-08-26.** Everything still standing between Frogg3rs and the
people who actually use it: a Windows application that has never built, and a
published site that a phone cannot operate.

Supersedes `frogg3rs-desktop-reaches-downloaders`, which delivered its
hygiene, signing and release-documentation sections and stalled on Windows,
and absorbs `frogg3rs-web-mobile-ux`, which was sequenced behind it. Those two
were sequential and share a site republish, so they are one change here rather
than two that each deploy.

## Why the Windows work stalled

Not a toolchain mystery. Three attempts, each finding one error, because the
sources were never audited before a new compiler was asked to build them —
CI was used as the compiler.

- **Attempts 1–5 (`32583714787`–`32585757125`).** MinGW g++ could not compile
  JUCE's own headers: `memcpy`/`memset` missing from `std::` in
  `juce_UMPFactory.h` and `juce_Memory.h`, and `pointer_sized_int` resolving
  to 32-bit `int` on a 64-bit target in `juce_FloatVectorOperations.cpp` and
  `juce_HashMap.h`. Real, and correctly diagnosed: vendored JUCE code under a
  compiler JUCE does not primarily test, which is why the CMake/MSVC decision
  stands.
- **Attempt 6 (`32924518787`).** `cmake --build ... -- -j2` reached MSBuild,
  which spells parallelism `/m` and rejects `-j2` with `MSB1001`. The
  `-- -j2` idiom was copied from `app/vst`'s macOS build line; everything
  after `--` is generator-specific, and one working invocation was read as if
  it were the rule that produced it.
- **Attempt 7 (`32924746650`).** JUCE compiled clean under MSVC — that much of
  the decision is vindicated — and the build stopped on THIS repository's code:
  `app/FroggersMain.cpp:57,69` call
  `juce::MenuBarModel::setMacMainMenu`, which JUCE declares inside
  `#if JUCE_MAC` (`juce_MenuBarModel.h:151-182`). That file had already been
  read, with that call in the output, and was never audited for portability
  before a CMake target was written to compile it on Windows.

The predecessor then began editing `app/FroggersBundledDocs.hpp` inline to get
past it. That header compiles into the shipping macOS VST3 and AU editors
(`app/vst/FroggersPluginEditor.{hpp,cpp}`), so the fix would have altered a
shipping macOS path — the exact thing that change's own task 3.3 existed to
prevent. The work stopped there rather than continuing.

## Why the site is unusable on a phone

**Encoder drags are cancelled.** Sheaf's browser UI drives knobs with Pointer
Events and `setPointerCapture` (`External/Sheaf/projects/synth/browser/src/ui.ts`).
No element declares `touch-action`, so a phone browser reclaims the gesture
for scrolling: a real touch drag emits `pointerdown`, one or two
`pointermove`s, then `pointercancel`. The accumulated delta is flushed and the
knob does not move.

**Randomize/Reset are buried.** `FroggersUiSurface.hpp` emits the right block
as bank tabs → header → 16 encoder slots → Randomize → Reset. The mobile shell
(`app/browser/site/mobile-stack.mjs`) repositions whole blocks, not rows
inside one, so on a narrow viewport those two rows land below the entire
encoder grid and the sidebar, far from Play/Stop/Scene/BPM.

## The audit the Windows work is built on

Performed before writing anything, against the tree at `de64f44`.

**Clean — no Windows work needed:** Sheaf's runtime shell (`Shell.hpp`,
`Runtime.hpp`, `MainPane.hpp`, `AudioConfigPage.hpp`, `FilePage.hpp`,
`LauncherWindow.hpp`, `JuceRuntimeMainServices.hpp`,
`MidiConnectionManager.hpp`) contains no `JUCE_MAC`, no `__APPLE__`, no
bundle-layout path and no Objective-C symbol. `HostDataPaths.cpp:13` resolves the
data root through `juce::File::userApplicationDataDirectory`, which JUCE maps
per platform. JUCE modules are confirmed compiling under MSVC by attempt 7,
not by assumption.

POSITIVE CONTROL for that clean result: the same search DID hit
`External/Sheaf/projects/synth/runtime/juce_build.mk:41,204`, which writes
`Contents/MacOS` paths. That hit is off the Windows path —
`External/Sheaf/projects/synth/Makefile:52` is `build: $(LIB)` and never
includes `juce_build.mk`, so the standalone's libsynth step cannot reach it.
The instrument was live; the eight headers are clean because they are clean.

**The complete set of gaps, all in this repository's own code:**
1. `app/FroggersMain.cpp:57,69` — `setMacMainMenu` is macOS-only. The
   cross-platform equivalent is `DocumentWindow::setMenuBar`
   (`juce_DocumentWindow.h:169`), which attaches the same `MenuBarModel` as an
   in-window menu bar.
2. `app/FroggersBundledDocs.hpp:24-25` — locates documents at
   `Contents/Resources`, a macOS bundle layout that exists on no other
   platform. On Windows `currentApplicationFile` is the executable itself.
3. `app/standalone/CMakeLists.txt` copies neither `MANUAL.md` nor
   `QUICK_DICT.md`, so even a working menu would open nothing.

**The gap the audit missed, found in preflight: the link stage.** The three
above are the complete set of gaps in this repository's own COMPILATION. The
link has never run, and it crosses a toolchain seam nothing in the audit
names:

- `External/Sheaf/projects/synth/Makefile:1` is `CXX ?= clang++` with
  GCC-style flags (`:2`), and `:104` is `ar rcs` producing
  `build/libsynth.a`.
- `app/standalone/CMakeLists.txt:41` hardcodes that `.a` path and `:110-111`
  links it into a target MSVC compiled.
- `desktop-release.yml`'s `build-windows` job sets no `CXX` or `CC`, so
  `make` picks its own compiler entirely outside CMake's selection.

Whether MSVC's linker accepts a `ar rcs` archive built by that separate
compiler is UNVERIFIED and unverifiable here — it needs the Windows runner.
Task 1.5 marks it as a first attempt with a decided fallback, so a failure
there reads as expected discovery rather than as a new mystery.

## What Changes

**Windows**
- **`app/FroggersMain.cpp`**: the Help menu attaches through `setMacMainMenu`
  on macOS and `DocumentWindow::setMenuBar` elsewhere. The macOS branch is the
  existing call, unchanged.
- **`app/FroggersBundledDocs.hpp`**: the document is located per platform. The
  macOS branch is the existing expression, unchanged, because this header is
  compiled into the shipping VST3 and AU editors.
- **`app/standalone/CMakeLists.txt`**: copies both documents to where
  `OpenBundledDoc` looks on the platform being built, from the repository's
  single copy.

**Site**
- **`app/browser/site/site.css`**: `touch-action: none` on `Draw` nodes so the
  browser leaves knob drags alone; containers and non-canvas areas keep the
  default so the page still scrolls.
- **Mobile-only surface topology**: a `kRightRowsNarrow` table in
  `FroggersCellMap`, selected by a `narrowViewport_` flag that defaults false,
  consumed by the same `AppendRightBlock` emission code. The surface's own
  header comment anticipates exactly this: "a future mobile or VST topology
  would replace with a DIFFERENT table consumed by analogous emission code,
  without forking this surface."
- **Viewport signal**: the browser host dispatches `froggers.viewport.narrow`
  through the existing `dispatch-action` → wasm → `HandleAction` path.
- **Site header logo**: the application's own mark in the header, sourced from
  `app/Resources/Icon.png`, inside `.site-header` so the blank-frame guard's
  sampled band still excludes it.

**Both**
- **`.github/workflows/desktop-release.yml`**: the Windows artifact ships once
  it builds, and the release states both platforms. Three edits, none of them
  present today: `:105` `needs: [build-macos]` gains `build-windows`, a second
  `download-artifact` step pulls `windows-zip` alongside `macos-dmg` (`:115`),
  and the zip joins the `gh release create` argument list (`:139`). Coupling
  `needs:` is the decided behaviour: a red Windows job blocks the release,
  because the delta forbids stated coverage and actual coverage drifting
  apart, and an uncoupled job lets a macOS-only release look green while the
  documentation says both.
- **`MANUAL.md`**: stops saying a Windows build is in progress; states what a
  Windows downloader sees.
- **One republish** carries the site logo and the mobile work together.
- **`openspec/changes/frogg3rs-desktop-reaches-downloaders`** is archived by
  this change. It is superseded but still active, and its two ADDED
  requirements — a signature matching bundle contents, and a release stating
  what opening it requires — are DELIVERED in code yet absent from the live
  `frogg3rs-distribution` spec, which has never seen them. Superseding it
  without archiving it drops delivered requirements permanently.

## Non-goals

- Changing the macOS build, its bundle, or its signing — the predecessor's
  section 1 is delivered and must not be disturbed.
- Changing the VST3/AU plugin's behaviour on any platform. The shared
  documentation header is edited, so this is a claim to VERIFY, not assume.
- A Windows VST3, or changing how encoders render or how drag deltas compute.

## Impact

- Affected specs: `frogg3rs-distribution` (both platforms ship; the site
  carries the app's mark; the signature requirement the predecessor delivered
  gains its Windows case, where no Authenticode certificate exists), `froggers-sheaf-runtime-app` (the documentation
  requirement names a macOS-shaped mechanism), `froggers-web-host` (mobile
  control placement), new `frogg3rs-web-mobile-ux` (touch-gesture contract and
  surface-owned topology).
- The desktop release ships a Windows application for the first time, and the
  site becomes operable on a phone.
