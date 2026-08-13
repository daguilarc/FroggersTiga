# FroggersTiga Manual

Operator manual for **Frogg3rs — the Sheaf app** under [`app/`](app/), the current line of development.

Two other instruments share this repository and are documented separately, because their parameter
models do not map onto this one:

- **Daisy Field hardware firmware** (`src/FroggersTiga/`, frozen) → [`DAISY_MANUAL.md`](DAISY_MANUAL.md)
- **Frozen web / desktop-v1 simulator surface** → [`SIM_MANUAL.md`](SIM_MANUAL.md)

Terse one-line-per-parameter glossary for this app: [`QUICK_DICT.md`](QUICK_DICT.md).

---

Six parameter banks — **Audio**, **Envelope**, **Filter**, **Drive**, **Delay**, **Reverb** — each with
16 encoder slots: 14 page parameters (slots 0–13), a bank-local **Crispy** (slot 14), and one shared
global **Crunchy** (slot 15, the same control in every bank). All six banks process audio every sample
regardless of which one is on screen — switching banks only changes what you're looking at.

## Global controls

### Crispy and Crunchy (fuego)

Both controls apply the same bit-scramble ("fuego") to parameter values before they reach the DSP —
they are not audio effects on their own; they corrupt the *values* other knobs are already set to.
Internally, a parameter's value is treated as an 8-bit number; the higher the fuego amount, the more of
that number's low bits get folded into an XOR/shift scramble that depends on which slot the parameter
sits in. At the fuego knob's minimum, this is an exact no-op — parameters pass through completely
unscrambled, bit-for-bit. As the fuego amount rises, small knob or modulation moves stop being smooth:
values snap between "islands" instead of sweeping continuously, and different slots on the same bank
scramble differently from each other because the scramble pattern is keyed by slot index.

- **Crispy** (slot 14, one instance per bank, colored like that bank) scrambles only that bank's own 14
  page parameters (slots 0–13). It does not touch Crunchy, and it does not touch any other bank.
- **Crunchy** (slot 15) is a single shared parameter — the literal same value — wired into all six
  banks at once. It scrambles every page parameter in every bank, *and* it scrambles every bank's own
  Crispy value before that Crispy value is used to scramble its bank (Crunchy stacks underneath Crispy,
  never the other way around). Crunchy itself receives no scramble.
- Both default to 0 (no-op). Turning up Crunchy alone is a fast way to add grit everywhere at once
  without touching six separate Crispy knobs.

### Bank selection

Six named buttons — Audio, Envelope, Filter, Drive, Delay, Reverb — pick which bank's 16 slots populate
the encoder grid on screen. This only changes what's visible/editable; every bank keeps processing audio
in the background regardless of which one is selected.

### Transport and the envelope gate

**Play** / **Stop** control the master clock's transport — there is no separate note-on/note-off or MIDI
note input in this app. Instead, while the transport is running, the shared envelope gate that drives all
three VCOs' Attack/Decay/Sustain/Release stages (Envelope bank) pulses automatically: **open for the
first half of every quarter note, closed for the second half**, at whatever tempo the BPM control (30–300
BPM) is set to. That's what re-triggers the envelopes on every beat — there is nothing to "press" to play
a note; starting the transport is the note-on.

**Stop** forces a fast ~50 ms fade on all three voices regardless of what the Release knobs are set to
(so Stop always reads as immediate, never as a multi-second tail), and clears the Delay/Reverb tails once
every voice has fully gone silent.

### Modulation assignment

Click any parameter's encoder — a page parameter or a bank's own Crispy (Crunchy is excluded) — to open
a modulation view for that one parameter. It exposes **15 independent modulation sources**, each with its
own signed depth (depth 0 = that source off for this parameter). Turning a source's depth changes how
hard that source pushes the target; multiple non-zero sources on the same parameter **sum together**
rather than crossfading one at a time. Click the parameter's encoder again (or the view's back target) to
leave the modulation view.

The 15 sources are: six Random S&H/LFO-style random lanes, each VCO's own raw audio-rate signal (VCO1–3
Audio), each VCO's own slow envelope follower (VCO1–3 EF), one broadband Noise source, and External Audio
plus its envelope follower. External Audio and External Audio EF are always listed but currently always
**unavailable** — this app has no external audio input wired up (unlike the older Daisy/desktop/web
hosts, documented in [`DAISY_MANUAL.md`](DAISY_MANUAL.md) and [`SIM_MANUAL.md`](SIM_MANUAL.md)).

---

## Audio bank

Three independent oscillators. Unlike older hosts, there is **no cross-coupler** — nothing here reads
another VCO's phase or output except each VCO's own internal ring-mod carrier, and mixing is a simple
three-way balance, not pairwise coupling.

**VCO1 / VCO2 / VCO3** (`VCO1`/`VCO2`/`VCO3`, slots 0–2) — each VCO's pitch, mapped exponentially from
20 Hz to 20 kHz. Default values land on 110 Hz, 220 Hz, and 330 Hz respectively, so a freshly launched app
already makes an audible chord with no knobs touched.

**Shape 1 / 2 / 3** (`Shp1`/`Shp2`/`Shp3`, slots 3–5) — each VCO's own waveform morph: sine at the
bottom, through saw at the middle, to square at the top, blending continuously rather than switching.
Independent per VCO.

**Phase mod 1 / 2 / 3** (`PM1`/`PM2`/`PM3`, slots 6–8) — each VCO's own phase-modulation depth, driven
by that VCO's own internal sine LFO (no cross-VCO modulation). Below a small floor near 0 it's silent;
above it, it grows from a subtle vibrato into an increasingly warbly, FM-like wobble at full depth. All
three VCOs' LFOs share one rate (see PM rate, slot 12).

**Ring mod 1 / 2 / 3** (`RM1`/`RM2`/`RM3`, slots 9–11) — each VCO ring-modulates against its *own*
internal carrier oscillator (20 Hz–5 kHz), never another VCO's signal. Has a genuine zero at the very
bottom of its travel — below a small floor, ring mod is completely off, not just quiet — then blends in
more of the metallic ring-modulated product as it's raised, fully replacing the dry tone at maximum.
Defaults to 0 (off), so a fresh app sounds unchanged from before Ring mod existed.

**PM rate** (`PMrt`, slot 12) — one shared knob (0.05 Hz–20 Hz) setting the phase-mod LFO rate for all
three VCOs at once. Previously each VCO's own PM depth knob doubled as its rate; that coupling is gone —
depth and rate are now fully independent.

**VCO balance** (`VBal`, slot 13) — a single tilt sweeping mix emphasis from VCO1 (bottom of travel)
through an even three-way split (center, the default) to VCO3 (top of travel). By construction, every
VCO always keeps at least 10% and never exceeds 80% of the mix — this knob can shift emphasis but can
never silence a VCO outright.

---

## Envelope bank

Per-voice Attack/Decay/Sustain/Release for each of the three VCOs, interleaved: slot = 4×VCO index +
{Attack, Decay, Sustain, Release}. So the slot order reads A1 D1 S1 R1, A2 D2 S2 R2, A3 D3 S3 R3, then
two knobs shared across all three voices.

**Attack VCO1/2/3** (`A1`/`A2`/`A3`, slots 0/4/8) — time for that VCO's level to rise to full once the
transport's envelope gate opens (0.5 ms–1 s). Defaults to the floor (fastest, essentially instant-on).

**Decay VCO1/2/3** (`D1`/`D2`/`D3`, slots 1/5/9) — new. Time for that VCO's level to fall from the
Attack peak down to its Sustain level (0.5 ms–1 s).

**Sustain VCO1/2/3** (`S1`/`S2`/`S3`, slots 2/6/10) — the level held while the gate stays open. Floored
at 10% — it can never be modulated down to a true, silencing zero — and defaults to full level (100%) so
a freshly launched app makes sound without touching any knob.

**Release VCO1/2/3** (`R1`/`R2`/`R3`, slots 3/7/11) — time for that VCO's level to fall to silence once
the gate closes (0.5 ms–2.5 s). Defaults to the floor (fastest). Stop always overrides this with a fast
~50 ms fade regardless of this knob's position.

**Curve** (`Curv`, slot 12) — new, shared across all three voices. Reshapes every Attack/Decay/Release
ramp from a straight linear ramp (bottom of travel, the default — bit-identical to the ramp shape before
Curve existed) toward an increasingly "slow start, fast finish" ease-in curve at the top. Sustain, being
a level rather than a ramp, is unaffected.

**Grace** (`Grac`, slot 13) — new, shared across all three voices. A minimum-hold: once a note reaches
Sustain, Grace keeps it there for at least this long (0–1 s) before honoring a gate-close, so a very
short gate pulse can't cut a note off mid-way through its own Attack/Decay. At its default (0) it's an
exact no-op — a note behaves exactly as it did before Grace existed, cutting to Release the instant the
gate closes. Grace never changes the length of Attack, Decay, or Release themselves — it only delays
*when* Release is allowed to start.

---

## Filter bank

Signal path (at the default Topology): input feeds a resonant peaking EQ ("peak") and, in parallel, a
short pure delay into a comb filter; the two paths blend, then a resonant notch ("Scoop") is dipped in
before this bank hands off to Drive/Delay/Reverb downstream (see the Reverb bank's opening note for the
actual per-sample processing order, which differs from this bank's own visual left-to-right slot order).

**Comb offset** (`CmbOff`, slot 0) — a short pure delay ahead of the comb (1 ms–100 ms). Smears the
comb's attack transient without changing the comb's own pitch.

**Peak freq** (`PkFreq`, slot 1) — center frequency of the resonant peaking EQ, 20 Hz–20 kHz.

**Peak gain** (`PkGain`, slot 2) — height of that peak's boost. Flat (no boost) at the bottom of travel;
up to +6 dB (2×) at the top.

**Peak Q** (`PkQ`, slot 3) — width/resonance of the peak: a wide, gentle bump at the bottom, a narrow,
ringing resonance at the top.

**Comb delay** (`CmbDly`, slot 4) — the comb filter's own delay time, expressed as a pitch (20 Hz–10 kHz)
— this sets the comb's characteristic pitched ringing.

**Comb feedback** (`CmbFb`, slot 5) — comb resonance. Neutral (no resonance) at the exact center of
travel; pushing toward either end raises feedback up to ±0.95, where the comb rings almost
indefinitely — but always, eventually, decays; it can never truly self-sustain forever.

**Comb LP** (`CmbLP`, slot 6) — a low-pass filter inside the comb's own feedback loop. Turning it down
darkens/dampens the comb's repeats faster; turning it up brightens and sustains them longer, up to
20 kHz.

**Comb/Peak** (`Cmb/Pk`, slot 7) — blend between the peak path and the comb path (0 = pure peak, 1 =
pure comb).

**Scoop** (`Scoop`, slot 8) — how deep the resonant notch dips (0 = no dip, up to roughly a 95% deep dip
at maximum). This is the notch's own height/depth; whether the notch is audible in the output at all is
a separate control (Scoop depth, slot 13).

**Topology** (`Topo`, slot 9) — new. A **continuous morph**, not a switch, between running the comb path
and the peak **in parallel** (bottom of travel, the default — the comb's output is not fed into the
peak, bit-identical to how this bank always behaved before Topology existed) and running them fully **in
series** (top of travel — the comb's output becomes the peak's own input, so the peak now further shapes
an already comb-colored signal). Every value in between blends smoothly.

**Scoop freq** (`ScFq`, slot 10) — new. The notch's own center frequency (20 Hz–20 kHz), now independent
of Peak freq — it used to just copy Peak freq's value.

**Scoop width** (`ScWd`, slot 11) — new. The notch's own Q/width, now independent of Peak Q.

**Comb drive** (`CDrv`, slot 12) — new. Pre-gain (0.25×–4×) into the comb's own saturator; unity gain at
the center default. Raising it pushes the comb's ringing into progressively harder, more distorted
saturation without raising the hard ceiling the saturator still enforces on the comb's output level.

**Scoop depth** (`ScDp`, slot 13) — new. How much of the notch is actually blended into this bank's final
output — independent from the notch's own height (Scoop, slot 8). At 0, the notch is still computed but
has no audible effect at all; at 1, it's fully applied.

---

## Drive bank

Signal path: 2×-oversampled polynomial waveshaper (with a sine-fold/tanh-fuzz blend) → digital
reorganizer (bit XOR + bit-scramble) → two sample-rate reducers in series → a tone low-pass → dry/wet
Blend with an allpass Phase stage on the wet side.

**Drive** (`Drive`, slot 0) — input gain into the polynomial waveshaper (1×–5×). Higher drive pushes the
shaper into denser, more extreme harmonic territory.

**Shape** (`Shape`, slot 1) — recomputes the waveshaper's five polynomial coefficients along a
space-filling curve, continuously changing its harmonic character. (Distinct from the Audio bank's
per-VCO waveform Shape knobs — same name, different control.)

**SRR 1** (`SRR1`, slot 2) — first sample-rate-reducer stage. Lower knob values mean more reduction —
heavier, stair-stepped decimation.

**SRR 2** (`SRR2`, slot 3) — a second, identical reducer stage running in series right after SRR 1, for a
second layer of decimation.

**XOR** (`XOR`, slot 4) — an 8-bit XOR mask applied to the (quantized) sample, producing bit-flip
glitching. 0 = no flip.

**Bit depth** (`BitDp`, slot 5) — how many of the sample's low bits the digital reorganizer scrambles.
0 = untouched; higher values add progressively harsher low-bit digital noise.

**Fuzz** (`Fuzz`, slot 6) — blends between the sine-folded wet path (bottom of travel) and a
tanh-style hard saturator (top of travel) inside the waveshaper stage.

**Blend** (`Blend`, slot 7) — crossfades the dry (pre-Drive) signal against the fully processed Drive
chain output. 0 = dry only, untouched by everything above; 1 = fully wet.

**Phase** (`Phase`, slot 8) — a first-order allpass filter on the wet signal, applied *before* the Blend
crossfade above. At Blend 0 this has no audible effect at all, since dry passes through unfiltered
regardless of this knob's position.

**Anti-alias brightness** (`ABrt`, slot 9) — new. Fine-tunes the cutoff of the oversampler's anti-alias
filter within a narrow range around its original fixed point — mostly a brightness trim; center default
reproduces the exact original fixed cutoff.

**Link** (`Link`, slot 10) — new. How strongly the Drive knob's amount couples into (skews) Shape's own
coefficients. Center default reproduces the original fixed coupling exactly; doubling toward the top of
travel unlocks a stronger coupling than the app had before this knob existed.

**Fold** (`Fold`, slot 11) — new. Divisor inside the sine-fold stage (1×–16×). A lower divisor folds
harder/more aggressively; a higher divisor folds more gently. Center default reproduces the original
fixed divisor exactly.

**Tone** (`Tone`, slot 12) — new. A low-pass filter at the very end of the Drive chain. Fully open/exact
bypass at the top of travel (the default); gets progressively darker and more muffled as it's turned
down.

**Waveshaper offset** (`Bias`, slot 13) — new, short name `Bias`. Shifts the waveshaper's input by a
small DC offset (up to ±0.02) before shaping, then removes that same offset from the output afterward —
biases the shaping asymmetrically without adding audible DC. Center default is exactly zero offset (no
change from before this knob existed).

---

## Delay bank

A stereo delay effect, positioned after Filter and before Reverb in the actual audio chain.

**Delay time** (`DlyTm`, slot 0) — base delay length, roughly 1 ms–2 s, exponential.

**Send** (`Send`, slot 1) — how much signal is sent into the delay line at all. At 0, this stage produces
no output — an exact bypass.

**Feedback** (`Fb`, slot 2) — how much of each repeat feeds back for another pass, clamped below 100%
(98% max) so repeats always eventually die out even at maximum.

**Stereo width** (`Width`, slot 3) — cross-feed and time-spread between the left/right taps. At 0 the two
channels behave almost identically; higher values spread the taps further apart in time and blend them
into each other less.

**Detune** (`Detune`, slot 4) — pitches the left and right repeats apart from each other, up to ±50
cents, for a chorus-y, slightly out-of-tune stereo repeat.

**Mod depth** (`ModDp`, slot 5) — amount of a slow LFO wobble on the delay time itself — chorus/vibrato
motion on the repeats.

**Wet mix** (`WetMx`, slot 6) — how much of the delay's wet output continues on toward Reverb.

**Color** (`Color`, slot 7) — folds into and biases Detune (averaged 50/50 with Detune's own value).

**Halo** (`Halo`, slot 8) — folds into and biases Mod depth (averaged 50/50 with Mod depth's own value).

**Feedback drive** (`FbDr`, slot 9) — new. Pre-gain (0.25×–4×, unity at the center default) into the
feedback path's saturator. Raising it drives the repeats into more obvious saturation without raising the
hard ceiling the saturator still enforces.

**Feedback tone** (`FbTn`, slot 10) — new. A low-pass filter inside the feedback loop itself, so
successive repeats get progressively darker over time as this is turned down. Fully open/exact bypass at
the top of travel (the default).

**Mod rate** (`MdRt`, slot 11) — new. Rate of the delay-time LFO that Mod depth (slot 5) controls the
depth of (0.05 Hz–1.25 Hz). Previously fixed at 0.25 Hz; center default reproduces that exact rate.

**Width balance** (`WBal`, slot 12) — new. An overall scalar on how strongly Stereo width's own cross-feed
and time-spread apply. At the top of travel (the default) it reproduces the original fixed stereo
behavior exactly; turning it down narrows the stereo image Width itself can produce.

**Crush** (`Crsh`, slot 13) — new. A sample-rate reducer applied only to the feedback tap (not the dry
signal), so just the repeats get progressively more bit-crushed/lo-fi as this is raised from its off
default (0 = exact bypass, no crushing).

---

## Reverb bank

The signal actually reaches this bank last, after Audio/Envelope, Drive, Filter, and Delay have all
already processed it.

**Wet/dry** (`Wet`, slot 0) — reverb mix, internally capped at 70% wet so at least 30% dry always remains
audible even at maximum.

**Room size** (`Room`, slot 1) — sets both of the tank's internal delay-line lengths; larger room means
longer, more spacious-sounding reflections.

**Decay** (`Decay`, slot 2) — feedback amount inside the tank — tail length. Longer tails at higher
settings.

**Pre-delay** (`PreDly`, slot 3) — time before the input reaches the tank at all, separating a clean dry
transient from the onset of the reverb tail.

**Damping** (`Damp`, slot 4) — high-frequency loss in the feedback path. Higher knob values mean a
brighter, less-damped tail; lower values mean a darker, duller tail.

**Stereo width** (`Width`, slot 5) — spread between the tank's two internal taps in the final left/right
output.

**Diffusion** (`Diff`, slot 6) — cross-feed between the tank's two internal lines. Higher values smear
the two lines into each other more.

**Mod depth** (`ModDp`, slot 7) — depth of a slow sinusoidal wow on the tank's read taps, for chorus-y
movement in the tail. 0 = no movement.

**Hold** (`Hold`, slot 8) — pushes the tank's internal feedback coefficient toward, but never quite to,
self-oscillation — indefinitely extending the tail's sustain without ever letting it hang forever. At 0
it adds nothing beyond ordinary Decay.

**Mod rate** (`MdRt`, slot 9) — new. Rate of the Mod depth LFO (0.07 Hz–1.75 Hz). Previously fixed at
0.35 Hz; center default reproduces that exact rate.

**Tank drive** (`TkDv`, slot 10) — new. Pre-gain (0.25×–4×, unity at the center default) into the tank's
own feedback saturator, for more obvious saturation on the tail as it's raised.

**Grit** (`Grit`, slot 11) — new. Routes the tank's feedback taps through the same bit-scramble/XOR
digital reorganizer used in the Drive bank, adding digital grit/noise to the tail. Exact bypass at 0.

**Tilt** (`Tilt`, slot 12) — new. A bipolar tone control on the final reverb output, crossfading between
a darker (lowpass-emphasized) tail and a brighter (highpass-emphasized) tail around a fixed ~1 kHz
corner. Center default = no change.

**Tuned** (`Tund`, slot 13) — new. A static (non-LFO) offset on the tank's own delay-line lengths, up to
±300 samples around whatever Room size already set. Center default = exactly zero offset. Unlike a pitch
tracker, this does not follow incoming pitch — it's an ordinary hand-tuned control.

---
