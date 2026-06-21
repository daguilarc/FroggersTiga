#!/usr/bin/env node
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const generatedPath = join(root, "web/src/hostDisplay.generated.ts");
const generated = readFileSync(generatedPath, "utf8");

function readConst(name) {
  const re = new RegExp(`export const ${name} = (\\[[\\s\\S]*?\\])(?:\\s+as const)?;`);
  const match = generated.match(re);
  if (!match) {
    console.error(`verify-host-display-shape: missing export const ${name}`);
    process.exit(1);
  }
  return JSON.parse(match[1]);
}

function assertEqual(actual, expected, label) {
  const a = JSON.stringify(actual);
  const e = JSON.stringify(expected);
  if (a !== e) {
    console.error(`${label}: expected ${e}, got ${a}`);
    process.exit(1);
  }
}

const desktopModRack = readConst("DESKTOP_MOD_RACK_INDICES");
const webScopeIndices = readConst("WEB_SCOPE_MOD_INDICES");
const webModBaySpec = readConst("WEB_MOD_BAY_SPEC");
const vstModRack = readConst("VST_MOD_RACK_INDICES");
const vcvModRack = readConst("VCV_MOD_RACK_INDICES");

assertEqual(desktopModRack, [0, 1, 4, 5, 6], "desktop mod rack");
assertEqual(webScopeIndices, [0, 4, 5, 6], "web scope indices");
assertEqual(
  webModBaySpec.map((cell) => cell.modIndex),
  [0, 4, 5, 6],
  "web mod bay"
);
assertEqual(vstModRack, [4, 5, 6], "vst mod rack");
assertEqual(vcvModRack, [4, 5, 6], "vcv mod rack");

console.log("host display projection shape ok");
