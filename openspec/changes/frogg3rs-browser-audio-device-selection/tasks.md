# Tasks — `frogg3rs-browser-audio-device-selection`

Supersedes `frogg3rs-browser-device-enumeration`, whose implementation is
already in both working trees, uncommitted and unbuilt.

Gates: `cd app && nice make -j2 test`; `app/vst` ctest; browser e2e. Sheaf:
build and run `portable_ui_tests`, `portable_ui_layout_tests` and `dsp_tests`
BY PATH — `projects/synth/Makefile:233`'s `test` is one linear recipe and the
pre-existing `braid4_*_deadline*` failures abort it before those binaries run.
`browser-audio-device-test` and `browser-unit-test` are phony targets that build
AND run. Never above `-j2`, always `nice`, never two builds at once.

## 1. Already in the tree — verify, do not rewrite

- [ ] 1.1 Input devices enumerate, cross the ABI, and build the Input list
      through `Layout::BuildDeviceOptions`. No second option-list builder.
- [ ] 1.2 Capture fires on operator selection, not on the declared channel
      count, and carries the selected device's id into `captureConstraints`.
- [ ] 1.3 ABI version 5, mirrored consistently on both sides.
- [ ] 1.4 The Pages-blocking MIDI spec measures its host's MIDI backend.
- [ ] 1.5 The descriptor packing (JS) and decode (C++) are each single-sourced
      and shared with the MIDI endpoint path.

## 2. Output routing — the work this change adds

- [ ] 2.1 Extend the pending request to carry an OUTPUT selection. Do NOT add a
      second consumable: input selection, retry, and output selection are one
      mechanism with different arguments, and a second one is the duplication
      that gets written every time this path is extended without looking.
- [ ] 2.2 Call `AudioContext.setSinkId` with the selected device. Resolve the
      option's label to a `deviceId` from the list JS submitted, the same way
      the input path resolves its index.
- [ ] 2.3 Where `setSinkId` is absent, the control reports that. Establish
      absence by feature detection at runtime, not by sniffing the browser.
- [ ] 2.4 A stored output selection naming a device that is gone falls back to
      system default rather than claiming it.
- [ ] 2.5 `setSinkId` REJECTS as well as resolving — a device can disappear
      between enumeration and the call. Decide and state what the control shows
      when it rejects; it does not report a route it does not have.

## 3. Tests

- [x] 3.1 POSITIVE CONTROL, DONE 2026-08-28. The empty-label filter in
      `BuildBrowserAudioSnapshot` was inverted so every submitted device is
      dropped, and `browser-audio-device-test` was rebuilt and run against it:

          libc++abi: terminating due to uncaught exception of type
          std::runtime_error: system default plus every submitted output device
          (exit 2)

      The header was restored from a byte-compared backup. The suite detects a
      snapshot that ignores its device list, so a later pass means something.
- [ ] 3.2 C++ snapshot coverage for the output list: multi-device, empty,
      unlabelled entries, absent stored selection, present selection.
- [ ] 3.3 A test that asserts the `deviceId` reaching `getUserMedia` is the
      selected one. The predecessor's delta required this and nothing checked it.
- [ ] 3.4 A test that asserts `setSinkId` is called with the selected device,
      and that nothing is called where it is unavailable.
- [ ] 3.5 e2e with `--use-fake-device-for-media-stream`: input list grows,
      unpermitted entries are not named, listing starts no capture.

## 4. Operator — deferred until the site is published, by necessity

None of these are observable in headless Chromium with fake devices: each needs
real hardware against the deployed page. They close AFTER the commit and push,
not before, and this change is not complete until they do.


- [ ] 4.1 OPERATOR: on a phone, the Input dropdown lists the real microphone
      after permission, and selecting it is possible.
- [ ] 4.2 OPERATOR: audio arrives from the selected input device.
- [ ] 4.3 OPERATOR: selecting an output device moves the audio to it, and
      unplugging that device does not leave the instrument silent.
- [ ] 4.4 OPERATOR: opening the page prompts for no microphone.

## 5. Delivery

- [x] 5.1 DONE 2026-08-29. App suite 310 PASS / 0 FAIL; `app/vst` ctest 3/3;
      Sheaf `browser-audio-device-test` and `browser-unit-test` green;
      `portable_ui_tests`, `portable_ui_layout_tests` and `dsp_tests` run by
      path, all green (dsp 106/106) — `make test` was NOT used, since its one
      linear recipe aborts on the pre-existing `braid4_*_deadline*` failures
      before those binaries run. Sheaf browser suite 225 passed / 0 failed /
      2 skipped. frogg3rs e2e 54 passed against freshly started servers.
      Two failures during this run were NOT code defects and are recorded so
      the next reader does not re-derive them: a stale test binary make would
      not rebuild because a restoring copy landed in the same second as the
      link, and a fixture server running since 2026-08-27 whose in-process
      handlers still answered with a pre-change ABI version.
- [ ] 5.1b App suite with counts; `app/vst` ctest; Sheaf binaries by path with
      the braid4 caveat stated.
- [ ] 5.2 Rebuild the wasm. The snapshot is C++ compiled into it; repackaging
      alone does not carry it. Then the full e2e.
- [ ] 5.3 Sheaf pushed to `fork/fix-out-of-tree-app-gaps`; pin bump as its own
      commit; both trees clean.
- [ ] 5.4 Pages green. Nothing here is testable on the site until it is.
- [ ] 5.5 POSTFLIGHT: check every scenario in the delta against a check that
      exercises it, and check that every SHALL in the delta has a task that
      delivers it. The predecessor failed the second half.
