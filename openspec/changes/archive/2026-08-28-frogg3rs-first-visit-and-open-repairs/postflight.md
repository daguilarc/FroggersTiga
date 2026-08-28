# Postflight — `frogg3rs-first-visit-and-open-repairs`

Implementation compared against the proposal only. Divergences reported
strictly, without reinterpreting intent.

## Divergences

**Section 1 — the proposal's mechanism was WRONG, and the fix is larger than
proposed.** The proposal modelled a race: boot wins, the panel paints, "the
shim reloads once afterwards and the second load works." Suppressing the boot
during that window was therefore expected to be sufficient. It was not — 0/5,
a permanently blank page.

There was no reload. The worker calls `skipWaiting()`
(`coi-serviceworker.js:31`) and `clients.claim()` (`:34`), so on a first visit
it often claims the document before `register()` resolves, leaving it
CONTROLLED and still NOT ISOLATED, because that document's own navigation
response was already served header-less. The shim keyed its reload on
`navigator.serviceWorker.controller` and read that as "isolation holds."

So the shipped fix also corrects the shim's reload condition — removing the
controller checks entirely, since `crossOriginIsolated` is fixed for a
document's lifetime and this branch only runs when it is already false. That
was not in the proposal. It is the actual defect, and the defect was worse than
described: the panel was PERMANENT until a manual reload, not transient.

**Section 2 — outcome is neither (a) nor (b).** The plan named two outcomes and
required one be chosen. Neither can be: the run is VOID as a phone model
because `Emulation.setCPUThrottlingRate` does not reach the AudioWorklet
thread. Audio held 375.9 blocks/s against 375.0 real-time IDENTICALLY at 1x and
20x, and load FELL as throttling rose. Zero samples above 100% at any rate, so
the run never produced the phenomenon either explanation is for. The shipped
one-second window is neither confirmed nor refuted, and only a real phone
(6.3) can settle it.

**Section 3 — built, reverted, rebuilt under an operator decision.** The first
attempt was reverted on the grounds that `criteria::ColumnAlignment` asserts
equal widths within a column and a passing assertion should not be weakened.
The operator overruled that: the criterion is an inferred invariant frozen into
a test, not a stated requirement, so it is what moves. The delta now carries
both the width requirement and a MODIFIED alignment requirement. The criterion
retains its shared-`x` half and drops equal-width only.

`Toggle` was carved out of `FormButton` so the Sync page is untouched — the
operator asked about the button, not the toggles. That is a deliberate scope
limit, not an omission.

**Section 4 — the plan under-estimated headless.** It assumed Chromium could
not grant Web MIDI honestly and instructed asserting only "the reachable half."
Headless CAN: `grantPermissions(["midi", "midi-sysex"])` reaches
`midi:online`. `midi` alone does not, because the manager requests
`{ sysex: true }`. Both assertions shipped instead of one.

**Section 5.2 — scope grew from one symbol to three.** The plan named
`RollingBuffer` and asked whether to fix a `Min()` defect. Enumerating the
batch it arrived with (§7, a named symbol is one member of a family) found
`BoundedAudioBuffer` and `BufferResampler` equally unused. All three deleted
with their 5 tests. There is no `Min()` defect to fix because there is no
`Min()`.

## §7 re-run against the DIFF

| new symbol | sites | verdict |
|---|---|---|
| `window.frogg3rsCoiAttempt` | 1 definition, 1 write, 2 reads | single-sourced |
| `BOOT_ERROR_DETAIL_SELECTOR` | 1 export, 2 importers | single-sourced |
| `ControlStyle::controlWidth` | 1 definition, 1 consumer, 2 declarations | single-sourced |
| `Extent` equal-width check | removed | shared-`x` check retained and still asserted |
| deleted primitives | 0 code references remain | prose in `docs/` and archived changes deliberately kept as historical record |
| `midi-activation.spec.mjs` | no overlap with `desktop-layout`'s status assertion | distinct property |

`.boot-error-detail` is written at four sites — both painters, the stylesheet,
and the test helper. The two painters are pre-existing, documented, and
load-bearing (the inline one cannot import; that is why it exists). The NEW
concept, the test selector, is single-sourced. No duplication was introduced.

## Requirements vs implementation

- `froggers-browser-package`: three ADDED requirements. All three implemented
  and asserted — first visit paints no panel (`first-visit-race.spec.mjs`),
  suppression is bounded by a settleable attempt (the registration-failure
  test, with its own positive control), and both painters honour the same
  condition (`index.html` guard).
- `froggers-sheaf-runtime-app`: two ADDED plus one MODIFIED. Implemented; the
  MODIFIED alignment requirement matches the amended criterion.
- `froggers-modulation-slate`: one ADDED requirement recording the encoder
  grid's own palette. Documentation of an existing deliberate decision; no code
  change, as intended.

## Gates

Recorded in section 7 of `tasks.md` as they run. The Sheaf `make test` target
must NOT be read as the gate: it is one linear recipe and the pre-existing
`braid4_*_deadline*` failures abort it before the portable-UI binaries run, so
its log can read "clean except braid4" while hiding later failures. Run the
binaries directly.
