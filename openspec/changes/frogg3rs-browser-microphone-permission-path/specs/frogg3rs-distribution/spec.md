# Delta — `frogg3rs-distribution`

## ADDED Requirements

### Requirement: The plugin ships for every platform the application ships for

The VST3 plugin SHALL be released for both macOS and Windows. The Audio Unit
SHALL be released for macOS only, because the format exists only there, and a
build off macOS SHALL NOT request it.

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

### Requirement: A shipped bundle carries the declarations its platform requires of it

A released artifact SHALL carry every platform declaration the operating system
requires for the capabilities that artifact actually uses. An application that
opens a capture device on macOS SHALL declare a microphone usage description in
the bundle that ships.

The declaration SHALL live on the build path that produces the shipped artifact
for that platform. Where a project builds different platforms through different
build systems, a declaration made in one of them SHALL NOT be counted as
satisfying the other. A declaration on a path that does not produce the artifact
is absent from the artifact.

#### Scenario: The shipped macOS application declares its microphone use
- **WHEN** the released macOS application bundle is inspected
- **THEN** it declares a microphone usage description

#### Scenario: A declaration is checked in the artifact, not in the source
- **WHEN** a platform declaration is claimed to be present
- **THEN** the check reads the built artifact for that platform

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

A test run SHALL NOT trust an already-running server whose own code it cannot
date. Where a test configuration reuses such a server rather than starting its
own, the run SHALL establish that the server's code matches the tree before
trusting its responses, or SHALL refuse to reuse it.

A server that serves files from disk while answering from in-process handlers
SHALL NOT be treated as current on the strength of the files alone. The stale
case SHALL be reported loudly enough that it is not mistaken for a defect in the
code under test.

#### Scenario: A server older than the code is not silently reused
- **WHEN** a run finds an existing server whose code predates the working tree
- **THEN** the run restarts it or fails loudly, rather than testing against
  responses the tree no longer produces

#### Scenario: The stale case names itself
- **WHEN** a run refuses or restarts a stale server
- **THEN** the reason given identifies the server as stale rather than reporting
  a failed assertion
