# Upstream asks for Sheaf — for jvictor0, updated 2026-07-29

Context: Frogg3rs is an out-of-tree Sheaf app (`daguilarc/frogg3rs`), built against
`External/Sheaf` pinned at **`77a3019e`** via the `EXTRA_APP_*` registration hook. (Pinned at
`1940ddcb` when items 1-11 below were written; bumped 2026-08-01 — see the RE-CHECK section.)

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

## RE-CHECK against upstream `origin/main` = `77a3019e` (2026-08-01)

424 commits past our old pin `1940ddcb`, fetched and read — every verdict below is traced to the
named file:line at `77a3019e`, per OMNI §1. **The submodule pin was bumped to `77a3019e` on
2026-08-01**, after this re-check and on the strength of it. Item write-ups below this section
still describe the state at `1940ddcb` and are retained as the record of what was asked.

### SECOND RE-CHECK — 2026-08-03, `77a3019e` → `origin/main` = `508d9d68` (27 commits)

Fetched and read after the operator reported the audio item fixed. **Ask 8 landed**, and it is the
only verdict in the table that changes; the row above now cites `508d9d68`, every other row still
describes `77a3019e`, which is still our pin. All 27 commits are the `synth audio input` change
(archived upstream at `fbcf89f5`) plus a one-second-delay demo app.

**What landed, traced.** `RuntimeConfig::numAudioInputs` (`AppContext.hpp:33`) is documented as a
*request* the host negotiates against the device. `AudioBlock::numRequestedInputChannels`
(`:190-193`) carries that request per block — `Engine` asserts it equals the immutable config
(`Engine.hpp`) — and `AudioBlock::InputView()` (`:194-202`) clamps the device's actual count into
`[0, requested]` and returns an `AudioInputView` (`:127-171`) exposing `RequestedChannelCount()`,
`ActiveChannelCount()`, `HasActiveChannel(ch)`, `Empty()`, `Sample()` and `SampleOrSilence()`.
`AudioInputFrameView` (`:87-124`) is the per-frame equivalent. Both are callback-lifetime-only
non-owning views and must not be retained past `ProcessBlock`.

**Precise about what this does and does not give us.** It is a genuine *requested vs active*
distinction, which is what our raw `block.inputs != nullptr && block.numInputChannels > 0` test
could never express. In practice it also delivers the user opt-in we wanted, because the Audio
page's input-device selector governs whether any channel goes active: pick no input device and
`ActiveChannelCount()` is 0, so source #6 stays dark and cannot steal randomization.

The residual gap is narrower than the original ask's wording: a user who *selects* an input device
but patches nothing into it still yields an active, silent channel. We judge that acceptable — they
explicitly chose an input device — but it is recorded rather than glossed, because "active channel"
means the device is delivering it, not that a human plugged in a cable.

| Item | Verdict at `77a3019e` | Evidence |
|---|---|---|
| 1 | **LANDED** — plain click dispatches from `Draw` nodes in both backends | commit `48d20cb7`; `acceptsClick_` + `mouseUp` dispatch, `projects/synth/juce/PortableJuceBackend.hpp:534-621`; plain-click action carried by `ControlStyle::action`, `projects/synth/include/synth/PortableUIBuilders.hpp:28` |
| 2 | not landed | `DrawCommand::Kind` has no image kind, `projects/synth/include/synth/PortableUI.hpp:83-95` |
| 3 | **LANDED, by a different route** — `TextColourForNode` still has no `selected` branch (`PortableJuceBackend.hpp:1036-1043`), but the new appearance contract supersedes the ask: `ControlStyle` carries `selected`, per-node `color`, and `textStyle` (glyph colour), and selected/disabled treatment is derived from the carried colour | `PortableUIBuilders.hpp:20-33`; appearance contract comment, `PortableUI.hpp:189-235`. A green Play glyph is now reachable via `textStyle` on a `Button`; the `" *"` convention is obsolete |
| 4 | **LANDED** — the Audio page's two selectors are captioned | `"Output device"` / `"Input device"` via `PageControls::Field(...)`, `projects/synth/include/synth/RuntimePages.hpp:763-774` |
| 5 | not landed — text box still unconditional | `setTextBoxStyle(TextBoxBelow, ...)`, `PortableJuceBackend.hpp:1162` |
| 6 | not landed as asked — Slider `label` still reaches only `setName` (`PortableJuceBackend.hpp:1163-1166`) — **but `ControlStyle::caption` now does the workaround's job in the library**: a caption is emitted as a sibling `Label` node with a stable derived id | `PortableUIBuilders.hpp:31`; caption contract, `PortableUI.hpp` appearance comment. Our hand-rolled `kSceneBlendLabel`/`kBpmLabel` nodes become captions on migration |
| 7 | not landed — still a bare percentage | `FormatDeadlineText` renders `%.1f%%` with no label, `RuntimePages.hpp:285-290`, rendered at `:649-651` |
| 8 | **LANDED at `508d9d68`** (not at `77a3019e`, where it was still open — see the 2026-08-03 re-check below) | `RuntimeConfig::numAudioInputs` is a *request*; `AudioBlock::numRequestedInputChannels` carries it per block and `AudioBlock::InputView()` returns an `AudioInputView` exposing `RequestedChannelCount()` vs `ActiveChannelCount()`, plus `HasActiveChannel(ch)`, `Empty()`, `SampleOrSilence()` — `AppContext.hpp:84-200`. `kExternalAudioOptedIn` can go |
| 9 | not landed | `ConfigureProcessingTiming` unchanged, `projects/synth/src/ParameterModulation.cpp:859-865`; no default conversion at prepare time, no new callers under `runtime/`/`include/` |
| 10 | not landed | same independent-draw coin loop, `ParameterModulation.cpp:2894-2899`. Our distinct-draw helper stays |
| 11 | not landed | `Bank` still exposes only full `Deselect()`, `projects/synth/include/synth/ParameterModulation.hpp:620`. Our synthesized one-level pop stays (and is required behaviour regardless) |

**Item 1 postscript:** upstream's implementation supersedes our `froggers-fork` commits
(`04818deb`, `7fa9ce34`) — it uses its own drag-threshold discrimination, so the branch is now
historical and must not be rebased onto the new pin. `ControlStyle::action` also makes the
post-`Build()` `SetNodeAction` field-patch helper (`app/FroggersUiSurface.hpp`) deletable, the
"related, optional" part of the ask.

**Compile-relevant API changes seen during the re-check** (the bump will not be a clean build;
these are Sheaf API changes, not regressions in our work):

- `DrawCommand::Kind` renamed/split its members: `StrokeRect`, `FillEllipse`/`StrokeEllipse`,
  `FillRoundedRect`/`StrokeRoundedRect` (`PortableUI.hpp:83-95`) — our draw builders name the old
  kinds.
- `Builder` control methods now take a `ControlStyle` parameter object
  (`PortableUIBuilders.hpp:194-213`); the old style-less overloads are gone.
- `Node::variant` is **retired**, enforced by `scripts/check_ui_boundary.sh`
  (`PortableUI.hpp` appearance comment).
- The UI command buffer is version 2, with a publish-time protocol assertion (upstream commit
  `05ae5968`) — touches the D.4 publish pipeline and the wasm host.
- The runtime pages and sidebar were rebuilt on the component library — the shell chrome may
  differ visually from what the operator last saw.

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

## 7. The runtime chrome shows an unlabelled percentage — **FILED as jvictor0/Sheaf#2** (2026-08-05, verified still open at 508d9d68)

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

## 12. No way to launch a single app without the picker — **FILED as jvictor0/Sheaf#2** (2026-08-05, with the realized drift cost: the G.4 window-size bug our copy inherited from Main.cpp:87-99, which still carries it upstream)

`apps/sheaf-patch/Main.cpp:35-51` always builds the full app vector and always constructs
`LauncherComponent`; `initialise(const juce::String&)` (`:29`) discards its command-line argument,
so there is no flag, environment variable, or single-app-registered shortcut. An app that wants to
ship as itself — ours does, the picker is a click between the operator and the instrument every
launch — has to supply its own `main`.

That is what we did, and it works without patching Sheaf (`APP_SOURCES` overridden on the make
command line). The cost is that our `FroggersMain.cpp` is a ~60-line near-copy of yours, because
`MainWindow` and the `LaunchRegisteredApp<App>` template are private members of
`SheafPatchApplication` and unreachable from outside it. Any duplication of that kind drifts.

Two shapes would both fix it, either is fine: a launch argument / registry hook that activates one
`appId` directly and skips the picker, **or** hoisting `LaunchRegisteredApp` and the window
plumbing into a reusable header so an out-of-tree `main` is a dozen lines instead of sixty.

## 13. `APP_NAME` and `APP_INFO_PLIST` are coupled, but documented and validated as independent

`runtime/juce_build.mk:6-10` documents them as separate inputs. They are not: `:25-27` derives the
binary path from `APP_NAME` while `:152-156` copies `APP_INFO_PLIST` **verbatim, with no
templating**. Override one without the other and you ship a bundle whose `CFBundleExecutable` names
a binary that does not exist.

The failure mode is what makes this worth reporting. **Nothing catches it**: the build exits 0, the
bundle is produced, the binary is valid, and running it directly from `Contents/MacOS/` works. Only
a Finder/LaunchServices double-click fails, because that is the one path that resolves through the
plist. We shipped exactly this and it survived a green build and a full green test suite.

Either templating `CFBundleExecutable` from `APP_NAME` at copy time, or a make-level guard that
fails when the plist's `CFBundleExecutable` does not equal `APP_NAME`, would turn a silent
mis-bundle into a build error.

## 14. `ControlStyle::caption` can only lead its control, never trail it

`Builder::FinishControl` (`PortableUIBuilders.hpp:428-465`) always emits the caption `Label` before
the control it names, wrapped with it in an implicit Row. There is no trailing option.

Caption is otherwise exactly right and we adopted it immediately — it replaced hand-rolled adjacent
`Label` nodes and is the reason item 6 is effectively solved for us. But one of our two sliders
needs its label **after** it: leading, it falls between two adjacent sliders and reads as labelling
the wrong one. Our operator hit that, told us to make the pair deliberately asymmetric, and we now
keep one hand-rolled `Label` node purely for placement — a workaround whose only remaining cause is
this.

A placement field on `ControlStyle` (leading/trailing, defaulting to today's leading) would let us
delete it. Low priority; genuinely cosmetic. Reporting it because "captions exist but can't go
where this one needs to go" is the sort of gap that quietly keeps a workaround alive.

## 15. Embedded app surfaces cannot resolve against a live extent — **FILED as a GitHub issue**

**<https://github.com/jvictor0/Sheaf/issues/1>** — filed 2026-08-05, the first of these asks sent as
an issue rather than email. Traced at pin `77a3019e`.

The layout engine makes an app's *internal* layout resolution-independent, but an embedded app
surface can never be handed a live extent: `RuntimeMainComponent::BuildTree()`
(`include/synth/RuntimeMainComponent.hpp:110-140`) receives an **already-resolved** app tree at
`:112` and then places the sidebar by arithmetic —
`sidebarTree.nodes.front().bounds.x = App::Config().uiWidth` (`:118`) — against a compiled-in
static. An app resolving against the live window would desync from a sidebar frozen at that x.
Today's inverse symptom: the renderer grows on resize while the composed tree stays compiled-in
size, leaving dead space.

Notably every refresh layer is already wired — `MainPane::resized()` (`runtime/MainPane.hpp:66-70`),
`ShellComponent::resized()` (`runtime/Shell.hpp:65`), and `Runtime::timerCallback()` at `uiFrameHz`
(`runtime/Runtime.hpp:718-722`, `:299`). Only the composition step is fixed-size. Nor is there an
app-side workaround: `RuntimeSessionOwner` exposes only `juce::Component&`
(`runtime/Shell.hpp:110-114,121`), `MainPane` has no public renderer accessor, and
`CollectPortableComponents` is test-only.

**Not blocking us** — F.3 ships a declared-layout grid resolving against the `Config()`-sized
region, and adopting a live extent later is a change to where `RootBounds()` sources its extent
rather than a redesign.

## What we are NOT asking for

Anything Frogg3rs-specific. Every item above is general to Sheaf; none of it
encodes our product's behavior. If any of it is unwelcome, say so and we'll adapt
app-side — with the single exception of item 1, where double-clicking a transport
button is a genuinely bad user experience we'd like to stop shipping.
