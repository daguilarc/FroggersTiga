import { mkdirSync } from "node:fs";
import { dirname, join } from "node:path";
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const require = createRequire(join(root, "web", "package.json"));
const esbuild = require("esbuild");

const entry = join(root, "web", "src", "froggers-processor.ts");
const outfile = join(root, "web", "public", "froggers-processor.js");

mkdirSync(dirname(outfile), { recursive: true });

await esbuild.build({
  entryPoints: [entry],
  outfile,
  format: "esm",
  target: "es2020",
  platform: "browser",
  bundle: true,
});

console.log(`build-worklet: OK → ${outfile}`);
