# Proposal — `frogg3rs-browser-microphone-permission-path`

**Created 2026-08-29. Successor to `frogg3rs-browser-audio-device-selection`,
which is DEPLOYED and does not work. Merged 2026-08-29 with
`frogg3rs-version-single-source-and-windows-vst`, which overlapped this change
on `audio.ts` and on the Playwright server state, and is now folded in whole.**

On the published site, on desktop, a fresh visitor gets no microphone prompt and
an Input control offering nothing but "No Input", permanently. There is no
sequence of actions that reaches a microphone.

## The deadlock, traced

1. An unpermitted page's `enumerateDevices()` returns entries whose `label` AND
   `deviceId` are both empty strings. Measured, and the measurement is recorded
   in the tree: `app/browser/e2e/playwright.config.mjs:40-50` states that a
   plain launch reports three placeholder entries with both fields empty, and
   that granting the `microphone` permission alone does not change that.
2. `audio.ts:370` submits them unfiltered, deliberately: the comment at
   `:351-368` records that deciding what is presentable is the native side's
   job.
3. `BuildBrowserAudioSnapshot` drops every entry with an empty label
   (`BrowserAudioDevices.hpp:238`, reasoned at `:225-227`), because an unnamed
   entry that resolves to a real microphone is a choice nobody made.
4. So the Input list contains exactly "No Input".
5. `getUserMedia` — the only call that can raise a permission prompt — is at
   `audio.ts:464`, reachable only from `acquireInput()` (`:429`), reachable only
   from `retryInput()` (`:208`) and `acquireInputDeviceAtIndex(index)` (`:222`).
6. `acquireInputDeviceAtIndex` requires an index into a list that has no
   devices in it.
7. `retryInput()` is reachable: `BrowserAudioInputOffline` includes
   `NotRequested` (`BrowserAudioDevices.hpp:118-126`), so `showInputRetry` is
   true on a fresh page and the control renders. But retry arms from the
   persisted input name (`BrowserRuntimeMainServices.hpp:136-137`,
   `ArmPendingAudioRequest(Input, engine_.AudioDeviceSnapshot().inputDeviceName)`),
   that name is empty while No Input is selected, and `ArmPendingAudioRequest`
   (`:304-319`) sets `kReleaseAudioRequest` (`BrowserAudioDevices.hpp:43`) for
   an empty name. Clicking Retry releases nothing and prompts for nothing.

Both doors are painted on. Permission requires capture, capture requires a
selection, a selection requires a label, and a label requires permission.

Step 7's last sentence is the one claim here that is structural rather than
observed, so it is not relied on: task 1.2 runs it before anything is designed
on top of it.

## Why it shipped green

The device e2e launches Chromium with `--use-fake-device-for-media-stream`
(`app/browser/e2e/playwright.config.mjs:120`), which populates labels WITHOUT
any permission grant. That flag removes the exact condition that breaks the
feature, so the suite proved the list grows when devices are already named and
never exercised a page that has to ask.

A positive control was run and passed — inverting the label filter made the C++
tests fail — which established that the tests detect a broken filter. It did not
establish that anything tests the unpermitted path, because no test enters it.

The operator items were written asserting a state no code path can produce. That
is the same defect the superseded change was opened to correct.

## What must become true

A visitor who has never granted permission must be able to reach a working
microphone through the interface, and the route must be their own act.

Constraints that shape it, all standing and all verified:

- `getUserMedia` is the ONLY browser call that raises a permission prompt.
  `navigator.permissions.query` reports state without prompting; enumeration
  never prompts.
- `froggers-modulation-slate:90-93` forbids opening a capture device on the
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

## The macOS build ships without a microphone declaration

The predecessor recorded this as an open question, on the strength of a search
for `NSMicrophoneUsageDescription` that returned nothing. That search was for
the concept's OUTPUT spelling. Searched by the operand JUCE actually uses, the
declaration is there, and has been all along:

    app/standalone/CMakeLists.txt:107-108
      MICROPHONE_PERMISSION_ENABLED TRUE
      MICROPHONE_PERMISSION_TEXT "Frogg3rs needs audio input access for
        external ring-mod (mic, line-in, or interface)."

So it is not an omission. It is a declaration made on the wrong side of a
two-build-system split:

- `app/standalone/CMakeLists.txt:1` states that file IS the Windows standalone
  build, and `.github/workflows/desktop-release.yml:62-75` confirms it, giving
  the reason: Sheaf's `juce_build.mk` drives MinGW on that runner and cannot
  compile JUCE's headers there, so Windows uses CMake and MSVC instead.
- macOS ships from `app/build-launcher.sh` (`desktop-release.yml:32-36`), which
  drives `juce_build.mk`. That Makefile has no plist-generation step at all: its
  bundle rule (`:198-207`) copies `APP_INFO_PLIST` verbatim at `:205`.
- `app/build-launcher.sh:42` supplies `app/Frogg3rs-Info.plist`, which declares
  only `CFBundle*` keys, `LSMinimumSystemVersion` and `NSHighResolutionCapable`.

Measured on the built bundles on disk, not inferred: the shipped
`app/build-launcher/Frogg3rs.app/Contents/Info.plist` contains 0 occurrences of
`NSMicrophoneUsageDescription`; the CMake bundle at
`app/standalone/build/FroggersStandalone_artefacts/Release/Frogg3rs.app/Contents/Info.plist`
contains 1, carrying the string above.

The declaration therefore sits on the platform where the key is inert — Windows
has no equivalent gate — and is absent from the only platform that enforces it.
Frogg3rs does request capture: `app/FroggersAppCore.hpp:218` sets
`config.numAudioInputs = 1`, and `Runtime.hpp:287` gates input selection on that
being nonzero. Sheaf's own README states the invariant this breaks
(`External/Sheaf/projects/synth/README.md:675-678`): macOS bundles built from
`juce_build.mk` carry the key, which macOS requires before it will show the
prompt for an app that opens an input device. All three sibling bundles built
that way carry it — `apps/miniapp/Info.plist:24`,
`apps/controllers_harness/Info.plist:18`, `apps/sheaf-patch/Info.plist:24`.
Frogg3rs is the only one that does not.

Why the earlier search missed it twice over is worth recording, because the
shape recurs: the key is generated, so it exists as a literal only in build
output, and `app/.gitignore:6` and `:11` exclude `build/` and `build-launcher/`
from a repo-root recursive grep. Searching the generated spelling could only
have found a file the search tool was configured not to read.

What remains genuinely behavioural is what macOS DOES when the shipped bundle
opens the device — whether it prompts, denies silently, or terminates. Task 6.1
runs it. The fix does not wait on that answer; the run decides only how the
failure reads today.

The browser deadlock does not reach the JUCE host. It enumerates through
`deviceType->getDeviceNames(true)` (`JuceRuntimeMainServices.hpp:102`), which
macOS answers regardless of microphone permission, and the empty-label filter
exists only in `BrowserAudioDevices.hpp`. The VST owns no devices at all; the
DAW does, and `app/vst/CMakeLists.txt` sets no plist source and no
`MICROPHONE_PERMISSION_ENABLED`, which is correct for a plugin that opens
nothing.

## Delay's Wet mix fades the instrument to silence

Turning Delay's Wet mix up makes everything quieter, and at maximum the output
is silent. Traced:

- `Delay.hpp:136` — `float dsnd = 0.0f`. Send defaults to zero, and the Delay
  bank's layout sets no override (`FroggersParameters.hpp:91`, `defaultValue`
  defaults to `0.0f`; the Delay entry at `:219-222` names its slots without
  overriding Send's), so a fresh instrument has it at zero.
- `inSignal = bumpIn * send` is the ONLY signal written into the delay line.
  With Send at zero the line is fed silence; `Delay.hpp:792` early-outs on
  `p.dsnd <= 0.0001f`, which `:569-570` describes as the path "most patches
  take, since Send defaults to" zero.
- `ToReverbMono` (`Delay.hpp:984-990`) is a crossfade:
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
with a bank-named `kMaxReverbWetMix`, at the MAPPED value rather than the knob
range, so the control keeps its full sweep while the mapped maximum leaves dry
signal behind. Its proposal states the cap is "exactly how little dry signal the
knob can leave"
(`archive/2026-08-27-frogg3rs-reverb-wetness-and-damping-floor/proposal.md:24`).
Delay's Wet mix is the identical `(1 - mix) * dry + mix * wet` crossfade and
never received one: a family capped one member at a time.

Capping Delay with a SECOND constant of the same value would leave the family
still expressed twice, which is the same defect one level up. Both banks
therefore read one `kMaxWetMix`, declared beside the other knob constants
`RouteAudioSample` already shares (`FroggersAppCore.hpp`, the block that
carries `kStopUnityDriveKnob`). The bank-named constant is gone. If a bank ever
needs a different ceiling, splitting it then carries a reason; splitting it now
would only record that two edits happened on different days.

That family already has a requirement — `froggers-sheaf-parameter-model:283`,
"The reverb wet mix always leaves dry signal in the output". This change
GENERALIZES that requirement rather than adding a second one beside it. Adding
would reproduce, in the spec, the exact defect the code is being fixed for.

### The cap alone is not sufficient here, and the difference matters

Reverb's wet path is fed unconditionally — `ProcessReverb(output)` runs every
sample — so capping its mix bounds a signal that exists. Delay's wet path is fed
only through Send, which defaults to zero. A cap alone therefore converts
"silent at maximum" into "quieter at maximum, still with no echo", which is an
improvement to the failure mode and not a repair of it: the knob would still
only ever attenuate on the patch the instrument ships with.

RULING, 2026-08-29, all three parts:

- CAP: the mapped value, bounded by the same `kMaxWetMix` Reverb already uses, so
  no knob position removes the dry signal entirely.
- AUTHORITY SCALES WITH THE WET LEVEL: Wet mix's ability to remove dry signal
  is proportional to how much signal the wet path actually holds, measured
  rather than inferred. This is what makes a low-Send high-Feedback patch behave
  — the echo is loud, so the knob earns its full travel even though little is
  being written into the line.
- THE DISCONTINUITY LIVES IN THE TARGET, NOT IN THE OUTPUT: at Send exactly zero
  the follower's target is forced to zero at once, rather than tracking the tail
  the line still holds. Authority then decays at the release constant, so what
  an operator hears is a fade of about 100ms, not a cut. The spec delta states
  it this way; an earlier draft said "inert at once", which describes the target
  and would have failed any test written against the output.

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
- RELEASE 100ms: `kSharedReleaseSeconds` (`Limiter.hpp:76`), already in the
  tree, its comment at `:72-75` recording it as derived from measured residual
  decay and "confirmed correct for delay and reverb too". The wet limiter
  already applies it to this exact signal (`Delay.hpp:129`), so authority tracks
  at a rate this path is known not to pump at, and no new constant is
  introduced.
- ATTACK 10ms, matching `VcoEnvelopeFollowers`. Authority rising quickly is the
  safe direction. The limiter's 1ms attack exists to catch peaks and would be
  needlessly twitchy for a control's authority.

Two structural facts constrain where the follower can live, and the plan states
them rather than leaving them to be discovered:

- `ToReverbMono` is `const` (`Delay.hpp:985`), so it cannot hold or update
  follower state. The follower belongs in `Process`, which already computes
  `lastWet`.
- `Process` early-returns at `Delay.hpp:792` when `p.dsnd <= 0.0001f`, BEFORE
  any wet computation. Forcing the target to zero on that path therefore
  requires the follower update to be reached on it — by hoisting the update
  above the early return, or by folding the early return into it. An earlier
  draft claimed this cost "no extra branch"; the early return is the branch, and
  the work is to route through it, not to avoid it.

## The stereo delay is mono by the time it is heard

`StereoDelay` is genuinely stereo inside: two lines, a cross-feed, per-channel
limiters. But `ToReverbMono` folds L and R together, and everything downstream
is a single float — `delayOut` (`FroggersAppCore.hpp:1826`) and `reverbOut`
(`:1872`) are both scalars. So no stereo image from this stage reaches the
output.

The consequence for the Delay bank's "Stereo width" control is that it cannot
widen anything, and its cross-feed cancels exactly in a sum:

    fbL + fbR = dL(1-cross) + dR·cross + dR(1-cross) + dL·cross = dL + dR

independent of `cross`. Its only remaining influence is indirect, through how
the feedback loop evolves.

This does NOT contribute to the reported quietness. The code's own comment
records that the cross-feed "already keeps L and R close to each other in
practice", so the fold is not losing level to decorrelation.

RULING, 2026-08-29: the signal SHALL NOT be collapsed to mono in the middle of
the chain. Folding belongs at the output, and only where the device itself is
mono — a phone speaker, a single-channel interface.

Traced, so the cost is known rather than discovered:

- Everything upstream of the delay is already scalar: `driveOut` and `filterOut`
  (`FroggersAppCore.hpp:1637`, `:1777`). The source is mono and stays mono.
- The delay is the first and only stereo stage, and `ToReverbMono` discards it.
- `Reverb` is ALREADY stereo inside. It carries `wetL`/`wetR`
  (`Reverb.hpp:225-226`), a two-line tank, and a Width control, then folds at
  `:577`: `const float wet = 0.5f * (wetL + wetR);`
- `FroggersAppCore.hpp:1158-1163` writes the same sample to every output
  channel, so the fold to N channels is a copy today.

So carrying stereo to the output is PLUMBING, NOT COMPUTATION. Both stages
already compute their pairs and discard them on the next line. No second reverb
instance is needed: the topology is stereo already.

### Both Width controls are mathematically inert

The Reverb's Width cancels exactly, for the same reason the Delay's does. At
`Reverb.hpp:573-577`, with `mid = 0.5(aOut + bOut)`:

    wetL + wetR = 2·mid + width·((aOut − mid) + (bOut − mid))
                = 2·mid + width·(aOut + bOut − 2·mid)
                = 2·mid

so `wet == mid` at every knob position. Reverb Width does nothing at all today,
and Delay's Stereo width survives only through the feedback loop's evolution.
Both become real controls the moment the fold moves to the device — which is the
strongest argument for this work: it does not add a feature, it connects two
that are already built and paid for.

The Daisy firmware is NOT affected. It is a separate codebase:
`src/FroggersSolo/Makefile:2` builds `FroggersSolo.cpp` against `src/core/`,
which carries its own reverb (`FroggersEngine.hpp:501`), and nothing under
`src/` includes `app/`. This work touches the desktop, VST and browser hosts
only.

This is the largest item in the change and depends on none of the others. It is
sequenced last.

## The browser ABI version is a literal in dozens of places

`SUPPORTED_BROWSER_ABI_VERSION` exists (`protocol.ts:2`, currently 6), and
almost nothing uses it. The same number is hand-written as a bare literal across
the tree: the C++ return (`browser/cpp/BrowserRuntimeAbi.cpp:28-31`), the C++
contract test's assertion (`tests/browser_runtime_contract_tests.cpp:1170`),
every `_synth_browser_abi_version: () => 6` stub including
`browser/src/static-server.mjs:37`, the synthesized fixture catalog in the same
file at `:84`, and `abiVersion: 6` literals across the Playwright specs.
A bump means finding all of them.

This is not hypothetical drift. Bumping the version during the predecessor
missed the fixture-catalog literal in `static-server.mjs`, because it and the
stub above it express one concept in two syntactic forms and a search by form
finds one of them.

The version is one fact. It gets one definition that every other site reads,
including the fixtures. Where a language boundary makes literal sharing
impossible, the mirror is generated or asserted equal by a test that fails on
drift, not maintained by hand.

Not every nearby number is a version, and the classification is part of the
work: `_synth_browser_audio_input_channels: () => 4`
(`browser/tests/runtime-core.spec.ts:257`) is a channel count and
`_synth_browser_create: () => 41` (`static-server.mjs:40`) is a handle.

## A test run can silently use a server that predates the code

`static-server.mjs` had been serving for over 24 hours when the predecessor's
suite ran against it, because Playwright reuses an existing server rather than
starting its own — `playwright.shared-config.mjs:11` sets
`reuseExistingServer: true` unconditionally. It serves files from disk, so app
code was fresh, but its synthesized fixture catalog is in-process and therefore
frozen at whatever the process was started with. One test failed against a
fixture that predated the ABI bump by a day, and the failure was
indistinguishable from a real defect until the process start time was read.

A reused server must be proven current before a run trusts it, or not reused.

One detail the predecessor's framing got wrong, and which matters for whichever
mechanism is chosen: `static-server.mjs:169-171` binds 4173, 4174 AND 4175 from
ONE process, and `playwright.shared-config.mjs:10` gates readiness on 4173
alone. So the hazard is not a partially-started server — that window is
milliseconds — it is a fully-started STALE one. Any instruction to "make sure
all three ports are listening" addresses the wrong failure.

## There is no Windows VST3, and no recorded reason

The desktop application ships for macOS and Windows. The plugin ships for macOS
only. `2026-08-26-frogg3rs-windows-and-mobile` listed "A Windows VST3" as an
explicit non-goal (`proposal.md:164`) and nothing has been opened since, so the
absence is a scoping decision that was never revisited, not a technical finding.

Traced, so the work is known rather than guessed:

- `app/vst/CMakeLists.txt:98` is `FORMATS VST3 AU` with no platform
  conditional. AU is macOS-only, so that line cannot build on Windows as
  written. VST3 is cross-platform.
- The doc-bundling loop (`:127`) and the signing loop (`:152`) both iterate
  `VST3 AU` and use `$<TARGET_BUNDLE_CONTENT_DIR:...>` and `codesign`. Both are
  macOS-shaped: `codesign` does not exist on Windows, and a Windows VST3 is a
  directory laid out differently from a macOS bundle.
- `.github/workflows/vst-plugin.yml:20` builds only on `macos-14`; its
  `ubuntu-latest` job (`:107`) publishes. `desktop-release.yml:51` already
  builds on `windows-latest`, and `72700a9` fixed Sheaf submodule checkout
  there, so the runner, toolchain and submodule path are proven for this repo.
- The workflow's packaging step uses `ditto` (`:89-90`) and asserts `test -d`
  on `Frogg3rs.vst3` and `Frogg3rs.component` (`:71-72`) — both macOS-only
  assumptions.
- No Authenticode certificate exists, so a Windows VST3 ships unsigned, exactly
  as the Windows standalone already does.
- `MANUAL.md:21` is the `## Release platforms` section, extracted verbatim into
  every published release body by both workflows
  (`vst-plugin.yml:125`, `desktop-release.yml:134`, identical `awk` bounded on
  `^## ` and `^---$`).

The distribution delta ADDS its requirement rather than modifying one. No
requirement named "The plugin ships for every platform the application ships
for" exists in `openspec/specs/frogg3rs-distribution/spec.md`; the closest,
"Each artifact has its own release, named for what it is" (`:35`), already
covers release-notes drift and is left alone.

## Non-goals

- Output routing, which works.
- Changing what a granted, labelled device list offers.
- Removing the empty-label filter. Unlabelled entries genuinely carry no
  identity; the defect is the absence of a way to earn labels, not the filter.
- Changing plugin behaviour on any platform. The Windows work is packaging and
  build configuration.
- An LV2 or a Linux build.
- Signing Windows artifacts. There is no certificate, and inventing one is a
  business decision, not an engineering one.
- The Daisy firmware, which shares no code with `app/`.

## Impact

- `External/Sheaf/projects/synth/browser/src/audio.ts`
- `External/Sheaf/projects/synth/include/synth/browser/BrowserAudioDevices.hpp`,
  `BrowserRuntimeMainServices.hpp`, and the shared `RuntimePages.hpp` if a new
  control is needed
- `External/Sheaf/projects/synth/browser/src/protocol.ts`,
  `browser/cpp/BrowserRuntimeAbi.cpp`, `browser/src/static-server.mjs`,
  `browser/playwright.shared-config.mjs`, and the ABI version's test mirrors
- `app/Frogg3rs-Info.plist`
- `app/dsp/Delay.hpp`, `app/dsp/Reverb.hpp`, `app/FroggersAppCore.hpp`
- `app/vst/CMakeLists.txt`, `app/vst/FroggersPluginProcessor.*` if the stereo
  path reaches the plugin's output
- `.github/workflows/vst-plugin.yml`
- `MANUAL.md` "Release platforms"
- The browser e2e, which currently cannot enter the state being fixed
- `froggers-browser-package`, `froggers-sheaf-parameter-model`,
  `frogg3rs-distribution`
