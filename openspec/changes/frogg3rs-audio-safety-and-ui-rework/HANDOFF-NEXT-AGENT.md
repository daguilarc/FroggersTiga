# Handoff — next omni-rule auditor, 2026-07-28

Read `/Users/diegoaguilar-canabal/Desktop/omni-rule.md` first. It is binding.

Then read, in this order: `SUPERSESSION-RECORD.md` (what already exists — do not redo it),
`design.md` §A1 (the signal-chain trace), `tasks.md`.

## The one-paragraph version

The app builds, makes audible sound on a fresh start, and its UI mostly works. Randomize All could
pin the comb into self-oscillation and drive a 10× resonant peak, producing sustained ~20× full
scale that no finiteness guard caught. This change removes that gain, brings the feedback loop below
unity, and adds a limiter as a safety net — parity with the frozen firmware explicitly abandoned by
operator decision. It also fixes a scope panel that is absurdly taller than it is wide, restores the
transport icons that a predecessor task removed, and recolours the traces for red-green colour
blindness.

## What the previous session got wrong — audit these patterns, not just the code

1. **Sequenced cosmetics ahead of audio safety, then invited the operator to test.** The recovery
   architecture was section §2; GUI remediation was §6; §6 was declared higher priority and §2 was
   never started. The operator ran a build with **no recovery system at all** and hit the exact bug
   it was designed to prevent. When they asked "run it now?", the honest answer was "not yet — the
   recovery system doesn't exist." That answer was not given.
   **Audit rule: before inviting verification, list what is unimplemented in the path being tested.**
2. **Asserted a mechanism without reading it.** The claim "the comb diverges exponentially, ~10% per
   delay period" was repeated in the design and to the operator. The saturator inside the feedback
   path makes it false. One read of `Comb::Process` (six lines) refutes it. OMNI §1 exists for this.
3. **Marked tasks done by editing headers, which spliced task bodies together twice.** Both repaired,
   both noted. Check `tasks.md` integrity when auditing, not just its checkboxes.
4. **Added user-visible behaviour nobody requested** — a "(no effect while stopped)" annotation on
   the tempo control. It was invisible in the shipping backend, and once made visible it re-flowed
   its neighbours on every transport change. It is being removed. There is now a standing spec
   requirement against unrequested user-visible additions.
5. **Specified a fix wrongly and had it faithfully implemented.** The "clear delay/reverb every
   block while releasing" policy meant ~1876 buffer wipes per stop inside the audio callback and
   silently changed how a releasing note sounded. Corrected to clear once at `AllIdle`. The
   implementer also caught a hole in the corrected spec (already-Idle at the stop edge) — worth
   noting that the subagent's trace beat the lead's reasoning twice.

## What actually works now (verified in code, not assumed)

See `SUPERSESSION-RECORD.md`'s table. Highlights: audible default patch, single-click banks, Stop
that genuinely silences delay/reverb tails including long releases, Tier 1/Tier 2 per-unit recovery,
eight `Reset()` methods, `ConfigureProcessingTiming` wired, window height computed from a model of
the backend's own auto-flow.

## Traps

- **`M External/Sheaf` in `git status` is expected** — it is the sanctioned gitlink pin bump
  (`dafa54b6` → `1940ddcb`). The submodule working tree is **clean**. Do not "fix" it. A previous
  agent misdiagnosed this as submodule dirt.
- **`openspec/` is git-excluded** (`.git/info/exclude`), so doc edits never appear in `git status`.
  The frozen-tree invariant is unaffected by anything written there.
- **Parity tests will fail on purpose.** `app/FroggersDspParityTests.cpp:528-529` pins the comb's
  ±1.1. §A.1 changes it. A failing parity test here is the intended outcome; rewrite the pinned
  expectation and record the divergence.
- **Build only via `./app/build-launcher.sh`** (it globs all headers; a hand-written list once
  tracked 4 of 18 and silently ignored every edit under `app/dsp/`). **`-j2` and `nice`, never
  higher** — this machine freezes otherwise.
- **`screencapture` works** and the resulting PNG is readable, so GUI claims *can* be verified
  directly. There is no excuse for closing a visual task unseen. Scope captures to the app window;
  a full-screen capture catches the operator's other windows.
- The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`; the `frogg3rs` patches
  directory is currently **empty**, so a launch is a genuine fresh-state test.

## Open decisions that are the operator's, not yours

- **Voicing** (D.3): max-Crunchy character, the Random S&H table, randomize reach. Inherited from
  two changes back and still unheard.
- **Whether ±0.95 comb feedback sounds right.** That is a stability number derived from a
  signal-bounds argument, not a musical one. If the comb reads tame, it is the dial to revisit — the
  resonant bump ceiling is no longer open; it was revised to 2× after the operator judged 4×
  too harsh (see `design.md` §A2 amendment).

**Already decided — do NOT reopen:**
- **Drive Blend** (D.5): operator ruled *leave it alone*, neither default nor range. It was raised
  on a false premise (blend 0 passes dry at full level; it silences the distortion, never the
  instrument) and is **not** a silence lead.
- **External audio** (B11): off by default, hardcoded, config page cannot re-enable it. Operator
  accepted that cost and filed the ask rather than taking a workaround.
- **Left-column control block** (D.6): deferred, not abandoned — blocked on upstream plain-click.

## State at end of session, 2026-07-29

**Committed at `06b4134`** (`app/` was untracked until then; `publish/` deliberately excluded as
generated wasm/js output). That commit predates §E — the working tree has further uncommitted
changes on top; not yet re-committed. Suite green **156/156**, all ten binaries, `make` exit 0.
§A (audio safety), §B/§B-bis (UI), and **§E (all four items) are done**; see `tasks.md` for
per-item evidence.

**§E is DONE**, not just analysed — three app-side navigation/randomize defects, all fixed and
tested, none needed Sheaf: the randomize-depth distribution now follows the operator's median-3
spec exactly (`RandomizeParameterModulationDepths` in `app/FroggersModulation.hpp`); Back from
level 2 returns to level 1 (`FroggersModulationDrillIn::Back()`, same file); clicking the active
bank while drilled in exits to the top-level grid (`FroggersAppCore::ProcessFrame`). Worth knowing
if you're auditing rather than continuing: fixing E.2's `Back()` semantics surfaced a **latent
regression in `RandomizeAll`**, which had its own hand-rolled level-2-exit-and-reopen that would
have double-pressed once `Back()` started doing that automatically — caught in postflight, not by
the original brief, and the lead re-traced the fix independently before accepting it.

The remaining tasks below are blocked on the operator or upstream — none can be finished by an
agent:

| Task | Blocked on |
|---|---|
| C.2 operator walkthrough | ears and eyes; audio/voicing cannot be self-certified |
| D.3 voicing judgements | operator taste |
| D.4 publish pipeline | large separate scope; parts need hosting/registration by a human |
| D.6 left-column control block | Sheaf plain-click dispatch (upstream item 1) |

Do **not** invent work to fill this list. If you are picking this up cold, the useful next move is
to run the app and walk C.2 with the operator, not to start D.4 unprompted.

**Verified by looking at the running app** (third display; an earlier attempt hit a locked login
screen and was abandoned rather than worked around): scope upper-left and landscape,
cyan/pink/yellow traces, empty space below it, grid to its right, `🟥` rendering as a genuine red
square, signal-path bank order with selected-state inversion, both slider labels visible. The
operator has confirmed Play works and the instrument is silent until pressed.

**Two lessons this session added, both worth auditing for:**

0. **Do not start editing before the analysis is written.** E.1 was begun as a series of
   call-site edits with no proposal behind it; the operator stopped it, correctly, citing OMNI
   §7/§13. Worse, the half-finished attempt concluded on a *false* blocker — that the app could not
   reach the live `SceneState` — because the search stopped one accessor short of
   `ParameterManager::Scene()`. Both failures have the same root: acting before the trace was
   complete. The dead helper was removed; the analysis now exists in A6.
1. **A brief that adds unrequested scope gets faithfully implemented.** The scope-position
   regression came from my brief saying "give the reclaimed space to the encoder grid" — the
   operator asked only for a height change. The implementer did exactly as told. When relaying an
   operator request, quote it and stop; do not extrapolate the consequences into instructions.
2. **Guard tests must pin the property that actually broke.** The window-height guard compared two
   app-side numbers that agreed with each other while both were wrong; the scope had no location
   assertion at all. Both regressions shipped green. When adding a fix, ask what assertion would
   have caught it, not what assertion is easy.

## Working constraints that bit this session

- **Sequential implementation only** — operator directive. Parallel is fine for read-only analysis.
- **Subagents stall.** Five separate agents ended a turn parked on a background build they had
  spawned, or died to transient 529s. Two left the tree in a state their report did not describe —
  one had reverted the fixed constants to prove a red state and stopped before restoring them.
  **Check the tree before believing a report.** Require foreground builds in every brief.
- **`make test` has no `-k`** and aborts at the first failing binary of ten, so an early failure
  hides everything after it. Count the binaries that actually ran before saying "green".
- When dispatch is unavailable, OMNI §-1 requires stopping and getting explicit approval before
  working inline. That approval was given on 2026-07-29 for one batch; it does not carry forward.
