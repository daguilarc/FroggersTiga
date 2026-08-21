# Proposal — `frogg3rs-external-audio-consent-repair`

**Created 2026-08-21.** Takes priority over the remaining work of
`frogg3rs-automation-view-and-musical-ranges`, which is otherwise complete
and awaiting operator testing. That change is not blocked by this one; this
one is simply ahead of it.

## Why

`froggers-modulation-slate` has required this for weeks, in a requirement
that is not ambiguous:

> A platform-default device the host opened unasked SHALL derive not-routed
> and SHALL NOT connect the sources.

with a scenario named "A host-opened default device does not count as
routed." The standalone violates it. Opening the app shows the built-in
microphone selected, one input channel active, and the two external-audio
modulation sources connected — with the operator having chosen nothing.

**The mechanism, traced.** `Runtime.hpp`'s `RefreshInputRoutedState` computes

```
routed = inputDeviceName.isNotEmpty()
      && getCurrentAudioDevice() != nullptr
      && getAudioDeviceSetup().inputDeviceName == inputDeviceName
```

which is true for ANY non-empty device name, including the platform default
JUCE opens at launch. It cannot distinguish an operator's choice from a
device nobody asked for, because nothing in it refers to a choice.

**And there is no way to decline.** `BuildDeviceOptions` builds the input
list as "System default" followed by every device. There is no None entry, so
the operator cannot select no input even if they want to. The one host that
gets this right today is the plugin, where the input selection defaults to
None and consent is explicit.

**The second half is the encoders.** The two external-audio sources are meant
to be visibly unusable while nothing is routed, not merely inert underneath.
An operator should see two greyed cells they cannot edit, not two live-looking
encoders modulating from a constant.

## What Changes

- **froggers-modulation-slate** (delta): MODIFIED — the disconnected state is
  visibly disabled and rejects edits, not only inert; and no-input is a
  selectable, default state in every host that selects devices.
- **External/Sheaf**: input device options gain a real None entry that is the
  default, and the routed signal reports routed only for an operator-selected
  device. Appended to the open pull request on the pinned branch.
- Code: `External/Sheaf` runtime and audio pages, and `app/FroggersUiSurface.hpp`
  for the disabled rendering and edit rejection.

## Impact

- Affected specs: `froggers-modulation-slate`.
- Upstream: same fork branch and pull request as the bank-addressed write.
  Push Sheaf before the superproject pin.
- This is a conformance repair. The requirement it satisfies already exists
  and is unchanged in substance; the delta only makes the disabled state and
  the None default explicit, because leaving them implicit is what allowed a
  host to satisfy the letter and miss the point.
