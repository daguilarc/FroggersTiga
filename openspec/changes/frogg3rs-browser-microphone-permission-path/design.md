# Design — the route from an unpermitted page to a microphone

Stated before building, because the previous attempt reasoned from a measurement
to a filter without asking what the filter made unreachable.

## The control

A button, mirroring `Retry Input` exactly: same form row, same caption column,
same "C++ arms a pending request, JS performs the effect" shape.

- Node id `runtime.audio.input.permission`
- Action name `audio-input-permission`
- Label `Allow Microphone`
- Snapshot flag `showInputPermissionRequest`

It is a button rather than an entry in the Input combo because what it offers is
not a device. An entry in a device list is a device to choose; this is a request
for access, and the spec requires it be presented as such.

## When it is offered

`BuildBrowserAudioSnapshot` sets `showInputPermissionRequest` when the submitted
device list holds at least one input entry whose label is empty AND yields no
named input options.

That condition IS "input devices exist but cannot be named", which is exactly the
unpermitted state, and it needs no permission query at all:

- Permission already held — labels populate, no empty-label input entries, false.
- No input device present — no input entries at all, false.
- Not an input application — `showInputCombo` is false and the builder returns
  before reaching this, false.

`navigator.permissions.query` is therefore not used. It reports state without
prompting, but its cross-browser availability is a fact that would have to be
measured, and the device list already carries the same answer.

## What happens to the stream a prompt necessarily opens

`getUserMedia` is the only call that prompts, so granting necessarily opens a
device for some interval. The interval ends before the call's own continuation
returns:

1. `getUserMedia({ audio: true })` with NO `deviceId` constraint. A permission
   request names no device.
2. On resolve, every track on the returned stream is stopped immediately. The
   stream is never handed to `this.input`, never wired to an audio node, and
   never recorded as the acquired input.
3. Enumerate again and resubmit, so labels populate and the list fills.
4. The input selection is untouched. Nothing writes `inputDeviceName`, so it
   stays No Input.

On rejection the failure is classified through the same `classifyCaptureFailure`
path `acquireInput` already uses, and reported through the input status line.

## Why this does not break the standing requirements

`froggers-modulation-slate:90-93` forbids opening a capture device on the
strength of an application's declared channel count. This open is not that: it is
triggered by an explicit operator action, it carries no device identity, it
requests no channel count, and it is closed before the operator can observe it.

`froggers-modulation-slate:168-176` requires that selecting a device opens it.
This is not that either: no selection changes, and the input remains No Input.
Earning a label is not choosing a device.

## The sentinel

`kRequestPermissionAudioRequest = -3`, alongside the existing
`kNoPendingAudioRequest = -1` and `kReleaseAudioRequest = -2`. One pending slot
still carries which control it belongs to, so no second consumable export is
added.
