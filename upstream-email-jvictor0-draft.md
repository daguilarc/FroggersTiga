# Email to jvictor0 — draft for Diego to send

*Supersedes `upstream-email-external-audio-draft.md` (that one covered only the external-audio
item, which is folded in below as #4). Nothing sent from here.*

---

**Subject:** Four things from building Frogg3rs on Sheaf — one of them is a silent footgun

Hi jvictor0 — this is Claude, working with Diego on Frogg3rs, the out-of-tree Sheaf app he's
building against pin `1940ddcb`. We've turned up a handful of things and I'd rather write them up
properly than have him relay them secondhand. Roughly in order of how much I think you'd want to
know.

---

## 1. Parameter smoothing is silently wrong unless an app knows to call one method

This is the one I'd genuinely like you to look at, because it's not a missing feature — it's a
quiet correctness trap, and I think every app has it.

`ParameterConfig`'s smoothing constants are defined against a **48 kHz reference**.
`kDefaultProcessLiteAlpha` is commented "1 kHz one-pole cutoff at 48 kHz",
`kDefaultUiDisplayCenterAlpha` "about 10 Hz at 48 kHz"
(`include/synth/ParameterModulation.hpp:170-173`), and `ParameterConfig` initialises to exactly
those numbers (`:199-202`).

`ParameterGroup::ConfigureProcessingTiming` (`src/ParameterModulation.cpp:859-865`) is the only
thing that replaces them — and nothing inside Sheaf calls it on the app's behalf. Grepping your
tree, the only callers are `apps/braid-4/Braid4Core.hpp:218-220`, your tests, and the definition.

So an app that doesn't know to call it runs knob glide, modulation-depth smoothing and UI-display
slew at the wrong real-time rate at any host rate other than 48 kHz: ~9% off at 44.1 kHz, **2x off
at 96 kHz**, 4x at 192 kHz.

We shipped that bug for months without noticing. There's no warning, no assert, and the symptom is
"slightly wrong feel" rather than anything that looks broken. We only caught it by running a
differential sweep — every Sheaf API Braid 4 calls, against every one we call, specifically hunting
for call sites we'd dropped. That's methodology, not observation, which is why I suspect other apps
are carrying it right now.

**The ask:** have `Prepare()` (or whatever already knows the prepared rate) apply the conversion by
default, so the constants mean what their comments say at any sample rate.
`ConfigureProcessingTiming` stays for apps wanting something custom — Braid 4 included, since it
converts against its own oversampled internal rate rather than the host rate, so a default mustn't
fight that. Failing that, even an assert or a log line when a group processes at a rate it was
never configured for would have saved us completely.

## 2. `Bank::RandomizeModulationDepths` can draw the same source twice

Small and separable.

```cpp
while (manager_->NextRandomCoin() < 0.5f) {
    std::size_t ordinal = manager_->NextRandomIndex(connectedCount);
    ...
}
```
(`src/ParameterModulation.cpp:2894-2895`)

Each pass draws an ordinal independently with no exclusion, so a loop running three times can land
on the same source twice and randomize it twice. The number of sources actually touched ends up
lower than the nominal count, invisibly. Partial Fisher-Yates over the connected set, or just
rejecting a repeat, would do it.

**Deliberately not an ask, but worth your radar:** the same loop's count distribution is geometric
from *zero* — `P(0) = 50%`, mean 1 — so a per-parameter randomize press does nothing half the time.
Very visible on a single deliberate press; invisible inside a bank-wide randomize. We're overriding
the distribution app-side rather than asking you to change it, since the right shape is a taste
call and ours almost certainly isn't yours. Flagging it only so the symptom is familiar if someone
else reports "the randomize button does nothing".

## 3. No way to pop one level of a modulation drill-in

`Bank` has no level concept — one `Parameter* selected_` and a bool derived from it — and
`Deselect()` is a full exit from any depth. There's no "up one level".

Our operator drills parameter -> modulation source -> depth and expects Back to step back one
level; today it drops him to the parameter grid. We can work around it (remember the level-1
parameter, `Deselect()`, re-open it), so **not blocking** — but an app re-deriving intermediate
navigation state is more fragile than a native pop, and anything with multi-level drill-in will
want it eventually.

## 4. An app can't tell "input channel exists" from "user plugged something in"

`AppContext` (`include/synth/AppContext.hpp:91`) carries no audio-device state. The selected input
device name exists — `AudioDeviceSnapshot().inputDeviceName` in `runtime/Runtime.hpp`, empty until
the user picks one, exactly the signal we want — but it lives runtime-side with no route to an app.

We register two modulation sources fed by external audio. The only derivation available to us was
`block.inputs != nullptr && block.numInputChannels > 0 && block.inputs[0] != nullptr`, which on a
laptop is permanently true: the built-in mic always presents a channel. Diego's startup log reads
`1 in / 2 out` with nothing attached. So both sources sat marked `connected` forever, and your
randomizer — which correctly filters on `connected` — kept assigning depths to a source carrying
room noise. Your code was right; ours was lying to it. The symptom presented as a randomizer bug,
which is why I want to be explicit that it wasn't one.

We've hardcoded external audio off meanwhile. That's honest but costs us the Audio config page
being able to turn it back on, which we took deliberately over shipping a phantom source.

**The ask,** most useful first: an explicit "external input routed" flag on `AppContext` or the
audio block, with a change notification so apps can react live; or failing that just expose the
selected input device name and we'll treat empty as none. We considered reading your persisted
config JSON directly and decided against it — relaunch-only, and an app reaching into runtime state
will break quietly later.

---

None of these are urgent for us. #1 is the only one I'd actively encourage you to prioritise, and
only because the failure is silent and not specific to our app.

Happy to open PRs for any of them, or to test a branch against Frogg3rs — we've got a 200-draw
randomize storm test and a full parity suite that would exercise #1 and #2 quickly.

Thanks!
— Claude (with Diego)
