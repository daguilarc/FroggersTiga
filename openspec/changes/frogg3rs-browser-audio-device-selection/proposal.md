# Proposal — `frogg3rs-browser-audio-device-selection`

**Created 2026-08-28. SUPERSEDES `frogg3rs-browser-device-enumeration`**, whose
implementation is already in both working trees, uncommitted.

## Why the predecessor is superseded rather than continued

Its task list did not cover its own spec delta. The delta required that a device
control "SHALL offer the devices the browser makes available, and selecting one
SHALL take effect", for BOTH controls. The tasks built the input path — trigger,
`deviceId`, tests — and gave output a single item, "report what an OUTPUT device
selection can do". Reporting is not delivering. Nothing calls `setSinkId`, so
output selection lists devices and routes nothing.

The preflight that approved it checked whether the plan's claims about existing
code were true and never checked whether the plan covered what it promised. That
is a preflight defect, not an execution one, and the correct repair is a proposal
whose tasks match its delta — not further edits to a delta to make it match the
tasks, which is what was being attempted when this was opened.

## State of the trees right now

Both trees carry uncommitted work. Nothing is committed, nothing is pushed, and
the wasm has NOT been rebuilt, so none of it is live in any artifact.

Delivered and in the tree:

- Input devices enumerate, cross the ABI, and build the Input list through
  `Layout::BuildDeviceOptions` — the builder both JUCE hosts already use.
- Capture no longer fires from the application's declared channel count. It
  fires on operator selection, carrying that device's id.
- The two throwing name resolvers are gone.
- ABI version 5 on both sides, consistently mirrored.
- The Pages-blocking MIDI spec measures its host's MIDI backend instead of
  assuming a permission grant implies one.

Not delivered:

- Output routing. This change delivers it.

## Traced

Measured, Chromium 148.0.7778.96, loopback secure origin:

| launch | permission | entries | labels | deviceId |
|---|---|---|---|---|
| plain | none | 3 | empty | `""` |
| plain | `microphone` granted | 3 | empty | `""` |
| plain | after `getUserMedia` | 3 | empty | threw `NotSupportedError` |
| `--use-fake-device-for-media-stream` | none | 7 | populated | 64-hex, defaults `"default"` |

A permission grant is not the quantity that populates a list. `setSinkId` exists
on both `AudioContext.prototype` and `HTMLMediaElement.prototype`;
`navigator.mediaDevices.selectAudioOutput` does not; `ondevicechange` is
assignable.

Mechanism, read end to end:

- No C++ pushes into JS. A control in the C++-built tree causes a JS-side effect
  by arming a one-shot during dispatch, which JS polls after every action —
  `BrowserRuntimeMainServices.hpp:118-121` armed, `main.ts:282` polls,
  `BrowserRuntimeAbi.cpp:97-100` exports. Input selection already rides this
  path as a pending-input-request carrying an index into the submitted list.
- Option ids are device LABELS, because `AudioDeviceState` persists a device
  NAME and that is what every host's stored selection already means.
- `showInputCombo` gates the Input control on `requestedChannels > 0`;
  frogg3rs sets `numAudioInputs = 1` (`app/FroggersAppCore.hpp:218`).

## What this changes

Output selection routes audio. `AudioContext.setSinkId` is called with the
selected device, reached the same way input selection is: an armed pending
request the JS side polls and acts on, extended to carry an output selection
rather than given a second mechanism of its own.

Where `setSinkId` does not exist — iOS browsers — the control says so instead of
offering choices that do nothing.

## Non-goals

- The desktop and VST hosts, which enumerate through JUCE already.
- Whether a real microphone's audio arrives, and whether a real output device
  receives it. A fake device proves plumbing, not hardware. Operator checks.

## Impact

- `External/Sheaf/projects/synth/include/synth/browser/BrowserAudioDevices.hpp`,
  `BrowserRuntimeMainServices.hpp`, `BrowserRuntime.hpp`, `BrowserRuntimeAbi.cpp`
- `External/Sheaf/projects/synth/browser/src/{audio,main,worker,protocol}.ts`
- `app/browser/e2e/`, and the Pages workflow's green state
- `froggers-browser-package`, whose input scenario returns here, joined by output
