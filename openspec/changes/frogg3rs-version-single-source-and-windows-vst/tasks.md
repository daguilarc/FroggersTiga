# Tasks — `frogg3rs-version-single-source-and-windows-vst`

Successor to `frogg3rs-browser-audio-device-selection`.

There is no time pressure on any item. Limits on what is printed are not limits
on what is checked.

## 0. Hygiene

- [ ] 0.1 Sweep `app/vst/`, `.github/workflows/vst-plugin.yml`, and the browser
      version-definition sites. Establish invocation by searching for it, by
      bare name AND by path, never by a file's existence.
- [ ] 0.2 Long-lived development servers left listening from previous sessions
      are state nobody declared. Find every port the test configs bind, and say
      which are started per-run and which are reused.

## 1. One definition of the browser ABI version

- [ ] 1.1 ENUMERATE FIRST, BY OPERAND. Before changing anything, list every site
      that expresses the browser ABI version: the C++ return, the C++ contract
      assertion, every `_synth_browser_abi_version` stub, the synthesized
      fixture catalog, every `abiVersion` literal in a test or fixture, and
      `SUPPORTED_BROWSER_ABI_VERSION` itself. Search by BOTH the identifier and
      the bare number in an `abiVersion` context, because the two forms are what
      let a bump miss a sibling fourteen lines away. Report the count FOUND.
- [ ] 1.2 Classify each hit before touching any of it. Some numbers near these
      sites are not versions: `_synth_browser_audio_input_channels: () => 4` is
      a channel count and `_synth_browser_create: () => 41` is a handle.
      Report FOUND vs CLASSIFIED vs CHANGED.
- [ ] 1.3 Give the version one definition every other site reads. Where a
      language boundary prevents sharing the literal, the mirror is either
      generated from the definition or asserted equal to it by a test that FAILS
      on drift. A comment saying "keep in sync" is not a mechanism.
- [ ] 1.4 POSITIVE CONTROL: change the single definition and confirm every
      mirror moves, or that the drift test fails. Record the failure text. A
      single-source claim that has never been tested by moving the value is a
      claim about code shape, not about behaviour.

## 2. A run may not trust a server it cannot date

- [ ] 2.1 A reused server is proven current before a run trusts it, or is not
      reused. Decide which, and state why. Note that a static file server serves
      fresh files from disk while its own in-process handlers stay frozen, so
      "the server is up" says nothing about whether its synthesized responses
      match the tree.
- [ ] 2.2 Whatever mechanism is chosen must make the stale case LOUD. A stale
      fixture that merely produces a failing assertion costs the reader the time
      it took to find the process start time.
- [ ] 2.3 SAME CLASS, DIFFERENT SURFACE: `audio.ts`'s `submitAudioDevices`
      catches every enumeration failure and returns silently. A browser that
      refuses to enumerate should degrade rather than crash, so the catch stays
      — but a swallowed failure is indistinguishable from a browser with no
      devices, and it converted a plain `ReferenceError` in a test fixture into
      forty minutes of tracing a combo that would not populate. Make the reason
      observable without changing the degradation. Enumerate every other
      `catch` in the browser audio path by operand while doing it: a silent
      catch is a family, not an instance.

## 3. Windows VST3

- [ ] 3.1 `FORMATS VST3 AU` (`app/vst/CMakeLists.txt:98`) becomes conditional:
      VST3 everywhere, AU only under `APPLE`. AU cannot build off macOS.
- [ ] 3.2 The doc-bundling loop (`:127`) uses `$<TARGET_BUNDLE_CONTENT_DIR:...>`,
      a macOS bundle concept. Establish where MANUAL.md and QUICK_DICT.md belong
      inside a Windows VST3 directory, and put them there. The requirement that
      operator documentation ships with the plugin is not macOS-specific.
- [ ] 3.3 The signing loop (`:152`) runs `codesign`, which does not exist on
      Windows. It becomes macOS-only. Do NOT invent a Windows signing step:
      no Authenticode certificate exists, and the Windows standalone already
      ships unsigned.
- [ ] 3.4 Add a `windows-latest` build job to `vst-plugin.yml`, mirroring
      `desktop-release.yml`'s existing Windows job, including its Sheaf
      submodule checkout fix (`72700a9`).
- [ ] 3.5 The packaging step asserts `test -d Frogg3rs.vst3` /
      `Frogg3rs.component` and zips with `ditto`. Both are macOS-only. The
      Windows job needs its own existence check and its own archive step.
- [ ] 3.6 The release body lists install paths. Add the Windows VST3 location,
      and check whether the inline "this release carries no Windows VST3"
      sentence is still true before leaving it in place — it will not be.
- [ ] 3.7 `MANUAL.md`'s "Release platforms" section is extracted verbatim into
      every published release body by both workflows. Update it, and confirm the
      extraction still bounds correctly on the `## ` heading after the edit.

## 4. Tests

- [ ] 4.1 A check that the plugin's format list matches the platform: VST3 on
      both, AU only on macOS.
- [ ] 4.2 A check that operator documentation is present inside the built
      Windows VST3, not only the macOS bundles.
- [ ] 4.3 CODE THAT HAS NEVER EXECUTED GETS ITS INVOCATIONS TRACED, NOT
      REVIEWED. The Windows plugin job has never run. For each command it will
      run, find how the same thing is invoked by `desktop-release.yml`'s working
      Windows job and diff the two. Where nothing equivalent exists, mark that
      step a first attempt so a failure reads as expected discovery.

## 5. Operator

- [ ] 5.1 OPERATOR: the Windows VST3 loads in a Windows DAW and makes sound.
      Nothing in CI can show this.
- [ ] 5.2 OPERATOR: the unsigned Windows plugin's first-load warning, if any, is
      described accurately in the release body.

## 6. Delivery

- [ ] 6.1 Gates, with counts. The plugin's macOS build must be byte-identical in
      behaviour: this change is packaging, and a macOS regression means the
      conditionals took something away from the platform that already worked.
- [ ] 6.2 POSTFLIGHT: re-run repetition detection against the diff, and check
      that every SHALL in the delta has both a task and a check that runs.
