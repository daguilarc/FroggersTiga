import {
  coreKnobLabel,
  coreKnobCountForPage,
  EXPANDED_CORE_KNOB_COUNT,
  pairArKnobLabel,
} from "../src/hostDisplay.generated.ts";

export const PLAY_LABEL = "Play";
export const STOP_LABEL = "Stop";
export const EXTERNAL_OFF_LABEL = "External Audio: Off";
export const EXTERNAL_ON_LABEL = "External Audio: On";

export const STATUS_SELECTOR = "#status";
export const IOS_EXTERNAL_HINT_SELECTOR = "#ios-external-hint";
export const SUBTITLE_SELECTOR = ".subtitle";
export const PLAY_BTN_SELECTOR = "#play-btn";
export const EXTERNAL_BTN_SELECTOR = "#external-btn";

export const SUBTITLE_TEXT =
  "Browser simulator — press Play for sound; External adds mic ring-mod input";

export const STATUS_HINT_EXTERNAL_ON = "external on";
export const IOS_EXTERNAL_HINT_EARPIECE = "route to the earpiece";
export const IOS_EXTERNAL_HINT_HEADPHONES = "Headphones recommended";

export const PLAYING_STATUS_TEXT = "Playing";

export const KNOB_LABEL_VCO1 = coreKnobLabel(0, 0);
export const KNOB_LABEL_CROSS_COUPLER = coreKnobLabel(0, 3);
export const KNOB_LABEL_ATT_1_2 = pairArKnobLabel(0);
export const KNOB_LABEL_DRIVE = coreKnobLabel(4, 0);

export const GLOBAL_CRUNCHY_LABEL = "Crunchy";
export const GLOBAL_CRUNCHY_SELECTOR = ".global-crunchy";

export const KNOB_LABEL_SPREAD = coreKnobLabel(1, 7);
export const KNOB_LABEL_BIAS = coreKnobLabel(1, 8);
export const KNOB_LABEL_COMB_PEAK = coreKnobLabel(3, 7);
export const KNOB_LABEL_SCOOP = coreKnobLabel(3, 8);
export const KNOB_LABEL_BLEND = coreKnobLabel(4, 7);
export const KNOB_LABEL_PHASE = coreKnobLabel(4, 8);
export const KNOB_LABEL_COLOR = coreKnobLabel(5, 7);
export const KNOB_LABEL_HALO = coreKnobLabel(5, 8);

export const EXPANDED_PAGE_KNOB_COUNT = EXPANDED_CORE_KNOB_COUNT;
export const AUDIO_PAGE_KNOB_COUNT = coreKnobCountForPage(0) + 4;

export const MOBILE_KNOB_GRID_COLUMNS = 3;
export const KNOBS_SELECTOR = ".knobs";
export const KNOB_LABEL_SELECTOR = ".knob-label-main";
