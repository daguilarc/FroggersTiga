# FroggersTiga Manual Verification

Human sign-off after automated builds pass. Reference step IDs when reporting failures.

**Active implementation changes:** `desktop-ext-in-fix`, `web-sim-bootstrap-fix`

---

## Desktop Ext. In. + meter (`desktop-ext-in-fix`)

- [ ] Cold launch → **Ext. In.** unchecked by default
- [ ] **Ext. In. on + Play** + speak into mic → peak meter fills; ring mod above Schmidt threshold
- [ ] **Ext. In. on + Play** + line/interface input → meter moves with signal
- [ ] Uncheck **Ext. In.** → meter shows grey idle track (centre tick), VCO-only path
- [ ] No input channels / silent capture → route hint beside meter names the failure
- [ ] Audio Settings → cannot disable all input channels (min 1)
- [ ] Fresh install / reset privacy → permission prompt; grant → input works

## Desktop transport + chrome (`desktop-header-hit-test`)

- [ ] Cold launch → click **Play** → audio runs
- [ ] Click **Audio** → device dialog opens
- [ ] Click **MIDI** → MIDI dialog opens
- [ ] **RECORD** + **OGG** format toggle still work

## Desktop audio export (`desktop-audio-export` §6)

- [ ] Play → Record → play sim → Record → save WAV → file plays in external player
- [ ] Stereo width present in export when delay width active
- [ ] Record without Play shows feedback, no file
- [ ] FLAC/OGG/MP3 on builds with encoders linked
- [ ] Visual: RECORD label not clipped; MIDI | Audio | RECORD order; checkboxes vertical under RECORD

## Desktop patch + randomize (`desktop-host-mutation-safety` §6–7)

- [ ] Gray **SRR1** input → drag → **VCO level** output → cable persists
- [ ] Empty Delay **DTIM** input → drag → **Marbles 1** output
- [ ] Connected plug → void disconnect; reassign rows; fan-out one output → two inputs
- [ ] Global + per-panel **Randomize mod** — no lit jacks without cable; no mod index `{1,2,3}`
- [ ] Play → Randomize mod (all) → audio continues without restart

## Desktop wave + external (`desktop-host-corrections` §6)

- [ ] Play with line in — ring mod works; unplug → VCO path
- [ ] Wave buttons + **Rand waves** during Play — audio never dies permanently
- [ ] Mod rack shows **VCO level**; Marbles step only on **Marbles** press

## Desktop UX polish (`desktop-sim-ux-polish` §6–7)

- [ ] Engine at 44100 (log or delay max time sanity)
- [ ] Patch VCO level → Audio VCO1: knob tracks modulation while playing
- [ ] VCO1/2/3 labels visible at default 1440×720
- [ ] Mod rack scopes read as traces, not sliders

## Stereo delay (`stereo-delay-page` §B–I)

- [ ] Six panels including **Delay** at default size
- [ ] **DTIM** 0–3 s exponential; **DMIX**=0 bypasses wet; **RPRE** ⊥ **DTIM**
- [ ] **DWID** max + headphones → L/R separation
- [ ] Delay **FUEG** scrambles; patch Marbles → **DTIM**
- [ ] Web: page 6/6 Delay; WASM page unchanged on host page 5

## MIDI pitch CV (`desktop-qwerty-midi-pitch-cv` §4)

- [ ] Patch MIDI → knob; **A**, **W**, **P** → three distinct non-zero values
- [ ] **A** alone shows scope activity (not flat gate)
- [ ] Hardware keyboard: velocity changes level

## Help menu (`app-header-help-menu` §5)

- [ ] Desktop: Manual / Quick Dict / License open offline
- [ ] Web: header menu on 390 px; modal scrolls

## Web page UX + chrome (`web-sim-page-ux` §7 + `web-chrome-cohesion` §5)

- [ ] 390 px: no horizontal scroll; transport + strip ≥44 px
- [ ] Play → Marbles → stop: mod meters dim but non-zero
- [ ] Route summary + dropdown show **VCO level**
- [ ] Global strip: **Rand All**, **Rand Mods**, **Rand waves**, **Marbles**
- [ ] `#mod-route-summary` below `#page-chrome`
