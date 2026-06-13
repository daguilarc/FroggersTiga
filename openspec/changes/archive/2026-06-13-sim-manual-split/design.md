## Context

```
Today (wrong):
  Help → Manual → MANUAL.md (firmware: Pure delay, RMOD, FUEG, flash procedure)
  Knobs/OLED  → ParamDisplayNames (Comb offset, Diffusion, Crunch)

Target:
  Help → Manual → SIM_MANUAL.md (sim names, sim transport)
  Help → Quick Dict → QUICK_DICT.md (PRMT : gloss)
  MANUAL.md → repo only, firmware operators
```

`app-help-menu` spec currently requires `MANUAL.md` in sim bundles. This change supersedes that requirement for sim hosts only.

## Goals / Non-Goals

**Goals:**

- Sim Help **Manual** matches what users see on knobs
- Firmware `MANUAL.md` never edited or embedded in sim
- Desktop and web show identical sim manual text (sync script + BinaryData from same source)

**Non-Goals:**

- Rewriting firmware manual
- Merging sim + firmware into one document
- License menu changes

## Decisions

### D1: `SIM_MANUAL.md` at repo root

Single source beside `QUICK_DICT.md`. Sections:

1. **Intro** — browser/desktop sim; not Field hardware
2. **Quick start** — Play, pages, knobs, Crunch row
3. **Transport** — Play, Stop, External / Ext. In., global strip
4. **Mod bay** — three internal sources, dropdown assignment, CV scopes
5. **Pages 1–6** — six sim pages (Audio through Delay); tables use sim names from `ParamDisplayNames.hpp`; optional Field OLED symbol in parentheses once per page footnote, not as primary label
6. **Desktop vs web** — short subsections (patch cables, MIDI, record vs mic permission, no MIDI)
7. **Footer** — "Daisy Field hardware: see `MANUAL.md` in the repository"

Content authored from `ParamDisplayNames.hpp` — not duplicated ad hoc in prose.

### D2: Web static path `sim-manual.md`

`HELP_DOC_PATHS.manual.path = "/sim-manual.md"`. Remove `manual.md` from sync output.

### D3: Desktop BinaryData swap

`CMakeLists.txt`: replace `MANUAL.md` with `SIM_MANUAL.md` in `juce_add_binary_data`. `AppMenuBar.cpp`: `BinaryData::SIM_MANUAL_md`.

### D4: Quick Dict intro

Change line 3 from "Field OLED symbols and full signal flow → **Manual**" to "Full sim guide → **Manual** (in-app). Daisy Field hardware → `MANUAL.md` in repository."

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `SIM_MANUAL` drifts from `ParamDisplayNames` | Author tables from header; grep check in tasks |
| Old `web/public/manual.md` cached | Remove from sync; delete on build |
| Users want firmware manual in sim | README links both files; sim manual footer points to repo |

## Migration Plan

1. Author `SIM_MANUAL.md`
2. Update sync, web paths, desktop embed
3. Update Quick Dict intro
4. `npm run sync:docs` + desktop rebuild
5. Verify Help → Manual shows Comb offset, not Pure delay

## Open Questions

None.
