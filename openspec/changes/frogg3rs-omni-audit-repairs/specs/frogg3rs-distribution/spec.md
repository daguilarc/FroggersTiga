# Delta — `frogg3rs-distribution`

## ADDED Requirements

### Requirement: The repository carries no published artifact the deploy does not produce
The repository SHALL NOT commit a catalog, package, or build output that the publishing workflow does not itself generate and upload, so that the committed tree never describes a build that differs from the one being served.

#### Scenario: The served catalog is the generated one
- **WHEN** the site is deployed
- **THEN** the catalog and package it serves are generated from the current application during that deploy
- **AND** no other catalog or package for the application exists in the committed tree

#### Scenario: A stale committed catalog is a defect
- **WHEN** a committed catalog declares a protocol version older than the one the application is built against
- **THEN** it is removed rather than left beside the generated one
