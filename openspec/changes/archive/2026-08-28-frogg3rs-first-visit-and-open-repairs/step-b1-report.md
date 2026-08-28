# Step B1 -- deterministic repro of the first-visit COI boot race

## 1. Mechanisms tried

Only mechanism 1 was needed; it worked on the first attempt, so mechanisms
2-4 were not attempted.

**1. CPU throttling over a CDP session -- REPRODUCES, 10/10.**
`context.newCDPSession(page)` + `client.send("Emulation.setCPUThrottlingRate",
{ rate: 20 })`, applied before `page.goto("/")` against the `pages` project
(no isolation headers, port 8800). Run as two batches of 5
(`--repeat-each=5`): **5/5**, then **5/5** (10/10 total). Exact failure text
every time:
```
Failed to execute 'postMessage' on 'Worker': SharedArrayBuffer transfer requires self.crossOriginIsolated.
```
That contains the required substring inside `.boot-error-detail`.

**Negative control -- same first-visit navigation, no throttling.** Two
batches of 5: first batch **3/5 passed, 2/5 failed**; second batch (minutes
later) **5/5 passed**. Both failures in the first batch showed the
IDENTICAL panel and text as the throttled repro -- not a different symptom.
This machine is shared dev hardware with ambient load from other work this
session, so the race was already visible unassisted, matching the shim's
own comment (`coi-serviceworker.js:110-111`, "That window is wider the
busier the machine is"). CPU throttling reproduces the same, pre-existing
race on demand, rather than a different failure.

## 2. The winning repro

- **File:** `app/browser/e2e/first-visit-race.spec.mjs` (new)
- **Command** (from `app/browser/e2e/`):
  `npx playwright test --project=pages -g "CPU throttling" --repeat-each=5 --reporter=list`
- **Why it works:** `coi-serviceworker.js`'s page branch
  (`app/browser/site/coi-serviceworker.js:63-124`) is a synchronous classic
  script kicking off an async `register` -> `ready` -> conditional
  `reload()` chain on the browser's own internal SW lifecycle. `site-boot.mjs`
  is a deferred module that still has to fetch its own static imports
  before `boot()` reaches `installSynthBrowserApp` and starts the
  Emscripten pthread pool. On an idle machine the SW lifecycle
  (cross-process browser IPC) is usually slower than serving those imports
  off loopback disk, so the reload normally fires and interrupts the boot
  before the pthread pool starts -- the page comes back controlled and
  isolated, boots clean, and looks like a normal first visit to the test
  (only one navigation is asserted, so a self-reload mid-test is invisible
  to it). CPU throttling slows script-driven work (module fetch/eval,
  `Response` cloning in the SW fetch handler) but not the browser's own
  multi-process SW handshake, which was already the slower leg --
  widening the same gap the shim's comment describes until the pthread
  pool reliably wins the race. `rate: 20` was the first magnitude tried
  (per the prompt's suggestion) and produced 10/10; not tuned further
  since a reliable repro, not a minimal value, was the goal.

## 3. Positive-control evidence

Captured via `console.log` and independently via Playwright's own
`error-context.md` snapshot on a baseline-run failure. Both show:
```
Failed to execute 'postMessage' on 'Worker': SharedArrayBuffer transfer requires self.crossOriginIsolated.
```
`.boot-error-heading` read "frogg3rs could not start in this browser." in
every case, matching `fail()` (`site-boot.mjs:61-79`) -- the module's own
`unhandledrejection`/boot-catch path, not `index.html`'s inline
`renderBootError()` (which would render identical text had it fired
instead, i.e. on a pre-module-load failure).

## 4. First-visit guarantee

Every Playwright `test()` gets a brand-new `BrowserContext` by default,
so `sessionStorage` -- and the shim's one-shot `frogg3rs-coi-reloaded`
guard (`coi-serviceworker.js:81`) -- starts empty on every one of the 20
runs above (10 throttled, 10 baseline). No explicit
`sessionStorage.clear()` was added or needed: doing so inside a test would
require a page already loaded, by which point the race has already run
once. Confirmed structurally (`playwright.config.mjs`: `workers: 1`, no
`storageState`, no context reuse) and behaviorally: a carried-over guard
key would make throttled runs alternate pass/fail depending on prior-test
state, which they did not (10/10 identical panel).

## 5. Files created/modified, and final `git status --short`

Created: `app/browser/e2e/first-visit-race.spec.mjs` -- the repro spec
("CPU throttling" test plus a "no throttling (baseline)" negative
control), with a header comment on the mechanism.

Modified: `app/browser/e2e/playwright.config.mjs` -- added
`/first-visit-race\.spec\.mjs$/` to `PAGES_SPECS` (line 37) so the spec
runs under the `pages` project, the only project exercising this shim
path. This is the one config edit the prompt pre-authorized.

Nothing under `app/browser/site/` was touched; no commit, push, checkout,
or stash was run.

```
$ git status --short
 M app/browser/e2e/playwright.config.mjs
 D openspec/changes/frogg3rs-first-visit-and-open-repairs/form-control-width-requirement.md
 M openspec/changes/frogg3rs-first-visit-and-open-repairs/proposal.md
 M openspec/changes/frogg3rs-first-visit-and-open-repairs/tasks.md
 D openspec/changes/frogg3rs-web-release-repair/proposal.md
 D openspec/changes/frogg3rs-web-release-repair/specs/froggers-browser-package/spec.md
 D openspec/changes/frogg3rs-web-release-repair/specs/froggers-modulation-slate/spec.md
 D openspec/changes/frogg3rs-web-release-repair/specs/froggers-sheaf-runtime-app/spec.md
 D openspec/changes/frogg3rs-web-release-repair/tasks.md
 M openspec/specs/froggers-browser-package/spec.md
 M openspec/specs/froggers-modulation-slate/spec.md
 M openspec/specs/froggers-sheaf-runtime-app/spec.md
?? app/browser/e2e/first-visit-race.spec.mjs
?? openspec/changes/archive/2026-08-27-frogg3rs-web-release-repair/
?? openspec/changes/frogg3rs-first-visit-and-open-repairs/preflight.md
?? openspec/changes/frogg3rs-first-visit-and-open-repairs/specs/
?? openspec/changes/frogg3rs-first-visit-and-open-repairs/step-a-report.md
?? openspec/changes/frogg3rs-first-visit-and-open-repairs/step-b1-report.md
```

`playwright.config.mjs` and the new spec file are this task's only
changes; every other line above pre-dates it (the tree's existing openspec
doc changes) and was left untouched, as instructed.

## 6. Where the prompt didn't match the tree

- The prompt cited `app/browser/coi-serviceworker.js:63-121`; the file is
  actually at `app/browser/site/coi-serviceworker.js`, page branch at
  lines 63-124 (124 total lines, not 121; the cited `reloadOnce()` call at
  `:116-118` is at `:115-119` in the real file). Read from the correct
  path; no other discrepancy found in its content.
- Everything else checked out exactly as described: the two panel-painting
  sites, the `sessionStorage` guard key, `PAGES_SPECS` matching only
  `blank-frame.spec.mjs` beforehand, `pagesPort` = 8800, the config never
  rebuilding, `dist/site` existing and current, and both the e2e
  `node_modules` and the Chromium 1223 cache being present.
- Mechanism 1 was flagged as "a first attempt, nothing to copy" -- true:
  `grep -r "newCDPSession\|setCPUThrottlingRate"` under `app/browser` and
  `External/Sheaf` found no prior use outside `node_modules` internals. It
  reproduced without needing mechanisms 2-4.
