# Proposal — `frogg3rs-browser-microphone-permission-path`

**Created 2026-08-29. Successor to `frogg3rs-browser-audio-device-selection`,
which is DEPLOYED and does not work.**

On the published site, on desktop, a fresh visitor gets no microphone prompt and
an Input control offering nothing but "No Input", permanently. There is no
sequence of actions that reaches a microphone.

## The deadlock, traced

1. An unpermitted page's `enumerateDevices()` returns entries whose `label` AND
   `deviceId` are both empty strings. Measured, not assumed: plain Chromium 148,
   loopback secure origin, three entries, all fields empty.
2. `audio.ts:351-360` submits them unfiltered, deliberately: deciding what is
   presentable is the native side's job.
3. `BuildBrowserAudioSnapshot` (`BrowserAudioDevices.hpp:225-227`) drops every
   entry with an empty label, because an unnamed entry that resolves to a real
   microphone is a choice nobody made.
4. So the Input list contains exactly "No Input".
5. `getUserMedia` — the only call that can raise a permission prompt — is at
   `audio.ts:464`, reachable only from `acquireInput()`, reachable only from
   `retryInput()` and `acquireInputDeviceAtIndex(index)`.
6. `acquireInputDeviceAtIndex` requires an index into a list that has no
   devices in it.
7. `retryInput()` is reachable: `BrowserAudioInputOffline` includes
   `NotRequested` (`BrowserAudioDevices.hpp:118-126`), so `showInputRetry` is
   true on a fresh page and the control renders. But retry arms from the
   persisted input name (`BrowserRuntimeMainServices.hpp`,
   `ArmPendingAudioRequest(Input, engine_.AudioDeviceSnapshot().inputDeviceName)`),
   that name is empty while No Input is selected, and an empty name falls
   through to the release sentinel. Clicking Retry releases nothing and prompts
   for nothing.

Both doors are painted on. Permission requires capture, capture requires a
selection, a selection requires a label, and a label requires permission.

## Why it shipped green

The device e2e launches Chromium with `--use-fake-device-for-media-stream`,
which populates labels WITHOUT any permission grant. That flag removes the exact
condition that breaks the feature, so the suite proved the list grows when
devices are already named and never exercised a page that has to ask.

A positive control was run and passed — inverting the label filter made the C++
tests fail — which established that the tests detect a broken filter. It did not
establish that anything tests the unpermitted path, because no test enters it.

The operator items were written asserting a state no code path can produce. That
is the same defect the superseded change was opened to correct, and the
preamble's rule on authoring work for someone else names it directly: an
operator check is a claim that the thing is OBSERVABLE, and that claim was never
traced.

## What must become true

A visitor who has never granted permission must be able to reach a working
microphone through the interface, and the route must be their own act.

Constraints that shape it, all standing and all verified:

- `getUserMedia` is the ONLY browser call that raises a permission prompt.
  `navigator.permissions.query` reports state without prompting; enumeration
  never prompts.
- `froggers-modulation-slate:89-93` forbids opening a capture device on the
  strength of an application's declared channel count. A permission request the
  operator explicitly triggers is not that, but the distinction must be built
  deliberately rather than assumed: a prompt that leaves a live stream running
  has opened a device nobody selected.
- Unlabelled entries must still not be presented as named devices. Whatever the
  operator is offered is a request for access, not a device.
- The control should not offer this where it would do nothing — a page that
  already has permission, or a browser with no input devices at all, should not
  be told to ask.

The mechanism is deliberately not prescribed here. Whoever executes this traces
it and states the design before building, because the last attempt reasoned from
a measurement to a filter without asking what the filter made unreachable.

## The desktop app has the same subject and a different defect

Traced while answering whether the browser deadlock reaches other hosts. It does
not: the JUCE host enumerates through `deviceType->getDeviceNames(true)`
(`JuceRuntimeMainServices.hpp:102`), which macOS answers regardless of
microphone permission, and the empty-label filter exists only in
`BrowserAudioDevices.hpp`. The VST owns no devices at all; the DAW does.

But `NSMicrophoneUsageDescription` appears nowhere in this repository, and
`app/Frogg3rs-Info.plist` — the plist `app/build-launcher.sh:42` bakes into the
shipped bundle — declares only `CFBundle*` keys, `LSMinimumSystemVersion` and
`NSHighResolutionCapable`. macOS requires that usage string before a process
touches a microphone.

UNVERIFIED, and it must be verified before it is fixed: the documented
consequence is that the system terminates the process rather than denying the
request. Nobody has run it. Selecting an input device in the desktop app is
therefore expected to kill it, which would mean the desktop host's input
selection has never worked on macOS since it shipped.

Establish it by running it, on a build from `app/build-launcher.sh`, before
adding the key. A fix applied to a defect nobody reproduced is a guess that
happens to compile.

## Non-goals

- Output routing, which works.
- The VST, which never enumerates devices.
- Changing what a granted, labelled device list offers.
- Removing the empty-label filter. Unlabelled entries genuinely carry no
  identity; the defect is the absence of a way to earn labels, not the filter.

## Impact

- `External/Sheaf/projects/synth/browser/src/audio.ts`
- `External/Sheaf/projects/synth/include/synth/browser/BrowserAudioDevices.hpp`,
  `BrowserRuntimeMainServices.hpp`, and the shared `RuntimePages.hpp` if a new
  control is needed
- The browser e2e, which currently cannot enter the state being fixed
- `froggers-browser-package`
