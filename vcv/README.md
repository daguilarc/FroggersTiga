# FroggersTiga — VCV Rack plugin (GPL wrapper)

This directory is the **GPL-3.0-or-later** license boundary for VCV Rack 2. All Rack SDK code lives here only. DSP comes from the MIT-licensed [`../src/core/`](../src/core/) tree.

See [`LICENSE_BOUNDARY.md`](LICENSE_BOUNDARY.md) for the MIT/GPL split.

## Build (requires Rack SDK)

```bash
# Clone SDK once, e.g. next to this repo:
# git clone https://github.com/VCVRack/Rack-SDK.git ~/Rack-SDK

export RACK_DIR=~/Rack-SDK   # path to Rack-SDK checkout
cd vcv
./build.sh
```

Install the built plugin:

```bash
cp -r dist/FroggersTiga ~/.local/share/Rack2/plugins/FroggersTiga/
```

(`RACK_USER_DIR` may differ on macOS — use Rack’s “Open user folder” if needed.)

## Status

Minimal audio-through wrapper using `PagedHostIO` + `FroggersEngine`. Full Field-parity jacks and UI are §6.3 in `openspec/changes/sim-hosts-multi-ui/tasks.md`.
