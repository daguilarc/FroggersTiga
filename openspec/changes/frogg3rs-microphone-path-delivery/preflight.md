# Preflight — `frogg3rs-microphone-path-delivery`

Audited inline, 2026-08-29. Every claim below was read at the cited file:line
rather than carried from the proposal's own text.

## Claims checked and upheld

- 48/68 executed. `frogg3rs-browser-microphone-permission-path/tasks.md` holds
  68 checkboxes, 48 marked done.
- No file under `app/` includes `RuntimePages.hpp` or
  `RuntimeMainComponent.hpp`. 18 hits under `app/`, every one a comment.
- `RuntimePages -> PortableUI`, not back. `RuntimePages.hpp:8` includes
  `PortableUI.hpp`; `PortableUI.hpp` names `RuntimePages` nowhere.
- The app gate gained `check-microphone-usage`. `app/Makefile:224` lists it as a
  `test` prerequisite; the target is `app/Makefile:162`.
- The `kAudioInputRetry` sixth site is real. The pre-fix `IsAudioAction` body
  restated five names and omitted `kAudioInputPermission`; the diff replaces it
  with a read of `Actions::kAudioActions` (`RuntimeMainComponent.hpp:390`).
- Only the `audio-devices` project carries
  `--use-fake-device-for-media-stream` (`app/browser/e2e/playwright.config.mjs`
  :114 names the project, :120 the args). Four projects exist; all nine e2e
  specs are matched by one.
- The `"./protocol.js"` resolution failure is understood and documented at
  `External/Sheaf/projects/synth/browser/src/static-server.mjs:12-32`.
- The silent-fixture failure is remediated. Both Width tests and the Wet mix
  test carry positive controls that print their numbers
  (`app/FroggersAudioRoutingTests.cpp:2274,2284`).
- `kMaxWetMix` is genuinely single-sourced: declared `FroggersAppCore.hpp:1610`,
  read at :1839 and :1900. No second constant survives.
- The deleted `frogg3rs-version-single-source-and-windows-vst` was absorbed, not
  lost: its delta is folded into the predecessor's `frogg3rs-distribution`
  delta, and `MODIFIED` correctly became `ADDED` — no such requirement exists in
  `openspec/specs/frogg3rs-distribution/spec.md`. Its name survives only in the
  two predecessor documents that explain the absorption.
- Deleting Sheaf's `tests/static-server.mjs` leaves nothing dangling: every live
  reference is to `src/static-server.mjs`.
- `.gitignore`'s archive rule is effective — `openspec/changes/archive/` has 0
  tracked files, so the rule is not a no-op over already-tracked content.
- The Windows CI job marks its first attempts explicitly
  (`.github/workflows/vst-plugin.yml:100-108`).

## Blocking — execution is rejected until these are closed

### B1. The version family: 18 sites, 0 protected, requirement says otherwise

`SUPPORTED_BROWSER_ABI_VERSION` (6), `SUPPORTED_UI_PROTOCOL_VERSION` (2) and
`SUPPORTED_RUNTIME_CONFIG_VERSION` (1) are hand-written literals at 18 test
doubles: `runtime-core.spec.ts:177-179,211-213,252-254,363-365`,
`package-loader.spec.ts:296-298`, `midi-timing.test.mjs:203-205`.

FOUND 18. CHANGED 0. COVERED BY THE DRIFT CHECK 0 —
`version-drift.test.mjs:128` greps `abiVersion:\s*[0-9]+`, which matches none of
these shapes.

The predecessor promotes "A version that appears in more than one place has one
definition", whose text reads "Fixtures and test doubles are sites" and requires
each site to read the definition, be generated from it, or be asserted equal by
a check that fails on divergence. None of the three holds for any of the 18.
`version-drift.test.mjs:1-5` states that "every mirror is checked here"; that is
false for 18 mirrors.

### B2. "A rendered control is routable" is delivered for one router of six

`RuntimeMainComponent.hpp` routes six action families. Classified before
counting, because they are not the same shape:

- Pure restated lists, collapsible exactly as Audio was: `IsSidebarAction` :378
  (5 names), `IsAudioAction` :390 (collapsed), `IsFileAction` :402 (17 names),
  `IsSyncAction` :424 (6 names). FOUND 4, CHANGED 1.
- `IsControllersAction` :435 is a 20-name list OR'd with two `starts_with`
  prefixes. The list half duplicates; the prefix half cannot be expressed as
  membership, so collapsing it whole would destroy the rule.
- `IsAppPageAction` :469 is a single equality against `kAppBack`. There is no
  list to restate and nothing to collapse.

`TestEveryAudioPageActionIsRoutable` (`portable_ui_tests.cpp:3605`) walks the
Audio page only. `BuildSyncPageTree` and `BuildFilePageTree` are snapshot-driven
and already exercised in that file, so the same walk extends to both cheaply.
The sidebar's tree comes from `sidebarSurface_.BuildTree()` inside the component
(:195), not a standalone builder, so its walk belongs to a different binary.

The requirement's text — "Every action a runtime page can emit SHALL be routed
by the host that renders that page" — is a claim about all six.

### B3. The routable requirement is filed under the wrong capability

`openspec/specs/frogg3rs-distribution/spec.md`'s seven requirements are about
releases, artifacts, signatures and publishing. Runtime-UI action routing
belongs with `froggers-sheaf-runtime-app`, which already carries the runtime
chrome's control requirements.

### B4. The doc destination is computed twice in app/vst/CMakeLists.txt

`FROGGERS_VST_DOC_DEST` (the copy) and `FROGGERS_VST_DOC_CHECK_DIR` (the check)
are the same `if(APPLE)`/`else` path expression written twice. If they drift the
check inspects a directory the copy does not write, and passes.

### B5. Task 0.3's sweep is vacuous as written

0.3 binds the sweep to "EVERY directory this change's Impact names". Impact
names one openspec directory. The change touches `.github/workflows/`, the repo
root, `app/`, `app/browser/e2e/`, `app/dsp/`, `app/vst/`, `openspec/changes/`,
and in Sheaf `projects/synth/browser/` (with `src/`, `tests/`),
`projects/synth/include/synth/` (with `browser/`) and `projects/synth/tests/`.

### B6. Two delivery items have no task

Impact says this change archives the predecessor; no task does it. The commit
also carries three tracked deletions removing
`frogg3rs-version-single-source-and-windows-vst`, which no task accounts for.

## Non-blocking, fixed inside this change

- **N1** `playwright.global-setup.mjs:10` hard-codes port 4173 while
  `playwright.shared-config.mjs` sets `webServer.port: 4173`. On drift the fetch
  throws, the catch returns, and the currency check passes silently — a dead
  instrument inside the mechanism written to prevent dead instruments.
- **N2** `.github/workflows/vst-plugin.yml:76-78` cites `app/vst/CMakeLists.txt`
  sections "5.2"/"6.4"/"8.2" that do not exist in that file, and says "19 ctest
  cases across" three binaries where ctest now registers five tests.
- **N3** "26 files across two repositories" (proposal:11) is a frogg3rs-only
  count: 26 is exactly this change's frogg3rs entries with `External/Sheaf`
  counted as one submodule pointer. Sheaf carries 15 files of its own. 41 total.
- **N4** Tasks 0.2/0.3 and proposal failures 2/3 cite "§7 forward" and "§12.0
  sweep". The rule numbers forward enumeration under §9 and the hygiene sweep
  under §8.0.
- **N5** Task 4.2 assigns `.gitignore` to the guitar change. Its only edit is the
  archive-ignore rule, which belongs to neither feature and needs a stated home.
- **N6** The predecessor's scenario "A declaration is checked in the artifact,
  not in the source" is unbacked: `check_microphone_usage_strings.sh:20-21` reads
  `standalone/CMakeLists.txt` and `Frogg3rs-Info.plist`, both sources. Operator
  task 3.5 observes macOS behaviour but never opens the built bundle's plist.
- **N7** Task 1.2 predicts the failing e2e's cause and constrains the executor to
  it. A likelier mechanism is ordering: `void this.submitAudioDevices()`
  (`audio.ts:180`) is fire-and-forget, its catch sets
  `pendingEnumerationDiagnostic`, and the publish at :185 runs immediately after
  `await this.discoverRequestedInputChannels()` (:184). If the enumeration
  rejection settles second, the flag is empty and nothing is published.
- **N8** Running ctest on Windows has never happened and is not in the workflow's
  first-attempts list.

## Outside this change, reported not fixed

`openspec validate frogg3rs-guitar-and-solo-variants --strict` fails:
`external-ring-mod-mix/spec.md`'s MODIFIED requirement "External gate selects
VCO-only vs ring mod" contains no SHALL or MUST. That change is not this one's
to touch (task 4.2).
