## ADDED Requirements

### Requirement: Desktop window is user-resizable

The desktop `DocumentWindow` SHALL call `setResizable(true, true)` and `setResizeLimits` with minimum width **1024**, minimum height **600**, and generous maximum dimensions.

#### Scenario: User shrinks window

- **WHEN** the user drags the window edge narrower than the default width
- **THEN** the window width decreases below 1680 px without snapping back

#### Scenario: Minimum width enforced

- **WHEN** the user attempts to shrink below 1024 px wide
- **THEN** the window stops at 1024 px

### Requirement: Default window size 1440 by 720

The application SHALL open at **1440×720** by default. This supersedes 1680×720 from `desktop-compact-layout`. Audio panel content SHALL remain usable at this size (verified before release).

#### Scenario: First launch size

- **WHEN** the user launches the desktop app
- **THEN** the content area is 1440×720 before user resize

#### Scenario: Audio panel at default

- **WHEN** the user views the Audio panel at 1440×720
- **THEN** wave control rows are not clipped horizontally
