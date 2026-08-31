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

Add `paths-ignore: ['openspec/**', '**.md']` to `pages.yml`'s `push`
trigger and to `vst-plugin.yml`'s `push` and `pull_request` triggers.
An ignore list is chosen over a `paths` allow list deliberately: a forgotten
allow-list entry silently skips a real deploy, while a stale ignore merely
wastes a build.

## Why the ignore list is safe, checked per consumer

- No `.md` is an input to the Pages build: the site links to `MANUAL.md` on
  GitHub (`app/browser/site/index.html:79`) rather than bundling it, and
  `pages.yml` reads no markdown.
- `vst-plugin.yml` reads `MANUAL.md` only in its release job
  (`vst-plugin.yml:253-260`), which runs on the `frogg3rs_vst` tag — and
  GitHub does not evaluate `paths`/`paths-ignore` for tag pushes, so the
  release trigger cannot be filtered out.
- `MANUAL.md` and `QUICK_DICT.md` are bundled into the desktop app
  (`app/build-launcher.sh:72-73`), but that ships through
  `desktop-release.yml`, which gains no filter.
- `labels.md` is the authority for label TEXT by transcription into
  `FroggersApprovedLabels()`; the build consumes the code, not the file.

## Impact

`.github/workflows/pages.yml`, `.github/workflows/vst-plugin.yml`, this
change directory, and a delta to `frogg3rs-distribution`. Proof is
behavioural: one docs-only push that triggers nothing, after a
workflow-touching push that triggered both.
