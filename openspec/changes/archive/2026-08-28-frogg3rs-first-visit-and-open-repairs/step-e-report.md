# Step E — Delete dead DspBuffers.hpp primitives

Deletion of `RollingBuffer`, `BoundedAudioBuffer`, `BufferResampler` from
`projects/synth/include/synth/DspBuffers.hpp`. Repo:
`External/Sheaf`, branch `fix-out-of-tree-app-gaps`.

## 1. Re-verification

Grep for each bare name across the whole submodule, excluding `analysis/`,
`.git`, `node_modules`, `dist`. Every hit:

**RollingBuffer**
- `projects/synth/include/synth/DspBuffers.hpp:99` — definition.
- `projects/synth/tests/dsp_tests.cpp:1294` — test (`rolling_buffer_reports_min_and_max_over_ring_storage`).
- `docs/superpowers/plans/2026-07-02-midi-instrument-4-ui-miniapp.md:7` — prose, describes a SmartGridOne usage pattern as design inspiration, not an invocation.
- `docs/superpowers/plans/2026-07-07-add-smartgrid-dsp-processors.md:204,221` — prose, port-planning spec text.
- `openspec/changes/archive/2026-07-04-midi-instrument-config-ui/design.md:19,175` — prose, historical design record.
- `openspec/changes/archive/2026-07-07-add-smartgrid-dsp-processors/tasks.md:3`, `.../proposal.md:29` — prose, historical.

**BoundedAudioBuffer**
- `projects/synth/include/synth/DspBuffers.hpp:18` — definition.
- `projects/synth/tests/dsp_tests.cpp:1053,1263,1273,1274,1284,1285` — tests (one shared dependency-clean check plus two dedicated tests).
- `docs/superpowers/plans/2026-07-07-add-smartgrid-dsp-processors.md:84,103,203,414` — prose, port-planning spec text (includes a skeleton-stage code sample, not live code).

**BufferResampler**
- `projects/synth/include/synth/DspBuffers.hpp:301` — definition.
- `projects/synth/tests/dsp_tests.cpp:1313,1314,1331,1334` — tests (two dedicated tests).
- `docs/superpowers/plans/2026-07-07-add-smartgrid-dsp-processors.md:205,221` — prose, port-planning spec text.
- `openspec/changes/archive/2026-07-12-add-braid-4-synth-app/design.md:7,155,162` — prose, explicitly explains why Braid-4 does *not* use `BufferResampler` (allocates, block-edge filter resets, rounded frame counts unsuitable for real time). This is itself confirmation of zero production use, not an invocation.
- `openspec/changes/archive/2026-07-07-add-smartgrid-dsp-processors/proposal.md:29`, `.../design.md:11` — prose, historical.

No production invocation of any of the three symbols was found anywhere in the
submodule outside `analysis/`. All non-test, non-definition hits are prose in
`docs/superpowers/plans/**` or `openspec/changes/archive/**` describing the
July 2026 Smart Grid port and (for `BufferResampler`) explicitly documenting
why Braid-4 avoided it. Deletion proceeded for all three.

## 2. Inbound mentions (§12.0)

Searched `projects/synth/` (code comments, the Makefile) for anything that
would dangle once the symbols are gone. Result: none. No comment in any
`projects/synth/` source file mentions `RollingBuffer`, `BoundedAudioBuffer`,
or `BufferResampler` — the only in-tree mentions were the definitions
themselves and the tests, both removed in this change. The Makefile's
`DSP_HEADERS` (`projects/synth/Makefile:39`) lists `DspBuffers.hpp` as a whole
file, not per-symbol, and the header still exists, so no Makefile edit was
needed.

Prose mentions in `docs/superpowers/plans/2026-07-02-...md`,
`docs/superpowers/plans/2026-07-07-...md`, and
`openspec/changes/archive/**` were left untouched per the brief — they are
historical records of the port and of Braid-4's design rationale, not
descriptions of current code.

## 3. What was deleted

`projects/synth/include/synth/DspBuffers.hpp`:
- `BoundedAudioBuffer` (was lines 18–96, plus trailing blank) — struct with `samples`, section-extrema arrays, `ClearSectionExtrema`, `ComputeSectionExtrema`, `Clear`, `RealTimeFromNormalized`, `ReadRealTime`, `ReadNormalized`.
- `RollingBuffer<Size>` (was lines 98–117, plus trailing blank) — `Write`, `Min`, `Max`.
- `BufferResampler` (was lines 301–410 pre-edit, 200–309 after the first two removals) — `OutputFrameCount`, `AntiAliasLowpassInPlace`, `ResampleToRate`.
- Net: 412 → 195 lines (217 deleted).

`FirDecimator` (`:211` originally, now `:110`) and `OversampledOutputStage`
(`:272` originally, now `:170`) were left untouched.

`projects/synth/tests/dsp_tests.cpp` uses a self-registering `TEST_CASE`
macro (`static Register reg_##name(...)` pushes into a `Registry()` vector;
`main()` just iterates `Registry()`) — there is no hand-maintained call list,
so no runner lines needed removal. Deleted:
- `bounded_audio_buffer_reads_fractional_midpoints_and_normalized_positions` (whole `TEST_CASE`)
- `bounded_audio_buffer_computes_section_extrema_and_clears` (whole `TEST_CASE`)
- `rolling_buffer_reports_min_and_max_over_ring_storage` (whole `TEST_CASE`)
- `buffer_resampler_copies_when_rates_match` (whole `TEST_CASE`)
- `buffer_resampler_downsampling_attenuates_high_frequency_content` (whole `TEST_CASE`)
- One line inside `smartgrid_dsp_public_headers_are_dependency_clean` (`REQUIRE_TRUE(std::is_default_constructible_v<synth::BoundedAudioBuffer>);`) — this test's sole purpose is *not* exercising `BoundedAudioBuffer` alone (it also checks `BitCrusher`, `Meter`, `Ola<12>`), so per §12.0 the test itself stays and only the dangling line was removed.
- Net: 2999 → 2908 lines (91 deleted).

## 4. `#include` cleanup

After removing the three structs, grepped the remaining header for every
facility each include provided:
- `<algorithm>` — no remaining `std::min/max/clamp/fill/min_element/max_element`. Removed.
- `<cmath>` — no remaining `std::floor/sqrt/abs/fabs` (the `detail::` Fir* functions are hand-rolled, not `<cmath>`-backed). Removed.
- `<cstring>` — no remaining `memcpy`. Removed.
- `<memory>` — no remaining `unique_ptr`/`make_unique`. Removed.
- `<vector>` — no remaining `std::vector`. Removed.
- `<array>`, `<cstddef>`, `<cstdint>`, `<span>`, `<utility>` — kept; all still used (`std::array`/`std::size_t` throughout, `std::uint64_t` in `OversampledOutputStage`, `std::span` in `FirDecimator`, `std::move` in `OversampledOutputStage`'s constructor).

## 5. Build + run results

Machine limits observed: `nice make -j2`, foreground, one build at a time.

- `nice make -j2 build/dsp_tests` — compiled clean, no warnings.
  - `./build/dsp_tests` → `DSP_TESTS_EXIT=0`, 106 pass / 0 fail. No test name containing `bounded_audio`, `rolling_buffer`, or `buffer_resampler` appears in the output (all gone, as expected). `fir_decimator_*` and `oversampled_output_stage_*` tests present and passing, confirming the two live siblings are intact.
- `nice make -j2 build/braid4_system_tests` — compiled clean, no warnings. Confirms `apps/braid-4/Braid4Core.hpp:9`'s include of `DspBuffers.hpp` still compiles.
  - `./build/braid4_system_tests` → `BRAID4_SYSTEM_EXIT=0`, 29 pass / 0 fail.
- `nice make -j2 build/braid4_deadline_tests` — compiled clean, no warnings.
  - `./build/braid4_deadline_tests` → `BRAID4_DEADLINE_EXIT=1`, 3 pass, exactly the two pre-existing `braid4_meets_96000hz_256_frame_deadline_and_continuity` / `braid4_sparse_modulation_meets_96000hz_256_frame_deadline` timing failures (`stats.averageSeconds <= stats.blockSeconds * 0.60`), matching the known pre-existing gate failure on this Mac. Nothing related to `DspBuffers.hpp` or the deleted symbols.

All binaries that include `DspBuffers.hpp` compile; the only run failures are
the pre-existing, unrelated 96kHz deadline-timing tests.

## 6. `git status --short`

```
 M projects/synth/include/synth/DspBuffers.hpp
 M projects/synth/include/synth/PortableUIBuilders.hpp
 M projects/synth/include/synth/PortableUILayout.hpp
 M projects/synth/include/synth/RuntimePages.hpp
 M projects/synth/tests/dsp_tests.cpp
 M projects/synth/tests/portable_ui_layout_tests.cpp
 M projects/synth/tests/portable_ui_tests.cpp
 M projects/synth/tests/support/VisualCriteria.hpp
```

Two files touched (`DspBuffers.hpp`, `dsp_tests.cpp`) plus the 6 pre-existing
modified files, left untouched. Nothing else.

## 7. Contradictions with the brief

None. The only place the brief's phrasing needed interpretation rather than
literal action: it estimated "6 tests" / "1 test" / "4 tests" for the three
symbols; the actual counts are grep-hit counts (6/1/4 occurrences), which
resolve to 2 dedicated `TEST_CASE` blocks + 1 shared-test line for
`BoundedAudioBuffer`, 1 dedicated block for `RollingBuffer`, and 2 dedicated
blocks for `BufferResampler` — consistent with the brief once read as hit
counts rather than block counts. The brief's conditional about `dsp_tests.cpp`
possibly having "a hand-maintained runner list like its sibling test files"
did not apply here — this file self-registers tests via a `Register` helper,
so no runner-line edits were needed.
