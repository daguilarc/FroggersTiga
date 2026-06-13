## ADDED Requirements

### Requirement: Desktop download line below sim subtitle

The web sim SHALL display a download line directly below the browser simulator subtitle and above the Play/Stop transport row.

#### Scenario: Download line visible on load

- **WHEN** a user opens the web sim page
- **THEN** they see text equivalent to "Download the desktop app: macOS | Windows"
- **AND** **macOS** links to the GitHub Release macOS DMG asset
- **AND** **Windows** links to the GitHub Release Windows Setup asset

### Requirement: Copyright and license line

The web sim SHALL display copyright and license attribution on the line below the download line.

#### Scenario: Legal line content

- **WHEN** a user opens the web sim page
- **THEN** they see "© 2026 JoYo Fresh and Diego Aguilar-Canabal | MIT License"
- **AND** **MIT License** links to the repository LICENSE file on GitHub

### Requirement: Column layout preserved

The download and legal lines SHALL stay within the existing centered `#app` column without altering knob or mod-bay layout below the transport row.

#### Scenario: Desktop browser width

- **WHEN** the viewport is at least 960px wide
- **THEN** the meta lines align with the sim header and transport controls in the same centered column

#### Scenario: Mobile width

- **WHEN** the viewport is narrow (e.g. 375px)
- **THEN** the meta lines wrap within `#app` padding without horizontal overflow
