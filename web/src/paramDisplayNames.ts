/** Mirrors sim/ParamDisplayNames.hpp — client-side label authority before WASM screen updates. */
export const HOST_PAGE_KNOB_LABELS: readonly (readonly string[])[] = [
  ["VCO1", "VCO2", "VCO3", "Cross-coupler", "Phase mod 1", "Phase mod 2", "Phase mod 3", "Crispy"],
  ["Step chance", "Deja vu 1", "Bag size 1", "Slew 1", "Deja vu 2", "Bag size 2", "Slew 2", "Crispy"],
  ["Wet/dry", "Room size", "Decay", "Pre-delay", "Damping", "Stereo width", "Diffusion", "Crispy"],
  ["Comb offset", "Peak freq", "Peak gain", "Peak Q", "Comb delay", "Comb feedback", "Comb LP", "Crispy"],
  ["Drive", "Shape", "SRR 1", "SRR 2", "XOR", "Bit depth", "Fuzz", "Crispy"],
  ["Delay time", "Send", "Feedback", "Stereo width", "Detune", "Mod depth", "Wet mix", "Crispy"],
];

export const PAIR_AR_KNOB_LABELS = ["Att. 1+2", "Rel. 1+2", "Att. 2+3", "Rel. 2+3"] as const;

export function coreKnobLabel(hostPage: number, row: number): string {
  return HOST_PAGE_KNOB_LABELS[hostPage]?.[row] ?? "";
}

export function pairArKnobLabel(index: number): string {
  return PAIR_AR_KNOB_LABELS[index] ?? "";
}
