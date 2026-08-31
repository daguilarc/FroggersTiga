# Tasks — `frogg3rs-verify-reset-and-default-state`

## 0. Verify the tree being measured

    git status --short -- app External/Sheaf
    git -C External/Sheaf status --short

Both empty before any evidence run, and re-checked before each. Other changes
are live in this working tree; a build over their uncommitted edits measures a
tree no commit claims. Record the output alongside the evidence it qualifies.

## 1. Build the shipping app

    ./app/build-launcher.sh

The only path that compiles Sheaf's runtime shell. `FroggersAppCore.hpp`
changed and `Engine<FroggersApp>` instantiates through that shell, which no
committed gate touches.

If it builds, launch it and confirm by ear that Reset All after Randomize All
returns the instrument to its launch sound, and that New does too. Listen to
the randomized state BEFORE pressing Reset All, and count the check only if it
sounded different from launch: "sounds the same after Reset" is evidence only
when the ear demonstrably had a difference to hear.

## 2. Build the browser target

    make -C app/browser build
    make -C app/browser test

## 3. Reconcile the 40-vs-321 result-line contradiction

    rm -f app/build/froggers_audio_routing_tests
    make -C app test 2>&1 | grep -cE '^\[(PASS|FAIL)\]'

Then compile the same file by hand using the flags `app/Makefile` actually
defines for `AUDIO_ROUTING_BIN` — read them, do not assume — and compare.

If the hand build is short, every hand-built measurement in the archived change
is partial and the audible evidence for Defect A must be re-derived through
`make`. If they match, the 40-45 readings were an artifact of the broken probes
— but a match validates the flags-correct build class, not the archived
invocations themselves. Before citing the audible result, confirm the archived
runs' recorded command lines used these same flags; if they were not recorded,
rest the citation on the late-`TEST_CASE` output those runs printed, and say
that is what it rests on.

## 4. Decide what the evidence supports

Two decisions, on different criteria. They were conflated in the handoff and
must not be.

### 4a. The four pushed commits: stand, amend, or revert

Criterion, applied PER COMMIT: does evidence from a build that provably ran
support the claim that commit makes?

"Provably ran" means the run emitted a full result count -- 321 lines for
`make -C app test`, 918 for the Sheaf gate. A hand invocation that emitted 40
does not qualify, whatever it printed.

| commit | claim | load-bearing evidence | produced by |
|---|---|---|---|
| `d12b979` | Grace/Curve mappings give usable travel | app gate 321/0 | `make` |
| `aed087b` | Reset reseeds; the walk window is closed | 84 parameters differing at +0 blocks before, 0 after | `make` |
| `94eeb7c` | A fast sweep does not latch the instrument | swept 9.39e-13 vs Freeze control 0.509 | `make` |
| `0f00e49` | New restores the startup state | three-way detent equality, element-wise | `make` |

**Revert on a build failure. Amend the record on an evidence failure.** These
are different remedies for different faults and the distinction decides this
task:

- **Task 1 or 2 fails** -> committed code breaks a target nobody compiled. The
  code is WRONG. Revert or fix the offending commit.
- **Task 3 shows the hand builds were short** -> the audible 12/12 -> 0/12
  result is unsupported. The code is still right; the PROPOSAL over-claimed.
  Strike that evidence from the archived proposal and from `aed087b`'s message
  if it is cited there. Do not revert: `aed087b`'s regression check is the
  parameter transient, which came from `make` and is untouched by this.
- **Task 3 shows they match** -> the 40-45 readings were an artifact of the
  broken probes. Record that and cite the audible result normally.

Note what this means: the 40-vs-321 contradiction cannot by itself justify
reverting anything. It is a question about a corroborating measurement, not
about the check any commit rests on.

`0f00e49` carries one further dependency: it pins a Sheaf commit and its
behaviour requires the `HasRestoreStartupState` hook. If PR #10 is rejected
upstream, or the fork branch is rewritten, that commit's fix stops working and
the detent test fails. That is a dependency to track, not a reason to revert
now.

### 4b. The supersession itself: commit or correct first

Whether to commit the archive deletions and this change directory as a fifth
commit on the branch.

Criterion: whether this document's factual claims check out. It asserts commit
SHAs, the 84/0.19/8-block numbers, the 9.39e-13 and 0.509 readings, that
Sheaf's gate compiles no `Runtime.hpp`, and a list of specific instrument
failures. Verify them; correct what is wrong; then commit.

This is a documentation decision, not a judgement about the code. It does not
depend on 4a.

## 5. Finish or drop the drift proof

The carried-forward spec delta requires launch, Reset All and New to agree, and
that a check fail when any drifts. Proven for New only.

Launch drift is near-vacuous to prove — ~300 tests depend on the default patch.
Only Reset is worth proving, and only as a `TEST_CASE` constructing the drifted
state directly.

If not done, say so rather than leaving the claim ambiguous.

## 6. Constraint on how any of this is verified

Do not write bespoke shell harnesses. Every one written during the previous
change silently did nothing and returned a result that read as success; every
check written as a `TEST_CASE` in the existing binary worked first time. The
binary proves it ran by emitting 321 result lines. A shell probe proves nothing.
