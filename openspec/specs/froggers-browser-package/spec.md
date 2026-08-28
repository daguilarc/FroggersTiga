# froggers-browser-package Specification

## Purpose
Browser build, package assembly, catalog document, and HTTPS/CORS hosting so the Froggers app can be listed in the Sheaf registry and loaded cross-origin by a Sheaf launcher.
## Requirements
### Requirement: Browser build satisfying the Sheaf app ABI
The repository SHALL produce a browser build of the Froggers app that links Sheaf's browser runtime adapter and exports the browser ABI symbol surface Sheaf's launcher expects, built with the same toolchain settings Sheaf uses for its own browser apps.

#### Scenario: Entry module exposes the expected surface
- **WHEN** the browser build is produced
- **THEN** the emitted entry module exports the symbols the Sheaf launcher requires
- **THEN** the module loads in the launcher without ABI errors

### Requirement: Immutable, content-addressed package
The build SHALL be assembled into a package providing every artifact role the Sheaf package contract requires, with a build identifier derived from the content of those artifacts, and with each declared file carrying its media type, byte size, and content hash.

#### Scenario: Build identifier is content-derived
- **WHEN** the package is assembled twice from identical artifacts
- **THEN** the resulting build identifier is identical

#### Scenario: Changed artifacts change the identifier
- **WHEN** any artifact's content changes
- **THEN** the assembled package receives a different build identifier

### Requirement: Schema-conformant catalog document
The repository SHALL publish a catalog document conforming to the Sheaf catalog schema version this launcher accepts, declaring its own publisher identity and the Froggers app entry with its package metadata. The app identifier SHALL be `frogg3rs`, and the publisher identity SHALL be this project's own — not Sheaf's.

#### Scenario: App identifier is frogg3rs
- **WHEN** the published catalog is read
- **THEN** the app entry's identifier is `frogg3rs`
- **THEN** that identifier satisfies the catalog's identifier pattern

#### Scenario: Catalog validates
- **WHEN** the published catalog is checked with Sheaf's catalog validator
- **THEN** validation passes with no schema errors

#### Scenario: Publisher namespacing avoids collisions
- **WHEN** the catalog is merged with other catalogs by the launcher
- **THEN** the Froggers app is identified by publisher and app identifier together
- **THEN** it does not collide with a same-named app from another publisher

### Requirement: Permanent publisher identity
The publisher identity SHALL be treated as permanent once established, and an app's identity SHALL be the pair of publisher identity and app identifier, never the app identifier alone.

#### Scenario: Saved patches are addressed by the publisher-and-app pair
- **WHEN** a user saves a patch for the published app
- **THEN** the saved patch is addressed by the combination of this project's publisher identity and the app identifier
- **THEN** the publisher identity is never changed after publication, since doing so would address every existing saved patch incorrectly

### Requirement: Public hosting suitable for cross-origin loading
The catalog and its packages SHALL be served over HTTPS from a stable origin, with cross-origin requests permitted from any origin, script artifacts served with a JavaScript media type, and WebAssembly artifacts served with a WebAssembly media type.

#### Scenario: Launcher loads the app cross-origin
- **WHEN** a Sheaf launcher on a different origin loads the published catalog
- **THEN** the package's declared files are fetched successfully across origins with no cross-origin access failure
- **THEN** each file's served media type matches its declared kind (script or WebAssembly)
- **THEN** their media types, sizes, and content hashes verify before execution

#### Scenario: Publication is atomic
- **WHEN** a new build is published
- **THEN** clients never observe a catalog referencing partially uploaded artifacts

#### Scenario: Deployed catalog passes the framework's own deployment validator
- **WHEN** the live deployment is checked with the framework's deployment validator against its public catalog URL
- **THEN** validation passes: cross-origin access, media types, decoded size, and content hash all verify for every declared file

### Requirement: Listing executes third-party code in the launcher's origin; hash verification is integrity, not isolation
Listing this project's catalog SHALL cause its published build to execute within the Sheaf launcher's own origin when a user loads it. Content-hash verification SHALL establish that a fetched artifact matches what the catalog declares; it SHALL NOT be treated as a security sandbox against the artifact's own behavior.

#### Scenario: A tampered artifact is rejected before execution
- **WHEN** a published artifact's content hash does not match its catalog entry
- **THEN** the launcher rejects it before any code from that artifact executes

### Requirement: Registry listing requires no Sheaf source change
Listing SHALL be achievable by Sheaf adding this repository's catalog URL to its trusted catalog sources. This repository SHALL NOT require a first-party build slot, entry source file, or build target inside the Sheaf repository.

#### Scenario: Listing is a trust decision only
- **WHEN** Sheaf adds the published catalog URL to its catalog sources
- **THEN** the Froggers app appears in the launcher
- **THEN** no Sheaf source file was modified to build or package it

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

A boot that omits the context SHALL be treated as a broken build rather than as
a build without microphone support. Without it the audio bridge rejects every
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


#### Scenario: Supplying the context does not start audio

- **WHEN** the site boots and the operator does nothing
- **THEN** no audio is running and no capture has been requested
- **AND** activation still happens on the first in-app action, as before

### Requirement: A first visit does not report a failure the reload will fix

A first visit SHALL reach a working instrument on any browser capable of
running it. Where the site's isolation depends on a service worker that has not
yet taken control, the boot path SHALL wait for that attempt to settle, and
SHALL NOT report a startup failure while an isolation attempt is still owed to
the page.

An attempt is owed from the moment the shim decides to make one until that
attempt has settled — either by reloading, or by establishing that no reload is
coming. It is NOT owed once the shim's one-shot guard has already fired: the
page has had its reload, and any failure after that is real and SHALL surface
exactly as it does today.

The state that says whether an attempt is owed SHALL be published by the shim,
which owns the reload guard, and read by the boot path. The boot path SHALL NOT
re-derive the guard's storage key.

#### Scenario: A first visit under load
- **WHEN** the site is served without isolation headers and the service worker
  has not yet taken control
- **THEN** no boot-failure panel is painted before the shim's reload

#### Scenario: A failure after the reload still reports
- **WHEN** the shim's one-shot guard has already fired and isolation still does
  not hold
- **THEN** the boot-failure panel appears with the underlying error

### Requirement: Suppression is bounded by an attempt that can settle

Suppressing the failure report SHALL be conditional on an attempt that can
finish. Where the shim establishes that no reload will happen — its registration
fails, the page is already controlled, or its one-shot guard declines a second
attempt — it SHALL say so, and the boot path SHALL resume reporting.

A page that suppresses its own failure report indefinitely is worse than the
failure it hides: the operator is left with a blank frame and no reason for it,
which is the exact condition the boot-failure panel was added to end.

#### Scenario: The service worker cannot register
- **WHEN** service-worker registration fails outright
- **THEN** the boot-failure panel appears rather than the page waiting silently

### Requirement: Every path that paints the failure honours the same condition

Every code path able to paint the boot-failure panel SHALL honour the same
suppression condition.

The site paints that panel from more than one place: the boot module's own
handler, and an inline handler in the page shell that exists to catch a failure
of the boot module's own imports and therefore cannot import anything. A suppression applied to one of them
leaves the other painting the panel the suppression exists to prevent.

#### Scenario: The inline handler respects the pending attempt
- **WHEN** an isolation attempt is still owed and an error reaches the page
  shell's own handler
- **THEN** that handler paints nothing either

