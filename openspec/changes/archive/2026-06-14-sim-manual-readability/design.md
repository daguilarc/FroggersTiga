## Context

Sim operator documentation lives in six markdown files that should be three:

| Role | Canonical | Mirrors (must match) |
|------|-----------|----------------------|
| Manual | `SIM_MANUAL.md` | `docs/sim-manual.md`, `web/public/sim-manual.md` |
| Quick Dict | `QUICK_DICT.md` | `docs/quick-dict.md`, `web/public/quick-dict.md` |

`docs/sim-manual.md` is currently stale (missing CC enable/disable, outdated VST MIDI Settings). The canonical `SIM_MANUAL.md` is accurate but structurally dense: the Mod bay bullet list breaks after MIDI CC 2, Random S&H is explained four times, the 55% LED rule appears in four places, and page tables list names without glosses. `QUICK_DICT.md` repeats Crispy six times and uses `Randmod` while the UI says **Rand mod**.

UI labels remain authoritative via `sim/ParamDisplayNames.hpp`; this change does not alter code labels.

## Goals / Non-Goals

**Goals:**

- Teach operators in one pass: sound → pages → knobs → mod → host quirks → lookup tables.
- Deduplicate concepts; cross-link instead of copy-paste.
- Restore mirror parity and prevent future drift.
- Improve scan rhythm in Quick Dict without changing semantic content.

**Non-Goals:**

- Rewriting Daisy Field `MANUAL.md`.
- Changing DSP, mod routing, or UI control labels.
- Generating docs from `ParamDisplayNames.hpp` (future improvement; out of scope here).
- Rewriting `web/src/main.ts` `PAGE_BLURBS` unless a factual error is found (Audio blurb still says "output level" — flag for optional one-line fix).

## Decisions

### 1. New manual outline (replace current section order)

```
# FroggersTiga Simulator Manual
Intro (1 short paragraph — hosts, not ParamDisplayNames)

## Quick start (5 steps, play-first)

## Layout
- Six pages + Delay; knobs 1–7 params, knob 8 Crispy everywhere
- Web: one page at a time; Desktop: five panels + Delay overlay; Plugin: desktop layout, DAW transport

## Global controls
### Crispy (define once)
### Transport table (grouped: playback | external | randomize knobs | randomize mod | random | waveforms)

## Mod bay
### Sources (5 bullets — one sentence each)
### Routing (web dropdown depth vs desktop cable — one paragraph)
### MIDI CC enable/disable (one paragraph — grey, clear routes, QWERTY)
### Mod indicators (scopes, LED 55% rule, VCO Envelope trace — one subsection)
### Random S&H (step, slew, no clock — one subsection; Page 2 points here)

## Page reference (Audio … Delay)
Each: short intro (1 sentence) + 3-column table (Row | Parameter | What it does)
Audio: extra note on waveform icons + PM3/cross-coupler (no "no longer" phrasing)

## Host guide
### Desktop
### Web (External, External MIDI, permissions)
### VST3 / AU (transport, hidden settings, build one-liner)

---
Hardware pointer | Version history (unchanged factual bullets)
```

**Alternative considered:** Merge manual and Quick Dict into one file. Rejected — in-app help loads them separately; Quick Dict must stay scannable in ~90 lines.

### 2. Quick Dict rewrite pattern

- Separator: em dash throughout.
- New **Global** section after Transport:

  ```
  Crispy — Scramble knobs 1–7; on sim, also blends external ring-mod when Ext. In. is on
  ```

- Remove six per-page Crispy lines.
- Mod sources: add CC disable one-liner on CC 1/CC 2 entries.
- Keep gloss semantics from current dict; tighten wording only.

### 3. Sync enforcement

Add `sim/check_operator_docs_sync.sh` (mirrors existing `sim/check_param_display_names.sh` style):

```bash
cmp -s SIM_MANUAL.md docs/sim-manual.md && \
cmp -s SIM_MANUAL.md web/public/sim-manual.md && \
cmp -s QUICK_DICT.md docs/quick-dict.md && \
cmp -s QUICK_DICT.md web/public/quick-dict.md
```

Wire into existing sim checks or CI if a hook already runs `sim/check_*.sh`.

**Alternative considered:** Symlinks from `docs/` to root. Rejected — web build and GitHub pages may expect files in place; `cmp` check is lower risk.

### 4. Sample rewrite snippets (cadence targets)

**Mod bay — before (broken list):**

> - **MIDI CC 2** — …
>
> When a CC source is disabled, its mod rack…
> - **VCO Envelope** — …

**Mod bay — after:**

> **Sources**
> - **MIDI CC 1** — Hardware or Web MIDI CC latched to CV (default: channel 1, CC 1).
> - **MIDI CC 2** — Same for channel 1, CC 2.
> - **VCO Envelope** — Slow level from the VCO mix; shown as a scope trace.
> - **Random 1 S&H** / **Random 2 S&H** — Stepped random CV; see Random S&H below.
>
> **MIDI CC enable** — Disable a CC input to grey its mod column, block new routes, clear existing ones, and exclude it from random mod. Desktop: MIDI Settings **On** toggle; web: **CC 1** / **CC 2** when External MIDI is on. QWERTY keyboard drives MIDI CC 1 only and respects the CC 1 enable flag.

**Audio PM3 — before:**

> …it no longer controls PM3 on sim hosts.

**Audio PM3 — after:**

> **Phase mod 3** — VCO2 → VCO3 phase-mod depth when the cross-coupler is toward 2→3.

## Risks / Trade-offs

- **[Risk] Longer manual from gloss column** → Keep glosses to one line; Quick Dict remains the ultra-compact view.
- **[Risk] Mirror drift returns** → `check_operator_docs_sync.sh` fails CI on divergence.
- **[Risk] PAGE_BLURBS stale vs new manual** → Optional follow-up task to align Audio blurb ("output level" → coupling/PM); not blocking doc sync.

## Migration Plan

1. Rewrite `SIM_MANUAL.md` in place following outline above.
2. Rewrite `QUICK_DICT.md` in place.
3. Copy both to `docs/` and `web/public/`.
4. Add sync check script; run locally and in CI.
5. Smoke-test web help modal loads updated markdown.

Rollback: revert markdown files only; no runtime dependency.

## Open Questions

- Should `PAGE_BLURBS` in `web/src/main.ts` be updated in this change or a follow-up? Recommend one-line Audio fix if time permits.
- Should version history gain a v1.0.2 entry for doc refresh, or stay under v1.0.1 with no version bump?
