## Postmortem — why panels look unlabeled

### Symptom (user report)

Screenshot: 72 HP main + 36 HP FX, gray panels, knobs and jacks present, **zero faceplate text**, **no FroggersTiga header or frog logo**. Hover tooltips work; silkscreen and branding do not.

### Spec gap (user intent vs vcv-rack-panel-ux)

| User / web reference | vcv-rack-panel-ux captured | Shipped |
|----------------------|----------------------------|---------|
| Web header: `froggerstiga.png` + “FroggersTiga” h1 | “Tiny corner Comic Sans watermark” only | `<text>` in corner (invisible in Rack) |
| Readable labels on panel shell | Same | `<text>` rows (invisible) |
| Minimal, not hero branding | Correct intent | No logo art at all |

Assets exist: `web/public/froggerstiga.png`, `desktop/Resources/Icon.png` (same 320 KB PNG). **Never copied or traced into `vcv/res/`.**

### Root cause (verified)

| Layer | Expected | Actual |
|-------|----------|--------|
| Rack SVG renderer (nanosvg) | Draws panel art | **Ignores `<text>` and embedded fonts** |
| `generate_panels.py` | Silkscreen labels | Emits `<text font-family="Arial">…</text>` |
| `setPanel(createPanel(...))` | Shows SVG | Shows gray `<rect>` only; text nodes skipped |
| `configParam` / `configInput` | Tooltips | **Only** place user sees names (hover) |

Official source: [VCV Module Panel Guide — SVG limitations](https://vcvrack.com/manual/Panel): *“Text and fonts: All text objects must be converted to paths.”*

### OMNI / process violations that allowed this

| Violation | What happened | Correct rule |
|-----------|---------------|--------------|
| **Verification gap** | Tasks 5.3 “labels visible at 100%” left unchecked; marked other tasks done | No panel task complete without Rack screenshot at 100% |
| **False authority chain** | Design D2: “SVG carries all silkscreen; no Rack label widgets” treated SVG file existence as done | Verify **rendered output** in target host (Rack), not file contents |
| **Duplicate label table** | `generate_panels.py` `PAGES`/`ROWS` parallel `ParamDisplayNames.hpp` | Single generator input from header |
| **Confused “expander column” naming** | `addExpanderColumn` = knob column, not Rack expander | Rename in follow-up (`addKnobColumn`) |
| **Desktop/Rack conflation** | JUCE labels work; assumed SVG `<text>` works in Rack | Host-specific rendering rules |

**Not** an OMNI data-flow violation for widget tooltips — those correctly use `ParamDisplayNames` in `configParam`. The failure is **presentation layer** assuming SVG text = visible silkscreen.

### Fix direction (approved — static path SVG, typical VCV workflow)

**Decision:** Path-based SVG silkscreen via `fontTools` at build time (no live `<text>`, no runtime `ui::Label` for functional labels). Label strings parsed from `ParamDisplayNames.hpp`; anchor positions mirror `FieldParityWidget` + `VcvPanelLayout` constants. Frog logo traced from `web/public/froggerstiga.png` into `vcv/res/frog_logo.svg`.

```
ParamDisplayNames.hpp ──parse──► generate_panels.py
VcvPanelLayout.hpp    ──parse──► layout anchors (same math as FieldParityWidget)
        │
        ▼
fontTools outline → <path> in FroggersTiga*.svg
        │
        ▼
setPanel(createPanel(...)) → visible at 100% zoom
```

CI: `sim/check_vcv_panel_svg.sh` rejects live `<text>` in `vcv/res/FroggersTiga*.svg`.

Reject: live SVG `<text>`; duplicate PAGES/ROWS tables; hand-tuned mm coordinates detached from layout header.

### Branding header (successor agent)

Each module faceplate SHALL have a **top header strip** (left side, clear of screws):

```
┌─[🐸] FroggersTiga ─────────────────────────────┐
│  tiny logo   name (path text, ~3–4 mm cap height) │
│  ... knobs / jacks below ...                      │
└───────────────────────────────────────────────────┘
```

- Source art: `web/public/froggerstiga.png` (or traced simplified SVG paths — nanosvg may not render `<image>`; verify; prefer path trace like Befaco/Fundamental modules)
- Keep it **small** — web uses 48×48 px in a full-page header; on a Eurorack panel use ~6–8 mm logo height max
- Same header on **main** and **FX** modules for product identity
- Do **not** rely on Comic Sans `<text>` alone

Reserve top ~10–12 mm of panel height for header; shift mod-rack / column titles down if needed (`VcvPanelLayout` + bounds script).

- Current `font-size="2.4"` in 128.5 mm viewBox may be too small even as paths — match VCV Fundamental module text density (~3–4 mm cap height for row labels)
- Align text baselines to widget grid from `FieldParityWidget` (mod rack Y=1.5 grid, I/O bottom row, column titles above knob columns)
- PM3 row label must read **Phase mod 3** (row index 6), **Crispy** row 7 — same as desktop/web (`ParamDisplayNames`)

### Topology context (v2.5, do not regress)

- Main 72 HP (mod+I/O left, voicing right), FX 36 HP **to the right** of main
- FX expander: main walks `rightExpander`, FX walks `leftExpander`

### Handoff checklist for `/opsx-apply`

1. Regenerate SVGs with visible path text; install plugin
2. Rack: 100% zoom screenshot — main + FX, light and dark theme
3. Confirm CC/gate/I/O labels not overlapping widgets (bounds script + eyes)
4. Remove or gate `generate_panels.py` duplicate `ROWS` table — read from `ParamDisplayNames.hpp`
5. Mark vcv-rack-panel-ux task 5.3 done only after visual pass
6. Bump plugin to 2.5.1 or 2.6.0 after fix

### Out of scope

- VST/desktop/web (already have visible labels)
- Publishing `vcv/` to GitHub main
