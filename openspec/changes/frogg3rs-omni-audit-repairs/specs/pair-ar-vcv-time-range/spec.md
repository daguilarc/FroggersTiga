# Delta — `pair-ar-vcv-time-range`

## REMOVED Requirements

### Requirement: Pair-AR time constants match VCV Fundamental ADSR display range
**Reason**: `PairArEnvelope` was a desktop-simulator engine extension, added in `4e3d0a3`, that no firmware host ever wired. `MixOscVoices` on hardware has always been the plain average of the three VCOs; deleted from `src/core`.
**Migration**: None.

### Requirement: Level-follower semantics unchanged
**Reason**: The follower semantics belonged to `PairArEnvelope`/`AudioPairArState`, added in `4e3d0a3`, that no firmware host ever wired. Deleted from `src/core` along with the envelope it described.
**Migration**: None.

### Requirement: Manual documents time range and follower behavior
**Reason**: There is no pair-AR time range or follower behavior left to document; the envelope was a desktop-simulator engine extension, added in `4e3d0a3`, that no firmware host ever wired, and is deleted from `src/core`.
**Migration**: None.
