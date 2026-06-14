## ADDED Requirements

### Requirement: VCV process buffer reuse

VCV `FroggersTigaModule::process` SHALL reuse member audio buffers sized to the rack block, not allocate `std::vector` on every callback.

#### Scenario: No per-block heap alloc

- **WHEN** Rack calls `process` for N frames
- **THEN** extIn/mono samples are written to pre-sized member buffers

### Requirement: OpenSpec context accuracy

`vcv-vst-field-parity-panel/design.md` Context diagram SHALL reflect implemented Phase A items (PagedHostIO page APIs, DelayState on VCV stub, VST target exists) and mark remaining gaps (widget, expander stack).

#### Scenario: Design matches repo

- **WHEN** a developer reads vcv-vst design Context section
- **THEN** it does not claim PagedHostIO lacks page-indexed APIs or that VST does not exist
