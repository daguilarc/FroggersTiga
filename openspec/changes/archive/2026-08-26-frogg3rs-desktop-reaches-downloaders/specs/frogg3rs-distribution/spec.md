# Delta — `frogg3rs-distribution`

A download that cannot be opened is not a release. The capability said what each
artifact is and which platforms it covers; it said nothing about the artifact
being openable by the person who downloads it.

## ADDED Requirements

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
