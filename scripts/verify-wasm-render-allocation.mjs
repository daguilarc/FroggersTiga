#!/usr/bin/env node
/**
 * Static regression asserting that:
 * - WasmSimHost max chunk + scope capacity match HostPanelLayout authority
 * - Worklet allocates WASM heap once in constructor; render path reuses buffers
 * - Screen telemetry cadence is fixed (frameCount % 20)
 *
 * Does NOT instrument WASM malloc at runtime; native sim/WasmSimHostMalloc_test.cpp
 * covers repeated WasmSimHost process/scope calls on fixed std::array storage.
 * Playwright audio-start (startSimAudio) remains manual/CI e2e coverage.
 */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");

function read(path) {
  return readFileSync(join(root, path), "utf8");
}

function parseScopeCapacity(layoutHpp) {
  const match = layoutHpp.match(/kScopeSampleCapacity = (\d+)/);
  if (!match) throw new Error("could not parse kScopeSampleCapacity from HostPanelLayout.hpp");
  return Number(match[1]);
}

function parseProcessChunk(wasmHpp) {
  const match = wasmHpp.match(/kProcessChunkSize = (\d+)/);
  if (!match) throw new Error("could not parse kProcessChunkSize from WasmSimHost.hpp");
  return Number(match[1]);
}

function parseGeneratedScopeSize(generatedTs) {
  const match = generatedTs.match(/export const SCOPE_SIZE = (\d+)/);
  if (!match) throw new Error("could not parse SCOPE_SIZE from hostDisplay.generated.ts");
  return Number(match[1]);
}

function extractMethodBody(source, signature) {
  const start = source.indexOf(signature);
  if (start < 0) throw new Error(`could not find ${signature} in froggers-processor.ts`);
  const braceStart = source.indexOf("{", start);
  if (braceStart < 0) throw new Error(`could not find body for ${signature}`);
  let depth = 0;
  for (let i = braceStart; i < source.length; i++) {
    if (source[i] === "{") depth++;
    else if (source[i] === "}") {
      depth--;
      if (depth === 0) return source.slice(braceStart, i + 1);
    }
  }
  throw new Error(`unterminated body for ${signature}`);
}

function assertNoMallocIn(body, label) {
  if (/\bmalloc\s*\(/.test(body) || /\bfree\s*\(/.test(body)) {
    console.error(`verify-wasm-render-allocation: ${label} must not call malloc/free`);
    process.exit(1);
  }
}

const layoutHpp = read("sim/HostPanelLayout.hpp");
const wasmHpp = read("sim/WasmSimHost.hpp");
const generatedTs = read("web/src/hostDisplay.generated.ts");
const processorTs = read("web/src/froggers-processor.ts");

const scopeCapacity = parseScopeCapacity(layoutHpp);
const processChunk = parseProcessChunk(wasmHpp);
const generatedScope = parseGeneratedScopeSize(generatedTs);

if (generatedScope !== scopeCapacity) {
  console.error(
    `SCOPE_SIZE ${generatedScope} != HostPanelLayout::kScopeSampleCapacity ${scopeCapacity}`
  );
  process.exit(1);
}

if (!processorTs.includes("froggers_max_process_chunk()")) {
  console.error("worklet must read froggers_max_process_chunk() for heap sizing");
  process.exit(1);
}

if (!processorTs.includes("this.maxProcessChunk = this.wasm.froggers_max_process_chunk()")) {
  console.error("worklet must cache maxProcessChunk from WASM export");
  process.exit(1);
}

if (!processorTs.includes("this.scopePtr = this.wasm.malloc(SCOPE_SIZE * 4)")) {
  console.error("worklet must preallocate scopePtr once using SCOPE_SIZE");
  process.exit(1);
}

if (!processorTs.includes("this.frameCount % 20 === 0")) {
  console.error("worklet must decimate screen telemetry at frameCount % 20");
  process.exit(1);
}

const constructorBody = extractMethodBody(processorTs, "constructor(options");
const processBody = extractMethodBody(processorTs, "process(inputs");
const readScopeBody = extractMethodBody(processorTs, "readScopeSamples(modIndex");

assertNoMallocIn(processBody, "process()");
assertNoMallocIn(readScopeBody, "readScopeSamples()");
if (!/\bmalloc\s*\(/.test(constructorBody)) {
  console.error("constructor must preallocate WASM heap buffers via malloc");
  process.exit(1);
}

console.log(
  `verify-wasm-render-allocation: OK (scope=${scopeCapacity}, processChunk=${processChunk}, render-path malloc-free)`
);
