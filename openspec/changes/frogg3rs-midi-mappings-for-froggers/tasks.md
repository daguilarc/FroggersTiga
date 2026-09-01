# Tasks — `frogg3rs-midi-mappings-for-froggers`

PLANNED ONLY. A fresh session preflights this change (small-change rule:
delegated fresh-context preflight), fixes findings, then executes with
sequential bounded executors, my-side postflight per packet, and the
operator's live confirmation before archive. Sheaf PR #13 reserved.

1. Preflight: verify every citation in proposal.md; enumerate consumers of
   `WrldBldrDefaultProfileConfig` / `MfTwisterDefaultProfileConfig` and of
   `toggleRandomMod` / `toggleGestureSelect` / `setGestureSelect`; trace
   how installed mappings persist (name vs index addressing) and where the
   config page's offered-target list renders
   (`ControllersPageUI.hpp` / `MidiConfigViewModel`).
2. Sheaf: the catalog concept + catalog-driven profile construction +
   wizard/config page reading the app catalog + "reset to app defaults";
   sample app takes ownership of its former library tables, its suite
   green unchanged; synth-runtime-ui spec delta; gates: synth suite AND
   the miniapp target (runtime shell), counts reported.
3. frogg3rs: register the catalog (encoder/bank param ops + Randomize
   All/Page, Reset All/Page, Scene 1/2, Scene blend, BPM, Freeze,
   transport, bank select); a test walking every offered target to a real
   dispatch (the routable-control pattern already in the suite); delta to
   `froggers-sheaf-runtime-app`.
4. Branch to fork, upstream PR #13, pin bump own commit, push main, watch
   Pages+VST to green.
5. OPERATOR, deployed site: the MIDI page offers frogg3rs controls, none
   of the sample app's; "reset mappings" lands on frogg3rs defaults; a
   mapped hardware control moves the real thing. Archive on confirmation.
