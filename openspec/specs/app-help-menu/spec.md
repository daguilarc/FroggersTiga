# app-help-menu Specification

## Purpose
TBD - created by archiving change app-header-help-menu. Update Purpose after archive.
## Requirements
### Requirement: Desktop About menu exposes help docs

The desktop sim SHALL provide an **About** application menu (macOS menu bar) or **Help** menu (Windows/Linux) with items **Manual**, **Quick Dict**, and **License**. Selecting an item SHALL open a read-only scrollable dialog showing the bundled document text without requiring network access.

#### Scenario: Open Manual on macOS

- **WHEN** the user chooses **Manual** from the **About** menu
- **THEN** a dialog opens titled **Manual** with the full `MANUAL.md` content scrollable inside the app

#### Scenario: Open Quick Dict offline

- **WHEN** the user chooses **Quick Dict** with no network connection
- **THEN** the Quick Dict text loads from embedded binary data and displays in a dialog

#### Scenario: Open License

- **WHEN** the user chooses **License**
- **THEN** the MIT `LICENSE` file text is shown in a read-only dialog

### Requirement: Web header opens About menu

The web sim header SHALL treat the **FroggersTiga** title cluster (logo + heading) as an **About** menu trigger. Activating it SHALL reveal a menu labeled **About** with **Manual**, **Quick Dict**, and **License**. Selecting an entry SHALL open a modal overlay with the document text without navigating away from the sim page.

#### Scenario: Header click shows menu

- **WHEN** the user clicks the FroggersTiga header trigger
- **THEN** an **About** menu with three items appears below the header

#### Scenario: Manual in modal

- **WHEN** the user selects **Manual** from the header menu
- **THEN** a modal displays `manual.md` content fetched from the site static assets
- **AND** the underlying sim page remains loaded (no full page navigation)

#### Scenario: Dismiss menu and modal

- **WHEN** the user presses Escape or clicks outside the open menu or modal
- **THEN** the menu or modal closes and sim controls remain usable

#### Scenario: Mobile touch target

- **WHEN** the viewport width is ≤720 px
- **THEN** each header menu row has a minimum touch height of 44 px

### Requirement: Help docs stay in sync with repo root

Build or sync scripts SHALL copy `MANUAL.md`, `QUICK_DICT.md`, and `LICENSE` from the repository root into desktop `BinaryData` and web static assets so desktop and web show the same canonical text.

#### Scenario: Web build includes docs

- **WHEN** `npm run build` completes for the web sim
- **THEN** `public/manual.md`, `public/quick-dict.md`, and `public/license.md` exist and match repo root sources at build time

