# Proposal — `frogg3rs-drilled-in-randomize-floor`

**Created 2026-09-03. PLANNED ONLY: nothing executes before the operator's go.**

Paths are frogg3rs `app/` unless the text says otherwise. Line numbers are
2026-09-03 working-tree reads.

## What the operator asked for

At a drilled-in modulation level, Randomize All should never be a no-op. The
modal outcome should be one source, decaying geometrically from there, instead
of the current modal outcome of zero.

## What the code does now

One helper draws the count for every randomize path:
`detail::RandomizeParameterModulationDepths` (`app/FroggersModulation.hpp:867`).

    std::size_t count = 0;
    while (count < eligible.size() && manager.NextRandomCoin() >= 0.5f) {
        ++count;
    }

That is geometric from zero: P(k) = 0.5^(k+1), so 50% none, 25% one, 12.5% two.
Mean is 1 - 0.5^n, which is 1.0 for any realistic `eligible.size()`. `eligible`
is the connected sources only, of `kNumModulators = 15`
(`app/FroggersParameters.hpp:282`). Each chosen source then gets an independent
value in each of the two scene poles, so a chosen source always has a non-zero
depth: "picked" and "non-zero depth" are the same event.

**The helper is shared by four call sites, and this is the constraint the change
turns on:**

1. `RandomizeBankLevel1Depths` (`:1262`) — level-0 Randomize All, once per page
   parameter per bank.
2. `RandomizePage`'s level-1/2 branch (`:1453`).
3. `RandomizeAll`'s drilled-in branch (`:1536`).
4. `RandomizeAll`'s one-level descent (`:1571`).

Changing the loop in place therefore changes level 0 too. Level 0 currently
materializes about 84 depths across 84 parameters. A floor of one would take it
to about 168, above the ~151 the hand-tuned ladder produced before it was
replaced — the exact number the helper's own comment cites as the reason for
moving to the geometric draw, for sparseness and for storage pressure. The
operator asked for this at drilled-in levels, so the floor is scoped there.

## Design

`RandomizeParameterModulationDepths` takes a minimum:

    inline bool RandomizeParameterModulationDepths(synth::ParameterManager& manager,
                                                   synth::Parameter& parameter,
                                                   std::size_t minimumSources = 0)

and the count draw starts from it:

    std::size_t count = std::min(minimumSources, eligible.size());
    while (count < eligible.size() && manager.NextRandomCoin() >= 0.5f) {
        ++count;
    }

The `std::min` clamp matters: a parameter with exactly one connected source must
not be asked for more than it has, and the existing `eligible.empty()` early
return already covers zero.

With `minimumSources = 1` the distribution becomes P(k) = 0.5^k for k >= 1 —
50% one, 25% two, 12.5% three, zero impossible, mode one, mean about 2. With the
default 0 the draw is byte-identical to today's.

Call sites 2, 3 and 4 pass 1. Call site 1 keeps the default, so level 0 is
unchanged. Sites 3 and 4 are the two halves of one drilled-in press — the
selected parameter's own depths and the descent that writes the level below — so
both take the floor, matching the standing rule that a level-1 Randomize All
governs level-2 randomization.

**Cost, stated rather than discovered later.** A level-1 press today yields about
2 non-zero depths (about 1 on the selected parameter, about 1 across its new
depth parameters). With the floor it yields about 6: mean 2 on the selected
parameter, and mean 2 on each of those 2. That is roughly a tripling of depth
allocation per drilled-in press. `EnsureModulationDepth` returning null already
sets `partial` rather than short-counting silently, so the failure mode is
reported, but `partial` will be reported more often. Task 2.4 measures it instead
of assuming it.

## Hygiene found in the tree this change touches (§8.0)

Two test names in `app/FroggersModulationTests.cpp` describe a distribution the
code no longer has. Their bodies are correct — both call
`RequireGeometricCountDistribution` (`:903`) — but their names say `mode_two`,
left over from the weighted table the geometric draw replaced:

- `randomize_depth_helper_level_zero_count_distribution_has_mode_two_across_1000_trials` (`:953`)
- `randomize_all_level_one_press_gives_its_own_depths_and_each_depths_subdepths_the_mode_two_distribution` (`:986`)

A comment at `:800` carries the same stale claim: "`count` is 0 on 20% of draws
in RandomizeParameterModulationDepths's weighted table", and "~80% of draws
expected to produce at least one source". The code's actual figure is 50%. The
comment sits above a 500-trial aggregate assertion, so the stale number is load
bearing for a reader deciding whether that assertion is calibrated.

This change renames both tests and corrects that comment, because it is changing
exactly these distributions and leaving them would ship two names that describe
neither the old nor the new behaviour.

## §5 forward enumeration

| concept | files at the working tree | disposition |
|---|---|---|
| `RandomizeParameterModulationDepths` | `FroggersModulation.hpp` 1 definition, 4 call sites | signature gains a defaulted third argument; three call sites pass 1, one keeps the default |
| `minimumSources` (new) | 0 everywhere | created; one parameter, no second floor concept |
| `RequireGeometricCountDistribution` | `FroggersModulationTests.cpp` 1 definition (`:903`), 2 call sites (`:973`, `:1044`) | gains a floor argument; both call sites pass their level's floor |
| `NextRandomCoin` | the count loop only | unchanged; the floor changes the loop's start, not the coin |
| `eligible` / `EnsureModulationDepth` / `partial` | as today | unchanged; the clamp keeps `count <= eligible.size()` |
| `mode_two` (in test names) | 2 test names, 1 comment | renamed and corrected; see hygiene above |

## Impact

- `app/FroggersModulation.hpp`: the helper's signature and count draw; three
  call sites.
- `app/FroggersModulationTests.cpp`: the distribution helper's floor argument,
  two test renames, the stale-percentage comment, and a new zero-never-happens
  assertion.
- `openspec/specs/froggers-modulation-slate/spec.md` via this change's delta:
  the randomized-source-count requirement now states both distributions.

## Delivery

One commit on `main` after the postflight. `nice make -j2 -C app test`, never
more.

## Dispatch

Preflight delegated for fresh context (a small change). One implementer on
Sonnet. Postflight reviewer on Sonnet with fresh context. No fixes outside this
text: a defect this text does not name stops execution and supersedes the change.
