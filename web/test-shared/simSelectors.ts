import { coreKnobLabel, pairArKnobLabel } from "../src/paramDisplayNames.ts";

export const PLAY_LABEL = "Play";
export const STOP_LABEL = "Stop";
export const EXTERNAL_OFF_LABEL = "External Audio: Off";
export const EXTERNAL_ON_LABEL = "External Audio: On";

export const STATUS_SELECTOR = "#status";
export const SUBTITLE_SELECTOR = ".subtitle";
export const PLAY_BTN_SELECTOR = "#play-btn";
export const EXTERNAL_BTN_SELECTOR = "#external-btn";

export const SUBTITLE_TEXT =
  "Browser simulator — press Play for sound; External adds mic ring-mod input";

export const STATUS_HINT_EXTERNAL_ON = "external on";
export const STATUS_HINT_EARPIECE = "without headphones, iOS may use the earpiece";

export const PLAYING_STATUS_TEXT = "Playing";

export const KNOB_LABEL_VCO1 = coreKnobLabel(0, 0);
export const KNOB_LABEL_CROSS_COUPLER = coreKnobLabel(0, 3);
export const KNOB_LABEL_ATT_1_2 = pairArKnobLabel(0);
export const KNOB_LABEL_DRIVE = coreKnobLabel(4, 0);

export const KNOBS_SELECTOR = ".knobs";
export const KNOB_LABEL_SELECTOR = ".knob-label-main";
