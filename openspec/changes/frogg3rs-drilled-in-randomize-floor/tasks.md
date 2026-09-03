# Tasks — `frogg3rs-drilled-in-randomize-floor`

PLANNED ONLY. No fix outside this text.

## 1. Preflight, delegated for fresh context

- [ ] 1.1 Every citation resolves: `FroggersModulation.hpp:867` (the helper),
      `:1262`, `:1453`, `:1536`, `:1571` (the four call sites);
      `FroggersParameters.hpp:282` (`kNumModulators = 15`);
      `FroggersModulationTests.cpp:903` (`RequireGeometricCountDistribution`),
      `:953`, `:986` (the two stale test names), `:800` (the stale comment).
- [ ] 1.2 Confirm the four call sites are the complete set — grep by OPERAND,
      not by name: `NextRandomCoin`, `EnsureModulationDepth`,
      `RandomizeVisibleValue`. A fifth path that draws a source count is a
      finding that stops execution.
- [ ] 1.3 Confirm the level-0 figure the design rests on. Read the helper's own
      comment claim of ~84 depths per level-0 press and check it against
      `kFroggersParamsPerBank` x `kFroggersBankCount`. If the arithmetic does
      not give ~84, the sparseness argument for scoping the floor is wrong and
      the change stops.
- [ ] 1.4 Behavioural premise, measured before any code is written: run the two
      existing distribution tests as they stand and record the `[OBSERVED]`
      histogram lines they print. Those are the before numbers task 2.4 compares
      against. A premise about a distribution is not settled by reading the loop.

## 2. Implementation

- [ ] 2.1 `RandomizeParameterModulationDepths` gains
      `std::size_t minimumSources = 0` as a third parameter. The count draw
      becomes `std::size_t count = std::min(minimumSources, eligible.size());`
      before the existing while loop. Nothing else in the function changes; the
      `eligible.empty()` early return stays ahead of it.
- [ ] 2.2 Pass 1 at the three drilled-in call sites (`:1453`, `:1536`, `:1571`).
      Leave `RandomizeBankLevel1Depths` (`:1262`) on the default, and say so in
      a comment there: level 0 keeps the zero floor deliberately, because the
      floor would roughly double its depth count.
- [ ] 2.3 `RequireGeometricCountDistribution` gains a floor argument and
      asserts the shape from that floor: mode at the floor, each count about
      half the one below, and — when the floor is 1 — zero observed on no trial
      at all, not merely rarely.
- [ ] 2.4 Positive control, both numbers reported. Re-run the two distribution
      tests and print the `[OBSERVED]` histograms beside task 1.4's. The level-0
      histogram MUST be unchanged in shape (mode 0, P(0) about 50%). The level-1
      histogram MUST show P(0) = 0 and mode 1. A run where level-0 also moved
      means the floor leaked into the shared default — say so and stop.
- [ ] 2.5 Measure the allocation cost the proposal predicts: record depths
      materialized per level-1 press before and after, and how often `partial`
      is reported. If `partial` becomes common rather than rare, that is a
      finding this text does not resolve — report it and stop.
- [ ] 2.6 Rename the two stale tests to name the distribution they actually
      assert, and correct the `:800` comment's "20%" and "~80%" to the figures
      the code produces. No planning-document references in either.
- [ ] 2.7 `nice make -j2 -C app test`, never more. Run every test binary by
      path; the recipe is not trusted to reach them all.

## 3. Spec

- [ ] 3.1 `openspec/specs/froggers-modulation-slate/spec.md`'s randomized-source
      -count requirement, via this change's delta, states both distributions and
      which levels each governs. `openspec validate frogg3rs-drilled-in-randomize
      -floor --strict`. Keep every SHALL on a requirement's FIRST body line — the
      validator only finds it there.

## 4. Postflight, before any commit

- [ ] 4.1 Fresh-context Sonnet reviewer against this proposal and the §5 table.
      Divergences reported strictly. Commit only when it passes.

## 5. Operator

- [ ] 5.1 Drill into a parameter, press Randomize All repeatedly: every press
      moves something. It should never be a no-op.
- [ ] 5.2 On a parameter page, press Randomize All repeatedly: it still reads as
      a set of deliberate choices, with about half the parameters untouched.
