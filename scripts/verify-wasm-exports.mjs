import { readFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const processorPath = join(root, "web", "src", "froggers-processor.ts");
const wasmPath = join(root, "web", "public", "froggers.wasm");

const WASM_IMPORTS = {
  wasi_snapshot_preview1: { fd_write: () => 0 },
  env: { emscripten_notify_memory_growth: () => {} },
};

function requiredExportsFromProcessor() {
  const source = readFileSync(processorPath, "utf8");
  const match = source.match(/interface WasmExports \{([\s\S]*?)\n\}/);
  if (!match) {
    throw new Error("WasmExports interface not found in froggers-processor.ts");
  }
  const names = [];
  for (const line of match[1].split("\n")) {
    const fn = line.match(/^  ([a-zA-Z0-9_]+):/);
    if (!fn || fn[1] === "memory") {
      continue;
    }
    const name = fn[1];
    names.push(name);
  }
  return names.sort();
}

function hasExport(exports, name) {
  return exports.has(name) || exports.has(`_${name}`);
}

const required = requiredExportsFromProcessor();
const wasmBytes = readFileSync(wasmPath);
const { instance } = await WebAssembly.instantiate(wasmBytes, WASM_IMPORTS);
const actual = new Set(Object.keys(instance.exports));
const missing = required.filter((name) => !hasExport(actual, name));

if (missing.length > 0) {
  console.error("Missing WASM exports:");
  for (const name of missing) {
    console.error(`  ${name}`);
  }
  process.exit(1);
}

console.log(`verify-wasm-exports: OK (${required.length} exports)`);
