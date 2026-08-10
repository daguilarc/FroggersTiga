# Archive record — `field-button-latency-headroom`, archived 2026-07-25

**Status at archive: superseded by the pivot, NOT delivered. 0 of 22 tasks landed.**

An earlier archiving pass looked at this directory, confirmed its 22 tasks were genuinely
undone in `src/`, and concluded "not archivable — open scope." That was the wrong test: it
checked whether the work was *done* and never asked whether the work was still *wanted*. It
is not. This record archives the change as dead scope, not as finished scope — do not read
"archived" here as "shipped."

## What this change proposed (never built)

Follow-on latency/headroom work for the Daisy Field firmware control loop, targeting
`src/core/FroggersEngine.hpp`, `src/common/FieldMutationQueue.hpp`, and
`src/common/DaisyIO.hpp` (`proposal.md` "Impact"):

- **Block-rate `UpdateParams`** — call it once per audio block from `ProcessBlock` (after
  `ReadParamsBlock`) instead of once per sample from `ProcessSample`.
- **Page-cursor `RandomizeAllPages` / `RandomizeAllPagesMod` drain** — extend
  `FieldMutationQueue` with `m_active`/`m_activeType`/`m_pageIndex` so `DrainOne` randomizes
  exactly one page per call instead of the full page set in one shot; B1/B3 stay immediate.
- **LED transmit throttle** — compute LED levels every poll but call
  `led_driver.SwapBuffersAndTransmit()` only when dirty or on the same ~30 Hz budget as OLED
  (`kScreenThrottleMs`), instead of unconditionally every `ProcessControls` iteration.
- **Dry-reverb early-out with hysteresis** — skip the reverb delay-line body when smoothed mix
  is dry (`enter 1e-4` / `exit 5e-4` via a new `m_reverbDryBypass` member), while keeping the
  reverb page, parameters, and delay buffers unchanged (explicitly *not* a memory/page-removal
  change).
- Tightened `field-button-input-latency` acceptance criteria for poll rate and Rand-All spam.
- `MANUAL.md` / `docs/daisy-field-diagnostics.md` parity: attribute randomize freezes to
  CPU/poll headroom, not reverb RAM.

## Relationship to its predecessor — DO NOT CONFUSE THE TWO SLUGS

`../2026-06-27-field-button-input-latency/` **DID land** and **did fix** the operator-visible
randomization-latency problem: Phase 2 decoupled OLED redraw from the poll loop (throttle +
dirty flag) and routed B2/B4 through `FieldMutationQueue` instead of running them synchronously
in the poll loop (`phase2-release-notes.md`: binary built, 87,260 B, MD5
`81765ed607363b09fe13cde47e02e769`). Still present in the frozen tree today — see spot-check
citations below.

`field-button-latency-headroom` (this directory) is **follow-on headroom work on top of that
already-landed fix**, opened because "Daisy Field still feels slow or freezes under rapid
randomization" even after Phase 2 (`proposal.md` "Why"). It is not the original fix and never
became one. If anyone asks whether the randomization-latency bug was fixed: yes, by the
predecessor, in 2026-06-27. This directory is a separate, later, unbuilt headroom proposal.

## Independent re-verification of "still undone" — read fresh, not copied

All four claims below were re-read directly from `src/` during this archiving pass (OMNI §1:
trace, don't assert), not taken on the word of the brief that requested this archiving.

1. **`UpdateParams()` still runs per-sample, not per-block.**
   `src/core/FroggersEngine.hpp:850-852`:
   ```
   float ProcessSample(float input)
   {
       UpdateParams();
   ```
   `ProcessBlock` (`:653-658`) calls `ReadParamsBlock()` once, then loops
   `out[i] = ProcessSample(in[i])` — `UpdateParams()` is only reachable through the per-sample
   path; it was never hoisted into `ProcessBlock`. Task 1 not done.

2. **`FieldMutationQueue::DrainOne` still drains a full randomize in one call, no page cursor.**
   `src/common/FieldMutationQueue.hpp:42-59`: `DrainOne` pops one queued mutation and calls
   `pageManager.RandomizeAllPages()` (`:52`) or `RandomizeAllPagesMod()` (`:56`) directly — the
   entire 60-line file has no `m_pageIndex` or `m_active` member (confirmed by grep across the
   file: zero hits for either name). Task 2's state machine (D2: `Idle` /
   `DrainingRandAll` / `DrainingRandAllMod`) does not exist. Task 2 not done.

3. **`SwapBuffersAndTransmit()` still fires unconditionally every poll.**
   `src/common/DaisyIO.hpp:137`: `m_field.led_driver.SwapBuffersAndTransmit();` is a bare,
   unguarded statement inside `ProcessControls()` (called every poll iteration, `:215`). The
   only throttle/dirty gate in the file (`kScreenThrottleMs` / `m_screenDirty`, `:14`/`:218`)
   governs the separate OLED redraw call, not this one. No `m_ledDirty` or `m_lastLedMs` member
   exists anywhere in the file. Task 3 not done.

4. **`m_reverbDryBypass` does not exist anywhere in the tree.**
   `grep -rn "m_reverbDryBypass\|reverbDryBypass\|DryBypass" src/` → zero matches, whole `src/`
   tree. Task 4 not done.

0 of 22 `tasks.md` boxes are checked (`grep -c '^- \[ \]' tasks.md` = 22, `grep -c '^- \[x\]'` =
0). The checkboxes are left exactly as found — unchecked — because that is the true state; this
archiving pass did not tick any of them.

## Why archivable now: the tree it targets is frozen, and its target is gone

`proposal.md`'s "Impact" section is entirely inside `src/`: `src/core/FroggersEngine.hpp`,
`src/common/FieldMutationQueue.hpp`, `src/common/DaisyIO.hpp`. Every live change's standing
constraints in this repo declare frozen trees byte-identical, `src/` among them (verified
verbatim in the currently-active `frogg3rs-external-audio-phantom-input/tasks.md:23`:
"**Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`"). `git log -1
-- src/` is `66e3903`, 2026-07-25, "daisy: update committed firmware build output," whose own
commit message states it "establishes a clean baseline for the froggers-sheaf-app change, whose
frozen-tree proof compares src/ against a recorded baseline commit." Current project work is
entirely under `app/` (the Sheaf-based rewrite); the Daisy Field hardware target this change
serves is not the project's direction anymore.

This change cannot be executed without unfreezing `src/`, and unfreezing `src/` is a
project-direction decision, not an engineering one. Nothing here failed on its own merits — the
ground it stood on was pulled away by an unrelated, larger decision.

## Archive date: 2026-07-25, not today — reasoning

This repo's own convention, demonstrated in `../2026-07-28-frogg3rs-gui-and-dsp-robustness/` and
`../2026-08-05-frogg3rs-audio-safety-and-ui-rework/`, dates an archive directory by **when the
change stopped being current**, not by when someone got around to filing it — both of those sat
live for 11 and 35 days respectively after their own archive record says they were superseded,
and both keep the earlier date. Applying that same rule here, the change stopped being current on
**2026-07-25**, the day `src/` was frozen (commit `66e3903` above) as part of adopting
`froggers-sheaf-app` (`.openspec.yaml` in that change: `created: 2026-07-25`; the user's own
standing memory independently labels 2026-07-25 as "the pivot"). This is also not a novel choice
for this repo: three siblings killed by the same pivot are already archived at this exact date
prefix — `../2026-07-25-desktop-v2-external-audio-input/`,
`../2026-07-25-desktop-v2-sheaf-runtime-harmonization/`, and
`../2026-07-25-desktop-v2-unified-parameter-layout/`. The first of those is the closest possible
precedent: `froggers-sheaf-app/proposal.md:51` (carried to
`../2026-07-28-froggers-sheaf-app/proposal.md:51`) reasons through exactly this change's
situation for that sibling, by name, on the pivot day: "its entire scope was editing
`desktop-v2/`, which this change freezes, so it can no longer be executed."

**The honest gap:** that same line is the *only* repo-wide mention of
`field-button-latency-headroom`, and it calls this change "unrelated (Daisy)" rather than
applying the identical freeze logic to `src/` — the pivot's own author registered the sibling
`desktop-v2` casualties but did not register this one. So, unlike the two precedent records,
nobody actually ruled on this specific change's fate on 2026-07-25 by name; the mechanical fact
(target tree frozen) was true that day, but the recognition happened only in this archiving pass,
today (2026-08-09). I dated the directory to the mechanical fact rather than the recognition
because that is what this repo's own precedent does with its two clearest analogous cases, and
because the mechanical fact is the more objective, reproducible anchor (a single git commit)
rather than a memory of when someone noticed. Today's date (2026-08-09) is recorded here instead,
plainly, as the date this record itself was written and the archiving pass performed.

## Capability ownership — handoff status

This change's `specs/field-button-input-latency/` and `specs/field-operator-doc-parity/`
directories were delta proposals against those two main capabilities
(`openspec/specs/field-button-input-latency/spec.md`,
`openspec/specs/field-operator-doc-parity/spec.md` — **not moved, per instruction; still live at
those paths, unchanged**). Checked: this was the *only* active change claiming either capability
— the other active change, `frogg3rs-external-audio-phantom-input`, owns only
`specs/froggers-modulation-slate/` (confirmed by listing its `specs/` tree). No duplicate-ownership
conflict exists or existed.

Because none of this change's 22 tasks landed, its delta was never synced into either main spec —
confirmed by inspection: the main `field-button-input-latency` spec's "Heavy randomize is queued"
requirement has no page-cursor language, and the main spec has no LED-throttle or dry-reverb
requirement at all; both only exist in this (now-archived) change's unsynced delta.

**Handoff: none exists, because there is no successor.** Archiving this change releases its claim
on both capabilities. No other active change picks them up. Reviving Daisy Field firmware work —
which requires first unfreezing `src/`, a project-direction decision — would need a brand-new
change to re-propose against these two capabilities; there is nothing live to hand off to today.

## Citation sweep performed at archive time

Per this repo's citation-lesson convention (a slug has previously been found split mid-word
across a hard-wrapped comment line, invisible to a whole-slug grep): `grep -rn -B1` for the whole
slug, plus three independent fragment searches, both run repo-wide from the repository root.

**Whole slug**, `grep -rn -B1 --exclude-dir=.git "field-button-latency-headroom" .`: **1
occurrence**, in `openspec/changes/archive/2026-07-28-froggers-sheaf-app/proposal.md:51`,
inside the same OpenSpec-bookkeeping bullet quoted above ("`field-button-latency-headroom` is
unrelated (Daisy)"). Case-insensitive re-run confirms the same single match (no casing variant
exists elsewhere).

**Fragment searches** (`field-button-latency`, `latency-headroom`, `button-latency-`, each run
independently, no `-B1`): every fragment matched the same single line above and nothing else —
**0 additional occurrences**, and specifically no mid-word split of this slug anywhere in the
tree.

**Classification: 1 found, 0 changed.** The one hit is a bare-slug prose mention — `` `field-button-latency-headroom` is unrelated (Daisy) `` — with no `openspec/changes/` (or any other)
path prefix before or after it. It does not construct a path, so per instruction it is left
alone. It also remains true prose after this archiving: the sentence is explaining that adopting
`froggers-sheaf-app` did not need to touch this change (unlike its sibling
`desktop-v2-external-audio-input`, which the same bullet says "must be archived as part of
adopting this change") — it was making a narrower claim ("nothing to do here to adopt the pivot")
that happens to have been an incomplete read of the consequences, not a path or a claim about
this change's later disposition. Rewriting it would be editing someone else's already-archived
change's proposal text to reflect information only available in hindsight; it is left as
authentic history.

**Relative-path check inside the moved directory:** grepped this directory's own files (now at
their new location) for `../` — zero hits. No now-too-deep relative archive cites needed fixing;
none existed before the move (`proposal.md`, `design.md`, and `tasks.md` all reference the
predecessor and "archived Phase 2" only as bare prose, e.g. `proposal.md:3` and
`design.md:134,199`, never as a constructed path).

**No source files required a citation-text rewrite.** The task brief anticipated one might be
needed (comment-text-only, sanctioned even though `src/` is otherwise frozen); the sweep found no
source comment anywhere referencing this slug, so no such edit was made.

## What's carried forward

Nothing is carried into an active change — there is no successor. The technical analysis in
`design.md` (exact current-state line citations for `UpdateParams`, `DrainOne`,
`SwapBuffersAndTransmit`, `ApplyOutputFx`) remains readable here if Daisy Field work is ever
revived; it would need re-verification against `src/` at that time rather than being trusted as
current, per the same OMNI §1 standard applied above.
