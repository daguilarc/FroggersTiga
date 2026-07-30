## MODIFIED Requirements

### Requirement: Quick Dict desktop v2 boot outcome

`QUICK_DICT.md` SHALL state that a healthy desktop v2 standalone launch keeps the main window open (instant Dock bounce / exit indicates a fault). On macOS, the gloss SHALL reference launching via `scripts/open-desktop-v2.sh` or the Release path `desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app`. Engine and Stop transport controls remain documented in §Transport.

#### Scenario: Operator reads boot outcome

- **WHEN** an operator opens Quick Dict for desktop v2 startup
- **THEN** they find the expected healthy boot outcome and the canonical macOS launch path or script
