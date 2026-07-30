## Context

VCV Rack modules are not paged instruments. The current local-only VCV wrapper uses page-indexed APIs because the shared engine inherited its control model from Daisy Field and web/WASM. That naming was tolerable while VCV was a stub, but the audit found concrete risks:

- `PagedHostIO::SetPageKnob` writes the target bank and the shared `m_knobPositions` latch. `PagedHostIO::tickControls` later replays `m_knobPositions` onto `m_currentPage`, which VCV does not intentionally manage.
- The per-parameter CV helper computes `internalEffective + voltage/10`, but the wrapper then stores that effective value while internal routes may remain active, risking double application.
- The right FX expander currently writes and restores route state in its own `process()`, creating process-order risk relative to the main module's audio callback.
- VCV docs still mention removed MIDI/CC-enable concepts and an obsolete expander shape.

The omni host contract remains binding: VCV is local-only, CV-only, GPL-bounded under `vcv/`, has no Froggers-owned MIDI/CC state, uses internal mod sources `4/5/6`, and is absent from public SIM docs until launch.

## Goals / Non-Goals

**Goals:**
- Make the VCV Rack-facing architecture page-free: Rack controls address named sections and extension modules, never current pages.
- Keep one main module as the engine/audio owner and make left/right extensions control contributors.
- Preserve shared engine reuse through a VCV-safe adapter over existing parameter banks.
- Add global Crunchy with a main-panel knob and CV input.
- Ensure per-parameter CV, internal routes, and global Crunchy are evaluated as effective values for the current block without mutating stored base controls.
- Update specs/docs so future agents do not reintroduce page, MIDI, or CC-toggle concepts.

**Non-Goals:**
- Do not publish VCV to the Rack Library.
- Do not add MIDI widgets, MIDI CC latches, or Froggers-owned Rack MIDI queues.
- Do not change desktop, web, VST/AU, or Daisy operator behavior.
- Do not rename the shared engine's internal `Page`/`PageManager` types as part of the first implementation. The Rack-facing boundary changes first.
- Do not change desktop/web pair-AR behavior. A left VCO AR extension is VCV-specific and optional.

## Decisions

### D1: Introduce a VCV Section Adapter

Create a VCV-facing section adapter with names such as `Audio`, `Random`, `Filter`, `Drive`, `Reverb`, `Delay`, and `VcoAr`. The adapter may internally map these names to existing engine banks while implementation is incremental, but its public methods must be section-named:

```text
setSectionBaseValue(section, row, value)
setSectionInternalRoute(section, row, modIndex, depth)
setSectionJackVoltage(section, row, connected, volts)
effectiveSectionValue(section, row)
randomizeSection(section)
randomizeAllSections()
```

The adapter must not call APIs that mutate `m_currentPage` or shared hardware latch state. `m_knobPositions` remains a Field/web/desktop current-panel concern.

Alternative considered: rename `PageManager` across all hosts. Rejected for this change because it is wider than the VCV defect and risks desktop/web regression.

### D2: Main Owns Audio and Engine State

`Froggers Tiga` main owns `PagedHostIO`/engine state, mod source outputs, Random trigger, global Crunchy, global Crunchy CV, audio/CV/gate I/O, and the single audio `process()` authority.

Adjacent modules are read as state providers. Main collects all extension control snapshots before processing and applies them through the section adapter. Extension modules must not independently apply then restore engine route state in their own process callbacks.

Alternative considered: let each expander mutate the shared engine in its own callback. Rejected because Rack process order would determine audio behavior.

### D3: Extensions Are Optional Control Contributors

The left extension contributes VCO AR controls when present:

```text
Atk VCO1, Rel VCO1
Atk VCO2, Rel VCO2
Atk VCO3, Rel VCO3
Crispy
Randomize, Randmod
optional per-row CV jacks
```

If the left extension is absent, main uses default VCO AR behavior. If the right extension is absent, main uses defaults for sections it does not expose locally. The current right FX module contributes Reverb/Delay controls and stereo I/O; any future right section extension follows the same snapshot boundary.

Alternative considered: make extensions required. Rejected because Rack patches should remain patchable from the main module alone.

### D4: CV Produces Effective Values, Not Stored Values

Per-parameter Rack CV jacks and global Crunchy CV are evaluated at audio-block/process time:

```text
internalEffective = ModMgr::Modulate(base, modIndex, depth)
effective = clamp(internalEffective + cvVolts / 10, 0, 1)
```

Stored base knob, stored internal route, and stored depth must survive the calculation unchanged. The implementation may use a temporary effective-value view, a scoped override, or a section snapshot passed to the engine, but it must not persist physical CV as a knob value.

Alternative considered: temporarily clear routes and write effective knob values, then restore. Rejected because it is fragile under expander process order and invites double-application bugs.

### D5: Global Crunchy Is Additive To Section Crispy

VCV gains a global Crunchy knob and CV input on the main module. Global Crunchy is a pre-section/global fuego pass. Per-section Crispy remains page-local/section-local and still controls that section's rows. The effective global Crunchy value is:

```text
clamp(globalCrunchyKnob + globalCrunchyCvVolts / 10, 0, 1)
```

This mirrors VCV's direct-CV semantics without introducing a MIDI or DAW parameter boundary.

Alternative considered: replace all section Crispy rows with global Crunchy. Rejected because existing section Crispy behavior is part of the shared engine sound.

### D6: Randomization Uses Section State

VCV Random/Randmod actions randomize section state directly. They must not depend on `m_knobPositions`. Random All covers all VCV-visible sections plus optional left/right extension sections that are present. Random/Randmod on a specific extension only affects that extension's section.

Alternative considered: keep using `PagedHostIO::RandomizeAllPages`. Rejected because it uses the hardware/current-page latch path.

## Risks / Trade-offs

- [Risk] The shared engine still names internal banks as pages, so code reviewers may see page names below the adapter and think the rule is violated. -> Mitigation: define the rule at the Rack-facing boundary and add tests that reject VCV wrapper use of current-page/latch APIs.
- [Risk] Effective-value overlays could add allocation or per-callback storage churn. -> Mitigation: preallocate fixed section snapshots and run owned-allocation checks in VCV-adjacent tests.
- [Risk] Rack expander discovery can be brittle when multiple unrelated modules are adjacent. -> Mitigation: only consume known Froggers module types discovered through immediate/chain expander links; absence falls back to defaults.
- [Risk] Global Crunchy may interact musically with per-section Crispy in surprising ways. -> Mitigation: document the order and test minimum/maximum CV clamp behavior.
- [Risk] Existing patches saved with schema v2 may expect current parameter IDs. -> Mitigation: preserve model slugs and parameter IDs where possible; add migration only if new parameters shift existing IDs.

## Migration Plan

1. Add section/extension specs and tests before changing Rack processing.
2. Introduce the VCV section adapter over current engine banks without changing desktop/web/VST code paths.
3. Move main and right FX processing to snapshot collection consumed by main.
4. Add global Crunchy knob/CV to the main module without shifting existing saved parameter IDs where possible; if IDs shift, add a schema migration and test it.
5. Add or revive the left VCO AR extension only after the main section adapter is stable.
6. Regenerate VCV panel SVGs and update local VCV docs.
7. Run VCV-specific boundary checks, sim tests, and any available local Rack build smoke.

Rollback is straightforward before Rack publication: keep the local-only `vcv/` tree unshipped, revert the change, and preserve existing desktop/web release artifacts.

## Open Questions

- Should global Crunchy be placed in the existing main mod-rack strip or in a distinct global-control area near master I/O?
- Should the left VCO AR extension be part of the first implementation batch or a follow-up task after section adapter stabilization?
- Can the current parameter ID layout add global Crunchy/CV without migrating existing saved patches?
