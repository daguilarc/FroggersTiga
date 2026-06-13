## Why

Desktop chrome was built in layers (`sim-hosts-multi-ui` → `desktop-sim-ux-polish` → `desktop-compact-layout` → `desktop-audio-export`) with **no single layout policy**. The result: a 1680px-wide non-resizable window; mod-rack scopes that stretch to ~400px each; global-strip buttons sized with `getStringWidth + 4` so labels clip despite free space; a **68px dead band** in the header because RECORD+formats occupy a full-height right column while transport uses only the top 32px; Marbles CV traces that look dead when holding; and a RECORD cluster where **OGG** renders smaller than WAV/MP3/FLAC due to integer-division layout remainder.

## What Changes

- **Window** — `setResizable(true)`; default **1440×720** (supersedes `desktop-compact-layout` 1680 with explicit Audio-panel verification); `setResizeLimits(1024, 600, …)`.
- **Two-row header** (absorbs `desktop-audio-export` §5.1) — row 1: transport + **RECORD**; row 2: mod rack (left) + format toggles (right). Eliminates 68px dead zone.
- **Unified chrome layout** — one `DesktopChromeLayout.hpp` policy:
  - Mod rack: **fixed-width** boxes (~96px), **16px gaps**, **centered**; scopes do not stretch with window width.
  - Global strip: `TextButton::getBestWidthForHeight()`; labels **Rand All**, **Rand Mods**, **Rand waves**, **Marbles**; button group **centered**.
- **CV scope visibility** — Marbles step-and-hold: level fill + step oversampling; idle draws last CV level (not fixed 0.5 midline).
- **Input envelope box** — tooltip clarifies passive **level meter** (Play + External).
- **Record export cluster** — `kClusterWidth=120`; equal-height format rows; checkbox tick chrome (§5.3); full absorption of `desktop-audio-export` §5.1–5.3.

## Capabilities

### New Capabilities

- `desktop-window-resize`: Resizable window with explicit limits and 1440 default.
- `desktop-chrome-layout`: Shared constants; two-row header; mod-rack + global-strip layout.
- `desktop-cv-scope-visibility`: Marbles/MIDI scope readability for held CV and steps.
- `desktop-record-cluster-ui`: RECORD + format toggle layout, parity, and checkbox chrome.

### Modified Capabilities

- `desktop-mod-rack-scope` (delta over `desktop-sim-ux-polish`): Capped box width, centered rack, held-CV visibility, ≥16px gaps.
- `desktop-audio-export`: §5 transport layout polish fully folded into this change.
- `desktop-compact-layout` (footnote only): Global strip labels superseded to title-case **Rand All** / **Rand Mods**.

## Impact

- `desktop/Source/Main.cpp` — `setResizable`, resize limits, default 1440
- `desktop/Source/MainComponent.cpp` — two-row header reflow, shared layout constants
- `desktop/Source/DesktopChromeLayout.hpp` — new shared constants
- `desktop/Source/ModRackPanel.cpp`, `ModModuleBox.cpp`, `CvScopeDisplay.cpp`
- `desktop/Source/GlobalStrip.cpp` — label text + `getBestWidthForHeight` + center group
- `desktop/Source/RecordExportCluster.cpp`, `InputEnvelopeIndicator.cpp`
- `openspec/changes/desktop-compact-layout/design.md` — footnote 1440 default + strip label supersession
- `openspec/changes/desktop-audio-export/tasks.md` — mark §5.1–5.3 absorbed on apply
