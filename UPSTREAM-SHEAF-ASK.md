# Upstream asks for Sheaf — for jvictor0, updated 2026-07-29

Context: Frogg3rs is an out-of-tree Sheaf app (`daguilarc/frogg3rs`), built against
`External/Sheaf` pinned at **`1940ddcb`** via the `EXTRA_APP_*` registration hook.

**We are deliberately not forking Sheaf.** A local fork was tried on 2026-07-27 and
reverted the same day: because the two commits existed only on one machine, the
recorded gitlink was unresolvable from any other checkout, which broke the browser
publish and any fresh clone. The pinned-upstream property is worth more to us than
the features below, so everything here is an ask rather than a patch we carry.

Items 1 and 2 block us. Items 3 and 4 are general Sheaf UX bugs that affect Braid 4
and the Miniapp identically — we hit them, but they are not ours.

## SENT STATUS (track this — not tracking it caused a duplicate-numbering mixup)

| Item | Topic | Sent to jvictor0? |
|---|---|---|
| 1 | Plain-click dispatch for `Draw` nodes | **sent** |
| 2 | `DrawCommand::Image` | **sent** |
| 3 | Selected buttons invert background, not text | **sent** |
| 4 | `AudioConfigPage` dropdowns unlabelled | **sent** |
| 5 | `Slider` always shows a numeric text box | **sent** |
| 6 | `Slider` labels never drawn | **sent** |
| 7 | Unlabelled percentage in runtime chrome | **sent** |
| 8 | Can't tell input-channel-exists from user-routed | **sent** |
| 9 | Parameter timing silently wrong off 48 kHz | **sent** |
| 10 | `RandomizeModulationDepths` draws same source twice | **sent** |
| 11 | No one-level pop of a modulation drill-in | **sent** |

**All 11 items have now been sent (2026-07-29).** Email numbering differs from this file's numbering. The first email sent covered file items
9, 10, 11, 8 as its own items 1-4. The second email (`upstream-email-jvictor0-2-draft.md`) covers
file items 6, 5, 3, 7 as items **5-8**, continuing the sequence he has already seen. File items 1,
2 and 4 were sent by Diego outside those two emails. When adding
a new item here, give it the next FILE number and note separately which email carried it.

---

## 1. Plain-click dispatch for `Draw` nodes — **the code already exists, on a branch**

**What's missing:** `RetainedDrawComponent` dispatches `pointerDragAction` and
`doubleClickAction`, but nothing for `Node::action`. So a `Draw` node with a plain
action is inert, and `Builder::Draw`/`DrawInteractive` expose no plain-action
parameter at all.

**Why we care:** our Play/Stop transport controls and every encoder cell are `Draw`
nodes. Without plain-click they must be **double-click**, which is wrong for a
transport button — nobody double-clicks Play. We currently ship double-click and a
comment at each site to flip back.

**The work is done and waiting for you.** Local branch `froggers-fork` in our
checkout, two commits, ~40 lines, all in
`projects/synth/juce/PortableJuceBackend.hpp`:

- `04818deb` — adds an `actionDispatch` callback and `acceptsClick_` flag to
  `RetainedDrawComponent`, wired through `Configure(...)` and
  `setInterceptsMouseClicks`, dispatching from `mouseUp` when
  `node.action.has_value()`.
- `7fa9ce34` — makes the click/drag discriminator use **pointer travel**
  (`kClickTravelThreshold = 3.0f` px) rather than the knob-value formula. This one
  is a real bug fix worth reading: `kPointerDragSensitivity`'s `(x - y)` form is how
  a knob derives its turn amount, and it **cancels to zero on an equal diagonal
  drag** — so discriminating with it lets a diagonal drag fire a click on release.

Happy to open a PR against `main` if you'd rather review it that way — say the word
and where.

**Related, optional:** a `Builder::Draw` overload taking a plain `action` would let
us delete a post-`Build()` field-patch helper (`SetNodeAction` in
`app/FroggersUiSurface.hpp`) that reaches into `NodeTree::nodes[i].action` directly.
Legal — the fields are public — but plainly outside the intended Builder API.

**Update 2026-07-28/29 — the blast radius is bigger than this item originally said.**
Re-checked against `origin/main` at `1dd4d275`: still not landed, and
`RetainedDrawComponent` still dispatches only from `mouseDoubleClick`.

Two corrections worth having:

- **It is not just the transport.** Every **encoder cell** in the grid is a
  `DrawInteractive` node, so *modulation drill-in is double-click-only too*. The
  transport we could work around; the encoders we cannot, because they are
  custom-rendered and need bounds, which `Button` nodes do not have. That makes this
  the one item on this list that is genuinely unfixable app-side.
- **We stopped waiting on it for the transport.** Play/Stop are now `Button` nodes
  with emoji glyph labels (`▶️` / `🟥`) — Button labels render and Buttons dispatch on
  single click, so we get an icon-ish control and one-click operation with no
  dependency on this landing. Our `Draw`-command icon builders are kept in the file
  for the day it does land. Mentioning it so you can see how apps route around this,
  and because "just use a glyph" may be a reasonable answer for other apps too.

**Update 2026-07-29 — this is gating our visual design, not just click ergonomics.**
Three separate things we want are all blocked on the same missing dispatch, which is
a stronger case than the one this item originally made:

1. **Custom transport icons** — the original complaint.
2. **Layout.** `Button` nodes take no bounds, so the runtime auto-flows them
   full-width below the lowest positioned node. Our operator wants a small scope in
   an upper-left column with transport and scene controls stacked in two columns
   *beneath* it. Positioning those controls requires `Draw` nodes, which costs
   single-click. **Positioned controls or single-click controls, pick one** — so the
   layout is simply deferred until this lands.
3. **Colour.** `Node` carries no colour field at all, and `TextColourForNode`'s
   variant palette has no green — so a green Play glyph is unreachable on a `Button`.
   `DrawCommand::Text` takes any colour, but again that means a `Draw` node. We
   settled on a red-square emoji for Stop because the emoji carries its own colour;
   there is no green right-pointing-triangle emoji, so Play stays default-coloured.

A per-node colour field on `Node` (or even just a green/"success" variant) would
close item 3 independently, if that is easier to land than the dispatch change.

## 2. `DrawCommand::Image` (or any raster/SVG path)

**What's missing:** `DrawCommand::Kind` (`projects/synth/include/synth/PortableUI.hpp:63-74`)
has no image kind — `Fill`, `Stroke`, `Line`, `Arc`, `Text`, `Ellipse`,
`RoundedRect`, `Polyline`, `FillPolygon` only. Confirmed there is no image path in
`PortableJuceBackend.hpp`'s draw-command switch either.

**Why we care:** we want our product logo in the app header instead of a text
title. Today the only app-side option is hand-tracing the SVG into `FillPolygon`
calls, which we'd rather not maintain.

**What would help:** a draw command carrying image data (and ideally a bounds +
preserve-aspect rule). The constraint we'd flag: it needs to work in **both** the
JUCE backend and the browser/wasm backend, since our app ships to both — a
JUCE-only path doesn't unblock us.

No urgency on our side; we're deferring the logo until this exists rather than
carrying a workaround.

## 3. Selected-state buttons only invert background, never text — affects all apps

`ButtonColourForNode` (`PortableJuceBackend.hpp:1130-1148`) already inverts the
*background* when `node.selected` is true. But:

- `TextColourForNode` (`:1109-1127`) never branches on `selected`, so text colour
  stays the same against the inverted background.
- `Builder::Button` (`PortableUIBuilders.hpp:300-306`) has **no parameter to set
  `selected` at all**, so the field is effectively unreachable through the Builder.

The consequence shows up across the ecosystem: Braid 4 (`Braid4UiModel.hpp:388-399`)
and our app both indicate selection by appending `" *"` to the button label,
because that's the only mechanism actually available. A `selected` parameter on
`Builder::Button` plus a `selected` branch in `TextColourForNode` would let every
app drop the asterisk convention for a real inverted-chip look.

We can work around this app-side with `Draw` nodes, so it isn't blocking — but the
asterisk is a workaround for a gap, not a design choice, and it's yours to close.

## 5. `Slider` always shows a numeric text box, with no way to opt out

`PortableJuceBackend.hpp:1228` unconditionally attaches a numeric text box to every
`Slider` node. There is no per-node flag to suppress it.

**Why we care:** our scene-blend control is a normalised 0..1 crossfade. The raw float
means nothing to a musician — "0.4271" is noise, not information — but we cannot hide it
without changing Sheaf. Braid 4's own scene-blend slider carries the identical box, so
this is not specific to us.

**What would help:** a `showValue` (or `textBoxStyle`) field on the slider node, defaulting
to today's behaviour so nothing changes for existing apps.

Genuinely blocking for us in the sense that it is the one part of a UI change we were asked
to make that we could not make. Not urgent.

## 4. `AudioConfigPage` dropdowns are unlabelled — affects all apps

The sidebar Audio page shows two selectors with no indication of which is which
(input vs output, or device vs sample rate — a user genuinely cannot tell). This is
Sheaf's own page, not app code, and it reads identically in Braid 4 and the
Miniapp. Labels on the two selectors would fix it everywhere at once.

## 6. `Slider` labels are never drawn

`NodeKind::Slider`'s handling in `CreateControlForNode` (`PortableJuceBackend.hpp:1224-1245`)
routes `node.label` to `juce::Slider::setName(node.label)` only (`:1229-1232`) — `setName` sets
the JUCE component's internal/accessible name, it does not attach or draw anything. No
`juce::Label` is created or attached for a Slider node anywhere in the backend. Confirmed still
true at upstream `origin/main` as of 2026-07-28, so this is not something we can wait out at our
current pin.

**Why we care:** every Slider node's label is invisible in the running app — a musician sees an
unlabelled slider with no indication of what it controls. We work around it app-side by emitting
an adjacent `Builder::Label(...)` node immediately before each Slider so the label text renders
in the same auto-flowed row (see `app/FroggersUiSurface.hpp`'s `AppendChromeBand`). That works,
but it is a per-app workaround for every Slider in every app, not a fix.

**What would help:** an attached `juce::Label` for the Slider node (populated from `node.label`
and kept in sync the same way `node.text` already is for `Label`/`StatusText` nodes,
`PortableJuceBackend.hpp:1357-1361`), or any equivalent change that actually draws the text —
so apps stop needing the adjacent-Label workaround.

---

## 7. The runtime chrome shows an unlabelled percentage

**What we see:** a bare `NN%` under the File button, with nothing saying what it
measures. Our operator's reaction on first seeing it was, verbatim, *"what the fuck
is the % number showing up under the File button, is that just CPU? why not label
what the actual number is?"*

**What it is:** `RuntimeShell`'s deadline meter —
`DeadlineSamplePct()` → `deviceManager_.getCpuUsage() * 100.0`
(`projects/synth/runtime/Runtime.hpp:439`), with the sru-2 binding comment at `:432`
describing it as a rolling max over 256 samples.

**The ask:** label it, or tooltip it. `CPU 12%` costs four characters and removes the
guess. As it stands a number that means "how close the audio callback is to missing
its deadline" reads as decoration, which is a shame — it is genuinely useful, and our
operator's follow-up was *"i didn't ask for that but maybe it's good to show"*. It is
good to show. It just needs to say what it is.

This is entirely your chrome, not something an app can override, which is why it is
here rather than in our own backlog.

## 8. An app cannot tell "input channel exists" from "user plugged something in"

**What's missing:** `AppContext` (`include/synth/AppContext.hpp:91`) carries no audio-device state
of any kind. The selected input device name exists runtime-side —
`AudioDeviceSnapshot().inputDeviceName` in `runtime/Runtime.hpp`, empty until the user picks one,
which is exactly the signal an app wants — but there is no route to it from an app.

**Why we care:** we register two modulation sources fed by external audio (the raw input and its
envelope follower). The only derivation available to us was
`block.inputs != nullptr && block.numInputChannels > 0 && block.inputs[0] != nullptr`, which on a
laptop is **permanently true** — the built-in mic always presents an input channel. Our operator's
startup log reads `1 in / 2 out` with nothing attached.

**Your randomizer is not at fault.** It correctly picks only among sources whose metadata says
`connected` (`src/ParameterModulation.cpp:2886-2895`). We were marking phantom sources connected
and it faithfully randomized them. The symptom looked like a randomizer bug and wasn't.

**What we did meanwhile:** hardcoded external audio off, accepting that the Audio config page can
no longer re-enable it. An unusable source beats one wired to room noise.

**The ask,** most useful first:
1. An explicit "external input routed" flag on `AppContext` or the audio block, with a change
   notification so apps can react live.
2. Or just expose the selected input device name — we can treat empty as "none".
3. Or a documented convention. We considered reading your persisted config JSON directly, but that
   only takes effect on relaunch and reaching into runtime state from an app will break quietly.

Only (1) makes the config page behave the way a user expects: pick an input, sources light up, no
restart. Full write-up in `upstream-email-external-audio-draft.md`.

Not urgent for us. Flagged because any app doing external-audio modulation on a laptop hits this
identically, and the failure is quiet — the sources look connected, they are just wired to noise.

## 9. Parameter timing is silently wrong unless an app knows to call one method

**This is the one we'd most like you to look at** — it is a silent-wrongness footgun, not a
missing feature, and we only found it by accident.

`ParameterConfig`'s smoothing constants are defined against a **48 kHz reference**:
`kDefaultProcessLiteAlpha` is commented "1 kHz one-pole cutoff at 48 kHz",
`kDefaultUiDisplayCenterAlpha` "about 10 Hz at 48 kHz"
(`include/synth/ParameterModulation.hpp:170-173`), and `ParameterConfig` initialises to exactly
those values (`:199-202`).

`ParameterGroup::ConfigureProcessingTiming` (`src/ParameterModulation.cpp:859-865`) is the only
thing that replaces them, and **nothing in Sheaf calls it for the app.** Grepping the tree, the
only callers are `apps/braid-4/Braid4Core.hpp:218-220`, your own tests, and the definition. So an
app that does not know to call it runs knob glide, modulation-depth smoothing and UI-display slew
at the wrong real-time rate at any host rate other than 48 kHz — ~9% off at 44.1 kHz, **2x off at
96 kHz**, 4x at 192 kHz.

We shipped that bug for months. Nothing surfaced it: no warning, no assert, and it sounds
plausible-but-slightly-off rather than broken. We only caught it by running a differential sweep of
every Sheaf API Braid 4 calls against every one we call, specifically hunting for dropped call
sites — i.e. by methodology, not by noticing.

**The ask:** have `Prepare()` (or whatever already knows the prepared sample rate) apply the
conversion by default, so the constants mean what their comments say at any rate.
`ConfigureProcessingTiming` would remain for apps wanting something custom — including Braid 4,
which converts against its own oversampled internal rate rather than the host rate, so the default
must not fight that. Failing that, an assert or a log line when a group is processed at a rate it
was never configured for would have saved us entirely.

## 10. `Bank::RandomizeModulationDepths` can draw the same source twice

Small, and separable from taste.

```cpp
while (manager_->NextRandomCoin() < 0.5f) {
    std::size_t ordinal = manager_->NextRandomIndex(connectedCount);
    ...
}
```
(`src/ParameterModulation.cpp:2894-2895`)

Each iteration draws an ordinal independently with no exclusion, so a loop that runs three times
can land on the same source twice and randomize it twice. The effective number of sources touched
is therefore lower than the nominal count, in a way that is invisible from outside. A partial
Fisher-Yates over the connected set, or just rejecting a repeat, would fix it.

**Separately, and NOT an ask:** the same loop's count distribution is geometric from **zero** —
`P(0) = 50%`, mean 1 — so a single press does nothing half the time. That is very visible on a
per-parameter randomize button. We are overriding the distribution app-side rather than asking you
to change it, because the right shape is a taste call and ours (median 3, tail out to the full
source count) is unlikely to be yours. Mentioning it only so the behaviour is on your radar if
other apps report the same "button does nothing" symptom.

## 11. No way to pop one level of a modulation drill-in

`Bank` has no level concept — one `Parameter* selected_` plus a bool derived from it — and
`Deselect()` is a full exit from any depth. There is no "go up one level".

Our operator drills parameter -> modulation source -> depth, and expects Back to step back one
level; today it drops them all the way to the parameter grid. We can work around it app-side (track
the level-1 parameter, `Deselect()`, then re-open it), so this is **not blocking** — but a native
one-level pop would be less fragile than an app re-deriving the intermediate state, and any app
with multi-level drill-in will want it.

## What we are NOT asking for

Anything Frogg3rs-specific. Every item above is general to Sheaf; none of it
encodes our product's behavior. If any of it is unwelcome, say so and we'll adapt
app-side — with the single exception of item 1, where double-clicking a transport
button is a genuinely bad user experience we'd like to stop shipping.
