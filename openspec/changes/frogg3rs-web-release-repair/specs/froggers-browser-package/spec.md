# Delta — `froggers-browser-package`

The site's boot path is a hand-rolled equivalent of Sheaf's
`launchCatalogApplication` with the app picker removed. Removing the picker also
removed the activation lease, and the lease is what carries the audio context and
MIDI access. Nothing said the boot path owed those, so nothing caught it.

## ADDED Requirements

### Requirement: The browser boot supplies the runtime's activation resources

The site's boot path SHALL supply an activation lease to the Sheaf launcher, so
that the runtime receives the launch-owned `AudioContext` and MIDI access it can
obtain no other way.

A boot that omits the lease SHALL be treated as a broken build rather than as a
build without microphone support. Without it the audio bridge rejects every
capture request before reaching the browser's permission prompt, so the operator
is offered no input device, no permission dialog, and no way to change either —
the failure presents as an empty dropdown rather than as a missing capability.

Where the boot path reimplements part of the launcher, it SHALL be the launcher's
behaviour that defines what is owed, not the subset the reimplementation happens
to pass today.

#### Scenario: The audio context reaches the audio bridge

- **WHEN** the site boots
- **THEN** the runtime's audio options carry a launch-owned `AudioContext`
- **AND** the input status never reports that the microphone requires one

#### Scenario: An input device can be chosen

- **WHEN** the operator opens the Audio I/O page after granting microphone
  permission
- **THEN** the Input device control offers at least one device besides No Input

#### Scenario: MIDI access is carried by the same lease

- **WHEN** the site boots
- **THEN** the runtime receives the MIDI access the lease provides, on the same
  path as the audio context
