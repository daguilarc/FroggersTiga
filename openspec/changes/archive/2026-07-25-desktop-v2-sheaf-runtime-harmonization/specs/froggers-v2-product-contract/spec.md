## ADDED Requirements

### Requirement: Product contract matches Runtime Application surface
Froggers v2 product contract SHALL describe desktop as Runtime-hosted Application surface with dual scopes, 2-deep modulation, no Random S&H module page, unified module sections including ASR Envelope and no cross-coupler controls (VCO coupling is drilldown-matrix only, D11), and Froggers rand toggle/hold semantics.

#### Scenario: Contract omits Random module page
- **WHEN** product contract text lists modules
- **THEN** Random S&H appears only as modulation sources
- **THEN** Envelope is ASR after Audio
