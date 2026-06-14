#!/usr/bin/env node
/** paramDisplayNames.ts must match sim/ParamDisplayNames.hpp. */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const hpp = readFileSync(join(root, "sim/ParamDisplayNames.hpp"), "utf8");
const paramTs = readFileSync(join(root, "web/src/paramDisplayNames.ts"), "utf8");

function tableFromHpp(text) {
  const block = text.match(/kTable\[kNumHostPages\]\[kNumRows\] = \{(.*?)\};/s);
  if (!block) throw new Error("could not parse kTable from ParamDisplayNames.hpp");
  return block[1]
    .split("\n")
    .map((line) => line.trim().replace(/,$/, ""))
    .filter((line) => line.startsWith("{"))
    .map((line) => [...line.matchAll(/"([^"]+)"/g)].map((m) => m[1]));
}

function tableFromTs(text) {
  const block = text.match(/HOST_PAGE_KNOB_LABELS.*?=\s*\[(.*?)\];/s);
  if (!block) throw new Error("could not parse HOST_PAGE_KNOB_LABELS from paramDisplayNames.ts");
  return block[1]
    .split("\n")
    .map((line) => line.trim().replace(/,$/, ""))
    .filter((line) => line.startsWith("["))
    .map((line) => [...line.matchAll(/"([^"]+)"/g)].map((m) => m[1]));
}

function pairFromHpp(text) {
  const block = text.match(/kLabels\[4\] = \{(.*?)\};/s);
  if (!block) throw new Error("could not parse pair-AR labels from ParamDisplayNames.hpp");
  return [...block[1].matchAll(/"([^"]+)"/g)].map((m) => m[1]);
}

function pairFromTs(text) {
  const block = text.match(/PAIR_AR_KNOB_LABELS = \[(.*?)\]/s);
  if (!block) throw new Error("could not parse PAIR_AR_KNOB_LABELS from paramDisplayNames.ts");
  return [...block[1].matchAll(/"([^"]+)"/g)].map((m) => m[1]);
}

const hppRows = tableFromHpp(hpp);
const tsRows = tableFromTs(paramTs);
if (JSON.stringify(hppRows) !== JSON.stringify(tsRows)) {
  console.error("HOST_PAGE_KNOB_LABELS diverges from ParamDisplayNames.hpp");
  process.exit(1);
}

const hppPair = pairFromHpp(hpp);
const tsPair = pairFromTs(paramTs);
if (JSON.stringify(hppPair) !== JSON.stringify(tsPair)) {
  console.error("PAIR_AR_KNOB_LABELS diverges from ParamDisplayNames.hpp");
  process.exit(1);
}

console.log("paramDisplayNames.ts matches ParamDisplayNames.hpp");
