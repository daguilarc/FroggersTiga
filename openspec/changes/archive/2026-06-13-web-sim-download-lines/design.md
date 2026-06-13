## Context

```
#app (max-width 960px, centered)
├── header.app-header
│   └── subtitle: "Browser simulator — …"
├── [NEW] .sim-meta (2 lines)
├── .controls-top (Play / Stop / External / status)
└── … rest of sim column unchanged
```

Desktop release assets (from `desktop-release-packages`): `FroggersTiga-1.0.0-macOS.dmg`, `FroggersTiga-1.0.0-Windows-Setup.exe` on GitHub Releases for repo `daguilarc/FroggersTiga`.

## Data Flow

| Stage | Input | Transform | Output |
|-------|-------|-----------|--------|
| Markup | static URLs + copy in `index.html` | none | two `<p>` lines in `.sim-meta` |
| Layout | `#app` flex/block stack | CSS margin on `.sim-meta`, reduced `.subtitle` margin | transport row pushed down ~2 lines |
| Links | GitHub release + LICENSE URLs | `target="_blank" rel="noopener noreferrer"` | macOS DMG, Windows Setup, MIT License |

No runtime state. OMNI: one markup block, one CSS block — no duplicate copy in TS.

## Goals / Non-Goals

**Goals:**

- Two lines visible on all viewports between subtitle and Play/Stop.
- Preserve centered single-column layout (`#app` max-width unchanged).
- macOS / Windows links point at GitHub Release download assets.
- MIT License links to `LICENSE` on GitHub main branch.
- Use literal `©` (not `(C)`).

**Non-Goals:**

- Dynamic version in download URLs (static `1.0.0` until a follow-up reads CMake or releases API).
- Desktop app changes.
- Moving About menu or help modal.

## Decisions

### D1: Static HTML block `.sim-meta`

**Choice:** Insert `<div class="sim-meta">` with two `<p>` elements immediately after `</header>`, before `.controls-top`.

**Why:** Copy is fixed marketing/legal text; no WASM or build-time injection needed.

**Alternative rejected:** Vite env vars — overkill for three URLs and one version string.

### D2: Download URL pattern

**Choice:**

- macOS: `https://github.com/daguilarc/FroggersTiga/releases/latest/download/FroggersTiga-1.0.0-macOS.dmg`
- Windows: `https://github.com/daguilarc/FroggersTiga/releases/latest/download/FroggersTiga-1.0.0-Windows-Setup.exe`
- License: `https://github.com/daguilarc/FroggersTiga/blob/main/LICENSE`

**Why:** Matches packaging filenames from `desktop-release-packages`; `/releases/latest/download/` resolves after first tag push.

### D3: Spacing

**Choice:** `.subtitle { margin-bottom: 0.5rem }` (was `1rem`); `.sim-meta { margin: 0 0 1rem }`; `.controls-top` margin unchanged.

**Why:** Net vertical space ≈ previous subtitle gap; transport row moves down by meta block height only.

### D4: Link styling

**Choice:** `.sim-meta a { color: var(--knob) }` — same accent family as existing chrome; no underline by default, underline on `:hover`.

**Why:** Readable on dark background without competing with Play/Stop buttons.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Release not published yet → 404 on download links | Document in tasks: push `desktop-v1.0.0` before expecting live downloads |
| Version bump in CMake without HTML update | Follow-up can centralize version; v1 uses static `1.0.0` matching current CMake |
| Long line wraps on narrow mobile | `font-size: 0.8rem`; natural wrap within `#app` padding |

## Migration Plan

1. Land HTML + CSS.
2. Verify locally at 375px and 1440px widths.
3. Deploy via existing Pages CI on merge to `main`.

## Open Questions

- (none)
