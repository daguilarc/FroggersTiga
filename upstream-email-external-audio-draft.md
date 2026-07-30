# Draft email to jvictor0 — external audio / input device visibility

*Draft for Diego to send. Not sent.*

---

**Subject:** Sheaf ask: letting an app tell "input channel exists" from "user plugged something in"

Hi jvictor0 — this is Claude, working with Diego on Frogg3rs, the out-of-tree Sheaf app he's
building against pin `1940ddcb`. We hit something that I don't think we can solve on our side, so
I wanted to write it up properly rather than have him relay it secondhand.

**The short version:** an app can see *that* the audio device handed it an input channel, but not
*whether the user actually routed anything into it*. On a laptop those are very different things,
and right now they're indistinguishable.

**What happens to us.** Frogg3rs registers two modulation sources fed by external audio — the raw
input and its envelope follower. We were deriving their `connected` metadata the only way available
to us:

```cpp
const bool externalInputConnected =
    block.inputs != nullptr && block.numInputChannels > 0 && block.inputs[0] != nullptr;
```

On Diego's MacBook that is permanently true, because the built-in mic always presents an input
channel whether or not anything is plugged in — his startup log reads `1 in / 2 out` with nothing
attached. So both sources are always marked connected.

**To be clear, your randomizer is not the problem — it's doing the right thing.** It counts only
sources whose metadata says `connected` and picks among those
(`src/ParameterModulation.cpp:2886-2895`). We were feeding it a lie. The user-visible symptom was
Randomize assigning modulation depths to a source carrying nothing but room noise, which was
confusing enough that Diego reported it as a randomizer bug before we traced it.

**Why we can't fix it app-side.** The selected input device name exists — it's right there in
`Runtime.hpp`'s `AudioDeviceSnapshot().inputDeviceName`, and it's empty until the user picks
something, which is exactly the signal we want. But it lives runtime-side, and `AppContext` carries
no audio-device state at all (`AppContext.hpp:91`) — parameter manager, patch manager, buses, MIDI,
master clock, config, UI state, grid manager, and that's it. So the app has no route to it.

**What we did meanwhile,** so you know the current shape of our code: we hardcoded external audio
off. That's honest — an unusable source beats a phantom one that steals randomization — but it
does mean the Audio config page can no longer turn it back on, which is a real capability loss we
took deliberately rather than ship the confusing behaviour.

**The ask.** Some way for an app to know whether external input is actually available. In rough
order of how useful they'd be to us:

1. An explicit flag on `AppContext` (or on the audio block) meaning "the user has routed an input",
   with a change notification so apps can react live.
2. Failing that, just the selected input device name exposed to apps — we can treat empty as "none"
   ourselves.
3. Failing *that*, even a documented convention would help. We considered reading the persisted
   runtime config JSON directly, but that only takes effect on relaunch and reaching into your
   state file from an app feels like exactly the kind of thing that breaks quietly later.

Option 1 is the one that makes the config page work the way a user would expect — pick an input,
the sources light up, no restart.

No urgency from us; external audio isn't on Diego's critical path and we're happy sitting with it
off. Mostly flagging it because I suspect any app doing external-audio modulation on a laptop hits
this identically, and because the failure mode is quiet — the sources look fine, they're just
wired to noise.

Happy to send a PR if you'd like a particular shape, or to test a branch against our app.

Thanks!
— Claude (with Diego)
