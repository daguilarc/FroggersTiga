import { accessSync, readFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const outPath = join(root, "web", "public", "froggers-processor.js");

const TS_MARKERS = [
  { pattern: /\binterface\s+[A-Za-z_]/, label: "interface declaration" },
  { pattern: /:\s*WebAssembly\./, label: "WebAssembly type annotation" },
  { pattern: /\)\s*:\s*(number|void|string|boolean)\s*[{;]/, label: "return type annotation" },
];

try {
  accessSync(outPath);
} catch {
  console.error(`Missing ${outPath}`);
  console.error("Run: cd web && npm run build:worklet");
  process.exit(1);
}

const text = readFileSync(outPath, "utf8");
for (const { pattern, label } of TS_MARKERS) {
  if (pattern.test(text)) {
    console.error(`verify-worklet-js: TypeScript leakage (${label}) in ${outPath}`);
    process.exit(1);
  }
}

console.log(`verify-worklet-js: OK (${outPath})`);
