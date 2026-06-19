#!/usr/bin/env node
import {
  DESKTOP_MOD_RACK_INDICES,
  VCV_MOD_RACK_INDICES,
  VST_MOD_RACK_INDICES,
  WEB_MOD_BAY_SPEC,
  WEB_SCOPE_MOD_INDICES,
} from "../web/src/hostDisplay.generated.ts";

function assertEqual(actual, expected, label) {
  const a = JSON.stringify(actual);
  const e = JSON.stringify(expected);
  if (a !== e) {
    console.error(`${label}: expected ${e}, got ${a}`);
    process.exit(1);
  }
}

assertEqual(DESKTOP_MOD_RACK_INDICES, [0, 1, 4, 5, 6], "desktop mod rack");
assertEqual(WEB_SCOPE_MOD_INDICES, [0, 4, 5, 6], "web scope indices");
assertEqual(WEB_MOD_BAY_SPEC.map((c) => c.modIndex), [0, 4, 5, 6], "web mod bay");
assertEqual(VST_MOD_RACK_INDICES, [4, 5, 6], "vst mod rack");
assertEqual(VCV_MOD_RACK_INDICES, [4, 5, 6], "vcv mod rack");

console.log("host display projection shape ok");
