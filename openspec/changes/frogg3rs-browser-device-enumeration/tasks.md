# Tasks — `frogg3rs-browser-device-enumeration`

Supersedes `2026-08-28-frogg3rs-first-visit-and-open-repairs` on audio input and
takes over its operator item 6.2.

Gates: `cd app && nice make -j2 test` (310 PASS / 0 FAIL across 10 binaries);
`app/vst` ctest 3/3; browser e2e (51 specs). Sheaf: build and run
`portable_ui_tests`, `portable_ui_layout_tests` and `dsp_tests` DIRECTLY —
`make test` is one linear recipe and the pre-existing `braid4_*_deadline*`
failures abort it before those binaries run, so its log reads clean while
hiding them. Never above `-j2`, always `nice`, never two builds at once.

The layout headers are header-only and included by a 4,901-line binary, so a
full rebuild is minutes. Iterate against a small standalone translation unit
that includes only what it needs (~2s) and run the real binaries once at the
end.

## 0. Hygiene

- [ ] 0.1 Sweep `BrowserAudioDevices.hpp` and the browser audio TS layer.
      Report dead code, stale comments, and anything describing behaviour that
      no longer holds — establish invocation by SEARCHING for it, by bare name
      AND by path, never by a file's existence.
- [ ] 0.2 `:196`'s comment ("Browser device ids are privacy-scoped, so nothing
      is enumerated here") stops being true the moment this lands. It is the
      whole justification for today's behaviour, so it must be rewritten to say
      what the code does, not deleted silently.

## 1. Establish what the browser can actually offer

- [ ] 1.1 BEHAVIOURAL PREMISE, CHECK IT BEFORE DESIGNING (§13). Do not reason
      from the spec about what `enumerateDevices()` returns. Run it: on this
      Mac's Chromium, before and after a capture permission grant, and record
      what comes back — how many entries, which `kind` values, whether `label`
      is empty, whether `deviceId` is stable across reloads and across a
      storage clear. Playwright can grant `microphone`; `--use-fake-device-for-
      media-stream` provides a device without hardware.
      Everything below depends on this. If the answer differs from what the
      design assumes, the design changes, not the finding.
- [ ] 1.2 Report what an OUTPUT device selection can do. `setSinkId` is not
      universally available and is not available on all iOS browsers. Establish
      where it exists rather than assuming, and say what the control does where
      it does not — it reports the limitation, it does not pretend.
- [ ] 1.3 Enumerate every consumer of `BuildBrowserAudioSnapshot`'s two option
      lists and of `BrowserInputDeviceName`/`BrowserOutputDeviceName` before
      changing their shape. Both resolvers currently THROW for any unexpected
      id, so every caller is written against a single-valued list.

## 2. Carry a device list across the ABI

- [ ] 2.1 Nothing passes a device list from JS into the wasm today. Trace how
      an existing list-shaped value crosses that boundary and follow it, rather
      than inventing a second mechanism. If none exists, say so — that makes
      this a first attempt, and failure there is expected discovery.
- [ ] 2.2 The list is a LIVE QUERY, not a persisted table. Labels are empty
      until permission is granted and ids reset when storage is cleared, so a
      stored selection is RE-RESOLVED against the current enumeration and falls
      back rather than resolving to a device this origin can no longer name.
- [ ] 2.3 Devices appear and disappear while the page is open. Decide, and
      state, what happens to a selection whose device vanishes — it does not
      silently keep claiming a device that is gone.

## 3. The controls

- [ ] 3.1 Both dropdowns list what 1.1 established is available. Input keeps a
      "No Input" entry, which stays the default: supplying a device list does
      not start capture, exactly as supplying an AudioContext does not start
      audio.
- [ ] 3.2 Delete the `throw` in both name resolvers
      (`BrowserAudioDevices.hpp:205,214`). They exist only to reject a
      condition this change makes legal, and a guard pointed at nothing still
      passes.
- [ ] 3.3 Selecting a device takes effect on the running graph, not only on the
      next boot. Assert it.
- [ ] 3.4 An unpermitted page shows the control honestly: entries with no
      labels are not presented as named devices.

## 4. Tests

- [ ] 4.1 POSITIVE CONTROL: a test that FAILS against today's single-entry
      lists. Report its failure text before the fix.
- [ ] 4.2 Browser e2e with a fake capture device, asserting the input list
      grows past "No Input" once permission is granted, and that it does not
      before.
- [ ] 4.3 The C++ snapshot tests cover a multi-device list, an empty list, and
      a stored selection whose device is absent.
- [ ] 4.4 What headless CANNOT show, said plainly rather than faked: a real
      device's audio actually arriving. That stays an operator check.

## 5. Operator

- [ ] 5.1 OPERATOR: on a phone, the Input dropdown lists the real microphone
      after permission is granted, and selecting it is possible. This is item
      6.2 from the superseded change, which could not have passed.
- [ ] 5.2 OPERATOR: audio actually arrives from the selected device.
- [ ] 5.3 OPERATOR: the Output dropdown, where `setSinkId` exists.

## 6. Nothing else moved

- [ ] 6.1 App suite with counts; `app/vst` ctest; Sheaf binaries run directly
      with the braid4 caveat stated explicitly.
- [ ] 6.2 Rebuild the wasm — the snapshot is C++ compiled into it, so
      repackaging alone does NOT carry this. Then the full e2e, idle and under
      load.
- [ ] 6.3 Pin bump as its own step; both trees clean and pushed.
- [ ] 6.4 POSTFLIGHT: re-run §7 against the DIFF, and verify every requirement
      this change syncs against the IMPLEMENTATION, not against its own prose.
      That verification is exactly what the superseded change omitted.
