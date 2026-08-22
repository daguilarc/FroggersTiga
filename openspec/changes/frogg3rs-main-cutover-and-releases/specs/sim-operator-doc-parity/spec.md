# Delta — `sim-operator-doc-parity`

The capability holds one manual and four generated mirrors in sync. After this
change, nothing reads a mirror: the current app links `MANUAL.md` on GitHub
directly, the browser build copies no markdown, and the published site is
built from that browser output. The manual's own top-to-bottom structure and
the button-label requirements it restates move with `QUICK_DICT.md` and the
product's actual doc surface; this capability itself has no remaining reader
to keep synchronized.

## REMOVED Requirements

### Requirement: Operator docs match ParamDisplayNames
**Reason**: The requirement's own text lists the mirrors it applies to
(`docs/sim-manual.md`, `docs/quick-dict.md`, `web/public/sim-manual.md`,
`web/public/quick-dict.md`) alongside `SIM_MANUAL.md` and `QUICK_DICT.md`.
Those mirrors go with this change; a requirement stated over files that no
longer exist has nothing to check.
**Migration**: None. The row-7 naming fact this requirement protected is
unaffected by the mirror teardown; it simply has no remaining doc copy to
verify it against.

### Requirement: PM3 vs Crispy vs VCO Envelope glossary
**Reason**: Its scenarios name the desktop app's embedded Help (reading
`SIM_MANUAL.md` via `desktop/CMakeLists.txt`'s resource embed) and the web
help modal (fetching `web/public/sim-manual.md`). Both readers go with this
change.
**Migration**: None.

### Requirement: VCO Envelope scope documented
**Reason**: States a fact about operator docs generally with no reader left
to hold to it once the mirrored manual is gone.
**Migration**: None.

### Requirement: Mod blend semantics documented in sim manuals
**Reason**: Lists the same six-file mirror set as the first requirement
above; same disposition.
**Migration**: None.

### Requirement: Mod-then-fuego pipeline documented
**Reason**: States a fact about sim manuals generally with no reader left.
**Migration**: None.

### Requirement: Sim manual learner-first structure
**Reason**: Names `SIM_MANUAL.md` and its mirrors explicitly as the SHALL
subject.
**Migration**: None.

### Requirement: Desktop layout described correctly
**Reason**: States a fact about sim manuals generally with no reader left.
**Migration**: None.

### Requirement: Audio and Random explained in plain language
**Reason**: States a fact about sim manuals generally with no reader left.
**Migration**: None.

### Requirement: Quick Dict desktop v2 boot outcome
**Reason**: Describes desktop v2 standalone launch behavior; the desktop v2
tree is retired in this same change, taking its boot-outcome doc requirement
with it.
**Migration**: None.

### Requirement: Quick Dict desktop v2 carousel page navigation
**Reason**: Describes desktop v2's carousel, retired with the tree.
**Migration**: None.

### Requirement: Help doc mirrors stay synchronized
**Reason**: Requires running `scripts/sync-help-docs.sh`, which this change
removes along with the mirrors it produced.
**Migration**: None. `QUICK_DICT.md` itself stays as the single source; there
is no copy left to sync it to.

### Requirement: v2-operator-center-cluster-docs
**Reason**: Describes desktop v2 layout documentation; the desktop v2 tree
is retired with this change.
**Migration**: None.
