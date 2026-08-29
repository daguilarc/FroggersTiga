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

## Delay's Wet mix fades the instrument to silence

Carried here rather than opened as its own change, so one change is active at a
time. It shares nothing with the microphone work except being shipped and wrong.

Turning Delay's Wet mix up makes everything quieter, and at maximum the output
is silent. Traced:

- `Delay.hpp:136` — `float dsnd = 0.0f`. Send defaults to zero, and the Delay
  bank's layout sets no override, so a fresh instrument has it at zero.
- `inSignal = bumpIn * send` is the ONLY signal written into the delay line.
  With Send at zero the line is fed silence; `Delay.hpp:793` early-outs on
  `p.dsnd <= 0.0001f`, which `:570` describes as the path "most patches take,
  since Send defaults to" zero.
- `ToReverbMono` (`Delay.hpp:985-989`) is a crossfade:
  `(1 - mix) * bumpIn + mix * monoWet`.

So Wet mix crossfades the dry signal away against a wet path nothing feeds. At
mix = 1 the output is exactly zero. It is not a quiet delay; it is a mute knob
wearing a mix label, and it is what a new listener meets, because randomization
draws Send uniformly and randomized patches therefore do feed the line.

### The sibling control was already fixed, and Delay was left behind

`2026-08-27-frogg3rs-reverb-wetness-and-damping-floor` capped Reverb's wet mix
with `kMaxReverbWetMix`, at the MAPPED value rather than the knob range, so the
control keeps its full sweep while the mapped maximum leaves dry signal behind.
Its proposal states the cap is "exactly how little dry signal the knob can
leave". Delay's Wet mix is the identical `(1 - mix) * dry + mix * wet`
crossfade and never received one: a family capped one member at a time.

Delay therefore takes the same treatment, `kMaxDelayWetMix` mirroring
`kMaxReverbWetMix`, so no knob position can remove the dry signal entirely.

### The cap alone is not sufficient here, and the difference matters

Reverb's wet path is fed unconditionally — `ProcessReverb(output)` runs every
sample — so capping its mix bounds a signal that exists. Delay's wet path is fed
only through Send, which defaults to zero. A cap alone therefore converts
"silent at maximum" into "quieter at maximum, still with no echo", which is an
improvement to the failure mode and not a repair of it: the knob would still
only ever attenuate on the patch the instrument ships with.

RULING, 2026-08-29, both halves:

- CAP: `kMaxDelayWetMix` on the mapped value, mirroring `kMaxReverbWetMix`, so
  no knob position removes the dry signal entirely.
- AUTHORITY SCALES WITH THE WET LEVEL: Wet mix's ability to remove dry signal
  is proportional to how much signal the wet path actually holds, measured
  rather than inferred. This is what makes a low-Send high-Feedback patch behave
  — the echo is loud, so the knob earns its full travel even though little is
  being written into the line.
- WITH ONE DELIBERATE DISCONTINUITY: at Send exactly zero the knob is inert and
  the output is dry, regardless of what the line still holds.

Send's default is NOT changed. A fresh instrument still ships with no echo; what
changes is that reaching for Wet mix no longer costs the operator their sound.

The discontinuity is deliberate and has an audible cost that must be built
knowingly: a delay line holds a decaying tail after Send drops to zero. Under
wet-level scaling alone the knob would keep its authority while that tail rings
out; forcing it inert at Send zero means turning Send down CHOPS the tail rather
than letting it decay. Whoever builds this decides how abrupt that is — an
instant cut, or a short release on the authority itself so the tail is let go of
rather than severed — and says what an operator hears turning Send to zero
while echoes are still sounding.

Measuring the wet level means an adaptive stage, which can pump. Its time
constants are a decision: too fast and the knob's authority modulates with every
echo, too slow and it lags a patch change. State them and say why.

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
