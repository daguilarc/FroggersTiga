## ADDED Requirements

### Requirement: Quick Dict desktop v2 boot outcome

`QUICK_DICT.md` SHALL state that a healthy desktop v2 standalone launch keeps the main window open (instant exit indicates a fault). Engine and Stop transport controls are already documented in §Transport; this requirement adds the boot-outcome gloss only.

#### Scenario: Boot outcome documented

- **WHEN** an operator reads `QUICK_DICT.md` for desktop v2 standalone behavior
- **THEN** they find that the window remaining open after launch is the expected healthy boot outcome

### Requirement: Quick Dict desktop v2 carousel page navigation

`QUICK_DICT.md` SHALL document carousel left/right arrow buttons for module page changes. Rand / Rand Mod on the carousel header are already summarized in §Transport; this requirement adds explicit navigation controls only.

#### Scenario: Carousel navigation documented

- **WHEN** an operator reads the desktop v2 Quick Dict section
- **THEN** they find that left/right arrows on the carousel header change the active module page

### Requirement: Help doc mirrors stay synchronized

After Quick Dict edits, `scripts/sync-help-docs.sh` SHALL be run so `docs/`, `web/public/`, and embedded binary copies match.

#### Scenario: Sync script run after doc change

- **WHEN** `QUICK_DICT.md` is updated for this change
- **THEN** mirrored paths are updated via `scripts/sync-help-docs.sh` before merge

## Notes (OMNI audit — no new requirements)

The following are **already satisfied** in `QUICK_DICT.md` as of 2026-07-01 and are out of scope for this change delta:

- Engine / Stop (§Transport L17–18)
- Shift gestures (§Shift L51–58)
- Three gate meanings (§Gates L96–100)
- Rand / Rand Mod / Rand All (§Transport L24–27)
