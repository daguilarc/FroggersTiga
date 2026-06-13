## Context

**Desktop today:** `FroggersTigaApplication` uses `DocumentWindow` with native title bar. No `MenuBarModel`. macOS shows **FroggersTiga** in the menu bar with only default Quit/About items (if any).

**Web today:** `web/index.html` header is static — logo + `<h1>FroggersTiga</h1>` + subtitle. No click handler.

**Docs on disk:**

| File | Purpose |
|------|---------|
| `MANUAL.md` | Full Field + signal-flow manual (~300 lines) |
| `LICENSE` | MIT license text |
| `QUICK_DICT.md` | **Does not exist yet** — this change creates it |

Sim hosts use different labels in places (VCO1/2/3, VCO level, Delay DTIM…). Quick Dict SHALL include a **Sim hosts** section so desktop/web users are not sent to Field-only `V1VO` tables without context.

## Goals / Non-Goals

**Goals:**

- One click from **FroggersTiga** → Manual | Quick Dict | License on desktop and web.
- Offline desktop: docs embedded in binary, no network.
- Readable monospace-friendly viewer (wrapped preformatted text or lightweight markdown render).
- Quick Dict fits on one scroll screen per page group.

**Non-Goals:**

- Full markdown renderer (tables, links) v1 — plain text with preserved headings is enough.
- Editing docs in-app.
- Firmware OLED help.
- i18n.

## Decisions

### 1. Desktop — macOS application menu via `MenuBarModel`

```text
FroggersTiga (app menu)
  Manual…
  Quick Dict…
  License…
  ─────────
  (standard Quit — JUCE provides)
```

**Implementation:** Subclass `juce::MenuBarModel` in `Main.cpp` (or `AppMenuBar.h`). `getMenuBarNames()` returns `{ "FroggersTiga" }` on macOS; on Windows/Linux add a **Help** menu with the same three items (no system app menu).

`menuItemSelected`:

| ID | Action |
|----|--------|
| Manual | `HelpDocsDialog::show("Manual", BinaryData::MANUAL_md, …)` |
| Quick Dict | `HelpDocsDialog::show("Quick Dict", BinaryData::QUICK_DICT_md, …)` |
| License | `HelpDocsDialog::show("License", BinaryData::LICENSE, …)` |

Register with `juce::Desktop::getInstance().setMenuBar(model)` in `initialise()`; clear in `shutdown()`.

**Alternative rejected:** `AlertWindow` with truncated text — manual is too long.

### 2. `HelpDocsDialog` — one component, three titles

```text
HelpDocsDialog
├── title label
├── TextEditor (read-only, multiline, scroll)
└── Close button
```

Load `String` from `BinaryData` at show time — no file I/O at runtime. **OMNI:** one viewer, three static strings; no per-doc viewer classes.

### 3. `QUICK_DICT.md` structure

Single markdown file, sections:

1. **Pages** — Audio, Marbles, Reverb, Filter, Drive, Delay (sim): knob ID → one-line role.
2. **Mod sources (sim)** — MIDI, VCO level, Marbles 1/2.
3. **Transport (sim)** — Play, Randomize all/mod, Marbles, XCPL, Randomize waves.
4. **Field-only** — one line pointing to Manual for M1–M7, SW1/SW2, pickup badges.

Source of truth for knob tables: extract from `MANUAL.md` + `web-sim-page-ux` Delay hints + desktop display aliases. Maintain manually when params change (no codegen v1).

### 4. Web — header dropdown (separate pages constraint preserved)

```text
.app-header-title[role=button]
  click → toggle .app-help-menu
    Manual    → open modal #help-modal with fetched text
    Quick Dict
    License
```

- Wrap logo + `h1` in `<button type="button" class="app-header-trigger">` for a11y (`aria-haspopup="menu"`).
- Dropdown positioned under header; dismiss on outside click / Escape.
- Fetch `public/manual.md`, `public/quick-dict.md`, `public/license.md` once, cache in `Map` — **OMNI:** one fetch helper, three paths.
- GitHub Pages: copy root `MANUAL.md` → `web/public/manual.md` at build (npm script); keep in sync with `QUICK_DICT.md` + `LICENSE`.

**Mobile:** dropdown full-width below header; touch targets ≥44 px per row.

**Alternative rejected:** Navigate away to raw GitHub markdown — breaks offline/Pages and loses sim context.

### 5. Build / embed pipeline

| Host | Doc delivery |
|------|----------------|
| Desktop | `juce_add_binary_data` lists `../../MANUAL.md`, `../../QUICK_DICT.md`, `../../LICENSE` |
| Web | `npm run build` copies same three files to `public/` and `docs/` for Pages |

Add `scripts/sync-help-docs.sh` (or npm script) so one command refreshes web copies from repo root.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Manual drifts from sim UI | Quick Dict has sim section; Manual link notes sim-only pages |
| Large dialog on small screens | Scroll + resizable dialog desktop; web modal max-height 80vh |
| Windows/Linux no app menu | **Help** menu with identical items |
| Duplicate doc copies in web/public | Build script sync from repo root |

## Migration Plan

1. Author `QUICK_DICT.md`.
2. Desktop: BinaryData + MenuBarModel + HelpDocsDialog.
3. Web: header trigger + dropdown + modal + public assets.
4. README note: edit root docs, run sync script before web release.

## Open Questions

- None blocking. Rich markdown rendering deferred to v2 if needed.
