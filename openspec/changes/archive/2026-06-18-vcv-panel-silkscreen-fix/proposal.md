> **Superseded by `omni-repository-harmonization`.** Remaining silkscreen/panel work is absorbed into omni host-display authority and VCV panel regeneration. Archive with `--skip-specs`.

## Why

VCV Rack panels ship with gray faceplates and widgets but **no visible silkscreen text** at 100% zoom. Users only see labels on hover (Rack param/port tooltips). That violates the field-parity goal and the vcv-rack-panel-ux spec (“readable without hover”).

Root cause: panel SVGs use `<text>` elements, which **Rack’s SVG renderer does not draw** ([VCV Panel manual — SVG limitations](https://vcvrack.com/manual/Panel)). The v2.4 panel-ux change marked “labels on SVG” tasks complete without a Rack visual verification gate.

**Branding gap (user report):** There is no **FroggersTiga header with a tiny frog logo** on the module faceplate — the same identity strip used on web and desktop. vcv-rack-panel-ux only specified a bottom-corner Comic Sans watermark (no logo, no top header). Even that watermark is invisible in Rack because it is live SVG `<text>`.

## What Changes

### Product header (required)

Each VCV module (main 72 HP + FX 36 HP) SHALL have a **top header strip** matching web/desktop identity:

| Element | Web reference | VCV target |
|---------|---------------|------------|
| Frog logo | `web/public/froggerstiga.png` (48×48 px in header) | Tiny logo, ~6–8 mm height, top-left, clear of screws |
| Product name | `<h1>FroggersTiga</h1>` beside logo | “FroggersTiga” path text beside logo, readable at 100% zoom |
| Placement | Top of app shell (`web/index.html` `.app-header`) | Top of panel faceplate, not bottom corner |
| Assets | Same PNG as `desktop/Resources/Icon.png` | Copy/trace into `vcv/res/`; Rack-visible (paths or verified `<image>`) |

Same header on **main** and **FX** modules. Reserve ~10–12 mm panel height; shift column titles and mod-rack labels down.

**Reject:** bottom-corner Comic Sans `<text>` watermark only; logo-less branding; hover-only identification.

### Silkscreen labels (required)

- Convert all panel silkscreen from live SVG `<text>` to **rendered paths** (or equivalent Rack-visible drawing), sourced from `ParamDisplayNames`
- Column titles, row labels, mod-rack names, CC/MIDI/I/O port names visible on faceplate at 100% zoom without hover

### Process (required)

- Add a **Rack smoke gate**: open each module at 100% zoom; header + all functional labels must be visible before panel tasks are marked done
- Update `generate_panels.py`: parse `ParamDisplayNames.hpp` + `VcvPanelLayout.hpp`; emit path-based labels via `fontTools`; layout anchors mirror `FieldParityWidget`
- Add `sim/check_vcv_panel_svg.sh` CI gate (no live `<text>` in `vcv/res/FroggersTiga*.svg`)
- Remove stale `FroggersTigaVoicing*.svg`
- Document the nanosvg limitation in `vcv/DEVELOPMENT.md` so it is not repeated
- Postmortem handoff for successor agent (see `design.md`)

## Capabilities

### New Capabilities

- `vcv-panel-silkscreen`: Visible faceplate branding (frog logo + FroggersTiga name) and functional labels on main and FX modules at 100% zoom without hover

### Modified Capabilities

- (none in `openspec/specs/` — VCV is local-only; **supersedes** vcv-rack-panel-ux corner-watermark branding requirement)

## Impact

- `vcv/res/*.svg` (regenerated with path text + header art)
- `vcv/res/frog_logo.*` or traced path asset from `web/public/froggerstiga.png`
- `vcv/scripts/generate_panels.py` (+ optional Inkscape/text-to-path build step)
- `sim/VcvPanelLayout.hpp` — header strip vertical offset if layout constants need it
- `vcv/DEVELOPMENT.md`, vcv-rack-panel-ux design notes (cross-reference: branding amended)
- Manual Rack verification before calling panel work done

---

**Superseded by:** `vcv-rack-field-parity` (merged with randomize controls + verification gates). Do not apply this change separately.
