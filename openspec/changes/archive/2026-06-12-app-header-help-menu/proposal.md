## Why

`MANUAL.md` and `LICENSE` exist at repo root but sim users have no in-app path to them. The **FroggersTiga** title in the desktop menu bar (macOS) and web header is the natural affordance for help, licensing, and a compact parameter glossary — without hunting the repo or GitHub.

## What Changes

- **Desktop app menu** — Under the **FroggersTiga** menu in the system menu bar: **Manual**, **Quick Dict**, **License**. Each opens a read-only scrollable dialog with bundled markdown text (offline-safe).
- **Web header menu** — Clicking the **FroggersTiga** title (or logo+title cluster) opens a dropdown with the same three entries; content loaded from static assets shipped with the site (`/docs/` or `public/`).
- **`QUICK_DICT.md`** — New repo doc: one-screen abbreviations for all Field + sim-only labels (pages 0–5, mod sources, transport). Derived from `MANUAL.md`; not a second manual.
- **Unchanged** — Firmware, VCV, DSP behavior, page layout.

## Capabilities

### New Capabilities

- `app-help-menu`: Desktop menu bar + web header dropdown exposing Manual, Quick Dict, License.
- `quick-dict-doc`: Canonical abbreviated parameter glossary at repo root.

### Modified Capabilities

- (none)

## Impact

- `desktop/Source/Main.cpp` — `MenuBarModel` or macOS application menu wiring
- `desktop/Source/HelpDocsDialog.*` (new) — shared read-only markdown viewer
- `desktop/CMakeLists.txt` — `BinaryData` embed `MANUAL.md`, `QUICK_DICT.md`, `LICENSE`
- `web/index.html`, `web/src/main.ts`, `web/src/style.css` — header menu UI
- `web/public/` — ship `manual.md`, `quick-dict.md`, `license.md` (or fetch from `/docs/` on Pages)
- `QUICK_DICT.md` (new at repo root)
