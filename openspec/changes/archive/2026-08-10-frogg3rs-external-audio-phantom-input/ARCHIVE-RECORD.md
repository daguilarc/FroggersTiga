# Archive record — `frogg3rs-external-audio-phantom-input`, archived 2026-08-10

**Status at archive: DELIVERED and verified, not superseded and not dead scope.** Unlike
`../2026-07-25-field-button-latency-headroom/` (dead scope, its target tree frozen out from under
it) or the two archived under `../2026-07-28-`/`../2026-08-05-` (superseded same-generation, sat
live afterward only because nobody moved them), this change's own six tasks (T1–T6, 12 checkboxes)
are all `[x]`, and this archiving pass independently re-verified each against the current code
rather than trusting the checkmarks — see "Task-completeness re-verification" below. It stopped
being current **today**, 2026-08-10, on completion, which is why it is dated today rather than
backdated the way the two precedents above were.

## What landed, and how it was verified

`FroggersApp::Config()` now requests **zero** audio input channels
(`app/FroggersAppCore.hpp:203`, `config.numAudioInputs = 0`). `kExternalAudioOptedIn` is deleted
outright, not flipped; `externalInputHasChannel` and its `block.inputs[0]` read are deleted as
provably-dead branches; and a later postflight pass (T6) found and removed the per-sample
`SetExternalAudioConnected(...)` call inside `FroggersModulationSlate::Step()` that the T1/T2 fix
made newly-dead (both external-audio slots were already `false` at registration, so the per-sample
write — ~96,000×/second — was writing a constant over a constant). The `SetExternalAudioConnected`
*method* is kept, per T6's own hard requirement, now called only where a value can actually vary
(the test fixture's `StepOnce`, and a future re-enable derivation). Confirmed by reading the
current code, not by trusting `tasks.md`'s checkmarks: `grep -rn` for `kExternalAudioOptedIn` and
`externalInputHasChannel` across `app/` returns zero live declarations (history-only comments);
`Step()`'s signature (`app/FroggersModulation.hpp:362-363`) no longer carries an
`externalInputConnected` or `externalAudioSample` parameter; the two rewritten tests
(`froggers_config_requests_zero_audio_input_channels`,
`app/FroggersHeadlessTests.cpp:126`) and the new regression test
(`external_audio_sources_stay_registered_and_disconnected_with_zero_input_channels`,
`app/FroggersHeadlessTests.cpp:156`) are both present with the bodies `tasks.md` describes.

**Suite, as reported by the implementing/verification session and cross-confirmed by this
archiving pass against commit `04e7fd0`'s own message (which independently restates the same
numbers ahead of syncing the spec):** 10 binaries, 191 tests, 0 failures, 0 warnings.
`froggers_stop_flush_repro` (not part of the `test` target): 6/6 PASS, post-flush peak **exactly
0** in all three S1.2 scenarios (seeded, Flip=0 control, Blend=0 control).

**Session capture, `Audio prepared: 48000 Hz, 256 frames, 0 in / 2 out` post-change, replacing the
pre-change `1 in / 2 out`.** The pre-change value is independently confirmed **in this repo**, at
`../2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/F3DIAG-capture-2026-08-07.txt:8`
(`13:47:59 0 Message Audio prepared: 48000 Hz, 256 frames, 1 in / 2 out`, captured before this
change existed). The post-change `0 in / 2 out` reading is exactly what `config.numAudioInputs = 0`
mechanically produces through `Runtime.hpp:237`'s
`initialiseWithDefaultDevices(config.numAudioInputs, ...)` call (traced in `proposal.md` §2, and
the mechanism itself is code-verified above) — but this archiving pass could not locate a
repo-local session-log artifact of that specific post-change capture to cite by path the same way
the pre-change one is cited. (The only in-repo hits for `0 in / 2 out` are inside
`External/Sheaf/analysis/sdd-model-analysis/`'s bundled session-timeline data, which trace to a
different developer's unrelated `SynthMiniapp` run at `/Users/joyo/Sheaf/...` — not Frogg3rs, and
not evidence for this claim despite the matching log format.) Recorded here as reported, with that
gap noted rather than silently cited as independently confirmed.

## What this does NOT do — explicitly not restoring external audio

The two external-audio modulation sources (`kModSlotExternalAudio` / `kModSlotExternalAudioEf`,
slots 13/14) stay **registered and inert** — present in the slate, disconnected, not hidden, not
randomized. Restoring them needs upstream to expose a real routed-input signal
(`UPSTREAM-SHEAF-ASK.md` item 8's own asks 1/2 — an explicit routed-input flag, or the selected
input device name), tracked as **`jvictor0/Sheaf` issue #4** for the flag/device-name ask and, for
the separate `GangedRandomLfoVisualizer` defect noted below, **issue #5**. This archiving pass
could not independently confirm either issue number from repo-local state: `UPSTREAM-SHEAF-ASK.md`
itself (checked as of this archiving, `2026-08-09 21:06` mtime) still shows item 8 sent only by
email, with issues #1–#3 accounted for elsewhere in the file's own ledger and no `#4`/`#5` tag
anywhere in the tree. GitHub issue filing on `jvictor0/Sheaf` leaves no local trace to grep, so
absence of a local citation is not evidence against the filing — it is simply outside what this
pass, confined to the repo, can verify. Recorded as told; a future reader relying on the issue
numbers should confirm them against the actual tracker rather than this line.

Re-enabling is deliberately **two coupled edits**, per the proposal's own preflight amendment
(§1): raising `numAudioInputs` back to 1+ **and** writing `externalInputConnected` (or whatever
re-derivation replaces it) from a real routed signal — never one integer edit alone.

## The preflight finding that changed the implementation before it was built

`proposal.md` §1 originally planned to fold `externalInputHasChannel` /
`externalInputConnected` into a single expression. OMNI §14's preflight audit (2026-08-09) caught
that this would leave the invariant **two-part** — `numAudioInputs = 0` *and* that folded
derivation — so a future reader raising `numAudioInputs` back up (which §6 explicitly anticipates
someone eventually doing) would silently re-arm `externalInputHasChannel` to permanently-true and
recreate the phantom sources, with nothing else needing to change. The amendment moved the fix from
"delete this flag" to "raise this integer" being the landmine — i.e., **would have moved the
landmine, not removed it.** The amended plan instead deletes `externalInputHasChannel` and the
`block.inputs[0]` read outright as provably-dead branches (OMNI §12), so re-enabling requires
consciously writing a new derivation against a real signal rather than flipping one name back on.
`tasks.md` T2.2 carries the same finding at the task level, marked
"AMENDED BY PREFLIGHT 2026-08-09 (OMNI §14)."

## `UPSTREAM-SHEAF-ASK.md` item 8 — corrected in place (T4.1)

Before this change, item 8 claimed the ask had **LANDED at `508d9d68`** and that, in practice, it
delivered the user-opt-in signal the app wanted — both false, per `proposal.md` §0's trace:
`AudioInputView::HasActiveChannel(ch)` at `508d9d68` is `channel < activeChannelCount_ &&
inputs_ != nullptr && inputs_[channel] != nullptr`, the same three permanently-true conditions the
app's own pre-fix check already had, given a default-opened built-in mic — it answers "did the
device give us what we asked for," not "did the operator route something in." Both the FULL LEDGER
row (`UPSTREAM-SHEAF-ASK.md:52`) and the RE-CHECK row (`:118`) now read **"CORRECTED 2026-08-09 by
audit — NOT landed"**, confirmed present by this archiving pass.

**One secondary, pre-existing staleness noted but out of this task's scope to fix:** the FULL
LEDGER row (`UPSTREAM-SHEAF-ASK.md:52`) still ends "`kExternalAudioOptedIn` stays" — true when
T4.1 wrote that correction (ahead of the rest of this change, per its own "DONE 2026-08-09, ahead
of the rest of this change" note), false now that T1/T2 have since deleted the flag entirely. T4.1's
own claimed scope — correcting the false "LANDED"/"can go" verdict — is intact and accurate; this
is a *later* task's consequence the ledger row was never revisited for. Left unedited: this
archiving pass's edit authority is comment-text citation rewrites in source files plus the archive
move itself, not substantive doc content, so this is reported rather than fixed. (Flagged
separately as a follow-up suggestion.)

## Carried forward, still open (unchanged from the predecessor — exact wording below)

Per `tasks.md`'s own "Carried forward as open scope" section, quoting
`../2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/tasks.md`:

- **S4.3–S4.8 — operator eyes/ears only.** S4.1 (Stop works) and S4.2 (blowout fixed) PASSED and
  stay closed. Still open: **S4.3** (Randomize All inside a level-1 drilldown leaves the operator
  where they were, badges changing on that page — visually); **S4.4** (badge density reads as mode
  2 at every level, a drilled parameter shows only sources actually modulating — visually);
  **S4.5** (the drill-level header's placement — STEP 1 as of 2026-08-09; do not move it again from
  this entry); **S4.6** (sustain at minimum is quiet but audible, and audio-rate modulation of an
  envelope parameter no longer gates a voice to silence — by ear, `kMinSustainLevel = 0.05` was
  chosen by argument, not measurement); **S4.7** (saved patches still load from
  `~/Library/Sheaf/synth/sheaf-patch/` and are not rewritten by any default change); **S4.8** (the
  Drive bank's XOR/Flip and Bit-depth/Hash knobs still sound right after S1.3's DC-offset fix — a
  deliberate parity divergence from the frozen firmware the operator has never heard).
- **The fuegoization fixed point.** Zero is a mathematical fixed point of `Fuegoize`'s scramble:
  every step is self-referential XOR (`lowerBits ^= shift(lowerBits) & mask`), and at the minimum
  position every shift of zero is zero, so `0 ^ 0 = 0` for every mask, row, and fuego amount — a
  minimum-position parameter is not perturbed by Crispy/Crunchy while every other position is. This
  is a verbatim port of the firmware's `Parameter.hpp:143` behavior; fixing it would be a
  deliberate parity divergence and is an **operator decision, not taken.**
- **`GangedRandomLfoVisualizer`'s unconditional background fill.** Sheaf-owned
  (`External/Sheaf/.../GangedRandomLfoVisualizer.hpp`), out of scope to read or edit under this
  change's constraints; no app-side lever exists regardless. Was not independently re-verified by
  the executing session (Sheaf-owned code, out of scope to inspect) — now tracked as **Sheaf issue
  #5** per this archiving's own instruction, with the same local-verification caveat noted above.
- **S2 (the slew) was DROPPED by the operator**, predecessor `tasks.md` §S2, 2026-08-07 — do not
  revive it.
- **W4.1 — Sheaf pin bump `77a3019e` → `508d9d68`.** Independently deferred, unrelated to this fix.

## Task-completeness re-verification

All 12 checkboxes in `tasks.md` (T1.1–T1.3, T2.1–T2.3, T3.1–T3.3, T4.1, T5.1, T6.1) were `[x]`
before this archiving pass touched anything (`grep -c '^\s*-\s*\[x\]'` = 12, `grep -n '^\s*-\s*\[
\]'` = no matches). Each was independently re-derived against the current code rather than trusted
from the checkmark alone (see "What landed" above for the specific greps/reads); none were found
unchecked or misdescribing what's in the tree. T5.1's spec delta was confirmed already synced by
hand into `openspec/specs/froggers-modulation-slate/spec.md` at commit `04e7fd0` — see "Spec sync"
below.

## Spec sync — already done by hand, not re-applied here

`04e7fd0` ("Sync the modulation-slate delta into the main spec") added the "Availability is defined
narrowly..." paragraph and two scenarios to the main spec's "External-audio sources stay present
but inert when unavailable" requirement — confirmed via `git show 04e7fd0 -- openspec/specs/
froggers-modulation-slate/spec.md`: a single clean addition (20 insertions, 2 deletions, entirely
rewrapping/extending existing prose), not a duplicate block. **This archiving pass performed a pure
directory move only** — `mkdir`/`mv`, no `openspec archive` CLI invocation — consistent with this
repo's own established practice (`../2026-07-28-frogg3rs-gui-and-dsp-robustness/ARCHIVE-RECORD.md`:
"sync is a separate, deliberate step in this repo's own workflow, not implied by archiving").
Re-read `openspec/specs/froggers-modulation-slate/spec.md` after the move: the "Availability is
defined narrowly" paragraph and both new scenarios appear exactly once each. Nothing this pass did
touched that file.

## Citation sweep performed at archive time

Per this repo's citation-lesson convention (a slug has previously been found split mid-word across
a hard-wrapped comment line, invisible to a whole-slug grep — the specific reason a fragment search
is run independently, not as a fallback): `grep -rn -B1` for the whole slug, plus three independent
fragment searches (`external-audio-phantom`, `phantom-input`, `frogg3rs-external-audio`), all run
repo-wide (excluding `.git`) from the repository root, after the move.

**Whole slug** (`grep -rn -B1 --exclude-dir=.git "frogg3rs-external-audio-phantom-input" .`): 19
occurrences. **Fragment searches** (each independently, no `-B1`): the same 19, plus **2 more the
whole-slug grep could not see** — a slug split mid-word across a line break in two *other*,
already-archived, unrelated changes' own `ARCHIVE-RECORD.md` files
(`../2026-07-28-frogg3rs-gui-and-dsp-robustness/ARCHIVE-RECORD.md:17` and
`../2026-08-05-frogg3rs-audio-safety-and-ui-rework/ARCHIVE-RECORD.md:15`, both the identical
"Successor chain from here: ... → `frogg3rs-external-audio-phantom-` / `input` (live at time of
this archiving)" sentence, written during the immediately-preceding archiving pass when this change
genuinely was still live). **21 found total.**

**Classification: 21 found, 4 changed.**

- **4 genuine path citations, changed** (all sanctioned comment-text rewrites in `app/` source
  files, `openspec/changes/frogg3rs-external-audio-phantom-input/` → `openspec/changes/archive/
  2026-08-10-frogg3rs-external-audio-phantom-input/`): `app/FroggersModulation.hpp:408`,
  `app/FroggersModulationTests.cpp:97`, `app/FroggersHeadlessTests.cpp:170`,
  `app/FroggersAppCore.hpp:656`.
- **17 bare-slug prose mentions, left alone** (no path constructed): 3 in `UPSTREAM-SHEAF-ASK.md`
  (`:52`, `:102`, `:118`, all "`X`'s `proposal.md`" citations with no directory prefix); 6 more in
  `app/` source comments naming the change plus a bare task ID or `tasks.md` with no directory
  prefix (`FroggersModulation.hpp:601`; `FroggersHeadlessTests.cpp:79,112,131`;
  `FroggersAppCore.hpp:168,617`); 4 self-titles inside this directory's own moved files
  (`tasks.md:1`, `SUPERSESSION-RECORD.md:1`, `proposal.md:1`,
  `specs/froggers-modulation-slate/spec.md:8`); 2 in `../2026-07-25-field-button-latency-headroom/
  ARCHIVE-RECORD.md` (`:91`, `:141`) narrating this change's then-live status as of that archiving
  pass — left as authentic history, per that document's own established precedent for exactly this
  situation ("editing someone else's already-archived change's proposal text to reflect information
  only available in hindsight" is the wrong move); the same reasoning applies to the 2
  mid-word-split "successor chain" mentions found only by fragment search, in the two other
  already-archived directories named above — also left alone, also authentic history, also written
  correctly-at-the-time. (3 + 6 + 4 + 2 + 2 = 17.)

**Relative-path check inside the moved directory:** grepped for `../` post-move. Found and fixed 6
now-too-deep `../archive/2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/` cites (2 each in
`proposal.md`, `SUPERSESSION-RECORD.md`, `tasks.md`) — correct as `../archive/...` when this
directory lived directly under `openspec/changes/`, now one level too deep since the directory
itself moved under `openspec/changes/archive/`; corrected to `../2026-08-09-frogg3rs-parametric-
slew-and-stop-root-cause/`, the sibling-relative path from this directory's new location. One
additional `../` hit (`tasks.md:217`, `` External/Sheaf/.../GangedRandomLfoVisualizer.hpp ``) is an
elision marker, not a relative path — left alone.

**No further source files required a citation-text rewrite** beyond the 4 listed above.
