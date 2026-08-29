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
