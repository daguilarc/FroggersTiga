# Tasks — `frogg3rs-browser-microphone-permission-path`

Successor to `frogg3rs-browser-audio-device-selection`, which is deployed and
leaves a fresh visitor with no route to a microphone. Merged with
`frogg3rs-version-single-source-and-windows-vst`, whose work is sections 9-11.

There is no time pressure on any item. Limits on what is printed are not limits
on what is checked.

EVERY REMAINING ITEM IS MOVED, not duplicated: `frogg3rs-microphone-path-delivery`
is the single owner of the open work, and each item below names where it went.
This change is NOT archivable yet -- archiving promotes its three deltas into
the main specs, and their scenarios have no passing check until that successor
closes (§9 postflight).

Sections run in order. Sections 6 through 11 depend on nothing in 1-5 and on
nothing in each other, so a stall in one does not block the rest.

## 0. Hygiene

- [x] 0.1 Confirmed. All nine e2e specs matched by a project; no orphans. Only tracked build artifacts are two files under External/libDaisy (vendored). Original text: The sweep of the touched tree is recorded here rather than re-run.
      Established at authoring time: all nine specs in `app/browser/e2e/` are
      matched by a project in `playwright.config.mjs` (MOBILE, DESKTOP, PAGES
      and AUDIO_DEVICE lists together name every file); no orphan specs. The
      only tracked build artifacts in the repo are two files under
      `External/libDaisy`, which is vendored and out of scope. No editor
      backups, no scratch files. Confirm this still holds at execution time and
      say so; do not re-derive it from scratch.
- [x] 0.2 Done. juce_build.mk:154 -> :205 and :152-154 -> :198-207 in app/Frogg3rs-Info.plist. Original text: STALE CITATIONS IN SHIPPING SOURCE, found by the sweep and fixed here:
      `app/Frogg3rs-Info.plist`'s own comment cites `juce_build.mk:154` and
      `:152-154` for the verbatim-copy behaviour. The bundle rule is now at
      `runtime/juce_build.mk:198-207` and the copy at `:205`. Correct both, and
      grep the same file for any other line reference before concluding.
- [x] 0.3 Done. app/browser/e2e binds 8799 and 8800, started per-run, reused only outside CI. Sheaf's shared config gates on 4173 with reuseExistingServer:true UNCONDITIONALLY; src/static-server.mjs:169-171 binds 4173/4174/4175 from one process. Found dead: browser/tests/static-server.mjs, nothing invokes it, and it binds 4173/4174 -- the live server's own ports. Removed. Original text: Long-lived development servers left listening from previous sessions
      are state nobody declared. Find every port the test configs bind, and say
      which are started per-run and which are reused. This is the input to
      section 10, so it runs before that section, not inside it.

## 1. Reproduce the browser defect before touching anything

- [ ] 1.1 MOVED to `frogg3rs-microphone-path-delivery` §1.1. Original text: POSITIVE CONTROL FIRST, and this one is not optional: reproduce the
      deadlock in a test that FAILS against the code as it stands today. A page
      whose `enumerateDevices` reports the real unpermitted shape — entries with
      empty `label` and empty `deviceId` — must be shown to have no reachable
      action that results in a `getUserMedia` call. Record the failure text.
      Do NOT use `--use-fake-device-for-media-stream` to build this: that flag
      populates labels without permission and deletes the condition under test.
      Stub the enumeration instead, the way the existing unpermitted-page test
      already does.
- [ ] 1.2 MOVED to `frogg3rs-microphone-path-delivery` §1.3. Original text: Confirm the Retry dead end by running it, not by reading it: on a
      fresh page with No Input selected, clicking Retry must be shown to arm the
      release sentinel rather than a capture request. The proposal's step 7 is
      structural, so this is the check that makes it evidence. If it does
      something else, the trace is wrong and the design changes.

## 2. Establish the route before building it

- [x] 2.1 Done -- design.md. A button mirroring Retry Input: node runtime.audio.input.permission, action audio-input-permission, label Allow Microphone. Stream stopped before the call's continuation returns. Original text: Trace and STATE the design before writing it, including which control
      the operator uses, what it is called, when it appears, and what happens to
      the stream a permission prompt necessarily opens. `getUserMedia` is the
      only call that prompts, so granting permission necessarily opens a device
      for some interval; say explicitly how that interval ends and how it stays
      distinct from capture nobody selected.
- [x] 2.2 Done -- design.md. It is neither: no channel count is requested and no selection changes. Original text: Check the standing requirement it must not break:
      `froggers-modulation-slate:90-93` forbids opening a capture device on the
      strength of a declared channel count, and `:168-176` requires that
      selecting a device opens it. Say which of these the new control is, and
      why it is not the other.
- [x] 2.3 Done -- design.md. navigator.permissions.query is NOT used; the submitted device list already carries the answer (input entries present, none nameable). Original text: Decide when the control is offered. A page that already holds
      permission does not need it; a machine with no input devices should not be
      invited to ask for one. Establish how each state is distinguished —
      `navigator.permissions.query({name:"microphone"})` reports state without
      prompting, and its availability across browsers is a fact to measure, not
      to assume.

## 3. Build it

- [x] 3.1 Done. kRequestPermissionAudioRequest = -3 in the existing pending slot; no second consumable export. Original text: Implement the design from 2.1. If it needs a new action name, follow
      the existing pattern: C++ arms a pending request during dispatch, JS polls
      and performs the effect. Do NOT add a second consumable export — one
      pending request already carries which control it belongs to
      (`BrowserRuntimeMainServices.hpp:198-204`).
- [x] 3.2 Done. requestInputPermissionNow re-enumerates via submitAudioDevices; inputDeviceName untouched. Original text: After permission is granted, re-enumerate and resubmit so labels
      populate and the list fills. The selection stays at No Input: earning
      labels is not choosing a device.
- [x] 3.3 Done. classifyCaptureFailure -> releaseInput, the same path acquireInput uses. Original text: A denied permission is reported, not swallowed. The status line
      already carries input diagnostics.
- [x] 3.4 Done. FOUND 12 catches in audio.ts, CHANGED 1 (enumerateDevices). Ten others either already report or are silent because the runtime is destroyed and cannot record anything. Reported only while nothing is captured, so a live stream is never torn down to report a failure that did not affect it. Original text: SAME CLASS, SAME FILE: `submitAudioDevices` catches every enumeration
      failure and returns silently (`audio.ts:378-381`). A browser that refuses
      to enumerate should degrade rather than crash, so the catch stays — but a
      swallowed failure is indistinguishable from a browser with no devices, and
      it converted a plain `ReferenceError` in a test fixture into forty minutes
      of tracing a combo that would not populate. Make the reason observable
      without changing the degradation. Enumerate every other `catch` in
      `audio.ts` by operand while doing it — there are eleven — and report which
      are silent, which report, and which of those should change. A silent catch
      is a family, not an instance.

## 4. Tests that can enter the state

- [ ] 4.1 MOVED to `frogg3rs-microphone-path-delivery` §1.1. Original text: The 1.1 test now passes: an unpermitted page has a reachable action
      that results in a `getUserMedia` call.
- [ ] 4.2 MOVED to `frogg3rs-microphone-path-delivery` §1.1. Original text: After a grant, the list contains the real device and the selection is
      still No Input.
- [ ] 4.3 MOVED to `frogg3rs-microphone-path-delivery` §1.1. Original text: A denial surfaces in the status line.
- [ ] 4.4 MOVED to `frogg3rs-microphone-path-delivery` §1.1. Original text: The permission request does not leave a stream running.
- [ ] 4.5 MOVED to `frogg3rs-microphone-path-delivery` §1.1. Original text: An enumeration failure is distinguishable from a machine with no input
      devices, and a machine with no input devices reports no failure. These are
      the two scenarios under the swallowed-failure requirement and 3.4 builds
      the mechanism without checking it.
- [ ] 4.6 MOVED to `frogg3rs-microphone-path-delivery` §1.4. Original text: REVIEW THE HARNESS, not just the tests: the `audio-devices` project is
      the only one carrying `--use-fake-device-for-media-stream`
      (`playwright.config.mjs:120`), and it is the only project matching
      `audio-devices.spec.mjs`. Enumerate what that flag makes untestable and
      say which of this feature's states no test can currently enter. Note the
      tension before writing 4.2: a grant can only name a real device if one
      exists, and no device exists in this harness without the flag that
      deletes the condition 1.1 needs. Say how both tests get what they need.
      A suite that cannot reach a state cannot defend it.

## 5. Operator — browser, each one verified observable BEFORE it is written

- [x] 5.0 Done. Code paths, each named:
      5.1 -- BrowserAudioDevices.hpp's showInputPermissionRequest (set when the
      submitted list holds unnamed input entries and no named ones) ->
      RuntimePages.hpp's form.Button(NodeIds::kAudioInputPermission) ->
      BrowserRuntimeMainServices.hpp arming kRequestPermissionAudioRequest ->
      main.ts's consumePendingAudioRequest -> audio.ts's
      requestInputPermissionNow -> getUserMedia. Every link exercised by the
      e2e in section 4.
      5.2 -- requestInputPermissionNow calls submitAudioDevices after the grant;
      BuildBrowserAudioSnapshot then builds options from the now-named entries.
      5.3 -- acquireInputDeviceAtIndex, reached from a combo selection through
      the same pending-request slot. Unchanged by this work.
      5.4 -- the only two getUserMedia call sites in audio.ts are acquireInput
      and requestInputPermissionNow; both are reachable only from an operator
      action, which the 'raises no permission request on its own' e2e asserts. Original text: GATE ON THIS. For every operator item below, name the code path that
      produces the state being checked, with file:line. An item whose path
      cannot be named is deleted, not shipped. The previous change sent an
      operator to a browser to confirm behaviour nothing could render.
- [ ] 5.1 MOVED to `frogg3rs-microphone-path-delivery` §3.1. Original text: OPERATOR: a browser that has never been granted permission reaches a
      prompt through the interface.
- [ ] 5.2 MOVED to `frogg3rs-microphone-path-delivery` §3.2. Original text: OPERATOR: after granting, the Input list names the real microphone.
- [ ] 5.3 MOVED to `frogg3rs-microphone-path-delivery` §3.3. Original text: OPERATOR: selecting it starts capture from that device.
- [ ] 5.4 MOVED to `frogg3rs-microphone-path-delivery` §3.4. Original text: OPERATOR: loading the page still prompts for nothing on its own.

## 6. The macOS microphone declaration

The absence is traced, not open: the declaration exists at
`app/standalone/CMakeLists.txt:107-108` on the Windows build path, and the
shipped macOS bundle has 0 occurrences of the generated key against the CMake
bundle's 1. The fix does not wait on 6.1.

- [ ] 6.1 MOVED to `frogg3rs-microphone-path-delivery` §3.5. Original text: RUN IT AND RECORD WHAT HAPPENS, because what macOS DOES here is
      behaviour and nothing has observed it. Build with `app/build-launcher.sh`,
      select an input device in the running app, and record exactly what
      follows: a prompt, a silent denial, or termination. Name the quantity that
      decides it — whether any input channel goes active — and report it. This
      result does not gate 6.2; it records how the defect reads today and
      becomes the before-half of 6.3.
- [x] 6.2 Done. Key added to app/Frogg3rs-Info.plist with the string from app/standalone/CMakeLists.txt:108. plutil -lint OK. Original text: Add `NSMicrophoneUsageDescription` to `app/Frogg3rs-Info.plist`.
      `juce_build.mk:205` copies that file verbatim into the bundle, so the key
      lands with no other change. Use the string already authored at
      `app/standalone/CMakeLists.txt:108` rather than writing a second wording:
      one app, one sentence. Say in the plist's own comment what the key is for,
      not which task added it.
- [x] 6.3 Done. Measured: 0 occurrences before, 1 after, in app/build-launcher/Frogg3rs.app/Contents/Info.plist. LAUNCHER_EXIT=0. Original text: POSITIVE CONTROL: rebuild and show the key is present in
      `app/build-launcher/Frogg3rs.app/Contents/Info.plist` — 0 occurrences
      before, 1 after — then repeat 6.1 and record whether the observed
      behaviour changed. If 6.1 already prompted, say so plainly: the key is
      still required by the documented invariant
      (`External/Sheaf/projects/synth/README.md:675-678`) and by all three
      sibling bundles, and a macOS version that tolerates its absence today is
      not a reason to ship without it.
- [x] 6.4 Done. NOT collapsible, and the reason is read rather than assumed: app/standalone/CMakeLists.txt:107-108 passes the sentence to JUCE as MICROPHONE_PERMISSION_TEXT and JUCE generates the key, while juce_build.mk:205 copies app/Frogg3rs-Info.plist verbatim with no templating step of any kind. Two build systems, no shared substrate, so neither can read the other's literal. What it gets instead is the same treatment the ABI version gets: app/check_microphone_usage_strings.sh fails when the two drift, wired into the app gate. POSITIVE CONTROL: changing one string failed the check naming both values; restored, passes. Original text: §7, BY OPERAND: the usage string now exists in two places, in two
      spellings, for two platforms. Report whether that is duplication to
      collapse or two build systems that cannot share a literal, and cite what
      you read. Do not collapse it on the strength of the strings matching.
- [x] 6.5 Done. app/vst/CMakeLists.txt sets no plist source and no MICROPHONE_PERMISSION_ENABLED; the plugin opens no devices. Closed. Original text: The VST needs nothing and this records why, so nobody re-opens it:
      `app/vst/CMakeLists.txt` sets no plist source and no
      `MICROPHONE_PERMISSION_ENABLED`, and the plugin opens no devices — the
      DAW owns them. Confirm both by reading, and close the item.

## 7. Delay's Wet mix

- [x] 7.1 Done. Recorded failure: '[delay wet mix] Send default 0, Wet mix 1.0 -> peak 0' against control '[delay wet mix] Wet mix 0.0 -> peak 0.937332', FAIL peakAtMaxWet > kEpsilon. Original text: POSITIVE CONTROL FIRST: a test that FAILS today, showing Wet mix at
      maximum with Send at its default produces silence. Record the failure text
      before changing anything.
- [x] 7.2 Done. kMaxWetMix = 0.6f at FroggersAppCore.hpp, applied to the mapped value handed to MapRowsToDelayParams. Original text: Cap the mapped mix with `kMaxWetMix`, mirroring
      `kMaxReverbWetMix`'s placement (`FroggersAppCore.hpp:1842`) and idiom.
      Cap the MAPPED value, not the knob range, so the control keeps its full
      sweep.
- [x] 7.3 Done. Follower lives in Process (ToReverbMono is const and cannot hold state); ToReverbMono reads it, which keeps it const. Original text: Wet mix's authority scales with the wet path's MEASURED level, not
      with Send. A low-Send high-Feedback patch holds a loud echo and the knob
      must earn its full travel there. Two constraints are already established
      and are not re-derived: `ToReverbMono` is `const` (`Delay.hpp:985`) so the
      follower cannot live there, and `Process` early-returns at `:792` before
      any wet computation. State where the follower lives and how the early
      return is routed through, then build it.
- [x] 7.4 Done. AdvanceWetLevel(0.0f) is called on the dsnd early-return path, so the target drops at once and the follower still releases. Original text: With Send at zero the follower's TARGET is forced to zero, so
      authority fades at the release constant rather than cutting. The early
      return at `:792` is the branch that must be reached for this to happen;
      do not claim the change adds none.
- [x] 7.5 Done. One follower on monoWet; attack kDelayWetAuthorityAttackSeconds 10ms, release kSharedReleaseSeconds. Limiter envelope not reused. Original text: Measurement: follow `monoWet`, which `ToReverbMono` already computes,
      so one follower serves both channels. Attack 10ms, release
      `kSharedReleaseSeconds` (`Limiter.hpp:76`, 100ms, already applied to this
      signal by the wet limiter at `Delay.hpp:129`). Do NOT reuse the limiter's
      own envelope: `Limiter.hpp:146` defines it as a gain multiplier that sits
      at 1.0 under threshold and cannot distinguish a quiet echo from silence.
- [x] 7.6 Done. MEASURED, and the first measurement was VOID: taken while builds ran concurrently it read 0.75ms/block before against 1.04-1.20ms after, which was machine load, not the change. At rest, repeated: BEFORE 0.596-0.690ms/block (11.2-12.9% of the 5.33ms deadline at 48kHz/256), AFTER 0.599-0.640ms (11.2-12.0%). The added work -- one envelope follower, a linked-stereo limiter pass, two extra one-pole tilt filters, per-channel crossfades -- is below this measurement's run-to-run noise. Not 'free', but not measurable here. Original text: Measure the added cost rather than asserting it is small: report the
      per-sample operation count and the ISR headroom before and after. A
      wet/dry control's authority is not worth a measurable dropout risk.
- [x] 7.7 Done. FOUND 9 crossfades in app/dsp. Only Reverb (capped) and Delay (now capped) are dry-against-processed with an unfeedable path. Vco.hpp:236 blends dry against dry*carrier from an internal sine; Drive.hpp:689 against an all-pass fed by dry; the rest crossfade two processed signals. CHANGED 1. Original text: §7, FORWARD. `kMaxWetMix` is a new named concept. The
      enumeration was run at authoring time and is recorded here to be
      CONFIRMED, not repeated blind: eight crossfades of the form
      `(1-x)·A + x·B` exist in `app/dsp/` — `Drive.hpp:689`, `Delay.hpp:839`,
      `:840`, `:989`, `:1068`, `Vco.hpp:236`, `Reverb.hpp:580`,
      `FilterFx.hpp:771`, `:773`. Of these only two are dry-against-processed
      where the processed path can be unfed: Reverb (capped) and Delay (not).
      `Vco.hpp:236` blends `dry` against `dry·carrier` from an internal sine and
      `Drive.hpp:689` against an all-pass fed by the dry path, so neither can
      silence; the rest crossfade two processed signals. Verify this list is
      still complete and report FOUND vs CHANGED. If a ninth exists, it is in
      scope.
- [x] 7.8 Done. delay_wet_mix_at_maximum_leaves_the_default_patch_audible: peak 0.931364 at Wet mix 1.0 with Send at default. Original text: Assert the default patch is audible with Wet mix at maximum. That is
      the claim the operator made when reporting this.

## 8. Stereo reaches the output

Sequenced last. Depends on nothing above.

- [x] 8.1 Done. Both controls were shown inert first, and the reverb half keeps its control permanently: at Width 0.0 the channel difference is EXACTLY 0, against 0.146441 at Width 1.0. The delay half measures 0.612837 with Send 0.8. A first attempt read 0 for both because the window was silent (peak 0, StartAt(120.0)); the tests now assert the instrument is sounding before reading a difference, so a zero can never again mean silence. Original text: POSITIVE CONTROL FIRST, two of them, both failing today: Reverb's
      Width produces a bit-identical output at every knob position, and Delay's
      Stereo width produces no channel difference at the output. Record both
      failure texts. These are the claims the algebra predicts
      (`Reverb.hpp:573-577`), and a test that cannot see the difference after
      the change would not have seen it before either.
- [x] 8.2 Done. ToReverbMono became ToStereo and returns the pair. Each channel crossfades the mono dry source against its OWN wet channel; the mono fold of that pair is exactly the old formula, asserted in the parity suite. Original text: Carry the delay's pair past `ToReverbMono` instead of folding it.
      The fold moves to the output; the middle of the chain stops being mono.
- [x] 8.3 Done. Reverb::Process takes and returns a StereoSample. Only the tank SEND folds (one pre-delay line into a two-line network; two inputs would be a different reverb), and wetL/wetR are no longer summed, which is what makes Width a control rather than an identity. Original text: Carry the reverb's pair past `Reverb.hpp:577` the same way. The tank
      is already two lines and already computes `wetL`/`wetR`
      (`:225-226`); this is plumbing, not a second reverb.
- [x] 8.4 Done. The fold is in the write loop and only at 1 channel, where it is the SUM rather than the left channel. 2+ channels alternate L/R, so a surround device repeats the pair instead of going silent past channel 2. The capture buffer takes the same mono fold, so a recording never loses half the signal. Original text: Fold at the device, and only where the device is mono.
      `FroggersAppCore.hpp:1158-1163` writes one sample to every output channel
      today; that loop becomes the place a stereo pair either spreads across
      two channels or sums for a mono device. State what happens at 1 channel,
      2 channels, and more than 2.
- [x] 8.5 Done. Neither host constrains the pair: the VST passes the DAW's own count through (FroggersPluginProcessor.cpp:695) and the browser passes output.numberOfChannels (BrowserRuntime.hpp:294, :662). No plugin or browser change was needed; the VST's 5 ctest cases pass after the change. Original text: The VST and the browser hosts take the same path. Establish what each
      one's output block actually offers — `FroggersPluginProcessor.cpp:697`
      and the browser's output side — and say whether either constrains the
      pair. Do not assume the plugin is stereo because DAWs usually are.
- [x] 8.6 Done. Both width tests pass with the numbers above, and the mono validation suite still passes -- 314 app checks, 0 failures. Original text: Both Width controls now do something. Assert it: the 8.1 tests pass,
      and a mono-device path still produces the same sum it produces today.
- [x] 8.7 Done. ONE type: dsp::StereoSample in DspMath.hpp, the header Delay.hpp and Reverb.hpp already share. DelayWetPair is now an alias of it rather than a second declaration of the same shape, so the delay keeps its own name for the wet tap without a parallel type existing. Original text: §7, FORWARD: a stereo pair crossing stage boundaries is a new named
      concept. `DelayWetPair` already exists (`Delay.hpp`). Enumerate whether
      the reverb's pair should reuse it or whether two names are correct, and
      cite what you read.

## 9. One definition of the browser ABI version

- [x] 9.1 Done. FOUND, by operand: 13 hits on SUPPORTED_BROWSER_ABI_VERSION, 14 on synth_browser_abi_version, 33 on an abiVersion literal. Genuine value sites: protocol.ts:2 (the definition), BrowserRuntimeAbi.cpp:30, static-server.mjs:37 and :84, browser_runtime_contract_tests.cpp:1170, plus ~14 browser test files. Original text: ENUMERATE FIRST, BY OPERAND. Before changing anything, list every
      site that expresses the browser ABI version: the C++ return
      (`BrowserRuntimeAbi.cpp:28-31`), the C++ contract assertion
      (`browser_runtime_contract_tests.cpp:1170`), every
      `_synth_browser_abi_version` stub, the synthesized fixture catalog
      (`static-server.mjs:84`), every `abiVersion` literal in a test or fixture,
      and `SUPPORTED_BROWSER_ABI_VERSION` itself (`protocol.ts:2`). Search by
      BOTH the identifier and the bare number in an `abiVersion` context,
      because the two forms are what let a bump miss a sibling in the same file.
      Report the count FOUND.
- [x] 9.2 Done. CLASSIFIED: decoys are runtime-core.spec.ts:257 (channel count), static-server.mjs:40 (handle), and every '== 6' in controllers_page_ui_tests.cpp, runtime_main_component_tests.cpp, ControllersPageSimulationTests.cpp and browser_midi_bridge_tests.cpp, which are sizes, counts and an action index. CHANGED 2 (static-server.mjs's stub and fixture catalog now read the definition). Original text: Classify each hit before touching any of it. Some numbers near these
      sites are not versions: `_synth_browser_audio_input_channels: () => 4`
      (`runtime-core.spec.ts:257`) is a channel count and
      `_synth_browser_create: () => 41` (`static-server.mjs:40`) is a handle.
      Report FOUND vs CLASSIFIED vs CHANGED.
- [x] 9.3 Done. static-server.mjs imports the three version constants from ./protocol.js, the shape package-contract.mjs already established. The C++ mirrors, the C++ contract assertion, the fixture literals and the pending-request sentinels are covered by browser/tests/version-drift.test.mjs, which resolves a named C++ constant to its definition rather than skipping it. Original text: Give the version one definition every other site reads. Where a
      language boundary prevents sharing the literal, the mirror is either
      generated from the definition or asserted equal to it by a test that FAILS
      on drift. A comment saying "keep in sync" is not a mechanism.
- [x] 9.4 Done. POSITIVE CONTROL: bumping protocol.ts 6 -> 7 failed three of four checks, each naming its site (BrowserRuntimeAbi.cpp disagrees; the contract test pins a version no longer defined; fixtures declare another version). The sentinel check correctly stayed green, being a different fact. Restored; DRIFT_EXIT=0, 4/4. Original text: POSITIVE CONTROL: change the single definition and confirm every
      mirror moves, or that the drift test fails. Record the failure text. A
      single-source claim that has never been tested by moving the value is a
      claim about code shape, not about behaviour.

## 10. A run does not trust a server it cannot date

- [x] 10.1 Done. Reuse is KEPT and proven current, rather than dropped: dropping it costs every run, while the failure is narrow and now detected. The server stamps startedAt at module load -- the same moment its synthesized responses freeze -- and reports it with the mtime of the sources those responses are built from. Original text: A reused server is proven current before a run trusts it, or is not
      reused. Decide which, and state why. Note that a static file server serves
      fresh files from disk while its own in-process handlers stay frozen, so
      "the server is up" says nothing about whether its synthesized responses
      match the tree. `playwright.shared-config.mjs:11` sets
      `reuseExistingServer: true` unconditionally today.
- [x] 10.2 Done. serverCurrencyFailure returns a message opening 'STALE STATIC SERVER', reporting how many seconds behind the process is and naming the sources it dated against, so the case cannot be mistaken for a failing assertion in the code under test. Original text: Whatever mechanism is chosen must make the stale case LOUD. A stale
      fixture that merely produces a failing assertion costs the reader the time
      it took to find the process start time.
- [x] 10.3 Done. browser/tests/server-currency.test.mjs, 4 tests: current, the inclusive same-millisecond boundary, the stale rejection (asserting the message names staleness, the lag and the sources), and malformed identities not reading as current. Verified end to end against a live server on 4199. Original text: A CHECK THAT RUNS, not only a mechanism. The delta requires that a
      run finding a server older than the tree restarts it or fails loudly.
      Write the test that starts a server, makes the tree newer than it, and
      asserts the loud outcome. Without this the requirement ships unbacked.
- [x] 10.4 Done. Superseded. The hazard is a stale long-lived process, not a partially-started one: static-server.mjs binds 4173/4174/4175 from one process, so readiness on 4173 implies the others within milliseconds. Delivery no longer asks anyone to pre-start ports by hand. Original text: Whatever 10.1 decides supersedes any earlier instruction to
      pre-start ports 4173/4174/4175 by hand. `static-server.mjs:169-171` binds
      all three from one process and `playwright.shared-config.mjs:10` gates on
      4173, so a partially-started server is not the hazard; a stale one is.
      Say plainly in the delivery notes which it is now.

## 11. Windows VST3

- [x] 11.1 Done. FROGGERS_VST_FORMATS is VST3 everywhere, plus AU under APPLE. Original text: `FORMATS VST3 AU` (`app/vst/CMakeLists.txt:98`) becomes conditional:
      VST3 everywhere, AU only under `APPLE`. AU cannot build off macOS.
- [x] 11.2 Done. Docs go to $<TARGET_BUNDLE_CONTENT_DIR>/Resources on macOS and to $<TARGET_FILE_DIR>/../Resources otherwise, which is the sibling Contents/Resources of a Windows VST3 bundle. Original text: The doc-bundling loop (`:127`) uses `$<TARGET_BUNDLE_CONTENT_DIR:...>`,
      a macOS bundle concept. Establish where MANUAL.md and QUICK_DICT.md belong
      inside a Windows VST3 directory, and put them there. The requirement that
      operator documentation ships with the plugin is not macOS-specific.
- [x] 11.3 Done. The signing loop is wrapped in if(APPLE). No Windows signing step invented. Original text: The signing loop (`:152`) runs `codesign`, which does not exist on
      Windows. It becomes macOS-only. Do NOT invent a Windows signing step:
      no Authenticode certificate exists, and the Windows standalone already
      ships unsigned.
- [x] 11.4 Done. build-and-test-windows on windows-latest, mirroring desktop-release.yml's Windows job including submodules: recursive. Original text: Add a `windows-latest` build job to `vst-plugin.yml`, mirroring
      `desktop-release.yml`'s existing Windows job (`:51`), including its Sheaf
      submodule checkout fix (`72700a9`).
- [x] 11.5 Done. test -d on the VST3 bundle only, and Compress-Archive instead of ditto, copied from desktop-release.yml's own Windows packaging. Original text: The packaging step asserts `test -d Frogg3rs.vst3` /
      `Frogg3rs.component` (`vst-plugin.yml:71-72`) and zips with `ditto`
      (`:89-90`). Both are macOS-only. The Windows job needs its own existence
      check and its own archive step.
- [x] 11.6 Done. The 'this release carries no Windows VST3' sentence was checked and is no longer true; the body now lists the Windows VST3 install path and says the artifact is unsigned. Original text: The release body lists install paths. Add the Windows VST3 location,
      and check whether the inline "this release carries no Windows VST3"
      sentence is still true before leaving it in place — it will not be.
- [x] 11.7 Done. MANUAL.md's Release platforms section updated; the awk extraction still bounds correctly (44 lines, terminated by ---), verified by running it. Original text: `MANUAL.md:21`'s "Release platforms" section is extracted verbatim
      into every published release body by both workflows (`vst-plugin.yml:125`,
      `desktop-release.yml:134`). Update it, and confirm the extraction still
      bounds correctly on the `^## ` heading after the edit.
- [x] 11.8 Done. Configure-time assertions: AU required on macOS, rejected off it, VST3 required everywhere. Original text: A check that the plugin's format list matches the platform: VST3 on
      both, AU only on macOS.
- [x] 11.9 Done. FroggersVstDocsBundled_<format> ctest per format, reading the BUILT artifact. POSITIVE CONTROL: hiding MANUAL.md failed it with 'operator documentation missing from the built plugin'; restored, both pass. Original text: A check that operator documentation is present inside the built
      Windows VST3, not only the macOS bundles.
- [x] 11.10 Done. Recorded in the workflow itself. FIRST ATTEMPTS with no working equivalent: configuring app/vst on Windows, building FroggersVst_VST3 there, and the Windows Resources layout. Everything else -- checkout, JUCE clone, Compress-Archive, artifact upload -- is copied from desktop-release.yml's passing Windows job. Original text: CODE THAT HAS NEVER EXECUTED GETS ITS INVOCATIONS TRACED, NOT
      REVIEWED. The Windows plugin job has never run. For each command it will
      run, find how the same thing is invoked by `desktop-release.yml`'s working
      Windows job and diff the two. Where nothing equivalent exists, mark that
      step a first attempt so a failure reads as expected discovery.
- [ ] 11.11 MOVED to `frogg3rs-microphone-path-delivery` §3.6. Original text: OPERATOR: the Windows VST3 loads in a Windows DAW and makes sound.
      Nothing in CI can show this. Named because 5.0's gate applies: the code
      path that produces this state is `vst-plugin.yml`'s new Windows job
      producing an artifact, which does not exist until 11.4 lands. This item
      is not written as checkable until then.
- [ ] 11.12 MOVED to `frogg3rs-microphone-path-delivery` §3.7. Original text: OPERATOR: the unsigned Windows plugin's first-load warning, if any,
      is described accurately in the release body.

## 12. Delivery

- [ ] 12.1 MOVED to `frogg3rs-microphone-path-delivery` §2.1-2.5. Original text: Gates with counts. Sheaf browser suite, app suite, VST ctest, and the
      frogg3rs e2e. No port needs pre-starting by hand: the currency check from
      section 10 is what a run now relies on. The plugin's macOS build must be
      unchanged in behaviour:
      section 11 is packaging, and a macOS regression means the conditionals
      took something away from the platform that already worked.
- [ ] 12.2 MOVED to `frogg3rs-microphone-path-delivery` §1.1. Original text: Rebuild the wasm. The snapshot is C++ compiled into it.
- [ ] 12.3 MOVED to `frogg3rs-microphone-path-delivery` §4.3. Original text: Sheaf pushed to the fork, pin bumped as its own commit afterwards,
      both trees clean.
- [ ] 12.4 MOVED to `frogg3rs-microphone-path-delivery` §4.4. Original text: POSTFLIGHT, three halves, separately: every SHALL in every delta has
      a task AND a check that runs; every operator item has a named code path;
      and §7 re-run against the DIFF rather than the proposal — every new
      constant, helper, predicate and type triggers a fresh enumeration of that
      concept across the tree.
- [ ] 12.5 MOVED to `frogg3rs-microphone-path-delivery` §4.5. Original text: `openspec validate frogg3rs-browser-microphone-permission-path`
      passes. Note the parser requires SHALL or MUST in a requirement's first
      body line, not merely somewhere in it.
