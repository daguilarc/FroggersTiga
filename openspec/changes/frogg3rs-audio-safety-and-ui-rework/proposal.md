# Proposal — `frogg3rs-audio-safety-and-ui-rework`

**Created 2026-07-28. Supersedes `frogg3rs-gui-and-dsp-robustness`**, which is archived
*superseded, not done* — see `SUPERSESSION-RECORD.md` in this directory for exactly what carried
over, what was finished, and what was thrown away.

## Why this change exists

The predecessor was built on two premises the operator invalidated on 2026-07-28 after running the
instrument:

1. **"The port must preserve parity with the frozen firmware."** Operator, verbatim: *"parity is
   stupid. we need feedback stability and a limiter."* Parity was a means, not a goal. Where the
   frozen ranges produce an instrument that destroys its own output, the ranges change and the
   pinned parity expectations get rewritten deliberately.
2. **"The output stage is a hard clamp, never a limiter."** That was recorded as an operator
   decision on 2026-07-28 and written into a spec requirement
   (`frogg3rs-dsp-recovery`: *"SHALL NOT apply saturation, soft-knee limiting, or any other
   tone-shaping"*). The operator reversed it the same day. This change reverses it in the specs
   rather than leaving code that contradicts them.

The predecessor also mis-sequenced its own work: it put GUI remediation (§6) ahead of the DSP
recovery architecture (§2), so the operator was asked to test a build in which **the entire
recovery system did not exist**. They hit the blowout it was designed to prevent. That sequencing
error is the reason this proposal leads with audio safety.

## The actual failure mechanism, corrected

An earlier analysis in the predecessor claimed the comb feedback diverges exponentially. **That was
wrong.** `Comb::Process` (`app/dsp/FilterFx.hpp:292-298`) is
`out = in + fb * Saturate(lp(delayed))`, with the saturator **inside** the feedback path, so the
comb output is bounded by `|in| + 1.1`. Nothing diverges.

What actually happens:

| Stage | Range today | Effect |
|---|---|---|
| `Comb::GetFeedback` (`app/dsp/FilterFx.hpp:381-388`) | ±1.1 | \|fb\| > 1 ⇒ loop gain above unity ⇒ the comb drives itself into its own saturator and **holds there indefinitely**. Sustained self-oscillation, not divergence. |
| `ResonantBump` height (`app/FroggersAppCore.hpp:825`) | `ExpMapCompute(1,10)` → **10× / +20 dB** | multiplies that pinned comb output |
| Output (`app/FroggersAppCore.hpp:912-920`) | hard clamp | receives ~20× full scale; clipping it produces a square wave |

So the blowout is **bounded, sustained, and roughly 20× full scale** — which is exactly why every
finiteness-based guard missed it and why `sawNaN` stayed 0 throughout. The fix is to remove the
gain that creates it, and to add a safety net for whatever still gets through.

## What this change delivers

**1. Feedback stability.** Comb feedback magnitude drops below unity (±0.95) so the comb decays
instead of self-oscillating. Manual access to the extreme goes away deliberately — a knob region
that reliably destroys the audio is a defect whoever selects it, and constraining only the
randomizer would have left the same landmine reachable by hand while creating two contradictory
definitions of the parameter's range.

**2. Gain sanity.** `ResonantBump` maximum height drops from 10× (+20 dB) to **2× (+6 dB)** — revised
down from an initial 4× target after the operator heard it modulated and judged 4× "still too harsh
... very close to blowout territory" (`app/dsp/FilterFx.hpp:109-122`).

**3. A limiter as a safety net, not a tone-shaper.** Threshold 0.9, gain reduction rather than
clipping. With items 1 and 2 landed it should essentially never engage on musical material; it
exists so that anything unforeseen is attenuated instead of squared off.

**4. Attack range narrowed** from 2.5 s to 1.0 s (operator judgement).

**5. The UI rework the operator specified** after seeing the running app: the scope band is
grotesquely tall-and-narrow and must become at most a third of its current size and wider than
tall; VCO traces become cyan / pink / yellow (the existing red/orange/yellow is unreadable with
red-green colour blindness); Play/Stop return as visible controls using `▶️`/`🟥` **emoji** glyph
labels on `Button` nodes — icons *and* single click, with no dependency on upstream (`🟥` was chosen
over a text glyph because an emoji carries its own colour, and `Node` has no colour field); the
scope stops animating while nothing is sounding; and the unrequested "(no effect while stopped)"
BPM annotation is removed.

**5b. Modulation randomize and drill-in navigation** (added 2026-07-29/31, after the operator used
the running build):
   - **Randomize-depth count.** The framework's own draw is geometric from zero — half of all calls
     touched nothing. Invisible inside a bank-wide randomize, but a single deliberate press on a
     modulation page was a coin flip. The app now selects the count itself: never zero, **median 3**,
     sharp falloff above 4, geometric tail reaching the full connected-source count, and distinct
     sources within a call. The framework still performs every write.
   - **Randomize All no longer touches local Crispy.** Randomizing it on all six banks is
     effectively randomizing global Crunchy, which this app never randomizes. Randomize Page still
     does its own page's Crispy.
   - **Back from the second modulation level pops one level** instead of exiting to the parameter
     grid, and **selecting the bank you are already viewing** now escapes a drilldown instead of
     being a no-op.

**6. Everything the predecessor finished is carried, not redone.** See `SUPERSESSION-RECORD.md`.

**7. Scope extension, 2026-08-01/02 — the Sheaf pin bump and its consequences.** Recorded here
because this proposal's "Why this change exists" above predates it and would otherwise not account
for the largest remaining body of work. `External/Sheaf` moved `1940ddcb` → `77a3019e` (424
commits, operator-approved). That delivered four of the eleven upstream asks, which means several
app-side workarounds now have no cause, and it replaced the layout paradigm the surface was built
against — so the toolkit-limitation justifications behind parts of this change have lifted rather
than merely changed. The operator chose to adopt the new layout engine rather than defer it.

Also folded in: the build now launches straight into Frogg3rs instead of Sheaf's app picker.

The handoff's standing advice for a bump this size is *supersede rather than patch*. The operator
chose to consolidate in place instead (2026-08-02), so **this change absorbs that scope and
`tasks.md` is its single proposal layer** — §F-PLAN/§F for the bump and migration, §G-PLAN/§G for
direct launch, with the execution order declared in §0. If the layout adoption (F.3) turns out to
invalidate more requirements than the `specs/` delta already covers, that is the point to revisit
superseding — **and the constraint that lifted gets recorded, never silently edited away.**

## Out of scope

- **`External/Sheaf` modifications.** Pinned at **`77a3019e`** (bumped from `1940ddcb` on
  2026-08-01, operator-approved; 424 upstream commits). We still do not patch Sheaf — items needing
  Sheaf changes go to `UPSTREAM-SHEAF-ASK.md`, whose RE-CHECK section records which asks the bump
  delivered.
- **The frozen trees** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/` stay byte-identical. Note
  this change alters `app/dsp/` ranges **only** — the frozen sources they were ported from are not
  touched, so "the port diverges from the frozen firmware" is now an intended, recorded property
  rather than a defect.
- **The logo**, still blocked on upstream image support.

## Success criteria

Each is falsifiable and names who checks it.

1. **No parameter combination produces sustained output above full scale.** Verified by the
   full-range endpoint sweep plus a randomize-storm test, both on the real engine path.
2. **Randomize All never blows the audio out**, across at least 200 randomize draws in a headless
   test — the predecessor's ~1-in-7 failure rate must go to zero.
3. **Audio always recovers.** After any blowout or randomize sequence, changing parameters back
   restores normal output. No permanent silence, no permanent roar.
4. **The limiter is inaudible on musical material** — a normal-level patch passes through with no
   gain reduction applied.
5. **The scope band is wider than tall, at most a third of its former area**, traces cyan/pink/
   yellow, and flat when the instrument is silent. Confirmed by looking.
6. **Play and Stop are visible glyph controls that respond to one click.** Confirmed by clicking.
6b. **A randomize press always does something.** No modulation-depth randomize call affects zero
   sources; the median count is 3.
6c. **Back from the second modulation level returns to the first**, and clicking the active bank
   escapes a drilldown. Confirmed by clicking.
7. Frozen trees intact; `External/Sheaf` at `77a3019e`, clean, and unpatched.
8. **No task in this change is closed by the agent that implemented it if its spec says the
   operator must see or hear it.** The predecessor violated this and it is why the operator spent a
   session testing unfinished work.
