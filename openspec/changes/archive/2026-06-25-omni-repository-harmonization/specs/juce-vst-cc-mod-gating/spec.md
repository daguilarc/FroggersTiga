## REMOVED Requirements

### Requirement: Plugin-hosted CC enable controls
**Reason**: Two fixed hosted CC pairs are replaced by stable host parameters and DAW-owned MIDI mapping.
**Migration**: Map any MIDI channel/CC to an exposed Froggers parameter using the DAW's automation or controller-mapping facilities.

### Requirement: DAW CC respects enable flags
**Reason**: Hosted MIDI no longer enters fixed `CvMidiBridge` latches or uses pair enable flags.
**Migration**: DAW parameter automation applies mapped values through the bounded host-parameter path.

### Requirement: Toolbar MIDI button functional
**Reason**: Hosted mode has no Froggers-specific MIDI Settings dialog.
**Migration**: Use the DAW's parameter mapping UI; the plugin exposes stable target parameters.
