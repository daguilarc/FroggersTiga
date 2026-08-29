# Delta — `frogg3rs-distribution`

## MODIFIED Requirements

### Requirement: The plugin ships for every platform the application ships for

The VST3 plugin SHALL be released for both macOS and Windows. The Audio Unit
SHALL be released for macOS only, because the format exists only there.

A plugin release SHALL carry operator documentation inside every format it
ships, on every platform. Documentation shipping with the plugin is not a
macOS-specific requirement and SHALL NOT be dropped where the bundle layout
differs.

Windows artifacts SHALL ship unsigned while no Authenticode certificate exists,
and the release body SHALL say so rather than leaving a first-load warning
unexplained.

#### Scenario: The plugin release carries both platforms
- **WHEN** a plugin release is published
- **THEN** it carries a macOS VST3, a macOS Audio Unit, and a Windows VST3

#### Scenario: The Audio Unit is not attempted off macOS
- **WHEN** the plugin is built on Windows
- **THEN** only the VST3 format is requested and the build does not fail on a
  format the platform cannot produce

#### Scenario: Documentation ships on every platform
- **WHEN** any released plugin artifact is opened
- **THEN** the manual and quick dictionary are inside it

#### Scenario: Release notes match what shipped
- **WHEN** a plugin release body is generated
- **THEN** it does not state that the release carries no Windows VST3

## ADDED Requirements

### Requirement: A version that appears in more than one place has one definition

The browser ABI version SHALL have a single definition. Every other site that
names it SHALL read that definition, or SHALL be generated from it, or SHALL be
asserted equal to it by a check that fails when they diverge.

Fixtures and test doubles are sites. A synthesized fixture that hard-codes the
version is the same defect as a hand-maintained mirror, and is harder to notice
because it is not near the definition.

#### Scenario: Moving the version moves every mirror
- **WHEN** the single definition is changed
- **THEN** every site that names the version reflects the change, or a check
  fails naming the site that did not

#### Scenario: A fixture cannot silently disagree
- **WHEN** a fixture declares an ABI version that differs from the definition
- **THEN** a check fails rather than a test asserting against the stale value

### Requirement: A test run does not trust a server it cannot date

Where a test configuration reuses an already-running server rather than
starting its own, the run SHALL establish that the server's own code matches the
tree before trusting its responses, or SHALL refuse to reuse it.

A server that serves files from disk while answering from in-process handlers
SHALL NOT be treated as current on the strength of the files alone.

#### Scenario: A server older than the code is not silently reused
- **WHEN** a run finds an existing server whose code predates the working tree
- **THEN** the run restarts it or fails loudly, rather than testing against
  responses the tree no longer produces
