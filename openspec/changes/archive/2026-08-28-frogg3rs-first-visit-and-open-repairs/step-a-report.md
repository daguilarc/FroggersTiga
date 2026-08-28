# Step A report

## A1 — Ledger check-off, `frogg3rs-web-release-repair/tasks.md`

Method: for every `- [ ]`, read the concrete artifact the item names, and read it
in the current working tree or in the named commit (`git show <sha>`), not in a
commit message or the proposal. 49 items total (confirmed by
`grep -c "^- \[ \]"` before editing).

| Section | Examined | Ticked | Left open | Why left open |
|---|---|---|---|---|
| 0. Supersede bookkeeping | 2 | 2 | 0 | — |
| 1. Activation lease | 6 | 5 | 1 | 1.4: no e2e assertion of "external-audio stays disconnected after boot" exists anywhere in `app/browser/e2e/` (`ls`+`grep` came up empty); the file's own outcome note says it was "correctly NOT written" for the refuted lease path, and nothing was added under the shipped `audioOptions` path either. |
| 2. Disconnected sources render greyed | 7 | 7 | 0 | — |
| 3. Form-grid buttons | 4 | 0 | 4 | Per your explicit instruction. Independently re-verified: `PortableUILayout.hpp`'s `ApplyFormGrid` (:845-846) still unconditionally sets `cells[1]->bounds.width` to the full remaining row width — the section's own "DEFERRED" outcome note is accurate. |
| 4. Random S&H 6 background | 3 | 3 | 0 | — |
| 5. CPU readout | 10 | 9 | 1 | 5.0: per your explicit instruction (unrun positive control). |
| 6. Docs for a reader | 5 | 5 | 0 | — |
| 7. Nothing else moved | 6 | 1 | 5 | 7.1/7.2/7.3/7.4 require actually running the app/vst/Sheaf/e2e gates, which this task forbids (no build, no tests) and no persisted log/artifact in the tree substitutes for that (the one `.last-run.json` I found predates the commits and only says `{"status":"passed"}`, no counts, not proof of the specific "idle and under load" run 7.4 describes — though the technical details in 7.4's own RESULT text, e.g. `PAGES_SPECS` regex, `serve-site.mjs --no-isolation-headers`, `coi-serviceworker.js`'s reload guard, all check out against the current tree). 7.6: `git status -sb` shows the main tree is pushed to `origin/main` (no ahead/behind) and Sheaf is pushed to `fork/fix-out-of-tree-app-gaps`, but the main working tree is currently NOT clean — see the contradiction noted in §5 below. |
| 7b. Postflight findings | 4 | 0 | 4 | Per your explicit instruction. |
| 8. Operator | 2 | 0 | 2 | Per your explicit instruction. |
| **Total** | **49** | **32** | **17** | |

FOUND vs CHANGED: examined all 49; changed (`[ ]`→`[x]`) 32; left `[ ]` on 17.

Notable judgment calls, so nothing is a silent tick:

- **1.1–1.3, 1.5, 1.6 ticked.** 1.2 asked to "Build an `ActivationLease` in
  `site-boot.mjs` and pass it." That literal artifact does **not** exist in the
  tree — I read the whole file (`app/browser/site/site-boot.mjs`) and there is
  no `ActivationLease` reference anywhere in it. What the file's own
  `SECTION 1 OUTCOME` prose (already present, not written by me) documents, and
  what I independently confirmed by reading `site-boot.mjs:13-42,93-112`, is
  that the lease approach was tried, found to deadlock activation (no eager
  `.resume()` in the shipped file), and replaced by `audioOptions: {
  audioContext }` instead — with the header comment rewritten exactly as 1.2's
  second half demanded. I ticked 1.2 because the concrete deliverable (a
  corrected `site-boot.mjs` that supplies the context safely, with a header
  comment explaining why a lease is wrong here) exists and is verified, even
  though the specific mechanism named in the task text is not what shipped.
  1.3's "run the spec and confirm right reason" is a test execution I could
  not redo (no build/test allowed); I relied on the file's own recorded
  positive-control result (6/6 green) plus my own read confirming
  `site-boot.mjs` never calls `.resume()`, so the described risk cannot fire
  under the shipped code.
- **7.4 left open, not ticked**, even though its RESULT text is detailed and
  its supporting technical claims all check out against the tree (see table
  above). I did not re-run the suite, and the pass/fail counts are the actual
  claim of the task, so I did not tick it on cross-referenced detail alone.

## A2 — MIDI claim correction

File: `openspec/changes/archive/2026-08-27-frogg3rs-web-release-repair/specs/froggers-browser-package/spec.md`

Traced before editing: `main.ts:178` constructs `BrowserMidiManager`
unconditionally; `main.ts:174-177`'s `BrowserUiBackend` dispatch wiring calls
`startUserActivation()` after every action; `startUserActivation`
(`main.ts:267-276`) calls `this.midi.startFromUserActivation()` unconditionally
whenever `this.audio` is set (which it is, via `audioOptions`, with no lease at
all); and `midi.ts:108-114`'s `startFromUserActivation()` calls
`navigator.requestMIDIAccess({ sysex: true })` regardless of any lease. So MIDI
reaches the runtime through the same first-in-app-action path as audio, not
exclusively through a lease — the lease-eager branch at `main.ts:211-219` is
one path in, not the only one. This matches what a later session in this repo
already found and recorded (`main.ts:178`, `midi.ts:112-115`, `main.ts:267-276`
cited identically in commit `a11f3a6`/`6fe7085`).

**Before:**
> Removing the picker also removed the activation lease, and the lease is what carries the audio context and MIDI access. Nothing said the boot path owed those, so nothing caught it.

**After:**
> Removing the picker also removed the activation lease, and the lease is what carries the audio context. Nothing said the boot path owed it, so nothing caught it. MIDI access is not lease-exclusive: `BrowserUiBackend`'s dispatch wiring requests it directly on the same first-in-app-action path audio activation already uses (`startUserActivation`, `main.ts:267-276`; `midi.ts:112-115`), independent of whether a lease was ever acquired.

Nothing under `### Requirement:` or `#### Scenario:` in that file was touched;
only the intro paragraph (lines 5-6) changed.

## A3 — Sync into main specs

Followed `.claude/skills/openspec-sync-specs/SKILL.md` steps 4a-4d.
`openspec status --change frogg3rs-web-release-repair --json` reports
`actionContext.mode: "repo-local"` (not `workspace-planning`), so the sync is
in scope. All three target main-spec files already existed.

| Capability | Main spec existed? | Change |
|---|---|---|
| `froggers-browser-package` | Yes | ADDED requirement **"The browser boot supplies the audio context capture requires"** (with 3 scenarios: "The audio context reaches the audio bridge", "An input device can be chosen", "Supplying the context does not start audio") — synced from A2's corrected delta. Appended after the existing "Registry listing requires no Sheaf source change" requirement; nothing existing removed. |
| `froggers-modulation-slate` | Yes | MODIFIED requirement **"External-audio sources stay present but inert when unavailable"** — replaced the stale "A disconnected source SHALL present no control ... drawing no encoder" bullet with the corrected "A disconnected source SHALL render as a disabled control, not as nothing" bullet plus the delta's three supporting paragraphs (neutral-colour de-emphasis, no value arc, no press/drag/depth-parameter + surface-owned rendering). Updated the "Disconnection is the inert state, not a removal" scenario's stale "draw no encoder and carry no press or drag action" THEN-line to match (drops "draw no encoder"). Added two new scenarios: "A disconnected cell is drawn, and drawn as unavailable" and "Drawing it does not make it reachable". Left the pre-existing "A host-opened default device does not count as routed" scenario untouched (delta repeats it verbatim). All other paragraphs/scenarios in the requirement (affirmative-act connection, host-routed-signal semantics, declining-input default, persisted-selection reset) preserved unchanged. |
| `froggers-sheaf-runtime-app` | Yes | ADDED two new requirements, appended after the existing "Operator documentation ships with the app" requirement: **"The runtime chrome reports its load honestly"** (4 scenarios) and **"The shipped documentation addresses someone learning the instrument"** (3 scenarios). Both are new subject matter, not overlapping the existing "Operator documentation ships with the app" requirement, which is about offline bundling/embedding, not tone/content. |

None of the three changes were archived (`ls openspec/changes/` and
`openspec status --json` still show `frogg3rs-web-release-repair` as active,
`isComplete: false`).

## Files modified

- `/Users/diegoaguilar-canabal/Desktop/frogg3rs/openspec/changes/archive/2026-08-27-frogg3rs-web-release-repair/tasks.md`
- `/Users/diegoaguilar-canabal/Desktop/frogg3rs/openspec/changes/archive/2026-08-27-frogg3rs-web-release-repair/specs/froggers-browser-package/spec.md`
- `/Users/diegoaguilar-canabal/Desktop/frogg3rs/openspec/specs/froggers-browser-package/spec.md`
- `/Users/diegoaguilar-canabal/Desktop/frogg3rs/openspec/specs/froggers-modulation-slate/spec.md`
- `/Users/diegoaguilar-canabal/Desktop/frogg3rs/openspec/specs/froggers-sheaf-runtime-app/spec.md`

```
$ git status --short
 D openspec/changes/frogg3rs-first-visit-and-open-repairs/form-control-width-requirement.md
 M openspec/changes/frogg3rs-first-visit-and-open-repairs/proposal.md
 M openspec/changes/frogg3rs-first-visit-and-open-repairs/tasks.md
 M openspec/changes/archive/2026-08-27-frogg3rs-web-release-repair/specs/froggers-browser-package/spec.md
 M openspec/changes/archive/2026-08-27-frogg3rs-web-release-repair/tasks.md
 M openspec/specs/froggers-browser-package/spec.md
 M openspec/specs/froggers-modulation-slate/spec.md
 M openspec/specs/froggers-sheaf-runtime-app/spec.md
?? openspec/changes/frogg3rs-first-visit-and-open-repairs/preflight.md
?? openspec/changes/frogg3rs-first-visit-and-open-repairs/specs/
```

The four `frogg3rs-first-visit-and-open-repairs/*` lines (one deletion, two
modifications, two untracked) are **not mine** — they were already in the
working tree before I touched anything (confirmed by running `git status`
before any edit). See the contradiction note below.

## Contradictions found against this prompt

1. **"Repo: ... currently clean" is false.** Before I made any change,
   `git status --short` already showed an uncommitted deletion
   (`form-control-width-requirement.md`) and uncommitted modifications to
   `frogg3rs-first-visit-and-open-repairs/proposal.md` and `tasks.md`, plus two
   untracked paths (`preflight.md`, `specs/`). This is exactly the kind of
   state your own memory note ("Subagent launch & review discipline" — check
   `git status` after any stall) warns about. It does not touch any file this
   task asked me to edit, so I left it alone and did not investigate further
   (out of scope for A1-A3), but it is not the clean tree the prompt asserted,
   and it looks like unfinished, uncommitted work on the successor change from
   an earlier session.
2. Everything else in the prompt — anchors, line numbers, commit shas, the
   "must stay unchecked" list — checked out against the tree as given; no
   other contradictions found.
