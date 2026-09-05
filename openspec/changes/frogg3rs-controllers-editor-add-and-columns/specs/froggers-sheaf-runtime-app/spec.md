# Delta — `froggers-sheaf-runtime-app`

## ADDED Requirements

### Requirement: The library is tested on the target the product ships to
The library's own test binaries SHALL be built and run for the browser's wasm32 target as part of the test gate, so that behaviour depending on the width of `std::size_t` is exercised at the width the shipping build uses. The gate SHALL fail, rather than skip, when the toolchain that builds for that target is unavailable. Arithmetic guarding an index or a count SHALL be correct on every target the code is built for, independent of word size.

#### Scenario: A width-dependent defect fails the gate
- **WHEN** a size bound or overflow guard behaves differently under a 32-bit `std::size_t` than under a 64-bit one
- **THEN** the test gate fails on the wasm32 target
- **AND** the failure names the function whose behaviour differs
- Check: the wasm32 gate, proven live in both directions — with the pre-fix guard restored it exits non-zero on 23 `blocks_tests` failures, and after the block validator's overflow guard is corrected it runs both binaries to 249 passes and 0 failures. The gate is fail-fast, so the 26 `viewmodel_tests` failures the same guard causes are measured by building that binary for the target directly, not through the gate.

#### Scenario: Adding an encoder mapping succeeds in the browser build
- **WHEN** a controller row created from a preset has Add or Block pressed in the Encoders editor
- **THEN** the mapping or block is added, in the browser build as in every other
- **AND** the same holds on a row with no existing mappings, and in the Analogs and System Messages sections
- Check: `blocks_tests` and `viewmodel_tests` on wasm32, which fail on exactly these paths before the guard is corrected and pass after; and the operator on the deployed build.

### Requirement: A control's label is legible and its neighbours are separated
A text node SHALL be allocated a box wide enough for the text it renders, and adjacent controls within a row SHALL be separated by a non-zero gap, so that no label is clipped by, or visually continuous with, the control beside it. This SHALL be checked by a criterion applied to the page's rendered states, measuring text width finely enough to catch a sub-pixel overrun, rather than by inspection.

#### Scenario: A column header is not clipped by the button beside it
- **WHEN** the Encoders editor's Turn or Push group header is presented
- **THEN** every column label is fully legible
- **AND** a gap separates the last column from the Add button
- Check: the text-fit criterion, extended to the row-expanded/Encoders-open state and to a sub-pixel measurement, proven to fail on the 58px `BlockStartPos` allocation and the zero gap before they are corrected; and the operator on the deployed build.

#### Scenario: A rounding-width overrun is not reported as fitting
- **WHEN** a label's rendered text exceeds its allocated box by less than one pixel
- **THEN** the text-fit criterion reports a violation
- Check: the same criterion. `scrollWidth` and `clientWidth` are integers and both read 58 for the 58.3px "Start Pos" label, so the integer comparison this replaces reports no violation on a live defect.
