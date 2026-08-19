# Delta — `froggers-web-host`

**Added 2026-08-18 at the operator's instruction** (second omni-rule
audit session): two site-presentation requirements the change had left
implicit, both carrying forward observed behavior of the legacy site.
Everything else in this capability is implemented as written, no delta.

## ADDED Requirements

### Requirement: Mobile viewport stacks around a full-width encoder grid
On mobile-width viewports, THE published site SHALL render the
sixteen-slot (4×4) encoder grid spanning the full viewport width, with
every other control placed above or below the grid, never beside it.
The legacy site's mobile stacking is the reference behavior; on small
screens, full-width placement takes precedence over grid element size.

#### Scenario: Phone-width layout stacks
- **WHEN** the site loads at a phone-width viewport
- **THEN** the encoder grid spans the viewport width
- **THEN** no other control renders beside the grid — everything else
  sits above or below it

### Requirement: Site links carry the legacy roles under new references
THE published site SHALL present the same operator-facing link roles the
legacy site presents (desktop downloads, license, manual), each pointing
at the current product's equivalent: URLs minted under the renamed
origin, release links referencing the release being published, and the
manual link pointing at the manual that documents the published app.

#### Scenario: Link roles preserved, references renewed
- **WHEN** the published site is compared to the legacy site
- **THEN** every legacy link role is present and resolves
- **THEN** no link target carries the old repository name

### Requirement: Playwright layout regression for the published site
THE repository SHALL provide a Playwright suite for the new site — its
own harness, since the legacy `web/` tree and its suite stay dormant and
byte-identical — with mobile-emulated and desktop-emulated tests
asserting the mobile stacking scenario and the link roles without
starting audio, runnable in CI before any deploy.

#### Scenario: CI layout tests gate the deploy
- **WHEN** the site workflow runs
- **THEN** the mobile-emulated stacking assertion and the link-role
  check pass before the deploy step is reachable

### Requirement: Deploys happen only from the default branch
THE site workflow SHALL reach its deploy step only for the default
branch: a manual dispatch from any other ref SHALL build and test
without deploying, so the working branch can be exercised end to end
while the live site stays untouched until merge.

#### Scenario: Branch dispatch is a dry run
- **WHEN** the workflow is dispatched manually from a non-default branch
- **THEN** the site builds and its tests run
- **THEN** the deploy step does not execute and the live site is
  unchanged
