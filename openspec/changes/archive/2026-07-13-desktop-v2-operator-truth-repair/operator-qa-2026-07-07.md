# Operator QA — 2026-07-07 (desktop-v2-operator-truth-repair)

This is the evidence log for the manual-QA pass defined in `tasks.md` Packet 14
("Manual operator QA at 1280×920"). It exists so that no packet can claim a
layout or operator-truth fix is done without a human (or an agent driving the
built app) actually observing the behavior at the target resolution.

Every row below starts at **FAIL / not-yet-verified**. A row may only be
flipped to PASS by whoever performs the live check, and the flip must be
accompanied by a screenshot reference (or explicit note of what was observed)
in the "Evidence" column. Do not mark a row PASS from code-reading alone —
Packet 14's own contract requires driving the running app.

**Archive note (2026-07-13):** This change was archived with all Packet 14 rows
still **UNVALIDATED-AT-ARCHIVE**. Automated packets are green; live GUI has not
been signed off. See `ARCHIVE-STATUS.md` and `tasks.md` Packet 14.

## Screenshot references

Place any screenshots captured during this QA pass under:

`openspec/changes/desktop-v2-operator-truth-repair/evidence/` (create on first use)

and reference the filename in the table below, e.g. `evidence/14.1-no-overlap.png`.
No screenshots have been captured yet as of this document's creation.

## Pass/fail table

### Layout and chrome

| Item | Description | Status | Evidence |
| ---- | ------------ | ------ | -------- |
| 14.1 | No control overlap | FAIL / not-yet-verified | (none yet) |
| 14.2 | No Audio-page scroll; module rows visible | FAIL / not-yet-verified | (none yet — Packet 1 rebalanced kSequencerH/carousel budget; LayoutBounds_test's headless no-scrollbar assertion now passes at 1280x920, so this is expected to pass on live confirmation) |
| 14.3 | No label ellipsis (performance band, scenes, mod summaries) | FAIL / not-yet-verified | (none yet) |
| 14.4 | Top chrome: transport/signal + global-command bands visible | FAIL / not-yet-verified | (none yet) |
| 14.5 | Global oscilloscope with three VCO traces | FAIL / not-yet-verified | (none yet) |
| 14.6 | Global randomization scope radios visible and readable | FAIL / not-yet-verified | (none yet) |
| 14.7 | Full 16-step sequencer; untruncated direction/speed rows; no sequencer scroll | FAIL / not-yet-verified | (none yet) |
| 14.8 | Runtime pages accessible from top nav | FAIL / not-yet-verified | (none yet) |
| 14.9 | Readable S1/S2/S&H labels | FAIL / not-yet-verified | (none yet) |
| 14.10 | Record requires Play first (v1 parity) | FAIL / not-yet-verified | (none yet) |

### Operator truth (behavior)

| Item | Description | Status | Evidence |
| ---- | ------------ | ------ | -------- |
| 14.11 | Single randomization surface (no module-header dup; no sequencer All Steps dup) | FAIL / not-yet-verified | (none yet) |
| 14.12 | Global Rand Mods → live depths; Rand All respects scene scope | FAIL / not-yet-verified | (none yet) |
| 14.13 | All modules affect audio without visiting their carousel pages | FAIL / not-yet-verified | (none yet) |
| 14.14 | Mod pickers fixed width (`kModCellW`), not full-row | FAIL / not-yet-verified | (none yet — Packet 1 capped `moduleRowColumns().modW` at `kModCellW`; LayoutBounds_test's new width-cap assertion (1.4a) now passes, so this is expected to pass on live confirmation) |
| 14.15 | Performance band controls labeled | FAIL / not-yet-verified | (none yet) |
| 14.16 | Write Seq: click-write (stopped) + capture-on-advance (playing) | FAIL / not-yet-verified | (none yet) |
| 14.17 | All Steps + Rand Mods / Rand-seq behave per scope | FAIL / not-yet-verified | (none yet) |
| 14.18 | Delay mod randomization or explicit excluded state | FAIL / not-yet-verified | (none yet) |

## Notes

- This document is created by Packet 0 as evidence scaffolding only. It does
  not itself verify anything.
- **Packet 1 deferral (task 1.5):** Packet 1 landed the layout-authority
  fixes (1.1 `modW` capped at `kModCellW`; 1.2 rebalanced vertical budget so
  the Audio module page fits without a carousel-viewport scrollbar at
  1280x920; 1.4 headless `LayoutBounds_test` assertions for both). Re-verifying
  the archived convergence tasks 3.6–3.9 against the live app, and flipping QA
  items 14.2 (no Audio-page scroll) and 14.14 (mod pickers fixed width) to
  PASS, is a live-GUI step this subagent cannot perform headlessly and was
  **not** attempted or faked here. 3.6–3.9 and 14.2/14.14 remain
  FAIL / not-yet-verified below, now expected to pass given 1.1–1.4, pending
  the human/agent-driven manual QA pass in Packet 14.
- Packets 1–13 repair the underlying defects; Packet 14 is the only packet
  authorized to flip these rows to PASS, and only after driving the built app.
- Do not mark any `tasks.md` checkbox `[x]` on the basis of this document
  alone — this file tracks QA status, it does not substitute for the
  `tasks.md` contract.
