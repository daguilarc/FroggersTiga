# frogg3rs-distribution Specification

## Purpose
What the published site, the desktop download and the plugin download each are.
## Requirements
### Requirement: Every host ships from one app and one shell
The standalone, the plugin and the browser build SHALL be produced from the same
application sources and SHALL present the same runtime shell. A host SHALL NOT
be made to build on a platform by substituting a different application shell for
Sheaf's own, because that ships an application that differs from the documented
one in the controls it offers.

#### Scenario: A platform port keeps the shell
- **WHEN** a host is made to build on an additional platform
- **THEN** it presents the same runtime shell, including audio device selection
- **AND** every control documented in the manual is present on both platforms

### Requirement: The published site is built from the current app
The published site SHALL be produced from the current application's browser
build. It SHALL reference its own assets relatively, so that the page survives
being served from any base path, and SHALL NOT carry a repository or product
name that no longer resolves.

#### Scenario: The site survives a repository rename
- **WHEN** the repository or its published base path changes
- **THEN** the site's stylesheet, scripts and images still resolve
- **AND** the page renders styled, with its interactive parts running

#### Scenario: The site offers both downloads
- **WHEN** an operator opens the published site
- **THEN** the desktop application and the audio plugin are both offered
- **AND** each link resolves to that artifact's own release, not to whichever
  release happened to be published most recently

### Requirement: Each artifact has its own release, named for what it is
The desktop application and the audio plugin SHALL be released independently,
each on its own tag, so that either can be published without republishing the
other. A release SHALL state which platforms it contains, and SHALL NOT present
the absence of a platform-specific artifact as a failure.

#### Scenario: The plugin is released without the desktop app
- **WHEN** the plugin's tag is pushed
- **THEN** the plugin release is published with its plugin formats
- **AND** the desktop release is untouched

#### Scenario: A release names its platforms
- **WHEN** a release contains artifacts for some platforms and not others
- **THEN** the release states which platforms it covers

### Requirement: A publishing trigger that cannot fire is a defect
A tag intended to publish SHALL actually publish. A workflow SHALL NOT declare a
trigger whose pattern cannot match the tag it documents, nor a permission set
that forbids the publication it exists to perform.

#### Scenario: The documented tag produces the release
- **WHEN** the tag named in the release process is pushed
- **THEN** the workflow runs and publishes the artifacts for that tag

#### Scenario: A superseded release does not outrank the current one
- **WHEN** a release exists for a product line that no longer ships
- **THEN** it does not present itself as the current download

### Requirement: A downloadable build carries a signature matching its contents
Every bundle a release ships SHALL carry a code signature that covers the bundle
as assembled, including files placed into it after its executable was linked.
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

#### Scenario: Every shipped bundle verifies
- **WHEN** a release artifact is packaged
- **THEN** verifying its signature against its own contents succeeds
- **AND** a build whose signature does not match its contents fails the build
  rather than reaching a release
- **AND** this holds for every bundle the release publishes, not only the
  application

#### Scenario: A downloaded build is assessed, not rejected as damaged
- **WHEN** a packaged build is downloaded and carries the quarantine attribute
- **THEN** the operating system evaluates its signature and reports a verdict
- **AND** it does not report the file as damaged

### Requirement: A release states what opening it requires
A release SHALL state what the person downloading it will see and what they must
do to open it, whenever its artifacts are not signed by an identity the
operating system recognises. That statement SHALL appear both in the operator
manual and in the release itself, since someone who downloads an artifact has
not necessarily read the manual, and SHALL be written once and carried to each
release body from that one source.

#### Scenario: The extra step is documented where it is met
- **WHEN** a release ships artifacts an operating system will not open directly
- **THEN** the release body states what the operator will see and the step that
  opens it
- **AND** the operator manual states the same

#### Scenario: Recognised signing removes the step
- **WHEN** artifacts are signed by an identity the operating system recognises
- **THEN** the download opens without an extra step, and the documented step is
  removed rather than left standing

