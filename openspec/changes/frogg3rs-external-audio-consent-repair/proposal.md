# Proposal — `frogg3rs-external-audio-consent-repair`

**Created 2026-08-21. Rewritten 2026-08-21 after preflight.** Takes priority
over the remaining work of `frogg3rs-automation-view-and-musical-ranges`,
which is otherwise complete and awaiting operator testing. That change is not
blocked by this one; this one is simply ahead of it.

## Why

Opening the standalone raises the macOS microphone prompt and starts
capturing, with the operator having chosen nothing. There is no way to
decline: the input list offers "System Default" and every enumerated device,
and "System Default" is not a way to say no — it resolves to the concrete
platform microphone on purpose.

**The mechanism, traced.**

- The app asks for one input channel
  (`app/FroggersAppCore.hpp:211`, `config.numAudioInputs = 1`), and `Start()`
  passes that same count straight to
  `deviceManager_.initialiseWithDefaultDevices(config.numAudioInputs, config.numAudioOutputs)`
  (`External/Sheaf/projects/synth/runtime/Runtime.hpp:261`). A nonzero count
  opens a capture device at launch, before any selection exists.
- Asking for an input *channel* and opening an input *device* are separate
  acts. The channel count is portable state — `tests/support/SynthRig.hpp`
  consumes the same field to size a buffer and never touches
  `AudioDeviceManager` at all. Only `Runtime.hpp:261` opens a device.
- `ResolveInputDeviceName` (`Runtime.hpp:646-659`) then rewrites the empty
  host-neutral name into the concrete native default whenever the app asked
  for input, at both of its call sites (`:474`, `:944`). So choosing "System
  Default" captures too.
- `BuildDeviceOptions` (`include/synth/RuntimePages.hpp:349-357`) prepends
  "System Default" and nothing else. No entry means no input.

**What is already correct, and is therefore not in scope.** The preflight
trace found the two things the first draft of this proposal claimed were
broken already working:

- The routed signal does not believe a host-opened device. It compares the
  operator's *selected* name against the *open* one
  (`Runtime.hpp:688-694`); the platform default reaches only the second, so
  the predicate derives false. This is read from the predicate, not inferred
  from its test: `RuntimeShellSessionTests.cpp:592-631` asserts all three
  cases, positive control included, but the binary it lives in dies before
  reaching it and no gate builds that binary at all. The assertion is real and
  has never run.
- A disconnected cell is already invisible and inert, not merely inert.
  `BuildEncoderDrawCommands` returns no commands for it
  (`include/synth/EncoderDraw.hpp:649-656`), and the surface sets neither
  `action` nor `pointerDragAction` while disconnected
  (`app/FroggersUiSurface.hpp:1699-1712`). There is nothing to grey out and
  no edit seam to refuse at.

So the repair is consent at the device, not derivation at the signal.

## What Changes

- **froggers-modulation-slate** (delta): MODIFIED — declining input is a real
  and default choice in every host that selects devices, and the disconnected
  rendering already in place is stated rather than left implicit.
- **External/Sheaf**: the input list's first entry becomes **No Input** and
  the default selection; the empty device name recovers its plain meaning of
  "no input device" rather than being resolved to the platform default; the
  host opens no input device until the operator selects one, and opens the
  requested channels when they do. Appended to the open pull request on the
  pinned branch.
- No `app/` code changes are expected. The app already derives everything
  from the routed signal.

**Scope is wider than a Froggers-local repair.** `Runtime.hpp`'s
"an input-capable application opens the default input device at launch" is a
Sheaf contract that its own suite asserts in seven places. This changes that
contract for every app on the runtime, which is the point: no app should open
a capture device nobody asked for.

**Channel bits are part of the fix, not an afterthought.** Neither
`ApplyAudioDeviceInputSelection` nor `ApplyInputDeviceSetup` writes
`inputChannels` or `useDefaultInputChannels` — those fields are set once, by
the startup `initialiseWithDefaultDevices` call. Stopping that call from
requesting inputs without also setting them on the selection path would leave
a selected device open with zero input channels: consent honored, capture
broken.

## Impact

- Affected specs: `froggers-modulation-slate`.
- Upstream: same fork branch and pull request as the bank-addressed write.
  Push Sheaf before the superproject pin.
- Seven Sheaf tests assert the behavior being removed and are retargeted, not
  deleted; they are listed in the tasks.
