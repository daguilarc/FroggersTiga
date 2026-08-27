# Proposal — `frogg3rs-web-release-repair`

**Created 2026-08-26.** Supersedes `2026-08-27-frogg3rs-mobile-control-placement`,
which declared the phone web build delivered. Its own operator step 5.2 — spot-check
the live site on a real phone — was closed as "confirmed in part" on the strength
of a conversation, not a spot-check. The spot-check has now happened and it found
four things, three of them real. This change is what that step should have
produced.

It also ABSORBS the active change `frogg3rs-cpu-readout-says-it-is-a-peak`, which
was opened mid-conversation and is deleted rather than left running: two active
changes both repairing the browser runtime chrome is the same defect as two
functions computing the same value.

Nothing here is new feature work. Every item is something already shipped that
does not do what it says.

## 1. The browser can never reach a microphone. Ours.

`app/browser/site/site-boot.mjs:76` calls `installSynthBrowserApp` with `module`,
`runtimeVersions`, `runtimeIdentity` and `disposeModule` — and no
`activationLease`.

Sheaf's launcher only wires the audio context through a lease
(`External/Sheaf/projects/synth/browser/src/main.ts:336,343`):

    const resources = options.activationLease ? await options.activationLease.consume() : undefined;
    audioOptions: resources ? { ...options.audioOptions, audioContext: resources.audioContext } : options.audioOptions,

With no lease there is no `audioContext`, so `acquireInput()`
(`browser/src/audio.ts:213-216`) returns before it ever reaches `getUserMedia`:

    const context = this.options.audioContext;
    if (!context) { await this.releaseInput(AudioInputStatusCode.audioContextUnavailable, ...); return; }

which is exactly the line on the operator's screen: **"Input requested 1 / active
0 - microphone requires the launch-owned AudioContext"**. The Input device
dropdown offers nothing because nothing can ever be offered, and Retry Input
cannot help because the context was never there to retry with.

**It was not an oversight, and that matters — it is a reasoning defect, not a
slip.** `site-boot.mjs:13-22` omits the lease deliberately and says why:

> Deliberately does NOT pass an `activationLease`: `SheafPatchLauncher` only
> needs one because its "Launch" button click is the user gesture audio
> activation must be anchored to. This page has no such click --
> `SynthBrowserApp` defers audio activation to the first in-app UI action
> instead... No audio starts on load.

Every sentence of that is true, and it establishes that no lease is needed **for
output activation**. The conclusion drawn was that no lease is needed at all.

Input does not go through that reasoning. `startFromUserActivation`
(`audio.ts:107-129`) starts output AND, when the application requests channels,
calls `acquireInput()` — which needs `this.options.audioContext` specifically
(`audio.ts:213-216`), and that field is populated only from the lease
(`main.ts:343`). The gesture argument is irrelevant to it: the context is not
missing because no one has clicked, it is missing because nothing ever put one
there. So capture fails identically before and after any in-app action, which is
why Retry Input cannot help.

This is the same shape as the defect that started this whole sequence — a
justification that covers the easy half of a claim and is read as covering the
whole. There the scenarios tested "above the grid" and not "in the chrome
block"; here the comment proved "no lease for output" and concluded "no lease".

The same `resources` object also carries `midiAccess` (`main.ts:347`), so browser
MIDI is unreachable for the same reason, unnoticed because nothing has asked for
it yet.

## 2. The external-audio modulation cells render as nothing. The SPEC is wrong.

The cells are blank, and the code is doing exactly what the spec tells it to
(`openspec/specs/froggers-modulation-slate/spec.md:109-112`):

> **A disconnected source SHALL present no control.** While not connected, the
> two external-audio cells SHALL hold their grid positions while drawing no
> encoder...

`FroggersUiSurface.hpp:1797` implements it (`hidden = showingModulationView &&
!state.connected`), and Sheaf enforces it underneath —
`BuildEncoderDrawCommands` returns `{}` immediately for a disconnected state
(`EncoderDraw.hpp:652-656`).

**The operator has ruled that requirement wrong: greyed out is correct.** A blank
cell in a 4x4 grid reads as an empty slot or a rendering fault and gives no hint
that connecting an input would fill it. So the requirement changes, and the code
follows it — the reverse of the usual direction, and worth saying plainly because
the code was not defective against its own spec.

What the original requirement was protecting is kept: a disconnected source must
carry no press or drag action and no depth parameter. That was about INERTNESS,
and inertness does not require invisibility.

The rendering is the surface's own, NOT a change to Sheaf. `BuildEncoderDrawCommands`
takes an `EncoderDrawState`, so our DrawFactory can hand it a copy with
`connected` set true and the colours dimmed — drawing a de-emphasised ring
without a value readout — while the real `state.connected` continues to govern
actions and depth parameters. Forking Sheaf's encoder drawing would change every
other app's disconnected cells for a decision only this app has made.

UNVERIFIED, and settled by rendering rather than argument: how far to dim, and
whether the short label stays. `Color::AdjustBrightness` is the existing idiom
(FroggersModulation.hpp uses it for the EF lane colours).

## 3. Form-grid buttons stretch the full row width. Sheaf's, and cross-host.

`ApplyFormGrid` (`PortableUILayout.hpp:845`) gives every control cell the row's
entire remaining width, unconditionally:

    cells[1]->bounds.width = std::max(0.0f, row->bounds.width - controlOffset - rowOpts.padding);

A `<select>` ignores its box width in the browser, so the device combos look
right; a `<button>` fills it, so Retry Input renders as a control several
hundred pixels wide for a two-word label. It is not browser-only: `Toggle()` IS
`FormButton()` (`RuntimePages.hpp:444,454`) and the Sync page builds four of them
(`:778-793`), so every host stretches those the same way. The browser just made
it obvious.

The operator has accepted that the button EXISTS — it is Sheaf's, gated on
`showInputRetry`, and sru-3 makes it browser-only ("only a host whose capture is
offline offers the user a way back... JUCE reopens devices itself"). The width is
what changes.

## 4. A black box behind Random S&H 6 only. Ours, one argument.

Lanes 1-5 use `FroggersRandomShVisualizer`, which draws a polyline and a dot and
no background (`app/FroggersRandomShVisualizer.hpp:37-68`). Lane 6 uses Sheaf's
`GangedRandomLfoVisualizer`, whose constructor defaults `drawBackground = true`
(`GangedRandomLfoVisualizer.hpp:260`), and that flag makes it fill its whole node
with `Color::Rgb(12, 14, 16)` plus an axis line (`:57-70`). We construct it with
the default (`app/FroggersModulation.hpp:230`). Passing `false` is the fix; Sheaf
already supports it and needs no change.

## 5. The CPU readout holds a peak for eight seconds and calls it the load.

Carried in from the absorbed change, and REDESIGNED after the operator asked the
obvious question the first design talked itself out of: why label the number
instead of making the number right?

What it measures: time inside the audio callback divided by the real-time
duration of the audio that callback produced. Not "percent of your CPU" —
percent of the deadline. 100% means the DSP used exactly the whole budget it had;
above 100% is an underrun.

What it displays: `RuntimeMainComponent.hpp:283-284` writes each sample into a
`RollingMax256` and shows `.Max()` — the highest sample of the last 256 UI
frames. At `uiFrameHz = 30` that is an **8.5 second hold**. Measured in Chromium
on the shipped build, the readout sits at its startup peak (48.3% at a desktop
viewport, 63.2% at a phone viewport) for about seven seconds, then drops to
31.6% / 26.6% as that peak ages out. On a phone the startup peak clears 100% and
then sits there, looking current, long after the instrument is idle. The operator
reports no dropouts, which fits.

**The first design was to relabel it "CPU max".** That is defensible — the peak
is genuinely what matters for audio, because a spike is what makes a click, and
sru-2 requires the hold with a scenario for it. But it answers a misleading
number by explaining it rather than by fixing it, and it forces a cryptic word
into a 96px column that a three-digit value already strains.

**The window length is the actual defect, and sru-2 does not fix it.** The
requirement says "a rolling window of recent UI frames" — a number of frames is
nowhere in it. 256 frames is a value, not a requirement. Shortening the hold to
about a second (30 frames at 30Hz) keeps every property the requirement asks for
— a spike still displays, and still displays until it leaves the window — while
removing the eight seconds of staleness that made the number a lie. A one-second
peak is not something a reader can misread as "now".

So: **shorten the window, drop the decimal, and leave the label alone.** A tenth
of a percent was never real precision on a held maximum, and dropping it is also
what lets three digits fit the column — which removes the width problem the
relabelling created rather than solving.

### Why this fixes the phone, derived rather than assumed

The measured figures on this Mac at the phone viewport are steady **26.6%** and a
startup peak of **63.2%** — the peak is 2.4x the steady state. Deadline percent
is DSP time over block time, so it scales inversely with single-core speed, and a
phone running wasm in a worklet is somewhere between 1.5x and 3x slower:

| slowdown | phone steady | phone startup peak | transient needed to cross 100% |
|---|---|---|---|
| 1.5x | 39.9% | 94.8% | 2.5x over steady |
| 2.0x | 53.2% | 126.4% | 1.9x over steady |
| 2.5x | 66.5% | 158.0% | 1.5x over steady |
| 3.0x | 79.8% | 189.6% | 1.25x over steady |

Two things fall out, and together they explain everything the operator described.

**The startup peak clears 100% on any phone slower than about 1.6x.** That alone
puts the readout above 100% once per page load, for 8.5 seconds.

**And at a steady state of 50-80%, an ordinary transient crosses 100% too.** It
only takes a 1.25x-1.9x excursion in a single 100ms window — a garbage
collection, a reflow, the mobile-stack ResizeObserver burst, a scroll. Those
happen constantly. Each one then occupies the readout for 8.5 seconds. That is
the mechanism behind "regularly goes above 100%": not one condition lasting a
long time, but brief excursions each smeared across 8.5 seconds until they
overlap into near-permanence. On this Mac at 27% steady it takes a 3.8x
excursion to cross, which is why the same build never shows it here.

**And it explains the clean audio.** A single over-budget 100ms window is
absorbed by the output buffer the worklet renders ahead into; it is sustained
overrun that glitches. Steady state at 50-80% is comfortably under budget, so
there is nothing to hear — exactly as reported.

So the cause is transients against a high baseline, not sustained overrun, and
shortening the window is the right fix: a one-second hold cuts the time the
readout spends showing any given excursion by about 8x. The number will still
touch 100% at startup and during genuine spikes, because it genuinely does — but
it will stop *sitting* there.

Task 5.0 keeps the measurement as a POSITIVE CONTROL on this reasoning rather
than as an open question: Chromium CPU throttling should reproduce the predicted
steady state and excursion pattern. If a throttled run instead shows sustained
windows over 100%, the induction is wrong, the cause is DSP cost, and this
section is not the fix.

The window SHOULD be expressed in time, not frames, because `uiFrameHz` is
per-application configuration: the same frame count is a different hold on a
host that ticks at a different rate. Whether that is worth the change to
`RollingMax256` — used only by this readout and its own tests, across four files
— is settled by reading it, and is the one open question in this item.
UNVERIFIED: whether shortening to one second makes a genuine sustained overrun
harder to catch by eye. That is what the operator check in section 8 is for.

## 6. The README and the manual are written for the wrong reader.

The operator: they "should be helpful for someone trying to learn what this does,
not super technical about development and version control."

Measured, not asserted — `README.md` is 319 lines:

| lines | section |
|---|---|
| 49 | intro (what the instrument is — the part that works) |
| 28 | Sheaf app: build commands, submodule pin, `make test` has no `-k` |
| **229** | Daisy Field firmware: ARM toolchain paths, DFU addresses, linker scripts, `APP_TYPE` policy, bootloader binaries |
| 13 | Local Planning And Hygiene: where OpenSpec artifacts live, that subagents may not run git commands, where shared DSP belongs |

So 72% of it is a firmware build guide for frozen hardware that already has its
own manual (`DAISY_MANUAL.md`), and the tail is internal working process. A
reader arriving to find out what Frogg3rs is gets 49 good lines and then a wall
of `arm-none-eabi`.

## Non-goals

- The DSP's cost on a phone. There are no dropouts to chase.
- The rolling window's length behind the CPU readout. sru-2 fixes no number, so
  shortening it is available, but nothing has measured that eight seconds is
  wrong and it is a separate question from the label lying.
- External audio's consent default. Staying off until an operator selects an
  input is deliberate and specified (`froggers-modulation-slate`, and the
  archived `frogg3rs-external-audio-consent-repair`); a present microphone is
  explicitly not consent. Item 1 is about the choice being unavailable, not about
  the default.
- Deleting the Daisy firmware or its build system. Only its 229 lines of build
  instructions leave the README; `DAISY_MANUAL.md` is where they belong.

## Impact

- `app/browser/site/site-boot.mjs`, `app/FroggersModulation.hpp`, `README.md`.
- Sheaf, on the `fix-out-of-tree-app-gaps` branch `jvictor0/Sheaf#9` tracks: the
  form-grid width and the readout label. The pin moves with this change.
- Affected specs: `froggers-browser-package` (the boot path's obligations),
  `froggers-sheaf-runtime-app` (the readout), `field-operator-doc-parity` (who
  the docs are for).
