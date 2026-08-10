# Proposal — `frogg3rs-gui-and-dsp-robustness`

**Created 2026-07-28. Successor to `froggers-sheaf-app`** (archived same day as *superseded, not
done* — 82/99 tasks; see `openspec/changes/archive/2026-07-28-froggers-sheaf-app/ARCHIVE-RECORD.md`).

## Why this change exists

> **Revised 2026-07-28 (audit).** The original text below described the change as it stood before
> the operator's GUI test. That test found a second, worse layer of defects (design E6): the
> default patch is inaudible, Stop does not stop, nothing responds to a single click, and the
> restored scopes show the wrong signal. Section 6 was added to this change to cover them and now
> **is** the work. "What this change delivers" has been rewritten below to say so; the "Why" and
> "defect class" sections are unchanged because they only got more true.

The app builds, registers into the `sheaf-patch` launcher, and makes sound. The operator ran it
and it is not shippable. Every defect below was found by **running the instrument**, and none of
them were caught by 127 green unit tests across 11 binaries.

The predecessor's acceptance gate (task 12.2) required "app launches, **scopes live**, banks
switch." It was never executed. Had it run once, the dead oscilloscopes would have surfaced on day
one. That is the single most important lesson carried into this change, and it shapes the gate
design here: **no task in this change is complete on unit tests alone.**

## The defect class that produced two of these bugs

Two defects found on 2026-07-27/28 are the same bug wearing different clothes:

| Defect | Structures ported | Per-sample math ported | Required call site |
|---|---|---|---|
| `scoopNotch` unconfigured | yes | yes | **`SetFreq/Width/Height` never called** |
| Oscilloscopes dead | yes | yes (`Write()` called) | **`AdvanceIndex()` never called** |

The port faithfully copied every data structure and every formula from the frozen firmware and
from Sheaf's reference apps, then dropped *call sites*. **Unit tests are structurally blind to
this class**: a unit test constructs and configures the object itself, so it exercises a
configuration the application never actually produces.
`FroggersDspParityTests.cpp`'s scoop test calls all three `scoopNotch` setters by hand and passed
identically before and after the fix.

This change therefore does not merely fix the two known instances. It carries a task to
**systematically enumerate every Sheaf and frozen-firmware API the app consumes and verify each
required call site exists** (task 1.4). Fixing instances while the class remains is how this
recurs.

## What this change delivers

**0. Make the instrument usable at all (§6, added 2026-07-28 after the operator's GUI test — this
is now the priority work, ahead of everything numbered below).**
   - The default patch is **inaudible**: all three VCO pitches sit at the mapping floor of 20 Hz
     because `ApplyFroggersDefaultPatch` never touches the pitch knobs. "Makes sound with no user
     input" was satisfied by a signal a laptop speaker cannot reproduce.
   - **Nothing responds to a single click** — transport, bank buttons and encoders are all `Draw`
     nodes, and stock Sheaf dispatches Draw nodes on double-click only. The operator cannot change
     pages by clicking. Function beats cosmetics: the colour-inversion work that caused this is
     being reverted.
   - **Stop does not stop.** It closes the envelope gate; delay and reverb feedback keep ringing,
     because no unit in the chain has a reset. The recovery gap in item 2 turns out to be
     user-facing, not merely a fault-tolerance concern.
   - **Slider labels are never rendered** by the backend at all (`setName` draws nothing), so
     "BPM" and "Scene blend" are invisible. A previous task was closed by verifying the field
     instead of the pixels; that closure was wrong.
   - **The scopes show the wrong signal.** The cursor fix (item 1) worked, but the tap is pre-gate,
     so the traces move while the instrument is silent, and there is no envelope-follower band at
     all.

**1. Restore the oscilloscopes.** `ScopeWriter::AdvanceIndex()` is never called anywhere in
`app/`; Braid 4 calls it per-sample (`Braid4Core.hpp:487`). The write cursor is pinned at 0, the
reader is permanently `Empty()`, and the panel renders only its background fill and midline —
the "black box." Satisfies the already-recorded main-spec requirement in `froggers-vco-topology`
that scope panels render a live waveform trace, which the current build fails.

**2. Close the DSP recovery gap — architectural, not cosmetic.** Of nine stateful DSP units in
the audio path, **two** have any reset path, and both fire only on sample-rate renegotiation,
never on user action. `Reverb` has no reset method at all. Nothing resets on transport stop, on
Play, or on Randomize All. A single poisoned recursive state is therefore permanent.

**A constraint that must not be discovered during implementation:** the empirically reproduced
blowup (Randomize All followed by Crunchy-max, Filter stage exceeding the ±8.0 clamp at
t≈7.35s, peak ≈8.7) is **large but finite**. `sawNaN` stayed 0 throughout. **An
`isfinite()`-triggered recovery guard would not catch it.** Any design that only tests for
non-finite values will look correct in review and still ship the bug.

**3. Fix six UI defects**, all operator-reported, all app-side. **All six have landed** (see
design E0 for the evidence); two were then reversed or reopened by item 0, and one carries a
limitation that could not be fixed app-side (the scene blend's numeric readout — JUCE's slider
text box is unconditional upstream):
   - page/bank order does not follow the signal path
   - selected bank indicated by an asterisk rather than inverted colours
   - ASR labels truncated to 4 chars, destroying the VCO digit that carries the meaning
   - `"Frogg3rs Synth"` title text should not be on the canvas
   - scene controls: S1/S2 do not move the blend slider; the float readout is meaningless
   - default window size clips the bottom chrome row and right sidebar

**4. Address the Crunchy voicing problem.** Distinct from the blowup, and not a crash: Crunchy
swept to maximum drops RMS from 0.29 to ~0.01 and holds — real signal, drastically attenuated by
Fuegoize's chaotic scramble at `fuegKnob=1`. Nothing goes non-finite. This is a **voicing**
decision, not a defect fix, and is scoped as a by-ear judgement task alongside the inherited 8.6
and 12.6.

**5. Inherit and close the predecessor's 17 open tasks** — the entire publish pipeline
(11.6–11.13) and the entire acceptance gate (12.1–12.6), plus 6c.4, 8.2, 8.6.

## Out of scope

- **The logo.** Sheaf's `DrawCommand::Kind` has no image kind, so no raster/SVG can be rendered.
  Operator decision 2026-07-28: request `DrawCommand::Image` upstream rather than hand-trace the
  SVG into `FillPolygon` calls. This change **removes the title text** and leaves the header
  space; the logo lands in a follow-on once upstream ships. See `UPSTREAM-SHEAF-ASK.md`.
- **Any modification to `External/Sheaf`.** The submodule stays pinned at upstream `1940ddcb`.
  A local fork was tried on 2026-07-27 and reverted the same day because commits existing only on
  one machine made the gitlink unresolvable from any other checkout, blocking the browser publish.
  Items requiring Sheaf changes go to `UPSTREAM-SHEAF-ASK.md`, never into the submodule.
- **Play/Stop icon redesign beyond geometry.** The operator wants them to match the Froggers
  website. Squaring the bounds and insetting the icon is in scope; matching the site's exact
  artwork is blocked on the same missing image support as the logo.
- **The frozen trees.** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/` stay byte-identical.

## Known-unfixed, carried openly

The operator reports audio that "blows out and then gets shitcanned entirely and is unable to
restart." The blowout reproduced; **the unrecoverable state did not**, across 68 seconds of
testing. `scoopNotch` was one confirmed path to permanent silence and it is closed, but we cannot
claim the class is closed. Task 2.1 exists to reproduce it on the **real** engine path — the
existing repro drove a *shadow copy* of the audio chain, which is the same methodological error
as the predecessor's trap #1 and makes its numbers indicative rather than authoritative.

## Success criteria

Each one is falsifiable and names who checks it. "Confirmed by looking" means a human saw pixels
or heard audio — not that a field held the expected value (design E6-F6).

1. **Audible on start.** From a fresh data root, pressing Play produces sound the operator can
   hear on laptop speakers, and the headless test shows the default patch's energy sits above
   ~150 Hz. Nonzero RMS is explicitly not sufficient.
2. **Single click works.** Play, Stop and every bank button respond to one click, confirmed in the
   running app. (Encoders are exempt and stay double-click — a pinned-toolkit limitation recorded
   in design E7 and disclosed to the operator, not a silent shortfall.)
3. **Stop stops.** With delay and reverb driven into self-sustaining feedback, Stop brings output
   below −60 dBFS within ~250 ms and it stays there — measured in a test and confirmed by ear.
4. **The scope band shows what is heard.** Two visibly distinct panels; the top one flat while
   silent; a stable, non-scrolling trace while a sustained tone plays. Confirmed by looking.
5. **Labels are visible.** "BPM" and "Scene blend" are legible in a screenshot of the running app.
6. **Bounded recovery, not a universal claim.** The specific failures this change reproduces —
   the finite Filter-stage blowup (task 2.5) and the full-range endpoint sweep (task 2.7) — recover
   to sane output and stay there. This is deliberately *not* "no sequence ever produces permanent
   silence": that is unfalsifiable, and the one unreproduced report (see "Known-unfixed" above)
   stays open rather than being quietly covered by a claim nobody can test.
7. **No emitted sample exceeds ±1.0**, and in-range material passes through bit-identical.
8. The frozen-tree property still holds; `External/Sheaf` still at `1940ddcb`, clean.
9. The full suite is green **and** the GUI smoke test (inherited 12.2) has actually been run, and
   the §6 operator walkthrough (6.9) has actually happened.
