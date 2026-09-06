# Proposal — `frogg3rs-guitar-and-solo-variants`

**Created 2026-08-28. Revised after preflight (`preflight.md`).** The Daisy
Field firmware becomes two separate programs. The current build is **Froggers
Solo**. **Froggers Guitar** differs in exactly two ways: it has no reverb page,
and when an external signal is present the dry external signal runs through the
chain alongside the ring mod instead of being replaced by it.

They are built and flashed independently. Loading one replaces the other on the
device; there is no runtime switch and neither binary carries the other's code.

This change UNFREEZES `src/`. That tree has been frozen since the pivot, and the
freeze is why the diagnosed latency work below was archived undelivered rather
than fixed.

## Traced

The whole external/oscillator decision is `FroggersEngine.hpp:811-820`:

    float MixExternalAndOsc(float input, float v1, float v2, float v3, float olvl, bool hasExternal)
    {
        float oscMix = MixOscVoices(v1, v2, v3);
        if (!hasExternal)
        {
            return olvl * oscMix;
        }
        return (input * v1 + input * v2 + input * v3) * (1.0f / 3.0f);
    }

Its result is `chainIn`, which goes through `m_frogBlock.Process` and then
`ApplyOutputFx` (`:871-873`). So when the gate is open the oscillators' own
voice is not attenuated, it is ABSENT: the ring mod is the entire signal.

The gate is `m_extGate` over a smoothed `|extIn|` (`:868-870`).

`MixExternalAndOsc` has exactly ONE definition and ONE call site, both in
`src/core/FroggersEngine.hpp`.

`MixOscVoices` is called at `:813` before the gate test and its result is
discarded when the gate is open. `MixOscVoices` is a pure average of the
three VCOs with no state of its own; Guitar keeps calling it for parity with
Solo's control flow, and no early-out may skip it.

`src/core/FroggersEngine.hpp` is included by exactly one file,
`src/FroggersTiga/FroggersTiga.hpp:3`. Everything else that names it does so in
a comment. Editing the engine cannot reach the app hosts.

The Sheaf app has ring modulation too, and it is a DIFFERENT mechanism sharing
the word. Each VCO carries its own internal carrier oscillator — `ringCarrierPhase`
advanced by `RingModPhaseIncrement`, depth from `RingModDepthScale`, driven by a
per-VCO "Ringmod" knob (`app/dsp/Vco.hpp:181-235`). The carrier is generated
internally; the external input is never a factor. External audio reaches the app
only as a modulation SOURCE (`app/FroggersAppCore.hpp:1108`, into
`modulation_.Step`), so it can modulate a Ringmod knob's depth but never enters
the audio path itself.

So external-signal ring mod exists on the Daisy alone, and this change touches
only that.

## Two programs, not one program with a switch

`src/` already holds one app directory per program — `Blink/`, `TestControl/`,
`FroggersTiga/` — each with its own `Makefile` naming its own `TARGET`, each
including `../mk/daisy.mk`. `BUILD_DIR` is relative to the app directory
(`config.mk:10`), so two app directories cannot collide in `build/`.

Guitar follows that pattern. `src/FroggersTiga/` is renamed `src/FroggersSolo/`
with `TARGET := FroggersSolo`, and `src/FroggersGuitar/` is added with
`TARGET := FroggersGuitar`. The artifacts are `build/FroggersSolo.bin` and
`build/FroggersGuitar.bin`, each flashed on its own.

The variant difference is a `-D` set by each app Makefile before it includes
`daisy.mk`, which composes `DEFS := $(DEFS_COMMON) $(BOOT_DEFS) $(DEFS)`
(`daisy.mk:98`). The engine is shared so that a DSP fix lands once; the
divergence is resolved by the preprocessor, so the reverb page and its buffers
are absent from the Guitar binary rather than switched off inside it.

## The latency and freezing work was never delivered

`2026-07-25-field-button-latency-headroom` was archived as "superseded by the
pivot, NOT delivered", because its targets are all in the frozen tree. Its three
items were verified in the source, and one of them does not survive that
verification:

| item | state today | disposition |
|---|---|---|
| `UpdateParams()` once per block | called from `ProcessSample` (`:852`), inside `ProcessBlock`'s loop (`:656-659`) | REFUSED — measured, see below |
| LED transmit throttled | `SwapBuffersAndTransmit()` unconditional every poll (`DaisyIO.hpp:137`) | delivered here |
| dry-reverb early-out | absent; `ProcessReverb(output)` runs every sample regardless of mix (`:844`) | delivered here, Solo only |

**`UpdateParams()` does not move to block rate.** `RuntimeParam`'s smoother sets
its alpha from a 1 kHz natural frequency at the sample rate. Block size is 48
(`External/libDaisy/src/daisy_field.cpp:79`) and the rate is 48 kHz
(`App.hpp:15`), so a block-rate call happens at 1 kHz. Measured on the host with
the real headers, time to 90% of a step: **0.375 ms** today; **18.0 ms** at block
rate keeping the alpha; **1.0 ms** if the alpha is re-derived — and that
re-derivation asks for `cyclesPerCall = 1.0` against a `x_maxCutoff` of 0.499
(`OPLowPassFilter.hpp:9,28`), so it is silently clamped. A 1 kHz smoother cannot
exist at a 1 kHz update rate. The headroom comes from the deletion below instead.

## Six biquads per sample, one of which is wanted

`SetUseV2FilterParallel` (`:263`) has zero callers. The flag is `false` at `:113`
and nothing sets it, so `ApplyOutputFx`'s parallel branch (`:825-833`) never
executes. `m_scoopNotch` is therefore recomputed three times per sample —
`SetFreq`, `SetWidth`, `SetHeight` at `:561,:562,:564`, each calling
`UpdateCoefficients()` with `sin`, `cos`, `sqrt` and five divisions
(`ResonantBump.hpp:42-72`) — to configure a filter no audio passes through.

Two facts confirm the path is inert even if the branch were reached.
`m_filterScoop` reads `m_filterParams->GetParam(8)` (`:481`) but the filter page
only initialises positions 0–6 (`:632-638`), so its target is a
default-constructed zero and the notch height is 1.0, which
`ResonantBump.hpp:44-45` documents as transparent. `m_filterCombPeak` reads
position 7 (`:480`), which `SetFuegoization()` initialises as `FUEG`
(`Page.hpp:75,81`) — the blend is wired to the Crispy knob.

So the dead branch and everything reaching only into it come out: the branch,
`m_scoopNotch`, `m_filterScoop`, `m_filterCombPeak`, `m_useV2FilterParallel` and
its setter. `m_resonantBump` remains, and its three setters collapse to one
call. Six biquad recomputations per sample become one.

This also removes the last of three double-read smoothers. `m_bumpFreq` is read
at `:558` and `:561`, `m_bumpWidth` at `:560` and `:562`, `m_filterScoop` at
`:563` and `:830` — each advancing its one-pole twice per sample, so each
smooths at twice the rate of every other parameter and its two consumers see
different points of the same sweep.

## A spec that describes hosts that do not implement it

`external-ring-mod-mix` states the formula is shared "across all hosts" (`:5`)
and its scenario asserts "any host (Daisy Field, desktop, web WASM)" (`:40`).
Only the Daisy firmware implements THIS formula. The desktop and browser hosts
run the Sheaf app, which has ring modulation of its own from an internal per-VCO
carrier and no external-signal ring mod at all. Both the Purpose line and the
requirement are corrected to name the host they actually govern.
`field-operator-doc-parity` was checked for the same overreach and does not have
it.

## What Guitar changes

**No reverb page.** The page, its parameters, and its buffers are absent from the
Guitar build.

**Dry external in parallel with the ring mod.** With the gate open, Guitar's
pre-drive mix is

    (7/12) * extIn + (5/12) * ((extIn*v1 + extIn*v2 + extIn*v3) / 3)

The dry external path is 40% louder than the ring-mod path in RELATIVE terms
(7:5 is exactly 1.4:1), and the weights sum to 1 so total level is unchanged
from Solo's gate-open output. Both terms enter the SAME chain, `m_frogBlock`
then `ApplyOutputFx`, as one summed input rather than as two chain instances.
The chain is nonlinear, so the two signals are driven together; that is the
intended behaviour, not an approximation of separate chains.

With the gate CLOSED, Guitar behaves exactly as Solo: `olvl * oscMix`. The
oscillators still prevail when no external signal is present.

## Non-goals

- Any change to Solo's gate-open formula. Solo keeps `(extIn*v1 + extIn*v2 +
  extIn*v3) / 3` unchanged.
- Any change to the Sheaf app, desktop, VST or browser hosts.
- The app's per-VCO ring modulation. `app/dsp/Vco.hpp`'s internal carrier, the
  Ringmod knobs, their floor and taper stay exactly as they are. This change
  cites that code to establish it is a different mechanism, never to modify it.
- Reverb removal from Solo.

## Accepted collateral

Twenty comments in `app/FroggersDspParityTests.cpp` and `app/FroggersAppCore.hpp`
cite `FroggersEngine.hpp:LINE`. Editing the engine makes every one of them stale.
They are in `app/`, which this change may not touch, so they are left stale
deliberately rather than discovered later.

## Impact

- `src/core/FroggersEngine.hpp` — the mix, the dead-branch deletion, the reverb
  guards.
- `src/common/DaisyIO.hpp` — LED transmit rate, boot screen, drain granularity.
- `src/common/FieldMutationQueue.hpp` — one page per drain.
- `src/FroggersTiga/` renamed to `src/FroggersSolo/`; `src/FroggersGuitar/` added.
- `src/mk/daisy.mk~`, `src/Blink/Makefile.bak`, `src/Blink/Makefile~` — tracked
  editor backups, removed.
- `external-ring-mod-mix`, `field-button-input-latency`.
- `DAISY_MANUAL.md` — variant names, page order, build and flash instructions.
