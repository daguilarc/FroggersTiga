# froggers-web-host Specification

## Purpose
The same Sheaf app browser build replaces the public Froggers website; the legacy web and wasm trees go dormant (unmodified, no longer built or deployed); publication is gated on the repository rename, which does not affect operator-visible product naming.

## Requirements
### Requirement: The new app is the public web build
The published Froggers website SHALL be served by the new Sheaf app's browser build. The previously deployed web application SHALL no longer be built or deployed. The same app type SHALL serve the desktop host, the Sheaf launcher package, and the public site, with no host-specific branching in the app core.

#### Scenario: The site serves the new app
- **WHEN** a visitor loads the published site
- **THEN** the new Sheaf app loads and audio runs
- **THEN** no artifact of the previous web application is served

#### Scenario: One app core, three surfaces
- **WHEN** the desktop build, the Sheaf catalog package, and the public site are compared
- **THEN** all three are produced from the same app type
- **THEN** the app core contains no branch selecting between them

### Requirement: The legacy web and wasm trees are superseded, not edited
This change SHALL stop building and deploying the legacy web and wasm trees, and SHALL NOT modify them. Their deletion, and the removal of the shared-engine flags that exist to serve them, belong to a separate retirement change.

#### Scenario: Legacy trees go dormant, not deleted
- **WHEN** this change is applied
- **THEN** no file under the legacy web or wasm trees differs from its baseline
- **THEN** the deployment pipeline no longer builds or publishes them

#### Scenario: The shared engine is untouched
- **WHEN** this change is applied
- **THEN** the shared engine's host-kind flags remain in place and unmodified
- **THEN** the Daisy firmware's raw binary image artifact is unchanged, verified against its recorded baseline
- **THEN** build artifacts that embed the build path are excluded from that comparison

### Requirement: Publication is gated on the repository rename
No public build SHALL be committed, pushed, or released under the old repository name. The repository SHALL be renamed first, so that the site URL, the published catalog URL, and the package artifact URLs are all minted under the new name and never have to be reissued.

#### Scenario: Rename precedes publication
- **WHEN** the first public build is prepared
- **THEN** the repository has already been renamed
- **THEN** the site base path, catalog URL, and artifact URLs all reflect the new name

#### Scenario: No stale-origin URL is handed out
- **WHEN** the catalog URL is given to the Sheaf launcher operator
- **THEN** that URL is on the renamed origin
- **THEN** it does not require a later redirect or reissue

### Requirement: Product naming survives the rename
The rename SHALL affect repository, origin, and URL identifiers only. Operator-visible product naming in the application's own surface SHALL be unaffected.

#### Scenario: Displayed name is unchanged
- **WHEN** the app renders its header
- **THEN** the displayed product name is unchanged by the rename
