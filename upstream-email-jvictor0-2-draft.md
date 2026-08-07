# Second email to jvictor0 — draft for Diego to send

*Items numbered **5-8**, continuing from the first email (which covered 1-4). Covers the only asks
jvictor0 has NOT seen: `UPSTREAM-SHEAF-ASK.md` file items 6, 5, 3, 7. Plain-click (file 1), the
audio dropdowns (file 4) and the logo/image request (file 2) were all sent separately and are not
repeated. Nothing sent from here.*

---

**Subject:** Four more from Frogg3rs — small rendering gaps, all affecting any Sheaf app

Hi jvictor0 — Claude again, continuing the numbering from last time so nothing gets confused.

These four are lower stakes than the timing footgun in #1, and none of them block us. They are all
places where a `Node` field or a control exists but nothing surfaces it properly, which means every
app hits them identically — including Braid 4, which I checked in each case.

---

## 5. `Slider` labels are never drawn

`NodeKind::Slider`'s handling in `CreateControlForNode` routes `node.label` to
`juce::Slider::setName(node.label)` and nothing else
(`projects/synth/juce/PortableJuceBackend.hpp:1231`). `setName` sets the component's internal /
accessible name; it doesn't attach or draw anything, and no `juce::Label` is created for a Slider
node anywhere in the backend. Still true at `origin/main` as of yesterday, so it isn't something we
can wait out at our pin.

**Why we care:** every slider label is invisible in the running app. A musician sees an unlabelled
slider and has to guess. We work around it by emitting a separate adjacent `Label` node per slider,
which renders fine — but that's two nodes to say one thing, and the auto-flow then positions them
by width, so a label ends up nearer its neighbour's control than its own unless you order them
carefully.

This one cost us real time in a way worth mentioning: an agent working on our app "verified" the
label by asserting the node's `label` field was set, concluded it was working, and told Diego it was
done. The field was set. Nothing rendered. Attaching a `juce::Label` (or drawing the text) would
close it for everyone.

## 6. `Slider` always shows a numeric text box, with no way to opt out

`PortableJuceBackend.hpp:1228` unconditionally does
`setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18)` for every Slider node. There's no
per-node flag to suppress it.

**Why we care:** our scene-blend control is a normalised 0..1 crossfade. "0.4271" is noise to a
musician, not information. Braid 4's own scene-blend slider carries the identical box, so this isn't
specific to us.

**What would help:** a `showValue` (or `textBoxStyle`) field on the slider node, defaulting to
today's behaviour so nothing changes for existing apps. This is the one item on our list where we
were asked to make a UI change and simply couldn't.

## 7. Selected-state buttons invert their background but never their text

`ButtonColourForNode` (`PortableJuceBackend.hpp:1130-1148`) already inverts the background when
`node.selected` is true. But `TextColourForNode` (`:1109-1127`) never branches on `selected`, so the
text colour stays put against the inverted background — and `Builder::Button`
(`PortableUIBuilders.hpp:300-306`) has no parameter to set `selected` at all, so the field is
effectively unreachable through the Builder API.

The consequence is visible across the ecosystem: Braid 4 (`Braid4UiModel.hpp:388-399`) and our app
both indicate selection by appending `" *"` to the label, because that's the only mechanism
actually available. A `selected` parameter on `Builder::Button` plus a `selected` branch in
`TextColourForNode` would let every app drop the asterisk for a real inverted-chip look.

We can reach it with `Draw` nodes, so it isn't blocking — but the asterisk is a workaround for a
gap rather than a design choice, and it's yours to close.

Related, and the reason I'd bundle these: `Node` carries **no colour field at all**. Combined with
`TextColourForNode`'s fixed variant palette (which has no green), that's what makes a green Play
glyph unreachable on a Button — we ended up using a red-square emoji for Stop specifically because
an emoji carries its own colour. A per-node colour on `Node`, or even just a "success"/green
variant, would solve a surprising amount.

## 8. The runtime chrome shows an unlabelled percentage

There's a bare `NN%` under the File button with nothing saying what it measures. Diego's reaction on
first seeing it, near enough verbatim: *what is the % number showing up under the File button, is
that just CPU? why not label what the actual number is?*

It's `DeadlineSamplePct()` → `deviceManager_.getCpuUsage() * 100.0`
(`projects/synth/runtime/Runtime.hpp:439`), with the sru-2 comment at `:432` describing it as a
rolling max over 256 samples.

**The ask:** label it, or tooltip it. `CPU 12%` costs four characters. As it stands, a number
meaning "how close the audio callback is to missing its deadline" reads as decoration — a shame,
because it's genuinely useful and his follow-up was "maybe it's good to show". It is. It just needs
to say what it is. Entirely your chrome, not something an app can override, which is why it's here.

---

Nothing here is blocking and all four are small. If you only look at one, #6 is the one where we
were asked for something and had no way to deliver it at all.

Same offer as last time: happy to open PRs, or to test a branch against Frogg3rs.

Thanks!
— Claude (with Diego)
