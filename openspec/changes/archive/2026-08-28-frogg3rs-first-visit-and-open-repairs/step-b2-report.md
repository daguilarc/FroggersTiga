# Step B2 report — the first visit

## The plan's mechanism was wrong, and the first fix made things worse

The proposal said: `site-boot.mjs` boots before the worker controls the page,
the panel is painted, "the shim reloads once afterwards and the second load
works, so the panel is thrown away." Suppressing the boot during that window
was therefore expected to be sufficient.

It was not. Suppression alone turned a false panel into a **permanently blank
page** — 0/5 on the regression test, every run timing out at 45s with the panel
correctly absent and the surface never arriving.

Instrumenting the hang (polling registration state on a throttled first visit,
no other change) showed why:

    839ms  {"iso":false,"pending":true,"ctrl":true,"guard":null,
            "regs":[{"inst":false,"wait":false,"act":true}],"rows":0}
    ...unchanged through 36938ms...

The page **is** controlled (`ctrl:true`) by an **active** worker, is **not**
isolated (`iso:false`), and `guard:null` proves `reloadOnce()` never ran. No
reload was ever attempted, so nothing ever settled the attempt.

The cause is in the shim, and it predates this change. The worker calls
`self.skipWaiting()` on install (`coi-serviceworker.js:31`) and
`self.clients.claim()` on activate (`:34`), so on a first visit it frequently
claims the document *before* `register()` resolves. But that document's own
navigation response was already served without the isolation headers. The
result is a page that is controlled and still not isolated — and the shim's
reload logic was keyed on `navigator.serviceWorker.controller`, which reads
that state as "isolation holds, nothing to do."

So the real first-visit failure is not a slow reload. **There is no reload at
all.** The panel was permanent until someone reloaded by hand. Throttling
simply widens the window in which `claim()` wins, which is why the repro is
load-sensitive and why it looked intermittent.

`crossOriginIsolated` is fixed for a document's lifetime and the shim's page
branch only runs when it is already false, so that document can never become
isolated in place. Once a worker is active, a fresh navigation is the only
remedy — regardless of who controls the page. The controller check was
answering the wrong question and is gone.

## What changed

| File | Change |
|---|---|
| `app/browser/site/coi-serviceworker.js` | Publishes `window.frogg3rsCoiAttempt` synchronously in the page branch, before `register()`. Removes the `controller`-keyed early return and the `controller` check inside `ready.then` — once a worker is active, reload once. |
| `app/browser/site/site-boot.mjs` | Awaits the attempt before booting and before registering its own error backstops. Also corrects the false MIDI comment (task 0.4). |
| `app/browser/site/index.html` | Inline `renderBootError()` returns early while an attempt is pending. |
| `app/browser/e2e/helpers.mjs` | Exports `BOOT_ERROR_DETAIL_SELECTOR`. |
| `app/browser/e2e/first-visit-race.spec.mjs` | New: the regression test and its negative control. |
| `app/browser/e2e/playwright.config.mjs` | Adds the new spec to `PAGES_SPECS`. |

### The published object

`window.frogg3rsCoiAttempt = { pending: <bool>, settled: <Promise> }`, created
synchronously at `coi-serviceworker.js:78-86`, both fields driven by one
`settle()` (`:83-86`) so they cannot disagree. Settled at exactly two places,
both meaning "no reload is coming":

- `:161` — `ready` resolved and `reloadOnce()` returned false (guard already
  fired).
- `:166` — `register()` rejected.

Never settled when `reloadOnce()` actually reloads: the page is being replaced.

Absent entirely when the page branch does not run (the server sent real
isolation headers). Both consumers treat absent as "no attempt owed" —
`site-boot.mjs` via `await window.frogg3rsCoiAttempt?.settled`, `index.html`
via a truthiness check. That is the path the `desktop` and `mobile` projects
run on, and it is unchanged.

### `RELOAD_GUARD_KEY` is single-sourced

    $ grep -rn 'frogg3rs-coi-reloaded' app/browser --exclude-dir=node_modules --exclude-dir=dist
    app/browser/site/coi-serviceworker.js:101

One site. Neither `site-boot.mjs` nor `index.html` re-derives it.

## Results

**Regression test, 5x each, `--workers=1`, uncontended.**

- Before the shim's reload logic was corrected: **0/5** — panel absent, surface
  never rendered, 45s timeout every run.
- After: **5/5 passed (21.1s total).**

The RED baseline for "the panel appears on a first visit" is step B1's, which
reproduced it **10/10** against pre-fix bytes with the exact
`SharedArrayBuffer transfer requires self.crossOriginIsolated` text. It was not
re-derived here, because `dist/site` had already been re-assembled with the fix
and re-deriving it would have meant reverting shipping code.

**Full suite: 49/49 passed (17.6s).** 47 pre-existing + 2 new. The previously
failing `[pages] blank-frame.spec.mjs` now passes. `desktop` and `mobile`
(isolation headers present, published object absent) are unaffected.

### Task 1.6 — the negative control, with its own positive control

`a page whose service worker cannot register still reports the failure`
overrides `navigator.serviceWorker.register` to reject, which drives the
`.catch` → `settle()` path. Boot proceeds, the un-isolated SharedArrayBuffer
transfer throws for real, and the panel must appear. **Passes (1.3s).**

A test asserting a panel *appears* proves nothing unless it can fail, so the
instrument was verified live: `settle()` was removed from the `.catch`, the
site re-assembled, and the test went **red**; restoring it returned it to
green. `grep -c TEMP-BREAK` is 0 in both `site/` and `dist/site/`.

It reaches ONE painter — `site-boot.mjs`'s `fail()`, via `boot().catch`.
`index.html`'s `renderBootError()` is a backstop for errors outside that
promise chain, and a registration failure does not produce one, so this test
does not exercise it. Said plainly rather than claimed.

### Task 1.4 — what the page shows during the window, measured

Polled every 250-300ms from navigation commit:

| | unthrottled | CPU throttle 20x |
|---|---|---|
| isolated (reload landed) | 330ms | 592ms |
| surface rendered | 584ms | 2517ms |
| header + footer visible | throughout | throughout |
| boot-error panel | never | never |

**The page is never blank.** The site shell — heading, footer, download and
manual links — is server-rendered HTML that paints immediately and stays. Only
`#synth-root` is empty (`childElementCount: 0`, height 0) for ~0.6s
unthrottled and ~2.5s under 20x throttling, then fills in one step.

So a neutral starting state is NOT needed, and that is a measurement rather
than an argument. Nothing was added.

## §7 — selectors

- `.boot-error-detail`: FOUND 2 prospective sites (regression + negative
  control). CHANGED to 1 definition, `BOOT_ERROR_DETAIL_SELECTOR`
  (`helpers.mjs:79-85`), imported by both.
- `[data-synth-node-id="froggers.layout.right.row.0"]`: the B1 repro hardcoded
  it, duplicating `ENCODER_ROW_SELECTORS[0]` (`helpers.mjs:15`). FOUND 2,
  CHANGED to 1 — the spec now uses `waitForSurfaceReady` from `helpers.mjs`.

## Task 0.4 — the false MIDI comment

Before (`site-boot.mjs:37-38`):

> Browser MIDI still has no path in -- it arrives only with a lease -- and
> remains unreachable here.

After: states that `main.ts:178` constructs `BrowserMidiManager`
unconditionally, that the dispatch wiring calls `startUserActivation()` after
every action (`main.ts:174-177`, `:267-276`), and that this reaches
`navigator.requestMIDIAccess({ sysex: true })` (`midi.ts:108-120`) on the same
first-in-app-action path as audio — so MIDI is reachable without a lease.

## Divergences from the brief

1. **The brief's design listed three settle exits. There were four**, and the
   fourth — `ready` resolves while the page is already controlled — was the one
   the bug actually took. It is not in the final code because the controller
   check it depended on was removed as wrong.
2. **The brief treated the shim's reload logic as correct and only the boot
   path as racing.** Correcting the shim was necessary; suppression alone made
   the failure worse, not better.
3. The RED count was not re-derived in this step (see Results).
4. Work was taken over from a subagent that stalled repeatedly on backgrounded
   runs. Its edits to the three site files and the two e2e files are kept; the
   shim's reload-logic correction, all verification, and the 1.4/1.6 work were
   done directly.
