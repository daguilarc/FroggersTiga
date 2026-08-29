# Proposal — `frogg3rs-microphone-path-delivery`

**Created 2026-08-29. Successor to `frogg3rs-browser-microphone-permission-path`,
which is 48/68 executed, entirely uncommitted, and stopped by the operator.**

The predecessor's code works. What failed was the process around it, and this
change exists because carrying that process forward unchanged would repeat it.

## What is in the tree right now, unpushed

41 files across two repositories, none committed, frogg3rs on `main`. 26 of
them are frogg3rs entries, counting `External/Sheaf` as the single submodule
pointer it appears as there; Sheaf carries the other 15 in its own tree. A
`git status` run in frogg3rs reports the 26 and says nothing about the 15:

- Browser microphone route: a new `Allow Microphone` control, offered only when
  the page enumerates input devices it cannot name.
- Delay Wet mix: a shared `kMaxWetMix` ceiling and a wet-level follower.
- Stereo carried to the output; both Width controls now change what is heard.
- The macOS bundle's `NSMicrophoneUsageDescription`.
- Browser ABI version single-sourcing, a stale-server currency check, the
  Windows VST3 build. These came from a third change,
  `frogg3rs-version-single-source-and-windows-vst`, whose directory the tree
  deletes: its delta was folded into the predecessor's own
  `frogg3rs-distribution` delta, correctly as ADDED rather than MODIFIED, since
  `openspec/specs/frogg3rs-distribution/spec.md` carries no such requirement.
  The three deletions ship with this change.

## What is verified, and what only looks it

VERIFIED, with numbers:

- app suite 314 pass / 0 fail. CORRECTED: an earlier draft of this proposal
  called it stale "by the last two Sheaf header edits". That was asserted, not
  traced, and it is false — no file under `app/` includes `RuntimePages.hpp` or
  `RuntimeMainComponent.hpp`, and the dependency runs `RuntimePages ->
  PortableUI`, not back. It IS stale, for a reason that draft missed: `make -C
  app test` gained a `check-microphone-usage` prerequisite that has never run
  inside the gate.
- Sheaf synth gate: 2 failures, both the known pre-existing 96kHz braid4
  deadline tests. Run AFTER the last header edits.
- Sheaf browser suite 225 pass / 0 fail — predates the last `audio.ts` edit.
- VST ctest 5/5, including the two new bundled-docs checks.
- frogg3rs e2e `audio-devices`: 8 pass / 1 fail.

NOT VERIFIED, and not to be treated as done:

- The last `audio.ts` edit (reporting an enumeration failure once the channel
  count is known) is written and typechecks. It has never been run. The e2e it
  was written for was never re-run after it.

The correction above was found by checking one claim of this proposal against
the include graph, and it took under two minutes. It is recorded rather than
quietly fixed because it is the same defect as the ones below, committed inside
the document reviewing them: a dependency asserted from memory of the session
instead of read. A self-review is produced by the process that produced the
errors, so it inherits their blind spots; the checks in section 0 exist because
prose about being careful does not.

## The process failures this change is scoped around

Recorded because they are the reason the predecessor stopped, and because each
one has a cheap procedural fix.

1. **An enumeration was run, printed, and not acted on.** The
   `kAudioInputRetry` family has six sites. Five were updated. The sixth,
   `RuntimeMainComponent.hpp`'s `IsAudioAction` allow-list, appeared in the
   grep output and was read past — shipping a rendered button whose action the
   router silently dropped. Only the e2e caught it.
2. **The forward enumeration (§9) was run on the interesting concept only.** The
   plan named `kMaxDelayWetMix` "mirroring `kMaxReverbWetMix`" — a second
   constant holding the same value in the same function — and preflight
   approved it.
3. **The hygiene sweep (§8.0) covered one of the two trees the change touches.**
   `External/Sheaf/projects/synth/browser/` was never swept, and a dead
   duplicate static server binding the live server's own ports was found later
   by accident.
4. **A precedent was cited without tracing how it resolves.**
   `package-contract.mjs`'s `"./protocol.js"` import works only from `dist/`;
   copying it into a file that runs from `src/` broke the browser suite.
5. **Three measurements were believed while their instrument was dead**: a
   cost comparison taken under concurrent build load, and two positive controls
   run against stale binaries.
6. **A test asserted a difference against a silent fixture.** Output peak was
   0, so both Width tests read "no difference" and meant nothing.

## Non-goals

- Redesigning anything the predecessor built. Its code is verified where the
  gates above say so; this change delivers it, it does not revisit it.
- Any new feature.

## Impact

Directories this change touches, and therefore the directories its hygiene
sweep covers. Named here rather than left implicit, because a sweep bound to an
Impact section that names nothing is a sweep of nothing.

In `frogg3rs`: the repository root (`MANUAL.md`, `.gitignore`),
`.github/workflows/`, `app/`, `app/browser/e2e/`, `app/dsp/`, `app/vst/`,
`openspec/changes/`.

In `External/Sheaf`: `projects/synth/browser/` with its `src/` and `tests/`,
`projects/synth/include/synth/` with its `browser/`, and
`projects/synth/tests/`.

- Source changes are the preflight's blocking findings, the one unverified
  `audio.ts` edit, and whatever the remaining gates surface. Nothing else.
- `openspec/changes/frogg3rs-browser-microphone-permission-path` is archived by
  this change once its own delivery items close, which applies its three deltas
  to `openspec/specs/` and moves the directory into the ignored archive.
- `openspec/changes/frogg3rs-version-single-source-and-windows-vst` is deleted,
  its delta having already been folded into the predecessor's.
