# Froggers-on-Sheaf app

## Sheaf pin

`External/Sheaf` is a git submodule, pinned to a commit that carries Sheaf's
out-of-tree app-registration hook for the `sheaf-patch` launcher, which lets
an external app register itself with the launcher without editing anything
inside the submodule.

**No file inside `External/Sheaf` is edited by this project.** The gitlink is
the only change to the submodule's presence in this repo — everything the
hook needs is already upstream at the pinned commit.

The pin must always name a commit that exists upstream. Pointing the gitlink
at a branch that lives only on one machine makes it unresolvable from every
other checkout and blocks the browser publish. A plain-click dispatch path for
Draw nodes is held on a local `froggers-fork` branch for that reason, waiting
to go upstream rather than being pinned here.

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

A full port of the Froggers synth onto Sheaf: the DSP, parameter/bank model,
modulation slate, and portable UI surface live under `app/` alongside this
file; `app/vst/` hosts the VST3/AU plugin build, and `app/browser/` hosts the
browser build and its site/e2e suite. `Main.cpp` is a minimal build-skeleton
translation unit kept separate from the real app entry points
(`Froggers.hpp`/`FroggersMain.cpp`) -- see its own header comment.
