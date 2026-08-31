# Proposal — `frogg3rs-docs-only-pushes-skip-builds`

**Created 2026-08-31.** Every push to `main` rebuilds the site and the VST,
including pushes that touch only documentation or openspec artifacts. The
rebuilds are idempotent, so nothing breaks — they burn runner minutes and
bury meaningful runs in noise.

## The triggers, traced

- `pages.yml:4-5` — `on: push: branches: [main]`, no path filter.
- `vst-plugin.yml:4-8` — `on: push: branches: [main], tags:
  ['frogg3rs_vst']` plus `pull_request: branches: [main]`, no path filter.
- `desktop-release.yml:3-12` — tags and `workflow_dispatch` only; already
  never fires on ordinary pushes. Untouched.

## The fix

Add `paths-ignore: ['openspec/**', 'README.md']` to `pages.yml`'s `push`
trigger and to `vst-plugin.yml`'s `push` and `pull_request` triggers.
An ignore list is chosen over a `paths` allow list deliberately: a forgotten
allow-list entry silently skips a real deploy, while a stale ignore merely
wastes a build.

The list is deliberately NARROW (operator ruling): an ignored path is a
standing claim that the path can never be a build input, and only these two
hold timelessly. `README.md` feeds nothing — the only mentions of it in any
workflow or build script are these ignores. Broader candidates were checked
and rejected: `MANUAL.md` and `QUICK_DICT.md` are bundled into the desktop
app (`app/build-launcher.sh:72-73`) and `MANUAL.md` feeds the VST release
notes (`vst-plugin.yml:253-260`) — those consumers do not run on ordinary
branch pushes today, but ignoring the files would encode that wiring into
the trigger, where nothing would catch it changing. They stay un-ignored,
and their rare edits still rebuild.

Tag pushes are exempt from path filtering by GitHub's own trigger semantics,
so the `frogg3rs_vst` release trigger cannot be filtered out.

## Impact

`.github/workflows/pages.yml`, `.github/workflows/vst-plugin.yml`, this
change directory, and a delta to `frogg3rs-distribution`. Proof is
behavioural: one docs-only push that triggers nothing, after a
workflow-touching push that triggered both.
