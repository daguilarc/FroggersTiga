# Postflight — `frogg3rs-microphone-path-delivery`

Implementation against proposal. Divergence reported strictly.

## Gates: when each ran, and what moved since

All five ran after section 1's edits and section 2's rebuild. None is carried
forward from an earlier session.

| gate | result | inputs that moved before it ran |
|---|---|---|
| Sheaf synth suite | 2 failures, both `braid4_*_96000hz_*` | `RuntimePages.hpp`, `RuntimeMainComponent.hpp`, `ControllersPageUI.hpp`, `portable_ui_tests.cpp` |
| App suite | 314 [PASS] / 0 | `check_microphone_usage_strings.sh`, and the Sheaf headers above |
| Sheaf browser suite | 100/100 unit, 225 passed / 2 skipped | `version-drift.test.mjs`, `server-currency.mjs`, `static-server.mjs`, `playwright.shared-config.mjs`, `playwright.global-setup.mjs` |
| VST ctest | 5/5 | `app/vst/CMakeLists.txt` |
| frogg3rs e2e, all four projects | 60 passed / 0 | the wasm and site rebuilt in 2.1 |

The 96kHz failures are pre-existing on this machine. POSITIVE CONTROL: the
44100Hz and 48000Hz cases in the same binary passed at avg 2.54ms and 2.63ms
against 5.80ms and 5.33ms blocks, so the timing instrument was live.

## Every promoted scenario, and what checks it

`frogg3rs-distribution` (4 requirements, 12 scenarios):

- The shipped macOS application declares its microphone use — CHECKED,
  `check_microphone_usage_strings.sh`, in the app gate.
- A declaration is checked in the artifact, not in the source — CHECKED, same
  script, which now drives Sheaf's real `$(APP_BUNDLE)` rule and reads the key
  back out of the bundle it produced. Both failure paths proven.
- Moving the version moves every mirror — CHECKED, `version-drift.test.mjs`,
  now including the accessor stubs.
- A fixture cannot silently disagree — CHECKED, same file.
- A server older than the code is not silently reused — CHECKED,
  `server-currency.test.mjs`, and proven end-to-end against a real stale server.
- The stale case names itself — CHECKED, message read verbatim from that run.
- Documentation ships on every platform — CHECKED ON macOS,
  `FroggersVstDocsBundled_VST3`/`_AU`, and the files read directly out of both
  bundles. The Windows half is NOT YET DELIVERED: it ships with this change and
  cannot be observed until the new CI job runs once.
- The plugin release carries both platforms — NOT YET DELIVERED, same gate.
- The Audio Unit is not attempted off macOS — NOT YET DELIVERED. The
  configure-time guard's macOS half runs (it requires AU to be present, and
  configure passed); the off-macOS half has never executed.
- Release notes match what shipped — NOT YET DELIVERED. The body is generated
  in the release job; nothing checks it until a release runs.

`froggers-browser-package` (2 requirements, 6 scenarios): all six CHECKED by
`audio-devices.spec.mjs`, 9 passed against a site rebuilt from current sources.

`froggers-sheaf-parameter-model` (2 requirements, 10 scenarios after this
change removed one): all ten CHECKED in the app suite. The removed one is
recorded in tasks 4a.1 — "A mono device still receives the sum" had no check
and could not get one from the existing harness, so it was withdrawn rather than
archived as an unverified claim.

## Enumeration re-run against the diff

Every name this change introduced, with its definition and readers:

- `kSidebarActions`, `kSyncActions`, `kFileActions` — 1 definition each in
  `RuntimePages.hpp`, read by their router and (for sync and file) by the
  routable check. `kControllersActions` — 1 definition in
  `ControllersPageUI.hpp`, read by its router.
- `RequireEveryEmittedActionIsRoutable` — 1 definition, 4 call sites.
- `ACCESSOR_DEFINITIONS`, `STUB_SHAPES` — 1 definition each, used only by the
  check that owns them.
- `STATIC_SERVER_PORT`, `SERVER_IDENTITY_PATH` — 1 definition each in
  `server-currency.mjs`, read by the server, the Playwright config and the
  currency check.
- `froggers_vst_doc_dir` — 1 definition, 2 call sites (the copy and the check).

What writing this made redundant: `IsOneOf`'s `initializer_list` overload lost
its last caller and was removed, and `#include <initializer_list>` went with it.

FOUND vs CHANGED, reported including zeros:

- Version literals in test doubles: FOUND 18, CHANGED 0, now COVERED 18.
- Routers restating a list: FOUND 4 pure-list, CHANGED 3 (audio was already
  done); 1 hybrid shares its fixed half; 1 single-equality left alone.
- `4173`: FOUND 117, CHANGED 3. The other 114 are `http://127.0.0.1:4173/...`
  base URLs in spec files — a pre-existing duplication of a different fact,
  deliberately not touched, reported rather than left implied.
- Doc-destination path expression: FOUND 2, CHANGED 2 into 1.

## Operator items, each with a named code path

4.1-4.4 — `audio.ts`'s `requestInputPermissionNow` and `main.ts`'s
`consumePendingAudioRequest` handling `PendingAudioRequest.requestPermission`.
4.5 — `app/Frogg3rs-Info.plist`'s `NSMicrophoneUsageDescription`, copied into
the bundle by `juce_build.mk`'s `$(APP_BUNDLE)` rule.
4.6, 4.7 — `.github/workflows/vst-plugin.yml`'s `build-and-test-windows` job and
the release body it generates.

## Divergence from the proposal

The proposal planned no source change beyond the one unverified `audio.ts`
edit. Seven blocking preflight findings changed that: sections 1 and 4a of the
task list are work the proposal did not anticipate, all of it duplication or
overclaim already in the tree. The `audio.ts` edit itself needed no change — it
passed on first run.

## Reactive fix: source importing build output

Found by the Pages build going red after delivery, not by any gate here. Too
small for its own change; recorded under the same discipline.

**What it was.** `src/static-server.mjs` imported `../dist/src/protocol.js` --
source reaching into build output. That inversion makes the module's behaviour
depend on which tree it is loaded from, and it is loaded from two: `src/`, where
Playwright launches it, and `dist/src/`, the compiled copy frogg3rs's
`serve-site.mjs` imports for `contentTypeForPath`. From the second, the
specifier resolves one directory too deep.

**Why nobody saw it.** A stale `dist/dist/src/protocol.js` existed locally --
debris from tsc having once compiled its own output. It answered exactly the
wrong lookup. `dist/` is gitignored, correctly, so CI checks out clean and the
lookup 404s. Local passed with and without the defect; only CI could tell the
truth.

**Why preflight missed it.** The comment above that import enumerated the
consumers loading it from `src/` and concluded the file "RUNS from src/".
Preflight verified that claim instead of asking who else loads it and from
where. A reference resolves where it is EVALUATED; one of two evaluation sites
was checked and treated as all of them.

**The fix.** The three version literals moved to `src/protocol-versions.js`;
`protocol.ts` re-exports them, so every TypeScript consumer is unchanged and
there is still one literal per version. `static-server.mjs` imports the sibling,
which is copied into `dist/src/` beside it and resolves identically from either
tree. The first attempt resolved the root by branching on directory depth --
rejected: that branch existed only to accommodate the inversion, which is the
signal that the constraint is suspect rather than the implementer.

**What postflight caught that preflight did not**, both by running gates rather
than reading:

- Adding a module broke six Sheaf tests. `publish-site.mjs` carries a
  hand-maintained `browserRuntimeModules` list and `publish-site.test.mjs`
  asserts the published directory equals it exactly. That family's drift check
  fired. Module added to the list.
- Naming it `.mjs` broke all sixty frogg3rs e2e tests as 45-second timeouts.
  `package-catalog.mjs` copies `dist/src/*.js` into the browser runtime and
  deliberately drops `.mjs` as Node-only tooling. A browser-fetched module
  cannot be one. Renamed to `.js`; the extension is load-bearing.

**Checks added, each proven to fail by breaking it once:**

- both copies of the static server load (fails if a path is wrong at either
  depth)
- the build has not compiled its own output into a nested dist
- no browser-fetched module imports a Node-only `.mjs`

**Runs voided along the way, recorded because each looked like a pass:** a
positive control read off a stale binary; a green local smoke that owed its
result to `dist/dist`; a four-test "failure" that was a stale published root;
a `BUILD_EXIT=0` that was `tail`'s exit rather than tsc's; and two gate chains
run concurrently over the same fixed loopback ports.

**Gates after the fix**, all re-run clean and sequentially: Sheaf browser suite
exit 0 (103 node tests, 225 playwright passed / 2 skipped); build-browser exit
0; local smoke exit 0; frogg3rs e2e exit 0, 60 passed in 22.0s against 36
minutes of timeouts while broken -- the runtime is the control that the app
boots at all.

## Windows plugin: five failures, each found by running it

The predecessor marked the Windows VST3 job's steps first attempts. It had
never run. This records what running it actually found, in order, because each
failure was only reachable once the one before it was fixed.

1. **Compile.** `M_PI` undeclared. MSVC does not define it in `<cmath>` without
   `_USE_MATH_DEFINES`, and the DSP headers use it in twenty-two places.
   `app/standalone/CMakeLists.txt` has carried that guard since the Windows
   standalone shipped -- traced, then mirrored onto the plugin's shared-code
   target, PUBLIC so the format targets and the CTest binaries get it too.
2. **Link.** LNK2019 on every `MasterClock` and `ClockBlockPlan` symbol.
   Sheaf's Makefile drives `CXX ?= clang++` outside CMake's compiler
   selection, so `make` on that runner builds `libsynth.a` with a different
   toolchain than the one linking it. The standalone hit this exact error and
   solved it by compiling `src/*.cpp` into its own target on Windows; same
   split here.
3. **ctest ran nothing.** "Test not available without configuration." That
   generator is multi-config, so the configuration comes from the ctest
   invocation rather than the build directory. `--build-config Release`.
   The step's own comment also claimed three tests where there are four.
4. **Three binaries died instantly.** ctest called it SEGFAULT and printed
   nothing. A diagnostic step captured the exit code, which is the only thing
   that separates the two causes that fit: `0xC00000FD`, STATUS_STACK_OVERFLOW
   -- not an access violation, not an unresolved image. They construct
   `FroggersPluginProcessor` as a local; it embeds
   `synth::Engine<FroggersApp>`, measured at 1,204,864 bytes, against a
   Windows thread's 1 MB. macOS gives its main thread 8 MB, which is why the
   same construction is comfortable there. `/STACK:8388608` on the three test
   binaries -- the stack raised rather than fifty-five declarations rewritten,
   one mechanism instead of fifty-five copies of one edit.
5. **The editor test's own guard.** `PumpPendingCallAsync()` existed only for
   macOS and hard-fails by design elsewhere. JUCE delivers `callAsync` on
   Windows by posting to the message thread's queue, so the peek/dispatch loop
   is what `CFRunLoopRunInMode` is on macOS, with the same bounded-budget shape
   for the reason the macOS half documents.

Result: build, bundle, documentation inside the bundle, and all four Windows
tests pass. macOS was re-verified at 5/5 after every one of these, twice from a
clean configure.

## Process divergence, recorded rather than tidied away

These five fixes were each committed and pushed to `main` as they were found,
without a postflight between them. That is the pipeline inverted: six
deliveries in a row driven by whatever CI said last. It worked only because
each failure was unambiguous and the macOS gate was re-run every time. The
honest cost is that no single gate result covered the whole set until this
sweep.

## Enumeration against the final diff

- `FROGGERS_SHEAF_SYNTH_SOURCES`: FOUND 2 -- `app/standalone/CMakeLists.txt`
  and now `app/vst/CMakeLists.txt`, the same twenty-line Windows glob in two
  build files. CHANGED 0. This is a real duplication: Sheaf changing its build
  shape needs both edited, and one will drift. NOT FIXED -- the fix is a shared
  `.cmake` module both include, and both files are green as they stand, so it
  is named here rather than done late.
- `FROGGERS_VST_TEST_BIN`: 1 definition, loop-local to the stack block.
- `froggers_vst_doc_dir`: 1 definition, 2 call sites, unchanged by this work.

## Gates, and whether their inputs moved

- VST ctest (macOS): re-run after every one of the five fixes, 5/5, twice from
  a clean configure. Current.
- Windows VST job: green on `b736008`, the current `main`.
- GitHub Pages: green on `b736008`.
- App suite, Sheaf synth suite, Sheaf browser suite, frogg3rs e2e: last run
  before these five fixes. CARRIED FORWARD, and the reason is checked rather
  than assumed -- every one of the five touched only `app/vst/` and
  `.github/workflows/`, so nothing any of those four gates compiles or loads
  has changed since they ran.
