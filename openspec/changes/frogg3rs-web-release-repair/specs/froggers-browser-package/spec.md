# Delta — `froggers-browser-package`

The site's boot path is a hand-rolled equivalent of Sheaf's
`launchCatalogApplication` with the app picker removed. Removing the picker also
removed the activation lease, and the lease is what carries the audio context and
MIDI access. Nothing said the boot path owed those, so nothing caught it.

## ADDED Requirements

### Requirement: The browser boot supplies the audio context capture requires

The site's boot path SHALL supply the runtime with an `AudioContext`, so that
microphone capture has a context to attach to.

It SHALL supply that context WITHOUT asserting that audio activation has
already happened. An activation lease is not the means: a lease resumes its
context and requests MIDI when it is acquired, and a launcher that receives one
starts audio and capture immediately, because a lease records a user gesture
that has already occurred. A page with no launch gesture that acquires one
either stalls on a resume the browser will not complete or starts capture no
operator asked for.

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

#### Scenario: Supplying the context does not start audio

- **WHEN** the site boots and the operator does nothing
- **THEN** no audio is running and no capture has been requested
- **AND** activation still happens on the first in-app action, as before
