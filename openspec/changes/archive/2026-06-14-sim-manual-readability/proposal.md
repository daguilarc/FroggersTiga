## Why

The sim operator docs (`SIM_MANUAL.md`, `QUICK_DICT.md`, and their `docs/` / `web/public/` mirrors) are accurate after PM3 and CC-gating fixes, but hard to read: concepts repeat across sections, the mod-bay list breaks mid-bullet, page tables ship without glosses, and `docs/sim-manual.md` has drifted from the canonical root copy. Operators bounce between Manual and Quick Dict to understand one knob, and historical phrasing ("no longer controls PM3") adds noise without teaching current behavior.

## What Changes

- Restructure **SIM_MANUAL.md** for teach-first flow: play → pages → knobs → mod bay → host differences → page reference.
- Consolidate repeated concepts into single authoritative subsections (mod indicators at 55%, Random S&H stepping/slew, Crispy behavior, CC enable/disable).
- Add one-line glosses beside each page table row (sourced from `QUICK_DICT.md` content, not duplicated verbatim six times for Crispy).
- Rewrite **QUICK_DICT.md** for scan rhythm: consistent `Label — gloss` cadence, grouped transport buttons, one global Crispy entry, CC gating notes aligned with manual.
- Eliminate copy drift: one canonical write path (`SIM_MANUAL.md` / `QUICK_DICT.md` at repo root) synced to `docs/` and `web/public/`; add a CI or script check if none exists.
- Remove implementation-facing opener (`ParamDisplayNames` in the first paragraph) and changelog-style negatives from operator-facing prose.
- Align label spelling with UI (`Rand mod`, not `Randmod` in manual contexts).
- **Non-breaking** for DSP, UI labels (`ParamDisplayNames.hpp`), or firmware — documentation and copy only.

## Capabilities

### New Capabilities

- `sim-manual-structure`: Operator manual information architecture, section order, and deduplicated concept blocks for all sim hosts.
- `sim-quick-dict-cadence`: Quick Dict format, grouping, and gloss consistency for in-app reference.

### Modified Capabilities

- (none — no baseline `openspec/specs/` yet)

## Impact

- `SIM_MANUAL.md` — primary rewrite target
- `QUICK_DICT.md` — parallel rewrite
- `docs/sim-manual.md`, `docs/quick-dict.md` — sync from canonical
- `web/public/sim-manual.md`, `web/public/quick-dict.md` — sync from canonical (embedded in web help modal)
- Optional: `sim/check_*` script or CI step to fail on manual/dict drift between root and mirrors
- `web/src/main.ts` — page blurbs in `PAGE_BLURBS` only if they should match new manual tone (verify, minimal edits)
