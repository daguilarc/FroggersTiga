# App header help menu — tasks

## 1. Quick Dict document

- [x] 1.1 Create `QUICK_DICT.md` at repo root — pages 0–5 knob tables, sim mod sources, transport strip, pointer to Manual for Field-only detail
- [x] 1.2 Cross-check labels against `MANUAL.md`, `web-sim-page-ux` Delay hints, and desktop display aliases (VCO level, VCO1/2/3)

## 2. Doc sync pipeline

- [x] 2.1 Add npm script or shell script to copy `MANUAL.md`, `QUICK_DICT.md`, `LICENSE` → `web/public/` (and `docs/` if needed for Pages)
- [x] 2.2 Wire script into `web/package.json` `build` / `build:all`

## 3. Desktop menu + viewer

- [x] 3.1 Add `juce_add_binary_data` entries for `MANUAL.md`, `QUICK_DICT.md`, `LICENSE` in `desktop/CMakeLists.txt`
- [x] 3.2 Implement `HelpDocsDialog` — read-only `TextEditor`, title, Close
- [x] 3.3 Implement `AppMenuBar` (`MenuBarModel`): **FroggersTiga** menu with Manual / Quick Dict / License; **Help** menu fallback on Windows/Linux
- [x] 3.4 Register menu bar in `FroggersTigaApplication::initialise`; clear in `shutdown`

## 4. Web header menu

- [x] 4.1 Wrap header logo + `h1` in `button.app-header-trigger` with `aria-haspopup="menu"`
- [x] 4.2 Add dropdown markup + CSS (≥44 px rows; dismiss on outside click / Escape)
- [x] 4.3 Add `#help-modal` with scrollable pre/formatted body; single `openHelpDoc(name, url)` fetch+cache helper
- [x] 4.4 Wire Manual / Quick Dict / License to `public/*.md` paths

## 5. Verification

- [ ] 5.1 Desktop: each menu item opens correct bundled text offline
- [ ] 5.2 Web: header menu on 390 px viewport; modal scrolls; sim Play/Stop still works after close
- [x] 5.3 `npm run build` leaves synced copies in `public/`
- [x] 5.4 Desktop Release build
