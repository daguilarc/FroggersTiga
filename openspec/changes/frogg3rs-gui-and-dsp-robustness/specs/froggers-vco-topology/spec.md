# Delta — `froggers-vco-topology`

The existing requirement already states that the panel renders a live waveform trace. The current
build **fails** it: the scope's ring-buffer write cursor is never advanced, so every sample
overwrites slot 0, the reader is permanently empty, and the panel draws only its background.

This delta does not weaken the requirement. It adds the scenario that would have caught the
failure, because "reports connected" was satisfiable without the scope ever containing data.

**Revised 2026-07-28 (audit).** As first written, this delta kept a clause reading "no bespoke
waveform rasterization exists in the app" while the sibling delta
`froggers-app-surface-layout` and its task specified hand-built draw commands for the same band —
two requirements in one change demanding opposite implementations. The conflict is resolved in
favour of **keeping the generic visualizer**: every operator requirement for the scope band (a
post-gate top band, an envelope-follower bottom band, per-trace colours, a stable cycle-aligned
display) is reachable through it, so no app-side rasterization is needed. The clause therefore
stands, and the *tap point* — which sample gets written — becomes the thing this requirement
constrains. Two scenarios below are also corrected: a trace fed from a post-gate signal is flat
during silence, which the old "displays a moving waveform trace" wording would have failed.

## MODIFIED Requirements

### Requirement: Oscillators expose Sheaf scope state
Each VCO SHALL expose the oscillator UI-state shape Sheaf's scope visualizer consumes — connection flag, scope writer reference, scope channel, and scope color — and SHALL accept a scope writer holder and color, publishing its UI state each block. The app SHALL NOT implement its own waveform drawing for VCO scopes.

The app SHALL additionally perform every feed the scope writer requires, not only the sample
write. Storing samples without advancing the write cursor leaves the reader permanently empty,
which renders an empty panel while every connection and publication check still passes. Cycle
markers are such a feed: without them the display is an unaligned window slice per refresh, which
reads as noise even though every sample in it is correct.

The sample a VCO writes to its scope SHALL be the one that reaches the output — taken after
envelope gating, not from the free-running oscillator. Recovering the gated per-voice values
SHALL NOT duplicate the envelope computation at a second call site.

#### Scenario: Scope visualizer binds without app drawing code
- **WHEN** the surface is built with VCO scope panels
- **THEN** each panel is driven by the standard Sheaf scope visualizer over the VCO's UI state
- **THEN** no bespoke waveform rasterization exists in the app

#### Scenario: Scope reports connected after processing
- **WHEN** the app has processed at least one block
- **THEN** each VCO's scope state reports connected
- **THEN** the panel renders a trace of that VCO's contribution to the output

#### Scenario: The scope write cursor advances
- **WHEN** the app has processed successive blocks with the transport running
- **THEN** the published scope index has advanced between those blocks
- **THEN** the scope reader reports a non-empty sample range

#### Scenario: The trace follows the gate
- **WHEN** the envelope gate is closed
- **THEN** the VCO's trace is flat, because the sample written to the scope is the gated one
- **WHEN** the gate opens
- **THEN** the trace shows that VCO's waveform

#### Scenario: The envelope computation is evaluated once
- **WHEN** the gated per-voice values are obtained for the scope
- **THEN** they come from the same evaluation that produces the audio output
- **THEN** no second application of the envelope exists in the audio path

#### Scenario: A live waveform is visible in the running application
- **WHEN** the application is launched and the transport is started
- **THEN** each VCO's trace is visible and stable between refreshes, confirmed by observation
- **THEN** no panel renders only its background fill and midline
