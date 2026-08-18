# Proposal — `frogg3rs-bank-carousel-arrows`

**Created 2026-08-18 at the operator's instruction**, from a screenshot of the
built app: back and forward arrows, centered between the page-bank buttons
(Filter/Drive/Delay…) and the top row of encoders, that step through the bank
carousel. Clicking them changes the page, and the change is reflected in which
bank button is highlighted.

## Why

Bank navigation today is direct-selection only: six bank buttons, one click
each (`kBankSelect` with a target index; no next/previous mechanism exists
anywhere in `app/` — verified by grep, 2026-08-18). Stepping through banks in
order — the natural way to browse the whole instrument — takes aimed clicks at
six different targets. A back/forward arrow pair in the band between the bank
row and the encoder grid gives one-target-per-step browsing.

**Spec constraint honored, not fought:** `froggers-app-surface-layout`'s
"Bank selector with direct selection" requirement rules that arrow-based
paging SHALL NOT be the *primary* navigation. The arrows are secondary: the
six bank buttons remain, unchanged, and the arrows route through the same
single selection authority (`RequestBankSelect`). The delta modifies that
requirement to carve in the secondary role explicitly.

## What Changes

- **froggers-app-surface-layout** (one MODIFIED requirement):
  - "Bank selector with direct selection" gains: the surface SHALL also
    provide a back/forward arrow pair, horizontally centered in the band
    between the bank row and the encoder grid (the modulation-header row's
    reserved space), that steps the active bank previous/next with
    wrap-around, through the same selection authority as the buttons; the
    bank-button highlight SHALL reflect arrow-driven changes identically to
    button-driven ones. While a modulation drill-in is active (the band shows
    "Modulation Level N"), the arrows SHALL not render and SHALL not accept
    input; the band's geometry SHALL not change in either state.

## Impact

- Affected specs: `froggers-app-surface-layout`.
- Affected code (repo-root relative): `app/FroggersUiSurface.hpp`
  (`AppendModulationHeaderRow`, `FroggersNodeIds`, `FroggersActions`,
  `HandleAction`), `app/FroggersSurfaceTests.cpp`. No engine/audio-thread
  change: the arrows reuse `RequestBankSelect` (`app/FroggersAppCore.hpp`),
  which already owns bank selection end to end.
- Backward compatibility: no new vertical extent — the arrows live inside the
  existing 26px modulation-header row, whose outer geometry is untouched in
  both drill states (the row's own "only this row's content changes" contract,
  `FroggersUiSurface.hpp:1270-1277`). Bank buttons, encoder grid, and all
  sibling geometry are bit-identical.
- Delivery: single change, one commit per task group on the current branch,
  operator acceptance in the built app before archive.
