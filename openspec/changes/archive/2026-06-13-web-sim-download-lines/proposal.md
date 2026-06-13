## Why

The web sim header shows the browser subtitle but gives no path to the native desktop build or project attribution. Users landing on GitHub Pages need download links and license/copyright visible without opening the About menu.

## What Changes

- Add two static lines between the **Browser simulator** subtitle and the Play/Stop transport row:
  1. Desktop download line: `Download the desktop app: macOS | Windows` (linked)
  2. Legal line: `© 2026 JoYo Fresh and Diego Aguilar-Canabal | MIT License` (license linked to GitHub)
- Style as compact meta text within the existing `#app` column (`max-width: 960px`); transport row shifts down slightly.
- No JS changes; links are static constants aligned with `desktop-release-packages` asset names.

## Capabilities

### New Capabilities

- `web-sim-download-lines`: Static download and legal chrome below the sim subtitle, above transport controls.

### Modified Capabilities

- (none)

## Impact

- `web/index.html` — new `.sim-meta` block
- `web/src/style.css` — meta line typography and spacing; adjust `.subtitle` bottom margin
