## REMOVED Requirements

### Requirement: VCV CC ingest through host CvMidiBridge
**Reason**: Froggers Tiga VCV is now CV-only; MIDI decoding belongs to a separate Rack MIDI-to-CV module.
**Migration**: Patch the external module's CV output into the desired Froggers parameter CV input.

### Requirement: Single CvMidiBridge on PagedHostIO
**Reason**: VCV no longer ingests or emits MIDI and therefore has no VCV bridge ownership requirement.
**Migration**: Keep shared bridge code only for hosts that support fixed MIDI CC inputs; VCV uses a host-source policy that excludes indices 0 and 1.

### Requirement: VCV CC enable toggles
**Reason**: There are no Froggers-owned MIDI CC inputs to enable in VCV.
**Migration**: Remove CC toggles/lights and use Rack cable presence plus the external source module's controls.

### Requirement: VCV assignment respects core availability
**Reason**: VCV no longer assigns MIDI CC mod sources; its internal source pool contains only indices 4, 5, and 6.
**Migration**: Reject indices 0 and 1 through the VCV host-source policy and apply parameter-jack voltage directly.
