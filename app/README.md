# Froggers-on-Sheaf app

## Sheaf pin

`External/Sheaf` is a git submodule pinned at **`1940ddcb`**, superseding the
earlier pin `dafa54b6`. The bump was taken to pick up Sheaf's out-of-tree
app-registration hook for the `sheaf-patch` launcher (design D1a in
`openspec/changes/froggers-sheaf-app/design.md`), which lets an external app
register itself with the launcher without editing anything inside the
submodule.

**No file inside `External/Sheaf` is edited by this project.** The gitlink is
the only change to the submodule's presence in this repo — everything the
hook needs is already upstream at the pinned commit.

This held, was briefly broken, and now holds again: on 2026-07-27 the submodule
spent part of a day on a local branch `froggers-fork` carrying a plain-click
dispatch path for Draw nodes. Because those commits existed only on one
machine, the gitlink was unresolvable from any other checkout and the browser
publish was blocked, so the fork was reverted. The work is preserved on the
local `froggers-fork` branch for jvictor0 to upstream.

**Consequence you will notice:** encoder press and Play/Stop are **double-click**,
because `Node::doubleClickAction` is the only action hook stock Sheaf dispatches
for Draw nodes at this pin. When plain-click support lands upstream, flip
`SetNodeAction`/`SetNodeActionAndLabel` in `FroggersUiSurface.hpp` and the
matching assertion in `FroggersSurfaceTests.cpp` back to `node.action`.

## Build the launcher

```bash
./app/build-launcher.sh
```

Do not paste a hand-written `make` command. `EXTRA_APP_HEADERS` must list every
header the app compiles — the sheaf-patch Makefile treats them as literal
prerequisites (`Makefile:47-48`) and generates no `-MMD` dependency files, so an
unlisted header is silently untracked and edits to it produce a build that
succeeds while ignoring the change. A hand-written list once tracked 4 of 18
headers, missing all of `app/dsp/`. The script globs so it cannot rot.

## Status

This is packet 1 of the `froggers-sheaf-app` change: a build skeleton only.
It proves that this app's include paths resolve against Sheaf's synth-core
headers and that linking against Sheaf's `libsynth.a` works. It does not yet
contain any Froggers application code, JUCE integration, or launcher
registration — those land in later packets.
