# Tasks — `frogg3rs-docs-only-pushes-skip-builds`

- [x] 1. Add `paths-ignore: ['openspec/**', 'README.md']` under `push:` in
  `pages.yml` and under both `push:` and `pull_request:` in
  `vst-plugin.yml`. Touch nothing else in either file.
- [x] 2. Commit and push (this push changes workflow files, so both builds
  SHOULD run — that is the positive control's first half). Pushed as
  `b70db29`.
- [x] 3. Positive control, both directions, observed: `b70db29` (workflow
  files touched, filter present) started both Pages and VST runs at
  2026-08-31T07:07:32Z; `30e60d5` (openspec/** only) started nothing —
  `gh run list` shows no run for that sha while the earlier same-kind push
  proves the instrument was live. The list was then narrowed to
  `['openspec/**', 'README.md']` (operator ruling); `30e60d5`'s quiet proof
  transfers, since it touched only `openspec/**`, which both lists ignore.
- [ ] 4. Confirm the `frogg3rs_vst` tag trigger and `workflow_dispatch`
  still fire on demand the next time either is used; path filters do not
  apply to tags, so no pre-verification run is spent on this.
