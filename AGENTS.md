# Agent instructions — FroggersTiga

## Superpowers skills are for execution and debugging

Do not invoke or follow any `superpowers:*` skill for general exploration,
brainstorming, planning, design, or other creative tasks. In particular,
`superpowers:using-superpowers` and `superpowers:brainstorming` must not activate for
those tasks.

Superpowers skills may be used when the user explicitly asks to debug, diagnose, or
audit something, including read-only debugging without an explicitly requested fix.
They may also be used when the user explicitly asks to execute, implement, or modify
code or project files. Use only the Superpowers skills relevant to the requested work;
do not invoke them merely because a conversation started.

When selecting settings for delegated work, choose the cheapest level of reasoning
effort sufficient for the problem. Describe the choice in terms of effort, complexity,
and risk—not as using a "dumber" or less capable model.

## Desktop releases (mandatory)

**There is exactly one desktop release channel:** [`froggerstiga-v1`](https://github.com/daguilarc/FroggersTiga/releases/tag/froggerstiga-v1).

- Web sim download links in `web/index.html` point at `releases/download/froggerstiga-v1/…` — do not change them to `releases/latest` or any other tag.
- To publish new desktop binaries: move tag `froggerstiga-v1` to `main`, then `git push origin froggerstiga-v1 --force`.
- **Never** create, push, or document tags matching `desktop-v*`.
- **Never** create additional GitHub Releases for desktop.
- **Never** bump CMake/package version for a desktop-only fix unless the user explicitly asks for a new version number.

CI: `.github/workflows/desktop-release.yml` (trigger: `froggerstiga-v*` only). Release notes are rendered from `SIM_MANUAL.md` **Version history** via `desktop/scripts/render-release-notes.sh` — do not enable `generate_release_notes` in the workflow.
