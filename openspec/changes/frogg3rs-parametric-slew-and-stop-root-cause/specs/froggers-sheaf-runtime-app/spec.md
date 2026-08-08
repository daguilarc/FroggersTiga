# Delta — `froggers-sheaf-runtime-app`

Operator-ordered behaviour (S1a.1, verbatim: *"no, modulation should not free-run while stopped lol.
come on."*), landed independently of this change's root-cause fix for F3 (the drive stage's DC seed —
see the `frogg3rs-dsp-recovery` delta). Labelled separately here for the same reason `tasks.md`
labels it separately: freezing modulation cannot remove a static DC seed that is a pure function of
frozen knob state, so this is not a fix for F3. It is a correctness rule about modulation's own
behaviour across Stop/Play, decided on its own merits.

## ADDED Requirements

### Requirement: Modulation sources hold rather than free-run while the transport is stopped
While the transport is stopped, the app SHALL NOT advance its own modulation sources. The app SHALL
call `Modulators::Step()` only while the transport is running, gated on the same predicate the
amplitude-envelope gate already uses. Sources SHALL hold their last value across a stop rather than
reset to a neutral value — there is no neutral-reset path, because Sheaf's `RegisterSources()` hands
the framework raw pointers to this app's own member variables, and `Modulators::UpdateModValues()`
dereferences those same pointers every sample through `parameters_.ProcessSample()`, which SHALL
remain **ungated**.

`parameters_.ProcessSample()` stays ungated because a `SceneCenter` write only reaches the DSP
through its own periodic smoothed `Compute()`; gating that call as well would freeze patch edits and
Randomize All until the transport runs, which is not what this requirement asks for.

#### Scenario: The genuinely free-running sources stop advancing
- **WHEN** the transport is stopped
- **THEN** ganged random LFO 6, the three VCO audio-output sources, the three VCO envelope-follower
  sources, and the noise source no longer advance
- **THEN** each holds exactly the value it last held before the stop

#### Scenario: Transient sample-and-hold lanes are unaffected by this gate specifically
- **WHEN** the transport stops mid-glide on a Random S&H lane 1–5
- **THEN** that lane finishes its own short glide to its already-fixed target and then holds — a
  property of the lane's own clock-driven tick guard, not of this gate

#### Scenario: Patch edits still reach the DSP while stopped
- **WHEN** the operator turns a knob, or Randomize All fires, while the transport is stopped
- **THEN** the change reaches the DSP through the ordinary smoothed parameter path
- **THEN** no held modulation source has to move for that change to take effect

#### Scenario: Visualizers show a held value, not darkness
- **WHEN** the transport is stopped
- **THEN** modulation-source visualizers continue to publish their last state once per block
- **THEN** no visualizer goes blank purely because the transport is stopped
