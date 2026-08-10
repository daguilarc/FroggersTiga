# Archive record — `frogg3rs-audio-safety-and-ui-rework`, archived 2026-08-05

**Status at archive: superseded, not failed.** Its own `HANDOFF-NEXT-AGENT.md` already carried a
"SUPERSEDED 2026-08-05" banner and its successor's `SUPERSESSION-RECORD.md` (carried forward at
`../2026-08-06-frogg3rs-modulation-truth-and-voicing/SUPERSESSION-RECORD.md`) already has a detailed
FINISHED / carried-OPEN table. This record does not repeat that table; it exists because this
directory sat **live** (not archived) for 35 days after being superseded, and because two of its own
open items were never named in the successor's table at all — this archiving pass had to trace those
independently before concluding the directory was safe to move.

Successor chain from here: `frogg3rs-modulation-truth-and-voicing` (archived
`../2026-08-06-frogg3rs-modulation-truth-and-voicing/`) → `frogg3rs-blowout-and-drilldown-repair`
(archived `../2026-08-07-frogg3rs-blowout-and-drilldown-repair/`) →
`frogg3rs-parametric-slew-and-stop-root-cause` (archived
`../2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/`) → `frogg3rs-external-audio-phantom-
input` (live at time of this archiving). Predecessor: `frogg3rs-gui-and-dsp-robustness`, archived in
the same pass as this directory, at `../2026-07-28-frogg3rs-gui-and-dsp-robustness/`.

## Two items the successor's table never named — traced independently, 2026-08-09/10

The successor's "What the predecessor FINISHED" table covers §A, §B/§B-bis, §E, §F.0–F.3, §F.6,
§G.1/G.4 and the upstream asks as blocks, and its "Carried OPEN" table covers F.4, F.5, C.2/G.3, G.2,
D.3, D.4, §H, §I. Two items in this directory's own task list were in neither table:

- **F.2a–F.2e (the individual workaround-deletion sub-items)** — the parent line `F.2` is marked done
  in this file's own `§0` execution order (`~~F.2~~ — done 2026-08-03, 84f83e7`), but the five
  lettered sub-checkboxes in the body text were never individually ticked. **Confirmed landed by
  reading `git show --stat 84f83e7`**: "Encoders take a plain click" (F.2a), Play/Stop back to
  draw-command icons (F.2b), bank selection via `ControlStyle::selected` (F.2c), scene-blend label
  via `ControlStyle::caption` (F.2d, with the BPM label deliberately exempted per the B12 rationale
  recorded in the same commit), coloured glyphs via `textStyle` (F.2e). Stale checkboxes, not stale
  work — the commit message enumerates all five.
- **D.6 (left-column control block: Stop/Start, Scene 1/Scene 2, Scene blend beneath the scope)** —
  unchecked in this file, and absent from both of the successor's tables. **Confirmed shipped by
  reading the current `app/FroggersUiSurface.hpp`**: `FroggersCellMap::kLeftRows` drives
  `AppendLeftRow()` through `LeftKind::Scope → Transport → Scenes → SceneBlend → Bpm`, and
  `AppendTransportRow()`/`AppendScenesRow()`/`AppendSceneBlendGroup()`/`AppendBpmGroup()` build
  exactly the positioned, single-click controls D.6 asked for, beneath the scope, in the left column
  — landed as an unlabelled consequence of F.3's "declared grid" rebuild (commit `962f105`), which is
  exactly what D.6's own text predicted would unblock it ("lands cleanest after F.3 makes placement
  declarative"). Not a gap; a documentation omission in the successor's summary table.

No item in this directory's task list represents scope that is open today and untracked elsewhere.
D.3 (voicing) and D.4 (publish pipeline) are real, still-open, and were already correctly carried by
the successor chain before this archiving pass touched anything; D.4 in particular is still named,
verbatim, in the most recently archived change's "Deferred, untouched" footer.

## Citation sweep — combined report for both directories archived in this pass

Per the repo's citation-lesson convention: `grep -rn -B1 "<slug>"` across the whole repo, plus
fragment search (5 overlapping fragments per slug, to catch a slug split mid-word across a
hard-wrapped comment — the specific failure mode that whole-slug greps miss, per this repo's own
prior incident).

**`frogg3rs-gui-and-dsp-robustness`** — found 8 occurrences of the whole slug repo-wide, 0 additional
via fragment search. Classified: 1 was a genuine path reference (this directory's own
`HANDOFF-NEXT-AGENT.md`, "Canonical documents" list, self-referencing its own old location) —
**changed**, corrected to
`openspec/changes/archive/2026-07-28-frogg3rs-gui-and-dsp-robustness/`. The remaining 7 were bare
mentions of the slug as a document title (`# Proposal — `frogg3rs-gui-and-dsp-robustness``, etc.) or
as a name inside prose ("the same relationship that change had to `frogg3rs-gui-and-dsp-
robustness`", "supersedes `frogg3rs-gui-and-dsp-robustness`") with no path construction —
**left alone**, per instruction.

**`frogg3rs-audio-safety-and-ui-rework`** — found 9 occurrences of the whole slug repo-wide, plus 1
more via fragment search (a genuine hit no whole-slug grep could see). Classified:

- **3 genuine path references, changed**: `app/FroggersMain.cpp:2`, `app/FroggersSurfaceTests.cpp:294`,
  `app/FroggersUiSurface.hpp:7` — all source-file comments citing this change's `tasks.md` by path
  (sanctioned exception: comment-text citation rewrite in source files, no logic touched). All three
  corrected to the `archive/2026-08-05-` prefix.
- **1 genuine path reference found by fragment search, split mid-word across two comment lines,
  BLOCKED — NOT changed.** `app/FroggersAppCore.hpp:208-209`:
  ```
          // DEMOTED by task F.3 (openspec/changes/frogg3rs-audio-safety-and-
          // ui-rework/tasks.md, 2026-08-04/05): this literal is now just the
  ```
  The slug is split across the line break (`...-and-` / `ui-rework...`), exactly the pattern no
  whole-slug grep can see — confirming the fragment-search requirement is not defensive boilerplate.
  **`app/FroggersAppCore.hpp` is one of the four files a concurrent agent was editing at the time of
  this archiving pass, and this task's brief required stopping and reporting rather than editing any
  of the four.** This citation is left stale and must be fixed in a follow-up pass once that file is
  free: it needs the same `archive/2026-08-05-` prefix inserted, i.e.
  `openspec/changes/archive/2026-08-05-frogg3rs-audio-safety-and-ui-rework/tasks.md`.
- **5 bare mentions, left alone**: this directory's own `design.md`/`tasks.md`/`proposal.md` titles,
  a "Supersedes `X`" mention in `frogg3rs-modulation-truth-and-voicing/proposal.md` and its
  `SUPERSESSION-RECORD.md`, and a "live change (`X`, amended 2026-07-31)" mention inside
  `frogg3rs-parametric-slew-and-stop-root-cause/specs/froggers-modulation-slate/spec.md` — none
  construct a path.

**Additional fix, discovered while checking this directory's own internal relative links for
now-too-deep `../` cites (not a match for either slug above, so not counted in the tallies):** this
directory's own `HANDOFF-NEXT-AGENT.md` opened with `The live change is
`../frogg3rs-modulation-truth-and-voicing/`` — already stale before this archiving pass (that target
was itself archived on 2026-08-06, independently of anything done here), and now doubly so under the
new nesting. Corrected to `../2026-08-06-frogg3rs-modulation-truth-and-voicing/`, which is the
correct sibling-relative path from this directory's new location under `openspec/changes/archive/`.

No `../archive/`-prefixed relative cites were found inside either moved directory (both grepped for
`\.\./archive` and `\.\./` generally; the one hit is the case above).

## Follow-up required

Once `app/FroggersAppCore.hpp` is free of the concurrent edit noted above, fix the split-word
citation at (currently) lines 208–209 per the diff shown in this record.
