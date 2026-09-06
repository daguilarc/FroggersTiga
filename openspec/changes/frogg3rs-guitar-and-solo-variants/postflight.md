# Postflight audit — `frogg3rs-guitar-and-solo-variants`

Run 2026-08-28 against the implementation. Compares implementation to the
revised proposal and tasks, reports divergence, and re-runs §7 against the diff
rather than against the plan.

Verdict: **PASS with three recorded divergences, all deliberate and stated below.**

---

## A. Gates

| gate | result |
|---|---|
| `src/FroggersSolo` firmware | builds; text 87040, data 1500, bss 189792 |
| `src/FroggersGuitar` firmware | builds; text 85860, data 1500, bss 140516 |
| `test/firmware` host suite | 5/5 pass — HookIdentity, ModMgr, PairArEnvelope, RefreshGate, VariantMix (28 checks) |

The bss difference, 189792 − 140516 = **49,276 bytes**, is the positive control
on "Guitar has no reverb page": the three 4096-float delay lines are 49,152
bytes. They are absent from the binary, not switched off inside it.

## B. Divergence from the plan

### B1. `UpdateParams()` was not moved to block rate — refused on measurement

Task 0.5.1 required this be settled before anything else ran. It was, and the
answer was no. Block size 48 at 48 kHz puts a block-rate call at 1 kHz, which is
where `RuntimeParam`'s smoother wants its cutoff; measured time to 90% of a step
is 0.375 ms per sample, 18.0 ms per block at the same alpha, and 1.0 ms per
block re-derived — the last only because `OPLowPassFilter::x_maxCutoff` clamps
the request from 1.0 to 0.499. The quantity moved, so this is a real negative.

This is a divergence from the ORIGINAL plan and an exact match to the revised
one. The spec delta was rewritten to state what the system does rather than what
was wanted: `Parameter application runs at block rate` was replaced by
`Parameter smoothing is not moved to block rate`. Promoting the original wording
would have asserted a behaviour that does not exist and that nothing tests.

### B2. The boot splash was built and then removed — scope this change never had

Task 1.2 asked that the variant name reach the operator via "boot screen, manual,
and the firmware artifact's filename". The preflight established (D4) that there
is no boot screen: `DaisyIO::Init` clears the OLED and draws nothing. Satisfying
the clause therefore meant ADDING one — a new operator-facing feature, costing
~900 ms of `System::Delay` on every power-on, that nobody asked for.

It was built anyway, on the authority of the task list, and removed on operator
instruction. `DaisyIO::Init` is now byte-identical to its pre-change form. The
removal took 3536 bytes of text out of each binary (Guitar 89396 → 85860, Solo
90596 → 87040), which is the font and string data the splash pulled in.

`FROGGERS_VARIANT_NAME` went with it: after the splash, its only consumers were
the two test functions that existed to assert it — a macro kept alive by its own
test. Each variant is still identified behaviourally, by reverb presence, page
count, and mix formula, which is what the tests assert.

**The process failure is the preflight's, not the execution's.** D4 found the
fact that made the task suspect and recorded it as a note rather than raising it.
§1's corollary applies exactly: when honouring an instruction forces work the
structure does not want, the instruction is suspect. A task that says "label X"
where X does not exist is not asking for a label, and that is the operator's call
to make, not the task list's.

Task 1.2's boot-screen clause is deliberately unmet. Task 6.4, the operator check
that both artifacts identify themselves on boot, is dropped with it.

### B3. `ResonantBump`'s individual setters were removed, which task 0.5.2 said to keep

0.5.2 said to leave `SetFreq` / `SetHeight` / `SetWidth` "intact for any caller
that genuinely changes one value". After the scoop notch was deleted, that
caller did not exist: the only remaining uses of `m_resonantBump` are
`SetParams` (`FroggersEngine.hpp:567`) and `Process` (`:852`). A consumer that
might exist is not one, so the setters went with the branch. `SetParams` is now
the only setter, and the header says so.

## C. Delivered, task by task

| task | state |
|---|---|
| 0.2 editor backups | `src/mk/daisy.mk~`, `src/Blink/Makefile.bak`, `src/Blink/Makefile~` removed (all three were tracked) |
| 0.3 spec overreach | both the Purpose line and the requirement corrected; `field-operator-doc-parity` confirmed clean |
| 0.4 archive banner | added; recorded as working-tree only, since `.gitignore:39` ignores the archive |
| 0.5.1 | REFUSED, measured — see B1 |
| 0.5.2 dead branch | deleted: the parallel branch, `m_scoopNotch`, `m_filterScoop`, `m_filterCombPeak`, `m_useV2FilterParallel`, `SetUseV2FilterParallel`, and the `GetParam(7)`/`GetParam(8)` reads. Six biquad recomputes per sample → **one**, asserted by test |
| 0.5.3 smoothers | all three double-reads gone; each smoother in `UpdateParams` now advances once, asserted by test |
| 0.5.4 | satisfied by B1 plus 0.5.2 |
| 0.5.5 LED throttle | `RefreshGate` on the LED bus; transmit on change or at ~30 Hz |
| 0.5.6 reverb early-out | Solo only, hysteresis 1e-4 enter / 5e-4 exit, both directions asserted |
| 0.5.7 drain | one page per `DrainOne`, coalescing kept, B1/B3 still immediate |
| 1.1 two programs | `src/FroggersSolo/` and `src/FroggersGuitar/`, separate targets, separate `build/`, separate `.bin` |
| 1.2 identification | artifact filenames and manual. Boot splash built then removed — see B2; the clause is deliberately unmet |
| 1.3 rename inbound half | `DAISY_MANUAL.md` updated; no `FroggersTiga` left in it |
| 2.1–2.3 reverb page | absent from Guitar; page count 5 → 4, asserted by test |
| 3.1–3.4 mix | delivered, asserted against the exact weights |
| 5.1–5.6 tests | delivered; see §D |

## D. Every scenario, and what backs it

`external-ring-mod-mix`:

| scenario | backed by |
|---|---|
| Silent external input, either variant | `VariantMix_test` — gate-closed bit-identical across variants, and equal to `OLVL × average` |
| Guitar with an active external input | `VariantMix_test` — asserted against `7/12` and `5/12` as fractions, and that the weights sum to exactly 1 |
| Solo is unchanged by the Guitar variant | `VariantMix_test` — Solo gate-open equals the ring mod with no dry term |
| One chain, not two | `VariantMix_test` — the FX-insert hook counts 256 chain runs over 256 samples, bypassed and engaged |
| Pair-AR active with the gate open | `VariantMix_test` — output unchanged with pair-AR on, plus a positive control that pair-AR state actually advanced, plus identical envelope state across variants |

`field-button-input-latency`:

| scenario | backed by |
|---|---|
| Smoothing time is unchanged by this work | the measurement in B1, and `VariantMix_test` asserting each bump smoother advances exactly one step per `UpdateParams` |
| One coefficient recompute per sample | `VariantMix_test` — instrumented count is 1 |
| Each smoother advances once per sample | `VariantMix_test` — both bump smoothers land on the reference one-step value and on each other |
| Static LEDs do not transmit every poll | `RefreshGate_test` — 1000 polls with no change yield 30 transmits |
| Zero mix skips the reverb | `VariantMix_test` |
| A knob at the threshold does not chatter | `VariantMix_test` — the same mix gives opposite states depending on approach direction, which a bare threshold could not |
| A fix with no moved number is unproven | process requirement; every claim in §A and B1 carries its number |

**Not backed by an executing check, stated plainly:** tasks 6.1–6.3 are operator
checks on hardware and remain undone until someone runs them. Nothing here
claims the device has been heard. Task 6.4 is dropped with the boot splash (B2).

## E. §7 re-run against the diff

Every named concept the diff introduces, enumerated by operand across the tree:

| new concept | existing sites expressing it | action |
|---|---|---|
| `RefreshGate` | `kScreenThrottleMs` and `kLedThrottleMs`, both 33, with the same `dirty \|\| elapsed` body at two call sites | **collapsed into one struct** — the second was created by this diff, so this is duplication this change caused and this change removed |
| `ResonantBump::SetParams` | `SetFreq`/`SetHeight`/`SetWidth`, zero callers after the deletion | **removed** — see B2 |
| `x_guitarDryWeight` / `x_guitarRingWeight` | no other site computes a 7:5 external blend | none needed |
| `x_rvBypassEnter` / `x_rvBypassExit` | no other hysteresis pair in `src/` | none needed |
| `FROGGERS_HAS_REVERB` / `FROGGERS_EXTERNAL_DRY_PARALLEL` | single definition site, `src/core/FroggersVariant.hpp`, included by `FroggersEngine.hpp` alone after the splash removal | none needed |
| `RandomizePageFromKnobs` / `RandomizePageModFromKnobs` | `RandomizeAllPages` / `RandomizeAllPagesMod` open-coded the same loop | **both rewritten to call the new helpers**, so the loop body has one definition. `RandomizePage` was deliberately NOT merged: it randomizes against each parameter's stored value, not the live knob position, and collapsing them would have changed pickup behaviour |
| `FROGGERS_TEST_INSTRUMENTATION` | none | none needed; verified absent from both firmware binaries (Guitar text unchanged at 89396 with and without the test define) |

## F. Findings raised, not fixed — out of scope, stated so they are not lost

1. **`app/` carries ~20 comments citing `FroggersEngine.hpp:LINE`.** Every one is
   now stale. `app/` is out of bounds for this change. Named in the proposal's
   accepted collateral.
2. **`DaisyIO::ProcessControls` writes LED indices 16–25 in a loop bounded by
   `Parameter::x_numParameters` (10), but only 8 knob LEDs exist.** Indices 24
   and 25 are `LED_SW_1` / `LED_SW_2`, which receive the tracking state of
   parameters 8 and 9 and are then overwritten two lines later by the switch
   state. Harmless only by the ordering. Not touched: fixing it changes LED
   behaviour, which is outside this change.
3. **`FieldMutationQueue::Enqueue` coalesces against the last entry only**, so an
   alternating RandAll / RandAllMod sequence still fills the queue. Unchanged
   behaviour, unchanged by this work.
4. **The working tree carried unrelated uncommitted work when this change
   started** — `.gitignore` newly ignoring `openspec/changes/archive/` with 599
   files staged for deletion, plus `README.md` and `app/browser/e2e/` edits
   belonging to `frogg3rs-browser-audio-device-selection`. None of it was
   touched. It is not this change's to commit.
