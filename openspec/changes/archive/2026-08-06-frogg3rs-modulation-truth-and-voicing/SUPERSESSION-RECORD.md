# Supersession record — `frogg3rs-modulation-truth-and-voicing`

**Created 2026-08-05. Supersedes `frogg3rs-audio-safety-and-ui-rework`**, archived *superseded,
not done* — the same relationship that change had to `frogg3rs-gui-and-dsp-robustness`.

## Why superseded rather than extended

The predecessor's scope was audio safety + the UI rework + the Sheaf bump it absorbed mid-flight.
On 2026-08-05 the operator ran the F.6 build and reported (a) a modulation-visibility contradiction
(badges everywhere, depths reading zero), (b) filter parameter maxima that still sound wrong —
i.e. a claimed-done §A outcome that did not hold up by ear, and (c) new voicing scope (scene-2
defaults). A change whose §A is titled "do this first, it is why this change exists" cannot absorb
"§A's fix didn't work" as a task inside itself; the predecessor's own doctrine (supersede rather
than patch; record the constraint that lifted or the claim that failed) applies.

## What the predecessor FINISHED — carried as done, not redone

| Scope | State carried |
|---|---|
| §A audio safety | Comb feedback ±0.95, resonant bump 2×, master limiter + backstop, attack 1.0s / release 5.0s, storm test. **Carried but REOPENED IN PART: W2 here exists because parameter maxima still offend the operator's ear. The constants stay until W2's evidence says otherwise — do not pre-emptively "fix" them.** |
| §B / §B-bis UI rounds | Done, visually confirmed across sessions |
| §E randomize + drill-in | Done (`d6298f2`) — **but W1 here reopens its verification**: its tests may assert call counts rather than resulting visible depths |
| §F.0–F.3 | Pin `77a3019e`, migration, single declared 6×6 grid (`962f105`), all green |
| §F.6 | Unified labelled-slider emitter, both labels below, B12 retired (`a7844e6`) |
| §G.1/G.4 | Direct launch (`0b7899d`), window sized from shell component (`3cd9818`) |
| Upstream asks | 1–15 recorded in `/UPSTREAM-SHEAF-ASK.md`; 15 filed as jvictor0/Sheaf#1 |

## Carried OPEN into this change

| Item | Was | Now |
|---|---|---|
| F.4 second pin bump `77a3019e` → `508d9d68` | queued after F.3 | **W4.1** |
| F.5 remove `kExternalAudioOptedIn` | after F.4 | **W4.2** |
| C.2 operator walkthrough / G.3 patch-load check | open | **W5**, still closes only at the operator |
| G.2 blank-window-on-startup-failure decision | open | **W4.3** |
| D.3 voicing (±0.95 / 2× by ear) | open | folded into **W2** |
| D.4 publish pipeline | open, large | carried untouched |
| §H mobile-web UI layer | deferred | carried deferred (cell-map-as-data is the enabler) |
| §I VST layer | deferred | carried deferred |

## Thrown away

Nothing. The predecessor's artifacts remain in place as history; its `tasks.md` §0 execution-order
list is superseded by this change's, and both files carry pointers.
