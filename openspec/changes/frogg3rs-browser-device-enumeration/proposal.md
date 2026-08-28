# Proposal — `frogg3rs-browser-device-enumeration`

**Created 2026-08-28. SUPERSEDES `2026-08-28-frogg3rs-first-visit-and-open-repairs`**
for everything it said about audio input, and takes over its operator item 6.2.

That change synced a requirement into `froggers-browser-package` stating that
after granting microphone permission "the Input device control offers at least
one device besides No Input". The code cannot satisfy it and never could. The
requirement was inherited from `frogg3rs-web-release-repair`, carried through a
preflight that checked the delta's prose and not its scenarios, and adopted
into the main spec by syncing — which makes someone else's assertion the
project's own settled requirement. The scenario has been removed from the main
spec, and returns here as the thing to actually deliver.

Its operator item 6.2 — a real microphone reaching External Audio — was handed
over as a test that could pass. It could not. Nothing observable on that page
could have shown a working microphone, so time spent on a phone chasing it was
spent against a control that had already been made unobservable.

The browser build's audio device controls cannot select a device. Both
dropdowns are compile-time constants, and selecting anything other than the
single constant throws.

## Traced

`BuildBrowserAudioSnapshot` (`BrowserAudioDevices.hpp:180-202`) builds the Audio
I/O page's model. It hardcodes both lists:

    snapshot.outputOptions = {{kSystemDefaultOptionId, kSystemDefaultOptionLabel}};   // :183
    snapshot.inputOptions  = {{kNoInputOptionId, kNoInputOptionLabel}};               // :196

`:196`'s own comment states the intent — "Browser device ids are privacy-scoped,
so nothing is enumerated here" — so this is deliberate, not an oversight.

Selection cannot widen it. Both name resolvers reject anything else:

    BrowserOutputDeviceName (:205) throws unless optionId == kSystemDefaultOptionId
    BrowserInputDeviceName  (:214) throws unless optionId == kNoInputOptionId

Nothing on the browser path calls `navigator.mediaDevices.enumerateDevices()`.
The only `enumerateDevices` in the runtime is the JUCE-side callback
(`BrowserRuntimeMainServices.hpp:54`), which is a different mechanism.

So the Input control offers exactly one choice and that choice is "off", and
the Output control offers exactly one choice and that choice is "whatever the
system picked". Granting microphone permission cannot change either, because
neither list is derived from the browser at all.

Capture is a separate mechanism and is NOT the subject here: `requestedChannels`
comes from the application's own `AudioInputChannels()`
(`BrowserRuntime.hpp:589`), not from device selection, so whether the default
device is captured is independent of these dropdowns. That question is
UNVERIFIED and is not answered by this change.

## What this changes

Both dropdowns enumerate real devices, and selecting one takes effect.

The privacy constraint the comment names is real and shapes the design rather
than blocking it: `enumerateDevices()` returns entries with empty labels until a
capture permission has been granted, and device ids are origin-scoped and reset
when storage is cleared. So the list is a live query whose labels populate after
permission, not a persisted table, and a stored selection is re-resolved against
the current enumeration rather than trusted.

## Non-goals

- Whether capture works at all on the default device. Separate mechanism, and
  currently unverified either way.
- Output device routing on browsers without `setSinkId`. Where it is
  unavailable, the control reports that rather than pretending.
- The desktop and VST hosts, which enumerate through JUCE already.

## This cannot be done without touching Sheaf

Traced, because it is the first question anyone will ask:

- `BrowserAudioDevices.hpp` and `BrowserRuntime.hpp` exist ONLY under
  `External/Sheaf/`. There is no frogg3rs-side copy to edit instead.
- frogg3rs's own browser sources are `site-boot.mjs`, `mobile-stack.mjs`,
  `package-catalog.mjs` and `serve-site.mjs`. None touches the audio device
  model.
- The out-of-tree extension point that exists, `kAudioAppSection`
  (`RuntimePages.hpp:61,937`), mounts an app-supplied section BENEATH the
  device and status lines. It cannot replace the dropdowns, which
  `BuildBrowserAudioSnapshot` builds above that mount.

This is not special-casing frogg3rs and so does not cross sbac-8: no Sheaf
browser application can enumerate audio devices, so this is a capability gap in
the host, which is what the `fix-out-of-tree-app-gaps` branch and PR #9 are
for. It lands there, and frogg3rs takes it through a pin bump.

Consequence for delivery: the snapshot is C++ compiled into the wasm, so the
browser bundle must be REBUILT. Repackaging the site alone does not carry it.

## Impact

- `External/Sheaf/projects/synth/include/synth/browser/BrowserAudioDevices.hpp`
  — both option lists, and both name resolvers, which currently throw.
- The browser runtime's TypeScript layer and the wasm ABI that carries a device
  list across it, neither of which passes one today.
- `froggers-browser-package`, whose synced requirement asserted an input device
  could be chosen. That scenario was removed from the main spec because the code
  cannot satisfy it; it returns here as the thing this change delivers.
