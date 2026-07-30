## ADDED Requirements

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
