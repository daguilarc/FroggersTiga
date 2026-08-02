# Handoff — next agent, 2026-07-31

Read `/Users/diegoaguilar-canabal/Desktop/omni-rule.md` first. It is binding.

Then: `proposal.md`, `design.md`, `tasks.md`, `SUPERSESSION-RECORD.md` in this directory.

---

## Where things stand

Everything implementable at the current Sheaf pin is done and committed. Suite green **156/156**
across ten binaries, `make` exit 0. Branch `froggerstiga-desktop-v2`, pushed. `main` is deliberately
**not** merged — the operator wants v2 kept separate for now.

Done: §A audio safety (comb feedback ±0.95, resonant bump 2×, limiter + hard backstop, attack 1.0 s,
release 5.0 s, 200-draw randomize storm test at zero failures), §B/§B-bis UI, §E (randomize-depth
distribution, drill-in navigation). Per-item evidence is in `tasks.md`.

All 11 upstream asks have been sent to jvictor0; `/UPSTREAM-SHEAF-ASK.md` tracks sent status.

---

## YOUR PLAN

**Sheaf has been updated upstream, and the operator says it fixes most of the outstanding UI items.
Bumping the pin is your first task.**

The previous session did **not** fetch or inspect the new Sheaf — the operator explicitly deferred
that to you. So everything in these artifacts describes the app as of pin `1940ddcb` and is accurate
for that pin only. **Treat every "blocked on upstream" claim as stale until you re-check it
yourself.**

### 1. Bump `External/Sheaf`

Operator approval for the bump is already given. Record the new SHA in `proposal.md` (Out of scope
still says "pinned at `1940ddcb`"), `tasks.md` §0, and the Traps section below.

### 2. Rebuild and run the suite before changing anything else

`cd app && nice make -j2 test` — dispatched **through a subagent brief** (OMNI §16.1, tasks.md §0),
with the build **foreground within that brief**, never backgrounded. 156/156 across ten binaries is
the pre-bump baseline. Anything that breaks is a Sheaf API change, not a regression in our work —
diagnose it as such. **Known breakage awaiting you** (traced 2026-08-01, see the re-check section
in `/UPSTREAM-SHEAF-ASK.md`): `DrawCommand::Kind` members renamed/split, `Builder` control methods
now take a `ControlStyle` parameter object, `Node::variant` retired, UI command buffer at
version 2 with a publish-time protocol assertion.

### 3. Re-check all 11 items in `/UPSTREAM-SHEAF-ASK.md` against the new Sheaf

**DONE 2026-08-01 — the re-check ran against `origin/main` = `77a3019e` (424 commits past the
pin), before any bump.** Full per-item verdicts with file:line evidence are in the "RE-CHECK"
section of `/UPSTREAM-SHEAF-ASK.md`; the Status column below summarizes them. **Landed: 1, 3
(via the new per-node appearance contract), 4. Effectively addressed: 6 (via
`ControlStyle::caption`). Still open: 2, 5, 7, 8, 9, 10, 11.** The workaround-deletion doctrine
in this step still applies — what follows is now confirmed work, not conjecture.

**Assume every ask has a workaround behind it, including the ones not tabulated below.** The table
lists the workarounds whose location is already known — it is a starting point, not the full set.
Ask 7 (the unlabelled CPU percentage) and ask 6 (undrawn slider labels) are both the kind of thing
an app can paper over, and there may be others nobody wrote down. So at **every** step: before
implementing anything, check whether the workaround it depends on is still necessary, and delete it
if not. A workaround left in place after its cause is fixed is worse than the original bug — it is
invisible, it constrains the design around a limitation that no longer exists, and the next person
has no way to tell it apart from a deliberate choice.

For each ask that landed upstream, the app almost certainly carries a **workaround that should now
be deleted**, not left in place:

| Ask | Status at `77a3019e` | Workaround to remove if it landed |
|---|---|---|
| 1 — plain-click on `Draw` nodes | **LANDED** | Play/Stop are `Button` nodes with `▶️`/`🟥` emoji labels. `BuildPlayDrawCommands`/`BuildStopDrawCommands` were deliberately **kept in the file, unused**, precisely so they can be restored. Encoders are `DrawInteractive` + `doubleClickAction` and can finally become single-click. Also delete the post-`Build()` `SetNodeAction` field-patch helper — `ControlStyle::action` replaces it. Our `froggers-fork` branch is superseded by upstream's own implementation; do not rebase it. |
| 3 — selected-button text colour / no `Node` colour field | **LANDED** (per-node `color`/`textStyle` + `ControlStyle::selected`, not a `TextColourForNode` branch) | Bank selection uses background-only inversion. A green Play glyph was unreachable, which is the only reason Stop is a red-square emoji instead of a coloured glyph. |
| 5 — slider numeric text box | not landed | Scene blend still shows a raw float the operator explicitly did not want. Spec `froggers-app-surface-layout` currently declares this **outside the app's control** — that declaration stays accurate. |
| 6 — slider labels never drawn | not landed as asked — but `ControlStyle::caption` emits the adjacent label from the library | Every slider carries a separate adjacent `Label` node — two nodes to say one thing. `kSceneBlendLabel`/`kBpmLabel` exist only for this; migrate them to captions. |
| 8 — input-channel vs user-routed | not landed | `kExternalAudioOptedIn = false` is hardcoded in `FroggersAppCore::ProcessBlock`, so the Audio config page **cannot re-enable external audio at all**. Deliberate, operator accepted the cost — but it is the workaround most worth undoing. Stays for now. |
| 11 — one-level drill-in pop | not landed | `FroggersModulationDrillIn::Back()` synthesizes a pop by `Deselect()`-then-re-press using a remembered `level1Encoder_`. Note this one is now **required behaviour**, not just a workaround — the operator wants the one-level pop regardless. If the framework gains a native pop, switch to it; do not remove the behaviour. |
| 7 — unlabelled CPU percentage | not landed (`FormatDeadlineText` still bare `%.1f%%`) | No app-side workaround exists today (it is the framework's own chrome), but if it landed, check nothing in our surface duplicates or compensates for it. |

### 4. D.6 unblocks if ask 1 landed — **it landed; D.6 is unblocked once the pin is bumped**

The one deferred *feature*, not a workaround. The operator wants Stop/Start, Scene 1/Scene 2 and
Scene blend in two columns **beneath** the scope in the left column. The space is already reserved
and deliberately empty — `ScopeArea`/`GridArea` in `app/FroggersUiSurface.hpp` leave it free, and
`scope_sits_in_a_left_column_with_the_grid_to_its_right` fails if anyone reclaims it. It was blocked
only because positioned controls required `Draw` nodes, which were double-click.

### 5. Then C.2 — the operator walkthrough

Several §B items are marked `[x]·` meaning **code landed, pixels unseen**. Do not close any visual
item without the operator seeing it. `screencapture` works and the PNG is readable, so you can
verify some of it yourself — but audio and voicing are the operator's call.

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

| Task | Blocked on |
|---|---|
| C.2 operator walkthrough | ears and eyes; audio/voicing cannot be self-certified |
| D.3 voicing | operator taste — max-Crunchy character is already settled as *wanted*; open part is whether ±0.95 and 2× sound right |
| D.4 publish pipeline | large separate scope; parts need hosting/registration by a human |
| D.6 left-column control block | see step 4 above |

Do not invent work to fill this list.

---

## Traps

- **`M External/Sheaf` in `git status` is expected** — the sanctioned gitlink pin bump. The
  submodule working tree is clean. A previous agent misdiagnosed this as submodule dirt.
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
