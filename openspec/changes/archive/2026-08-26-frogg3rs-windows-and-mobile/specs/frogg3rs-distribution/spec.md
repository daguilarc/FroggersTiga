# Delta — `frogg3rs-distribution`

The capability already requires a release to state which platforms it covers
and not to present a missing platform as a failure. That was written when
Windows could not build. Once it does, "states its platforms" is satisfied by
a release that ships both, and the standing statement that a Windows build is
in progress becomes the stale half.

This delta applies AFTER `frogg3rs-desktop-reaches-downloaders` is archived
(that change's tasks 0.4 here). Its two ADDED requirements are delivered in
code but have never reached this spec, and the signature requirement below is
one of them — there is nothing to modify until it lands.

## MODIFIED Requirements

### Requirement: A downloadable build carries a signature matching its contents
Every bundle a release ships SHALL carry a code signature that covers the bundle
as assembled, including files placed into it after its executable was linked,
on every platform where this project holds a signing identity.
This applies to application bundles and to plug-in bundles alike, since both are
downloaded by the same person onto the same operating system. Signing SHALL
happen after assembly is complete, and every packaged result SHALL be verified
before it is published.

An operating system that quarantines downloads treats a bundle whose signature
does not match itself as damaged rather than as unsigned, which tells the person
who downloaded it that the file is broken and offers them no way forward. A
build signed only at link time, before its bundle is assembled, is in exactly
that state. So is a build signed partway through assembly, before the last of
its resources is copied in.

Where this project holds no signing identity for a platform, its artifact
SHALL ship ad-hoc signed or unsigned, and the release SHALL state what the
operator sees instead and the exact steps that open it. An unsigned artifact
under an unqualified signing rule is a requirement violated by the first
thing that ships under it; naming the condition is what keeps the rule true.
Holding no identity is a STANDING DECISION here, not a blocked task: neither
an Apple Developer Program membership nor a Windows code-signing certificate
is being purchased. Documentation SHALL describe the resulting step as
permanent rather than as a fix awaiting work, so that nobody reads it as a
defect to be closed.

#### Scenario: Every shipped bundle verifies
- **WHEN** a release artifact is packaged on a platform where this project
  holds a signing identity
- **THEN** verifying its signature against its own contents succeeds
- **AND** a build whose signature does not match its contents fails the build
  rather than reaching a release
- **AND** this holds for every bundle the release publishes, not only the
  application

#### Scenario: A downloaded build is assessed, not rejected as damaged
- **WHEN** a packaged build is downloaded and carries the quarantine attribute
- **THEN** the operating system evaluates its signature and reports a verdict
- **AND** it does not report the file as damaged

#### Scenario: A platform this project holds no identity for ships anyway, and says so
- **WHEN** a release contains an artifact for a platform where this project
  holds no signing certificate
- **THEN** that artifact ships unsigned rather than being withheld
- **AND** the operator manual states what the operating system shows on first
  open and the step that gets past it

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
