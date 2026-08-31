# Delta — `frogg3rs-distribution`

## ADDED Requirements

### Requirement: Docs-only pushes do not rebuild the deliverables

A docs-only push to `main` SHALL NOT start the site or VST builds, where
docs-only means every touched file is the repository README or an openspec
artifact. A push touching anything else SHALL start them as before. Tag
pushes and manual
dispatches are exempt from path filtering, so cutting a release is never
blocked by what a push contains.

#### Scenario: A docs-only push is quiet

- **WHEN** a push to `main` changes only files under `openspec/` or the
  repository `README.md`
- **THEN** no Pages run and no VST run starts for that push

#### Scenario: A code push still builds

- **WHEN** a push to `main` changes any other file
- **THEN** the Pages and VST builds run as they always have

#### Scenario: Releases are unaffected

- **WHEN** the `frogg3rs_vst` tag is pushed or a workflow is dispatched
  manually
- **THEN** the corresponding workflow runs regardless of which files recent
  pushes touched
