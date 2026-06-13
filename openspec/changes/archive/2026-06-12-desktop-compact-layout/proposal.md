## Why

The desktop sim defaults to **2016×720** — wider than most single monitors at 100% scale — while each panel column is only as wide as `windowWidth / 6`. Per-panel buttons still say **Randomize mod** (never shortened to **Randmod** as specified) and split the button row **50/50**, so **Randomize** gets half a column of empty padding. That wasted chrome was baked into the six-column layout math from `stereo-delay-page` / `desktop-sim-ux-polish` (“keep 2016 width”). `QUICK_DICT.md` uses verbose tables with Role columns; users want **`PRMT : Parameter Name`** only — depth stays in **Manual**.

## What Changes

- **Default window** — **1680×720** (fits 1728-wide and 1920×1080 laptops with margin). Supersedes 2016×720 default in prior change specs.
- **Per-panel buttons** — `Randomize mod` → **`Randmod`**; layout buttons at **intrinsic text width** (no 50/50 split); minimal horizontal padding on **Randomize**.
- **Global strip** — shorten labels to match density: **Rand all**, **Randmod all**, **Rand waves**, **Marbles** (unchanged).
- **Quick Dict rewrite** — flat `PRMT : Parameter Name` lines grouped by page; **PRMT** matches sim UI labels (`VCO1`/`VCO2`/`VCO3` on Audio, not firmware `V1VO`); one pointer to **Manual** for depth. Sync `web/public/quick-dict.md` + BinaryData embed.
- **Supersedes** `desktop-sim-ux-polish` decision “Keep 2016 width”.

## Capabilities

### New Capabilities

- `desktop-compact-layout`: Default window size, compact panel/global randomize button labels and intrinsic-width layout.
- `quick-dict-format`: Quick Dict document format `PRMT : Name` with Manual deferral.

### Modified Capabilities

- `quick-dict-doc` (from `app-header-help-menu`): Replace table/Role format with abbreviation lines.

## Impact

- `desktop/Source/Main.cpp`, `MainComponent.cpp` — default size 1680×720
- `desktop/Source/SubModulePanel.h`, `SubModulePanel.cpp` — Randmod label, intrinsic button bounds
- `desktop/Source/GlobalStrip.h`, `GlobalStrip.cpp` — shortened strip labels, intrinsic/fixed column widths
- `QUICK_DICT.md`, `web/public/quick-dict.md` — format rewrite
- `openspec/changes/desktop-sim-ux-polish/`, `desktop-host-corrections/desktop-wave-controls`, `app-header-help-menu/quick-dict-doc` — footnotes on apply (task 1.2)
- No DSP, firmware, or WASM page-count changes
