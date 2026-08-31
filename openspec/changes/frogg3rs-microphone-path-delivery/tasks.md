# Tasks — `frogg3rs-microphone-path-delivery`

Successor to `frogg3rs-browser-microphone-permission-path`. That change's code
is in the tree, uncommitted. This one delivers it.

There is no time pressure. Limits on what is printed are not limits on what is
checked.

## 0. Rules this change runs under

Not preamble. Each one is a procedure, and each exists because its absence cost
something in the predecessor.

- [ ] 0.1 EVERY enumeration is finished in the same breath it is run. Print the
      hits, then state what was done with EACH hit, including the ones needing
      nothing. The predecessor printed `RuntimeMainComponent.hpp`'s
      `IsAudioAction` line and read past it, shipping a dead button.
- [ ] 0.2 EVERY named concept gets the forward enumeration, not the interesting
      one. List every constant, helper, type, predicate and sentinel touched,
      and grep each by operand. Report FOUND vs CHANGED per concept, zeros
      included. Classify before collapsing: members of a family are not always
      the same shape, and a count that ignores that is wrong in both directions.
- [ ] 0.3 The hygiene sweep covers EVERY directory the Impact section names,
      submodules included. Name each directory swept.
- [ ] 0.4 No precedent is cited until it has been traced. "X does it this way"
      is a claim about how X RESOLVES, not about how X reads.
- [ ] 0.5 Before believing any measurement, state what would have moved if the
      instrument were dead, and check it. A binary's mtime against its source,
      a fixture's output peak, machine load during a timing run.

## 1. Preflight's blocking findings, before anything else

Each is duplication or an overclaim already in the tree. They come first
because the gates in section 3 would otherwise be run twice.

- [x] 1.1 DONE. FOUND 18 stub sites (3 accessors x 6 locations), CHANGED 0 literals, COVERED 18 by a new check in `version-drift.test.mjs`. POSITIVE CONTROL: `midi-timing.test.mjs`'s ABI stub set to 99 -- the check failed naming that exact file:line; restored, 5/5. Browser node suite 100/100. Original text: The version family is 18 unprotected sites. `runtime-core.spec.ts`
      :177-179,211-213,252-254,363-365, `package-loader.spec.ts`:296-298 and
      `midi-timing.test.mjs`:203-205 hand-write 6/2/1 for the ABI, UI protocol
      and runtime config versions. `version-drift.test.mjs`'s fixture test greps
      `abiVersion:\s*[0-9]+` and matches none of them. Extend that file with a
      check over the accessor-stub shapes, asserting each literal against
      `protocol.ts`. POSITIVE CONTROL: break one stub, watch the check name it,
      restore.
- [x] 1.2 DONE. Classified first. FOUND 4 pure-list routers (sidebar, audio, file, sync), CHANGED 3 (audio was already done) -- each now reads a `k*Actions` array in `RuntimePages.hpp`. `IsControllersAction`'s fixed half reads `kControllersActions` in `ControllersPageUI.hpp`; its two `starts_with` rules stay, since an action carrying a composed index is not membership. `IsAppPageAction` is one equality, unchanged. The `initializer_list` overload of `IsOneOf` lost its last caller and went, with its now-dead `#include`. Original text: Four routers restate a list the page already owns. `kAudioActions`
      collapsed one. Give the sidebar, File and Sync surfaces the same treatment
      in `RuntimePages.hpp`, and have `IsSidebarAction`, `IsFileAction` and
      `IsSyncAction` read those arrays. `IsControllersAction` shares its fixed
      half only — its two `starts_with` rules are not membership.
      `IsAppPageAction` is one equality and is left alone. Report FOUND vs
      CHANGED.
- [x] 1.3 DONE. `TestEveryRuntimePageActionIsRoutable` walks Audio, Sync and two File states through one helper, each with its own emitted-count positive control. POSITIVE CONTROL: removing `kSyncBack` failed naming the Sync page; removing `kFileBack` failed naming the File page. The first attempt at the File break ran against a stale binary and reported the Sync message -- VOID, not negative; re-run after `rm -f` the binary. portable_ui_tests, runtime_main_component_tests, controllers_page_ui_tests, browser_audio_device_tests all exit 0. Original text: Extend `TestEveryAudioPageActionIsRoutable` to the File and Sync
      pages, whose builders are snapshot-driven and already exercised in
      `portable_ui_tests.cpp`. Keep the emitted-count positive control per page:
      a page that emits nothing passes the loop without checking anything.
- [x] 1.4 DONE. `froggers_vst_doc_dir()` is the one definition; the copy and the check both call it. The path expression appears once. Original text: `app/vst/CMakeLists.txt` computes the doc destination twice, once for
      the copy and once for the check. One definition, read by both — if they
      drift the check passes against a directory the copy never wrote.
- [x] 1.5 DONE. FOUND 117 occurrences of `4173`. 3 are the server-port fact (the bind, the Playwright wait, the currency fetch) and now read `STATIC_SERVER_PORT` from `server-currency.mjs`; the `__server-identity` path joined it. The other 114 are `http://127.0.0.1:4173/...` base URLs in spec files -- a pre-existing duplication of a different fact, CHANGED 0, reported not fixed. Also added `src/server-currency.mjs` to `SERVER_IDENTITY_SOURCES`: the server imports its port and source list from there, so its contents are baked into the responses and were undated. Original text: `playwright.global-setup.mjs` hard-codes port 4173 while
      `playwright.shared-config.mjs` sets `webServer.port: 4173`. One
      definition. On drift the fetch throws, the catch returns, and the currency
      check passes silently — the exact dead instrument it exists to prevent.
- [x] 1.6 DONE. The ctest comment names the five registered tests instead of citing sections that do not exist; the Windows job's first-attempt list gained running the suite there, and its own ctest step records three tests rather than five. Original text: `.github/workflows/vst-plugin.yml` cites `app/vst/CMakeLists.txt`
      sections "5.2"/"6.4"/"8.2" that do not exist in that file, and claims "19
      ctest cases across" three binaries where ctest now registers five tests.
      Say what is there.
- [x] 1.7 DONE, with a check rather than a downgrade. `check_microphone_usage_strings.sh` now drives Sheaf's real `$(APP_BUNDLE)` rule with a stub binary and empty source prerequisites -- the shape traced from `scripts/check_app_bundle_plist.sh`, not cited from it -- and reads `NSMicrophoneUsageDescription` back out of the produced `Contents/Info.plist`. POSITIVE CONTROL: pointing the read at a missing path failed naming it; a mismatched value failed naming both sides. Original text: The predecessor's scenario "A declaration is checked in the artifact,
      not in the source" has no check: `check_microphone_usage_strings.sh` reads
      two source files. Either give it one that opens the built bundle's plist,
      or mark the scenario as delivered by operator observation and say which
      task observes it. Do not leave it promoted and unbacked.

## 2. The one unverified edit

- [x] 2.1 DONE, PASSES. Rebuilt wasm and repackaged the site first: served `dist/site/sheaf-runtime/audio.js` mtime moved to 1788039047 against `audio.ts`'s 1788036616 and carries `pendingEnumerationDiagnostic` five times, matching Sheaf's compiled copy -- so the run exercised the edit rather than a stale bundle, and both e2e ports were clear beforehand so no server could be reused. audio-devices: 9 passed, 0 failed. The previously failing case now passes. Original text: `audio.ts` reports an enumeration failure once the requested channel
      count is known. Written, typechecks, NEVER RUN. Rebuild the wasm, package
      the site, and run the `audio-devices` e2e project. It was 8 pass / 1 fail
      before this edit; the failing case is
      "an enumeration failure is distinguishable from a machine with no
      devices". Record the result either way.
- [x] 2.2 NOT REACHED. 2.1 passed, so neither the reporting hypothesis nor the ordering one had to be settled. The ordering race is real in principle -- the submission is fire-and-forget -- but the enumeration rejection settles before discovery returns in practice, which is what the passing case shows. Original text: If it still fails, trace before fixing, and do not treat either of
      these as established: the report may never reach the status line, or the
      flag may never be set in time. `void this.submitAudioDevices()`
      (`audio.ts:180`) is fire-and-forget; its catch sets
      `pendingEnumerationDiagnostic`; the publish at :185 runs immediately after
      `await this.discoverRequestedInputChannels()` (:184). If the enumeration
      rejection settles second the flag is empty and nothing is published.
      Establish which by instrumenting, then fix the mechanism rather than
      relaxing the test.
- [x] 2.3 DONE by a check that runs, not by a click. `browser_audio_device_tests.cpp` asserts `ConsumePendingAudioRequest` is unarmed, dispatches `kAudioInputRetry` with the selection at No Input, and requires `kReleaseAudioRequest` -- "retrying while the selection is No Input reacquires that same release, not an index" -- then requires the consumed request does not repeat. A second case covers the selection whose device left the submitted list. The leading unarmed assertion is the positive control. Binary rebuilt today and exits 0. The state is post-denial rather than a literally fresh page; said rather than rounded up. Original text: CARRIED, no successor counterpart existed: confirm the Retry dead end
      by RUNNING it. On a fresh page with No Input selected, clicking Retry must
      be shown to arm the release sentinel rather than a capture request. The
      predecessor's step-7 trace is structural and this is what makes it
      evidence.
- [x] 2.4 DONE. `--use-fake-device-for-media-stream` makes enumeration report populated, labelled devices without permission, and `--use-fake-ui-for-media-stream` auto-grants capture -- so under this project Chromium never shows a real prompt and never produces a real denial. The spec works around both by replacing `navigator.mediaDevices` wholesale with an in-page stub (`installUnpermittedMediaDevices`), so every permission-path assertion here is about the bridge's response to a stub, not about Chromium's media stack. States no test can currently enter: a real permission prompt granted or denied by a person; permission persisting through the browser's own store across a reload; real captured audio reaching the DSP; and the fake device's synthetic signal standing in for a microphone means no test can distinguish a working capture from a silent one. Operator items 4.1-4.3 are exactly these and remain the only cover. Original text: CARRIED: review the HARNESS, not just the tests. The `audio-devices`
      project is the only one launched with `--use-fake-device-for-media-stream`.
      Enumerate what that flag makes untestable and say which of this feature's
      states no test can currently enter.

## 3. Gates, all four, after 1 and 2

- [x] 3.1 PASS as predicted. Exactly 2 failures, both `braid4_*_96000hz_*` deadline tests. POSITIVE CONTROL: the 44100Hz and 48000Hz cases in the same binary passed (avg 2.54ms and 2.63ms against 5.80ms and 5.33ms blocks), so the timing instrument was live and the machine was not merely loaded. Nothing else failed. Original text: Sheaf synth suite. Expect exactly 2 failures, both the known 96kHz
      braid4 deadline tests; anything else is this change's.
- [x] 3.2 PASS. 314 [PASS] lines, 0 failures, exit 0 -- the recorded 314/0 reproduced. `check-microphone-usage` ran inside the gate for the first time and passed, including its new read of the built bundle. `check-no-juce` and `check-no-frozen-deps` also green. Original text: App suite. Last 314/0. NOT stale by the Sheaf header edits -- no file
      under `app/` includes `RuntimePages.hpp` or `RuntimeMainComponent.hpp`,
      and the dependency runs `RuntimePages -> PortableUI`, not back. It IS
      stale because `make -C app test` gained a `check-microphone-usage`
      prerequisite that has never run inside the gate.
- [x] 3.3 PASS. node unit 100/100, playwright 225 passed / 2 skipped / 0 failed, exit 0. Additionally proved the currency wiring is not inert: started the static server, touched its source so the process predated it, and ran one spec -- globalSetup refused with `STALE STATIC SERVER ... started 2s BEFORE its own sources were last changed`, naming all three sources including the `src/server-currency.mjs` this change added to the list. Original text: Sheaf browser suite. Last 225/0, stale by one `audio.ts` edit and by
      section 1's edits to `version-drift.test.mjs` and the routers.
- [x] 3.4 PASS. 5/5, including FroggersVstDocsBundled_VST3 and _AU after 1.4 collapsed the path to one definition. Read the artifacts directly as well: MANUAL.md and QUICK_DICT.md are inside both Contents/Resources, MANUAL.md carrying this build's timestamp. Original text: VST ctest. Last 5/5, stale by 1.4.
- [x] 3.5 PASS. All four projects, 60 passed / 0 failed, exit 0. Run against the site rebuilt in 2.1, with both ports clear beforehand so nothing was reused. Original text: frogg3rs e2e, every project, not only `audio-devices`.

## 4. Operator — each already has a named code path

The predecessor recorded these under its own 5.0 gate. They are carried, not
re-derived.

- [ ] 4.1 OPERATOR: a browser never granted permission reaches a prompt.
- [ ] 4.2 OPERATOR: after granting, the Input list names the real microphone.
- [ ] 4.3 OPERATOR: selecting it starts capture from that device.
- [ ] 4.4 OPERATOR: loading the page prompts for nothing on its own.
- [ ] 4.5 OPERATOR: build with `app/build-launcher.sh`, select an input device,
      and record what macOS does now that the bundle declares
      `NSMicrophoneUsageDescription` — prompt, silent denial, or termination.
      This is the only remaining behavioural unknown in the desktop half.
- [ ] 4.6 OPERATOR: the Windows VST3 loads in a Windows DAW and makes sound.
      UNBLOCKED: the job has now run green -- build, bundle, bundled
      documentation, and all four Windows tests. Five failures were found and
      fixed getting there; see postflight. The artifact still has to be opened
      in a real DAW, which no check covers.
- [ ] 4.7 OPERATOR: the unsigned Windows plugin's first-load warning matches
      what the release body says.

## 4a. Postflight finding: one promoted scenario had no check

- [x] 4a.1 DONE. The predecessor's `froggers-sheaf-parameter-model` delta
      promoted "A mono device still receives the sum". Nothing checks it:
      `numOutputChannels` is set in exactly one place in the whole app suite
      (`FroggersAudioRoutingTests.cpp:1221`, to 2), and the fold at
      `FroggersAppCore.hpp:1168-1176` has never executed under test. Traced why
      it cannot be reached from the existing harness rather than assuming:
      `SynthRig` takes its channel count from the application's own
      `numAudioOutputs` (`SynthRig.hpp:69`), so a rig cannot be asked for one
      channel, and a hand-built block needs the `clockPlan` the rig keeps
      private -- passing `nullptr` closes the gate and yields silence, which is
      what `missing_clock_plan_does_not_fault_and_leaves_gate_closed` relies on.
      The scenario is REMOVED from the delta rather than archived as a claim
      nobody has verified. Delivering it needs either an output-channel count
      the rig can be given or a rig-side mono render, which is Sheaf test-harness
      work and belongs to its own change. The sibling scenario, "A stereo device
      receives a stereo image", stays: both Width tests measure a channel
      difference with a printed positive control.

## 5. Delivery

- [x] 5.1 DONE. Branched to `frogg3rs-microphone-path-delivery` before staging anything; `main` was left where it was. Original text: Branch first. frogg3rs is on `main` and the predecessor's work is
      uncommitted there.
- [x] 5.2 DONE. 30 entries staged by explicit path, never `-A`. Everything left behind is the guitar change: `DAISY_MANUAL.md`, `src/FroggersSolo/`, `src/FroggersGuitar/`, `src/common/`, `src/core/`, `test/firmware/`, and its own openspec directory. `.gitignore` DID ship here, and the reason is traced rather than arbitrary: its only edit ignores `openspec/changes/archive/`, and archiving the predecessor writes into that directory, so the rule is load-bearing for this change and inert for the guitar one. Original text: Commit ONLY this work. The tree also carries
      `frogg3rs-guitar-and-solo-variants` (`src/`, `test/firmware/`, its own
      openspec directory, `DAISY_MANUAL.md`). Those are not this change's and
      must not be swept in. `.gitignore`'s only edit is the archive-ignore rule,
      which belongs to neither feature; it ships with whichever commit lands
      first, stated rather than assumed.
- [x] 5.3 DONE. The three deletions are in the commit. Git recorded the distribution delta as a rename into the predecessor rather than a delete plus an add, which is what the fold actually was. Original text: The commit carries the deletion of
      `openspec/changes/frogg3rs-version-single-source-and-windows-vst` (three
      tracked files), whose delta is folded into the predecessor's.
- [x] 5.4 DONE, in that order. Sheaf committed and pushed to `fork/fix-out-of-tree-app-gaps` (e1add4ab -> 22c2ffd7), then the pin bumped as its own commit. Verified all three agree: the pin recorded in `HEAD`, Sheaf's own `HEAD`, and `fork/fix-out-of-tree-app-gaps` are all 22c2ffd7. Both trees clean of this change's work. Original text: Sheaf pushed to `fork/fix-out-of-tree-app-gaps` first, then the
      submodule pin bumped as its own commit. Verify both trees clean after.
- [x] 5.5 DONE -- `postflight.md`. Every promoted scenario mapped to a check or marked not yet delivered; the enumeration re-run against the diff; each operator item given a named code path; divergence from the proposal stated. Original text: POSTFLIGHT: every SHALL in the predecessor's three deltas has a task
      AND a check that runs; every operator item has a named code path; the
      forward enumeration re-run against the DIFF.
- [x] 5.6 The predecessor's directory is in `openspec/changes/archive/`:
      superseding it closed it, its remaining tasks having moved here, so
      archiving is part of the supersession rather than a reward for finishing.
      What stays gated is PROMOTION, not archiving: applying its three deltas
      to `openspec/specs/` asserts the system does what they say, and four
      scenarios under `frogg3rs-distribution` are not delivered yet -- the
      release carrying both platforms, the Audio Unit not being attempted off
      macOS, documentation shipping on the Windows format, and the release body
      matching what shipped. None can be observed until the new CI job runs
      once. Promote the deltas, from the archive, after the CI job runs green
      and the operator items close.
- [x] 5.7 DONE. Both changes validate `--strict`. Reported not fixed: `frogg3rs-guitar-and-solo-variants` does NOT validate -- `external-ring-mod-mix/spec.md`'s MODIFIED requirement "External gate selects VCO-only vs ring mod" contains no SHALL or MUST. Not this change's to touch. Original text: `openspec validate` both changes.
- [ ] 5.8 Pages and releases are OPERATOR-GATED and are not triggered by this
      change without an explicit go. Say plainly what is needed to make the
      work testable and stop there.
