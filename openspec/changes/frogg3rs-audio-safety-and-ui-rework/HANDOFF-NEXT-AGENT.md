# Handoff — next agent, 2026-07-31

> **SUPERSEDED 2026-08-05.** The live change is `../frogg3rs-modulation-truth-and-voicing/`
> (`SUPERSESSION-RECORD.md` there maps every open item). Nothing in this file is an instruction
> anymore.

Read `/Users/diegoaguilar-canabal/Desktop/omni-rule.md` first. It is binding.

Then: `proposal.md`, `design.md`, `tasks.md`, `SUPERSESSION-RECORD.md` in this directory.

---

## Where things stand

Suite green **156/156** across ten binaries. Branch `froggerstiga-desktop-v2`. `main` is deliberately
**not** merged — the operator wants v2 kept separate for now.

Done: §A audio safety (comb feedback ±0.95, resonant bump 2×, limiter + hard backstop, attack 1.0 s,
release 5.0 s, 200-draw randomize storm test at zero failures), §B/§B-bis UI, §E (randomize-depth
distribution, drill-in navigation), §F.0/F.1 (Sheaf pin bumped to `77a3019e` and the build restored
green against it), §G.1 (the build opens straight into Frogg3rs, no app picker). Per-item evidence
is in `tasks.md`.

**`External/Sheaf` is now pinned at `77a3019e`, not `1940ddcb`.** Anything in this document that
reads as a statement about toolkit limitations was written at the old pin — check
`/UPSTREAM-SHEAF-ASK.md`'s RE-CHECK section, which re-traced all 11 asks against the new pin with
file:line evidence, before believing it.

All 11 upstream asks have been sent to jvictor0; `/UPSTREAM-SHEAF-ASK.md` tracks sent status. Two
more are queued from this session's work: a reusable direct-launch entry point (so
`app/FroggersMain.cpp` stops being a near-copy of Sheaf's `Main.cpp`), and either templating
`Info.plist` from `APP_NAME` or failing the build when they disagree.

---

## YOUR PLAN — superseded 2026-08-02. `tasks.md` §0 "Execution order" is the authority.

This section was the previous session's plan and it has been executed. It is kept only as the
record of what was asked for; **do not work from it.** The live, ordered plan is `tasks.md` §0,
with each scope's proposal in its own `§X-PLAN` section.

Keeping a second ordered plan here is the same defect that produced three parallel proposal
documents (see `tasks.md` §0): the order then lives in two places and they drift.

**What that plan asked for, and where it landed:**

| Original step | Status |
|---|---|
| 1. Bump `External/Sheaf` | **Done** — `1940ddcb` → `77a3019e`, 2026-08-01. `tasks.md` F.0 |
| 2. Rebuild and run the suite first | **Done** — 156/156 across ten binaries. `tasks.md` F.1 |
| 3. Re-check all 11 upstream asks | **Done** — `/UPSTREAM-SHEAF-ASK.md` RE-CHECK, file:line evidence at the new pin |
| 4. D.6 unblocks if ask 1 landed | Ask 1 **did** land. D.6 is sequenced in `tasks.md` §0 after F.2/F.3 |
| 5. Then C.2, the operator walkthrough | Still last, still not closable by an implementer |

The workaround-removal table that was here is now `tasks.md` F.2, with each item's cause confirmed
dead against the new pin rather than assumed. Two corrections to what that table asserted:

- **Ask 8 (external audio) did NOT land.** `AppContext`/`AudioBlock` still carry no input-routing
  signal, so `kExternalAudioOptedIn = false` stays. The table called it "the workaround most worth
  undoing"; it is not yet undoable.
- **Ask 3 landed by a different route** than the `TextColourForNode` branch that was asked for —
  nodes now carry `color`/`textStyle`/`selected`. The workaround still goes, but the replacement is
  not the one the table predicted.

Ask 11's one-level drill-in pop remains **required behaviour**, not a workaround: if the framework
gains a native pop, switch to it; do not remove the behaviour.

### A spec that disagrees with shipped code is stale, not authoritative

If you find the code and a requirement in conflict, **the shipped behaviour is probably right and
the spec was never updated** — that is exactly what happened to three requirements in
`froggers-modulation-slate`, fixed by the delta now in this change's `specs/`. Check the code and
the operator's stated intent before assuming the spec wins. Then fix the spec.

### Supersede rather than patch

If the bump invalidates enough of the specs — likely, since at least three requirements are written
around toolkit limitations that may no longer exist — open a successor change the way this one
superseded `frogg3rs-gui-and-dsp-robustness`, with a `SUPERSESSION-RECORD.md` listing what carried
over. **Do not silently edit a requirement whose justification has evaporated; record that the
constraint lifted.**

### Do not touch

**§A audio safety**, unless a Sheaf change forces it. Comb feedback ±0.95, resonant bump 2×, the
limiter + backstop, attack 1.0 s / release 5.0 s, and the storm test are settled and operator-tuned
by ear. The DSP ranges deliberately diverge from the frozen firmware — that is **intended and
recorded**, not drift to be "fixed".

**Parameter-value randomization.** It has no coin flip, moves every knob every time, and the
operator confirmed it correct. Only *modulation-depth* randomization was changed.

---

## Still open, and why

**Ordering lives in `tasks.md` §0, not here.** This table says *why* each item is open; it
deliberately does not say what runs first, because two ordered lists in two files drift.

| Task | Blocked on |
|---|---|
| F.2 workaround deletions | nothing — next in sequence |
| F.3 layout-engine adoption | F.2 (it rewrites the same call sites) |
| D.6 left-column control block | F.2a for plain-clickable `Draw` nodes; cleanest after F.3 |
| G.2 blank window on startup failure | a behaviour decision the operator has not made |
| C.2 operator walkthrough / G.3 patch load | ears and eyes; audio/voicing cannot be self-certified |
| D.3 voicing | operator taste — max-Crunchy character is already settled as *wanted*; open part is whether ±0.95 and 2× sound right |
| D.4 publish pipeline | large separate scope; parts need hosting/registration by a human |

Do not invent work to fill this list.

---

## Traps

- **`M External/Sheaf` in `git status` is expected** — the sanctioned gitlink pin bump to
  `77a3019e`. The submodule working tree is clean. A previous agent misdiagnosed this as submodule
  dirt. *(Audit note, 2026-08-01: this trap was **stale** when the audit read it — the earlier bump
  it described had already been committed, so `git status` was clean and the warning described a
  state that no longer existed. It is true again now, for the new bump. A trap that describes
  transient working-tree state goes stale silently; date it or delete it once committed.)*
- **Parity tests fail on purpose** where the DSP ranges diverge. Rewrite the pinned expectation and
  record the divergence; never restore a broken value to make a test pass.
- **Build only via `./app/build-launcher.sh`** — it globs all headers; a hand-written list once
  tracked 4 of 18 and silently ignored every edit under `app/dsp/`. **`-j2` and `nice`, never
  higher** — this machine freezes otherwise.
- **`make test` has no `-k`** and aborts at the first failing binary of ten, so an early failure
  hides everything after it. **Count that all ten ran** before saying "green".
- **Subagents stall.** Several this session ended parked on a backgrounded build they spawned, and
  two left the tree in a state their own report did not describe — one had reverted fixed constants
  to prove a red state and stopped before restoring them. **Check the tree before believing a
  report.** Require foreground builds in every brief.
- The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`.

---

## Process lessons worth auditing for

1. **Do not start editing before the analysis is written.** One task was begun as scattered
   call-site edits with no proposal behind it; the operator stopped it, citing OMNI §7/§13. The
   half-finished attempt had also concluded on a *false* blocker — that the app could not reach the
   live `SceneState` — because the search stopped one accessor short of `ParameterManager::Scene()`.
2. **A brief that adds unrequested scope gets faithfully implemented.** A scope-position regression
   came from a brief saying "give the reclaimed space to the encoder grid" when the operator had
   asked only for a height change. The implementer did exactly as told.
3. **Guard tests must pin the property that actually broke.** Three separate tests this session were
   green while wrong: a window-height guard comparing two app-side numbers to each other, a scope
   with no position assertion at all, and a resonance-ceiling test that typed its own expected value
   into both sides of the comparison. Ask what assertion would have caught the bug, not what
   assertion is easy.
