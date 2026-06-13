import { accessSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const wasmPath = join(root, "web", "public", "froggers.wasm");

try {
  accessSync(wasmPath);
} catch {
  console.error("Missing web/public/froggers.wasm");
  console.error("Run: cd web && npm run build:wasm");
  console.error("Or first clone: cd web && npm run build:all && npm run dev");
  process.exit(1);
}
