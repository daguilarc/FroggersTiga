# Delta — `frogg3rs-distribution`

The capability already requires a release to state which platforms it covers
and not to present a missing platform as a failure. That was written when
Windows could not build. Once it does, "states its platforms" is satisfied by
a release that ships both, and the standing statement that a Windows build is
in progress becomes the stale half.

## MODIFIED Requirements

### Requirement: Each artifact has its own release, named for what it is
The desktop application and the audio plugin SHALL be released independently,
each on its own tag, so that either can be published without republishing the
other. A release SHALL state which platforms it contains, and SHALL NOT present
the absence of a platform-specific artifact as a failure.

A platform's artifact SHALL be absent from a release only while it genuinely
cannot be produced. When a platform builds, its artifact SHALL ship in that
release and the documentation SHALL stop describing it as pending, so that the
stated coverage and the actual coverage cannot drift apart.

#### Scenario: The plugin is released without the desktop app
- **WHEN** the plugin's tag is pushed
- **THEN** the plugin release is published with its plugin formats
- **AND** the desktop release is untouched

#### Scenario: A release names its platforms
- **WHEN** a release contains artifacts for some platforms and not others
- **THEN** the release states which platforms it covers

#### Scenario: A platform that builds is shipped, not described as pending
- **WHEN** the desktop release is published and the Windows build succeeds
- **THEN** the Windows artifact is attached to that release
- **AND** no operator-facing document still calls the Windows build pending

## ADDED Requirements

### Requirement: The published site carries the application's mark
The published site SHALL show the application's own logo in its header,
resolving from a file the site build stages rather than from an external host.
The site is the first thing a downloader sees, and a title alone does not
identify the application the downloads belong to.

The header is the required position, not an incidental one. The blank-frame
guard measures the header's box and samples only the band beneath it, so that
the header's own colour cannot stand in for a rendering application surface. A
mark placed outside the header enters that sampled band and lets the guard pass
over a blank deployment.

#### Scenario: The header identifies the application
- **WHEN** the published site loads
- **THEN** the application's logo renders inside the site header
- **AND** the image resolves, rather than rendering as a broken reference

#### Scenario: The blank-surface guard still fails on a blank surface
- **WHEN** the application surface renders blank beneath the header
- **THEN** the guard fails, with the logo present in the header
