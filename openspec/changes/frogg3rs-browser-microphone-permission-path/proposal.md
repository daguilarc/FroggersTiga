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

What IS established: `NSMicrophoneUsageDescription` appears nowhere in this
repository, and `app/Frogg3rs-Info.plist` — the plist `app/build-launcher.sh:42`
bakes into the shipped bundle — declares only `CFBundle*` keys,
`LSMinimumSystemVersion` and `NSHighResolutionCapable`.

What happens when the desktop app opens an input device is an OPEN QUESTION,
not a claim. Nobody has run it. It is answered by running it, and the answer
decides whether there is anything here to fix at all.

## Delay's Wet mix fades the instrument to silence

Carried here rather than opened as its own change, so one change is active at a
time. It shares nothing with the microphone work except being shipped and wrong.

Turning Delay's Wet mix up makes everything quieter, and at maximum the output
is silent. Traced:

- `Delay.hpp:136` — `float dsnd = 0.0f`. Send defaults to zero, and the Delay
  bank's layout sets no override (`FroggersParameters.hpp:91`, `defaultValue`
  defaults to `0.0f`; the Delay entry at `:219-222` names its slots without
  overriding Send's), so a fresh instrument has it at zero.
- `inSignal = bumpIn * send` is the ONLY signal written into the delay line.
  With Send at zero the line is fed silence; `Delay.hpp:793` early-outs on
  `p.dsnd <= 0.0001f`, which `:570` describes as the path "most patches take,
  since Send defaults to" zero.
- `ToReverbMono` (`Delay.hpp:985-989`) is a crossfade:
  `(1 - mix) * bumpIn + mix * monoWet`.

So Wet mix crossfades the dry signal away against a wet path nothing feeds. At
mix = 1 the output is exactly zero. It is not a quiet delay; it is a mute knob
wearing a mix label, and it is what a new listener meets, because randomization
draws Send uniformly and randomized patches therefore do feed the line: page
parameters are randomized through `RandomizeVisibleValue(scene,
NextRandomValue())` (`ParameterModulation.cpp:2893`, depths at `:2931`), and
`NextRandomValue` returns `std::generate_canonical<float, 24>` (`:3595-3599`),
a uniform draw across the knob's whole range.

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

### The measurement, derived for least added compute

The wet limiter's envelope CANNOT serve as the measurement: `Limiter.hpp:146`
defines it as "current gain multiplier; 1.0f == no reduction", so it reports
limiting rather than level and sits at exactly 1.0 for any signal under
threshold. A quiet echo and silence are the same value to it.

Cheapest measurement that works, reusing what is already computed:

- `ToReverbMono` already forms `monoWet = (wet.l + wet.r) * 0.5f`. Follow THAT,
  so the measurement is one follower rather than one per channel, on a value
  already in hand: one `fabs`, one compare to select the coefficient, one
  multiply-add, one float of state. About three operations per sample.
- RELEASE 100ms: `kSharedReleaseSeconds`, already in the tree, its comment
  recording it as derived from measured residual decay and "confirmed correct
  for delay and reverb too". The wet limiter already applies it to this exact
  signal, so authority tracks at a rate this path is known not to pump at, and
  no new constant is introduced.
- ATTACK 10ms, matching `VcoEnvelopeFollowers`. Authority rising quickly is the
  safe direction. The limiter's 1ms attack exists to catch peaks and would be
  needlessly twitchy for a control's authority.

The fadeoff costs nothing additional. With Send at zero the follower's TARGET is
forced to zero, so authority decays at the same 100ms release — a fade rather
than a cut, with no second envelope, no extra branch, and no new constant. The
discontinuity lives in the target; what an operator hears is a fade.

### Found while tracing: the stereo delay is mono by the time it is heard

Not fixed here, recorded because it was found and because it is the same family
as the rest.

`StereoDelay` is genuinely stereo inside: two lines, a cross-feed, per-channel
limiters. But `ToReverbMono` folds L and R together, and everything downstream
is a single float — `delayOut` and `reverbOut` in `FroggersAppCore.hpp:1826` and
`:1847` are both scalars. So no stereo image from this stage reaches the output.

The consequence for the Delay bank's "Stereo width" control is that it cannot
widen anything, and its cross-feed cancels exactly in a sum:

    fbL + fbR = dL(1-cross) + dR·cross + dR(1-cross) + dL·cross = dL + dR

independent of `cross`. Its only remaining influence is indirect, through how
the feedback loop evolves. A control named for an image the signal path cannot
carry.

This does NOT contribute to the reported quietness. The code's own comment
records that the cross-feed "already keeps L and R close to each other in
practice", so the fold is not losing level to decorrelation.

RULING, 2026-08-29: the signal SHALL NOT be collapsed to mono in the middle of
the chain. Folding belongs at the output, and only where the device itself is
mono — a phone speaker, a single-channel interface.

Traced, so the cost is known rather than discovered:

- Everything upstream of the delay is already scalar: `driveOut`, `filterOut`
  (`FroggersAppCore.hpp:1637`, `:1777`). The source is mono and stays mono.
- The delay is the first and only stereo stage, and `ToReverbMono` discards it.
- `Reverb` is ALREADY stereo inside. It carries `wetL`/`wetR` (`Reverb.hpp:225`),
  a two-line tank, and a Width control, then folds at `:577`:
  `const float wet = 0.5f * (wetL + wetR);`
- `FroggersAppCore.hpp:1141-1143` writes the same sample to every output
  channel, so the fold to N channels is a copy today.

So carrying stereo to the output is PLUMBING, NOT COMPUTATION. Both stages
already compute their pairs and discard them on the next line. No second reverb
instance is needed: the topology is stereo already.

### Both Width controls are mathematically inert

The Reverb's Width cancels exactly, for the same reason the Delay's does. With
`mid = 0.5(aOut + bOut)`:

    wetL + wetR = 2·mid + width·((aOut − mid) + (bOut − mid))
                = 2·mid + width·(aOut + bOut − 2·mid)
                = 2·mid

so `wet == mid` at every knob position. Reverb Width does nothing at all today,
and Delay's Stereo width survives only through the feedback loop's evolution.
Both become real controls the moment the fold moves to the device — which is the
strongest argument for this work: it does not add a feature, it connects two that
are already built and paid for.

What is gained is a stereo delay-and-reverb ambience over a mono source, which
is the ordinary shape for a synth of this kind.

The Daisy firmware is NOT affected. It is a separate codebase:
`src/FroggersSolo/Makefile` builds `FroggersSolo.cpp` against `src/core/`, which
carries its own reverb (`FroggersEngine.hpp:501`), and nothing under `src/`
includes `app/`. This work touches the desktop, VST and browser hosts only.

This is the largest item in this change and depends on none of the others.
Sequence it last, or split it out the moment a second change is allowed to be
open.

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
