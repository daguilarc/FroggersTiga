# Tasks — `frogg3rs-external-audio-consent-repair`

**Rewritten 2026-08-21 after preflight.** The first draft's diagnosis was
wrong: the routed signal already refuses a host-opened device
(`Runtime.hpp:688-694`, read from the predicate itself — the assertion at
`RuntimeShellSessionTests.cpp:592-631` looks like corroboration but has never
executed, see 0.2), and a disconnected cell already draws nothing and carries
no action
(`EncoderDraw.hpp:649-656`, `FroggersUiSurface.hpp:1699-1712`). The defect is
that the host opens a microphone at launch and the operator cannot decline.
All work below is in `External/Sheaf`; no `app/` change is expected.

Gates: `cd app && nice make -j2 test` (300/300); plugin targets from
`app/vst/build` — `FroggersVstHostTests` 46/46, smoke 1/1, editor 3/3; Sheaf
`nice make -C projects/synth -j2 test`, green means 920 passed / 2 failed and
the two are the known braid-4 deadline tests, NEVER a zero exit code; and
`nice make -C projects/synth/apps/miniapp -j2 test`, which is the ONLY target
that builds `runtime/Runtime.hpp` and `juce/RuntimeShellSessionTests.cpp` —
the files this change edits. Never above `-j2`, always `nice`.

## 0. Hygiene

- [x] 0.1 Confirm the files this change touches carry no debug markers before
      and after. Sheaf's own `sar-NN`/`sru-NN`/`sprs-NN` comment tags are a
      pre-existing upstream convention across the whole submodule; leave them
      alone rather than half-stripping one file. Do not add new ones.
- [x] 0.2 The runtime shell session tests exit 139 (SIGSEGV at 0x0) inside
      `juce::CallbackMaxSizeEnforcer::audioDeviceIOCallbackWithContext`, from
      `CheckZeroInputApplication`. Verified pre-existing on committed HEAD, so
      it is not this change's doing — and it is this change's to fix, because
      the process dies before nine later checks run, `CheckInputRoutedSignal`
      and `CheckSystemDefaultInputSelection` among them. Those are the tests
      that decide whether input consent works. Fix the crash so they execute.
      Trace the consumers before changing the harness: a cleanup that breaks a
      shipping path is an outage, not hygiene.
- [x] 0.3 No gate builds that binary. `make -C projects/synth test` compiles
      only the portable library tests; `runtime/Runtime.hpp` and
      `juce/RuntimeShellSessionTests.cpp` reach a compiler solely through
      `make -C projects/synth/apps/miniapp test`. Put the miniapp test target
      into the gate this change is measured by, so the runtime shell cannot go
      unwatched again.
- [x] 0.4 Report which of the nine skipped checks pass once 0.2 lands, and
      which were failing silently behind the crash. A check that never ran is
      not a check that passed.

## 1. No input is a real option, and the default

Paths below are relative to `External/Sheaf/projects/synth`.

- [x] 1.1 Add `kNoInputOptionId` / `kNoInputOptionLabel` beside the existing
      system-default pair in `include/synth/RuntimePages.hpp:29-30`, and give
      `BuildDeviceOptions` (`:349`) the first option as a parameter rather
      than hardcoding the system-default one. One builder, both sides — do
      not add a second input-only builder that repeats the loop.
- [x] 1.2 `SelectedDeviceOptionId` (`:359`) takes the sentinel to fall back
      to. `DeviceNameFromOptionId` (`:376`) maps BOTH sentinels to the empty
      name; neither is ever a real device name, so no side parameter is
      needed there.
- [x] 1.3 `AudioPageSnapshot::selectedInputId` (`:202`) defaults to the
      no-input sentinel. `selectedOutputId` is unchanged.
- [x] 1.4 Update the input-side call sites to pass the no-input sentinel and
      the output-side ones to pass the system-default sentinel:
      `juce/RuntimePagesJuce.hpp:134-135,146,148,197,205`;
      `runtime/JuceRuntimeMainServices.hpp:109-110,115-118,143,148`;
      `include/synth/browser/BrowserAudioDevices.hpp:196-199`. The browser
      hardcodes its own input list rather than calling `BuildDeviceOptions`;
      its single entry becomes the no-input one.
- [x] 1.5 `BrowserInputDeviceName` (`BrowserAudioDevices.hpp:216-220`) throws
      for any id that is not `system_default`. It must accept the no-input id
      and return the empty name. Check whether `system_default` should still
      be accepted there at all once it is no longer offered for input.

## 2. The host opens no input device until asked

- [x] 2.1 `runtime/Runtime.hpp:261` passes `config.numAudioInputs` to
      `initialiseWithDefaultDevices`. Pass zero instead. The input-inclusive
      failure branch immediately below (`:262-270`) exists only to recover
      from that call failing on the input side; with zero requested it can no
      longer fire — remove it rather than leaving a dead recovery path.
      `requestedInputChannels_` (`:201`) keeps the app's real request; it is
      the channel count, not the device decision.
- [x] 2.2 Delete `ResolveInputDeviceName` (`:646-659`) and pass the raw name
      at both call sites (`:474`, `:944`). An empty input device name
      recovers its plain meaning — no input device — which is what JUCE's
      `AudioDeviceSetup` reads it as, and what makes declining real.
- [x] 2.3 Set the input channels on the selection path. Neither
      `ApplyAudioDeviceInputSelection` (`:459-484`) nor `ApplyInputDeviceSetup`
      (`:715-727`) writes `inputChannels` or `useDefaultInputChannels` today;
      those are set once by the startup call being changed in 2.1, so without
      this a selected device opens with zero input channels. On a non-empty
      name set the first `requestedInputChannels_` bits and
      `useDefaultInputChannels = false`; on the empty name clear them. Both
      call sites in 2.2 build a setup and hand it to `ApplyInputDeviceSetup` —
      put the name and channel assignment in ONE place they both go through,
      not in each.
- [x] 2.4 The audio status line (`ComposeAudioStatus`, `:876-883`) reports
      `Input requested N / active M`. Confirm it reads `active 0` with no
      device open, rather than something that looks like a failure.

## 3. Tests

- [x] 3.1 Retarget, do not delete, the seven assertions that encode the old
      contract in `juce/RuntimeShellSessionTests.cpp`: `:451` "opens the
      default input device", `:454`/`:459` the `17 / active 4` diagnostic,
      `:490` the `2 / active 2` diagnostic, `:572`/`:576`
      `CheckMissingPersistedInputDevice`, `:598` `CheckInputRoutedSignal`'s
      startup assertion, `:709`/`:711`/`:713` `CheckSystemDefaultInputSelection`,
      `:736` `CheckInputDeviceOpenFailure`. Each states the behavior being
      replaced; each gets the new one. Report found versus changed.
- [x] 3.2 New coverage, in Sheaf's own suite and style: an input-requesting
      app opens no input device at startup and reports zero active channels;
      selecting a named device opens it WITH the requested channels active;
      selecting no input closes it and returns the routed signal to false.
      POSITIVE CONTROL required — the middle case must show a nonzero active
      channel count, or "no device open" proves only that the harness never
      opened one.
- [x] 3.3 Lock the disconnected rendering that already works, if nothing
      asserts it yet: a disconnected external-audio cell emits no draw
      commands and carries neither `action` nor `pointerDragAction`, while a
      connected one emits both. Check first — do not add a duplicate of an
      assertion that exists.
- [x] 3.4 Update the option-list call sites in the test files that build
      input options: `juce/RuntimePagesJuceTests.cpp:111` and every
      `inputOptions = BuildDeviceOptions(...)` line in
      `tests/portable_ui_tests.cpp` (`:1450`, `:1494`, `:2105`, `:2218`,
      `:2619`, `:4357`).

## 3b. Existing installs must not inherit a device nobody chose

Found by launching the app: the input combo came up on a device the operator
had never selected, restored from a persisted `audioDevice.inputDeviceName`
written under the old meaning of that field. The new code reads any non-empty
value there as an affirmative choice, so every existing install would reopen
whatever device is sitting in its config — the exact capture this change
removes, arriving through persistence instead of through startup.

The write path that put that name there was NOT identified. It is not the
input combo handler (`Runtime.hpp:461` at HEAD, the only non-test writer) and
not the platform default (this machine's default input is the built-in
microphone, not the device that appeared). UNVERIFIED — recorded as unknown
rather than guessed. The repair does not depend on it: the field is
demonstrably not a reliable record of consent whatever wrote it.

- [x] 3b.1 Bump the runtime config schema and drop a persisted input device
      name when loading anything written under an older one. Output device
      selection is untouched.
- [x] 3b.2 The sync section's load is gated on an exact version equality;
      widen it, or bumping the schema silently discards every user's sync
      settings.
- [x] 3b.3 Tests: an old-schema config loads with the input name cleared and
      the output name preserved; a current-schema config keeps its input name.
      That second one is the positive control — without it the first proves
      only that the loader can produce an empty string.

## 3c. Randomize leaves some parameters alone

Found by pressing Randomize All: every parameter carried a modulation badge.
The count ladder in `app/FroggersModulation.hpp`'s
`RandomizeParameterModulationDepths` has no zero bucket, so every parameter with
a connected source gets at least one non-neutral depth. That contradicts the
existing requirement, which already called for zero sources on a share of calls
and for four-or-more on about one call in sixteen — the implementation draws
four-or-more 8% of the time, about one in twelve.

- [x] 3c.1 Add a zero bucket at 20% and renormalize the remaining weights by
      0.8, leaving the geometric tail's ratio alone. This also brings
      four-or-more to 6.4%, which is the one-in-sixteen the requirement always
      asked for.
- [x] 3c.2 A zero draw must leave the parameter with no non-neutral depth, so
      no badge is painted for it.
- [x] 3c.4 Correct the spec's Randomize All description. It claimed Randomize
      All randomizes "each bank's local Crispy control"; it does not, and never
      did — `FroggersModulationTests.cpp:726-732` presses it eight times against
      a neutral Crispy and asserts it stays neutral. Randomize Page is what
      moves Crispy. The code is right and the requirement was stale; the delta
      restates it to match, and the manual documents the real behavior.
- [x] 3c.3 Statistical tests for both fractions, with a positive control that a
      substantial share of draws are non-zero — otherwise a randomizer stuck at
      zero would pass.

## 4. Verify against the running app

- [x] 4.1 Launch the standalone and confirm what the operator sees on the
      Audio page: the input selection reads No Input, no microphone prompt
      appears, and the two external-audio cells are blank. The suite passing
      is not this step. DONE — and it is what surfaced 3b: the first launch
      came up on a persisted device instead of No Input, which no suite caught.
- [x] 4.2 Confirm the plugin still behaves as it already does — its input
      selection already defaults to None and gates consent correctly. This
      change must not regress the one host that was right.

## 5. Close

- [x] 5.1 All gates green, counts reported.
- [x] 5.2 Postflight: implementation versus proposal, plus a duplication pass
      over the whole diff for every new named concept.
- [x] 5.3 Push Sheaf before the superproject pin.
- [x] 5.4 OPERATOR: re-test. This change exists because a requirement was
      satisfied on paper and not in the running app.

## 6. Not covered here — merge to main and release

Archived with this gap stated rather than hidden. This change ends at the
branch: it commits and pushes the work, and says nothing concrete about how it
reaches anyone.

What is missing is not a line item, it is a plan. `main` has no `app/`
directory at all and its Pages workflow still deploys `web/dist`, so the
published site is the retired v1 build; `desktop-release.yml` fires only on
`froggerstiga-v*` tags, is pinned to `froggerstiga-v1`, and builds `desktop/`
rather than `app/`; and `vst-plugin.yml` runs as build-and-test with
`contents: read`, so it cannot publish anything. None of that is a detail to
append to section 5 — the site, the desktop release and the plugin release each
need their own trace and their own gate.

That work is being written up as its own change, with the detail this one
lacks. Nothing here is blocked by it.
