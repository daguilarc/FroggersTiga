## ADDED Requirements

### Requirement: Host maintenance surfaces are reproducible

Cross-host contracts referenced by the host master specification SHALL state whether they are public repo source or local-only planning/development state. Generated and local-only host surfaces SHALL have a declared source authority, freshness gate, or local-only policy before they are cited as verification evidence.

#### Scenario: Referenced OpenSpec artifacts are local-only

- **WHEN** a host requirement cites OpenSpec artifacts as source truth
- **THEN** those artifacts are treated as local-only planning state unless separately published
- **THEN** public CI does not claim to validate missing local-only planning truth

#### Scenario: Generated host mirrors do not become authorities

- **WHEN** web/public, docs, or generated host display files are used by a host
- **THEN** the host master identifies the root/source generator as authority and the generated file as a projection or publication output

#### Scenario: Host differences remain allowed

- **WHEN** one host exposes a UI feature that another host does not expose
- **THEN** this is compliant if the difference is expressed through an explicit host projection and does not create a second authority for the shared concept

### Requirement: Public operator docs are launch-gated

The host master SHALL distinguish internal host implementation contracts from public operator documentation. VST/AU and VCV Rack MAY remain in internal host specs and test plans before launch, but `SIM_MANUAL.md` and its published mirrors SHALL describe only launched desktop standalone and web sim surfaces until a later launch/documentation change adds those hosts back.

#### Scenario: Internal host specs retain pre-launch details

- **WHEN** host master or related internal specs describe VST/AU or VCV behavior
- **THEN** those requirements remain valid as implementation/testing contracts
- **THEN** they do not imply public SIM manual availability before launch

#### Scenario: Public SIM manual is desktop and web only

- **WHEN** public sim operator documentation is generated or checked
- **THEN** it omits VST/AU and VCV user-facing instructions until those hosts are launched
