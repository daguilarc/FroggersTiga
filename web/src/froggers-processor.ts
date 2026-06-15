const EXT_IN_PAD = 0.4;
const EXT_IN_DRIVE = 2.5;

function softLimit(x: number): number {
  const x2 = x * x;
  const y = (x * (27 + x2)) / (27 + 9 * x2);
  return Math.max(-1, Math.min(1, y));
}

const WASM_IMPORTS: WebAssembly.Imports = {
  wasi_snapshot_preview1: {
    args_sizes_get: () => 0,
    args_get: () => 0,
    proc_exit: () => {},
    fd_close: () => 0,
    fd_write: () => 0,
    fd_seek: () => 0,
  },
  env: {
    __main_argc_argv: () => {},
    emscripten_notify_memory_growth: () => {},
  },
};

const CORE_PAGE_NAMES = ["Audio", "Random S&H", "Reverb", "Filter", "Drive"];
const HOST_PAGE_NAMES = [...CORE_PAGE_NAMES, "Delay"];
const HOST_PAGE_COUNT = 6;
const SCOPE_MOD_INDICES = [0, 4, 5, 6];
const SCOPE_SIZE = 96;

interface WasmExports {
  memory: WebAssembly.Memory;
  froggers_create: () => number;
  froggers_destroy: (host: number) => void;
  froggers_set_sample_rate: (host: number, sampleRate: number) => void;
  froggers_set_knob: (host: number, index: number, value: number) => void;
  froggers_set_vco_morph: (host: number, index: number, value: number) => void;
  froggers_get_vco_morph: (host: number, index: number) => number;
  froggers_get_vco_display_morph: (host: number, index: number) => number;
  froggers_cycle_vco_morph: (host: number, index: number) => void;
  froggers_randomize_vco_morphs: (host: number) => void;
  froggers_set_row_mod_source: (host: number, row: number, modIndex: number) => void;
  froggers_get_row_mod_source: (host: number, row: number) => number;
  froggers_set_row_mod_depth: (host: number, row: number, depth: number) => void;
  froggers_get_row_mod_depth: (host: number, row: number) => number;
  froggers_mod_level: (host: number, modIndex: number) => number;
  froggers_page_next: (host: number) => void;
  froggers_page_prev: (host: number) => void;
  froggers_select_page: (host: number, page: number) => void;
  froggers_marbles: (host: number) => void;
  froggers_randomize_all_pages: (host: number) => void;
  froggers_randomize_all_mod: (host: number) => void;
  froggers_randomize_page: (host: number, page: number) => void;
  froggers_randomize_page_mod: (host: number, page: number) => void;
  froggers_process_stereo: (
    host: number,
    inPtr: number,
    outLPtr: number,
    outRPtr: number,
    n: number
  ) => void;
  froggers_row_name: (host: number, row: number) => number;
  froggers_mod_source_name: (modIndex: number) => number;
  froggers_row_value: (host: number, row: number) => number;
  froggers_row_badge: (host: number, row: number) => number;
  froggers_current_page: (host: number) => number;
  froggers_num_pages: (host: number) => number;
  froggers_delay_set_knob: (host: number, row: number, value: number) => void;
  froggers_delay_get_knob: (host: number, row: number) => number;
  froggers_delay_set_row_mod_source: (host: number, row: number, modIndex: number) => void;
  froggers_delay_get_row_mod_source: (host: number, row: number) => number;
  froggers_delay_set_row_mod_depth: (host: number, row: number, depth: number) => void;
  froggers_delay_get_row_mod_depth: (host: number, row: number) => number;
  froggers_delay_row_name: (host: number, row: number) => number;
  froggers_delay_randomize_knobs: (host: number) => void;
  froggers_delay_randomize_mod: (host: number) => void;
  froggers_copy_scope_samples: (host: number, modIndex: number, outPtr: number, maxCount: number) => number;
  froggers_push_midi_cc: (host: number, channel: number, cc: number, value: number) => void;
  froggers_assignable_mod_count: (host: number) => number;
  froggers_assignable_mod_index: (host: number, index: number) => number;
  froggers_mod_source_available: (host: number, modIndex: number) => number;
  froggers_set_cc_pair_enabled: (host: number, pairIndex: number, enabled: number) => void;
  froggers_cc_pair_enabled: (host: number, pairIndex: number) => number;
  froggers_set_audio_pair_ar_knob: (host: number, index: number, value: number) => void;
  froggers_get_audio_pair_ar_knob: (host: number, index: number) => number;
  froggers_get_audio_pair_ar_effective: (host: number, index: number) => number;
  froggers_set_audio_pair_ar_mod_source: (host: number, index: number, modIndex: number) => void;
  froggers_get_audio_pair_ar_mod_source: (host: number, index: number) => number;
  froggers_set_audio_pair_ar_mod_depth: (host: number, index: number, depth: number) => void;
  froggers_get_audio_pair_ar_mod_depth: (host: number, index: number) => number;
  froggers_audio_pair_ar_name: (index: number) => number;
  malloc: (size: number) => number;
  free: (ptr: number) => void;
}

type UiMessage =
  | { type: "knob"; index: number; value: number }
  | { type: "delayKnob"; row: number; value: number }
  | { type: "modSource"; row: number; modIndex: number }
  | { type: "delayModSource"; row: number; modIndex: number }
  | { type: "modDepth"; row: number; depth: number }
  | { type: "delayModDepth"; row: number; depth: number }
  | { type: "hostPage"; page: number }
  | { type: "hostPageDelta"; delta: number }
  | { type: "marbles" }
  | { type: "randomizeAll" }
  | { type: "randomizeMod" }
  | { type: "randomizePage"; page: number }
  | { type: "randomizePageMod"; page: number }
  | { type: "delayRandomizeKnobs" }
  | { type: "delayRandomizeMod" }
  | { type: "setSampleRate"; sampleRate: number }
  | { type: "vcoMorph"; index: number; value: number }
  | { type: "cycleVcoMorph"; index: number }
  | { type: "randomizeMorphs" }
  | { type: "external"; enabled: boolean }
  | { type: "setRunning"; running: boolean }
  | { type: "midiCc"; channel: number; cc: number; value: number }
  | { type: "setCcPairEnabled"; pairIndex: number; enabled: boolean }
  | { type: "pairArKnob"; index: number; value: number }
  | { type: "pairArModSource"; index: number; modIndex: number }
  | { type: "pairArModDepth"; index: number; depth: number };

interface ProcessorCtorOptions {
  processorOptions?: { wasmModule?: WebAssembly.Module };
}

class FroggersProcessor extends AudioWorkletProcessor {
  private host = 0;
  private wasm: WasmExports | null = null;
  private frameCount = 0;
  private externalEnabled = false;
  private audioRunning = false;
  private hostPage = 0;
  private wasmReady = false;
  private processErrorPosted = false;
  private inputPeak = 0;

  private postAssignableModOptions(): void {
    if (!this.wasm || !this.host) {
      return;
    }
    const assignableModOptions = [];
    const assignableCount = this.wasm.froggers_assignable_mod_count(this.host);
    for (let i = 0; i < assignableCount; i++) {
      const modIndex = this.wasm.froggers_assignable_mod_index(this.host, i);
      if (modIndex < 0) {
        continue;
      }
      assignableModOptions.push({
        index: modIndex,
        label: this.readCString(this.wasm.froggers_mod_source_name(modIndex)),
      });
    }
    const ccAvailability = [0, 1].map((modIndex) =>
      this.wasm!.froggers_mod_source_available(this.host, modIndex) !== 0
    );
    this.port.postMessage({
      type: "modAvailabilityChanged",
      assignableModOptions,
      ccAvailability,
    });
  }

  constructor(options?: ProcessorCtorOptions) {
    super();
    this.port.onmessage = (event: MessageEvent<UiMessage>) => {
      this.handleUi(event.data);
    };
    try {
      const wasmModule = options?.processorOptions?.wasmModule;
      if (!wasmModule) {
        throw new Error("Missing wasmModule in processorOptions");
      }
      const instance = new WebAssembly.Instance(wasmModule, WASM_IMPORTS);
      this.wasm = instance.exports as unknown as WasmExports;
      this.host = this.wasm.froggers_create();
      for (let i = 0; i < 8; i++) {
        this.wasm.froggers_set_knob(this.host, i, 0.5);
      }
      this.wasmReady = true;
      const modSourceNames = SCOPE_MOD_INDICES.map((modIndex) =>
        this.readCString(this.wasm.froggers_mod_source_name(modIndex))
      );
      this.wasm.froggers_set_cc_pair_enabled(this.host, 0, 0);
      this.wasm.froggers_set_cc_pair_enabled(this.host, 1, 0);
      const assignableModOptions = [];
      const assignableCount = this.wasm.froggers_assignable_mod_count(this.host);
      for (let i = 0; i < assignableCount; i++) {
        const modIndex = this.wasm.froggers_assignable_mod_index(this.host, i);
        if (modIndex < 0) {
          continue;
        }
        assignableModOptions.push({
          index: modIndex,
          label: this.readCString(this.wasm.froggers_mod_source_name(modIndex)),
        });
      }
      this.port.postMessage({
        type: "ready",
        numPages: this.wasm.froggers_num_pages(this.host),
        modSourceNames,
        assignableModOptions,
        ccAvailability: [false, false],
      });
      this.setHostPage(0);
    } catch (err) {
      this.port.postMessage({
        type: "error",
        message: err instanceof Error ? err.message : String(err),
      });
    }
  }

  private setHostPage(page: number): void {
    this.hostPage = ((page % HOST_PAGE_COUNT) + HOST_PAGE_COUNT) % HOST_PAGE_COUNT;
    if (this.wasm && this.host && this.hostPage <= 4) {
      this.wasm.froggers_select_page(this.host, this.hostPage);
    }
    this.postScreen();
  }

  private handleUi(msg: UiMessage): void {
    if (!this.wasm || !this.host || !this.wasmReady) {
      return;
    }
    const wasm = this.wasm;
    const host = this.host;
    if (msg.type === "knob") {
      wasm.froggers_set_knob(host, msg.index, msg.value);
    } else if (msg.type === "delayKnob") {
      wasm.froggers_delay_set_knob(host, msg.row, msg.value);
    } else if (msg.type === "modSource") {
      wasm.froggers_set_row_mod_source(host, msg.row, msg.modIndex);
    } else if (msg.type === "delayModSource") {
      wasm.froggers_delay_set_row_mod_source(host, msg.row, msg.modIndex);
    } else if (msg.type === "modDepth") {
      wasm.froggers_set_row_mod_depth(host, msg.row, msg.depth);
    } else if (msg.type === "delayModDepth") {
      wasm.froggers_delay_set_row_mod_depth(host, msg.row, msg.depth);
    } else if (msg.type === "hostPage") {
      this.setHostPage(msg.page);
    } else if (msg.type === "hostPageDelta") {
      this.setHostPage(this.hostPage + msg.delta);
    } else if (msg.type === "marbles") {
      wasm.froggers_marbles(host);
      this.postScreen();
    } else if (msg.type === "randomizeAll") {
      wasm.froggers_randomize_all_pages(host);
      this.postScreen();
    } else if (msg.type === "randomizeMod") {
      wasm.froggers_randomize_all_mod(host);
      this.postScreen();
    } else if (msg.type === "randomizePage") {
      wasm.froggers_randomize_page(host, msg.page);
      this.postScreen();
    } else if (msg.type === "randomizePageMod") {
      wasm.froggers_randomize_page_mod(host, msg.page);
      this.postScreen();
    } else if (msg.type === "delayRandomizeKnobs") {
      wasm.froggers_delay_randomize_knobs(host);
      this.postScreen();
    } else if (msg.type === "delayRandomizeMod") {
      wasm.froggers_delay_randomize_mod(host);
      this.postScreen();
    } else if (msg.type === "setSampleRate") {
      wasm.froggers_set_sample_rate(host, msg.sampleRate);
    } else if (msg.type === "vcoMorph") {
      wasm.froggers_set_vco_morph(host, msg.index, msg.value);
    } else if (msg.type === "cycleVcoMorph") {
      wasm.froggers_cycle_vco_morph(host, msg.index);
      this.postScreen();
    } else if (msg.type === "randomizeMorphs") {
      wasm.froggers_randomize_vco_morphs(host);
      this.postScreen();
    } else if (msg.type === "external") {
      this.externalEnabled = msg.enabled;
    } else if (msg.type === "midiCc") {
      wasm.froggers_push_midi_cc(host, msg.channel, msg.cc, msg.value);
    } else if (msg.type === "setCcPairEnabled") {
      wasm.froggers_set_cc_pair_enabled(host, msg.pairIndex, msg.enabled ? 1 : 0);
      this.postAssignableModOptions();
    } else if (msg.type === "pairArKnob") {
      wasm.froggers_set_audio_pair_ar_knob(host, msg.index, msg.value);
    } else if (msg.type === "pairArModSource") {
      wasm.froggers_set_audio_pair_ar_mod_source(host, msg.index, msg.modIndex);
    } else if (msg.type === "pairArModDepth") {
      wasm.froggers_set_audio_pair_ar_mod_depth(host, msg.index, msg.depth);
    } else if (msg.type === "setRunning") {
      this.audioRunning = msg.running;
      if (msg.running) {
        this.postScreen();
      }
    }
  }

  private heapF32(): Float32Array {
    return new Float32Array(this.wasm!.memory.buffer);
  }

  private readCString(ptr: number): string {
    const view = new Uint8Array(this.wasm!.memory.buffer);
    let result = "";
    for (let i = ptr; view[i] !== 0; i++) {
      const byte = view[i];
      if (byte < 0x80) {
        result += String.fromCharCode(byte);
      } else if ((byte & 0xe0) === 0xc0) {
        result += String.fromCharCode(((byte & 0x1f) << 6) | (view[i + 1] & 0x3f));
        i++;
      } else if ((byte & 0xf0) === 0xe0) {
        result += String.fromCharCode(
          ((byte & 0x0f) << 12) | ((view[i + 1] & 0x3f) << 6) | (view[i + 2] & 0x3f)
        );
        i += 2;
      } else {
        result += String.fromCharCode(byte);
      }
    }
    return result;
  }

  private readScopeSamples(modIndex: number): number[] {
    if (!this.wasm || !this.host) {
      return [];
    }
    const ptr = this.wasm.malloc(SCOPE_SIZE * 4);
    const count = this.wasm.froggers_copy_scope_samples(this.host, modIndex, ptr, SCOPE_SIZE);
    const heap = this.heapF32();
    const off = ptr >> 2;
    const samples = Array.from(heap.subarray(off, off + count));
    this.wasm.free(ptr);
    return samples;
  }

  private postScreen(): void {
    if (!this.wasm || !this.host) {
      return;
    }
    const rows = [];
    const morphs = [];
    const modLevels = [];
    const onDelayPage = this.hostPage === 5;

    for (let row = 0; row < 8; row++) {
      if (onDelayPage) {
        const namePtr = this.wasm.froggers_delay_row_name(this.host, row);
        rows.push({
          name: this.readCString(namePtr),
          value: this.wasm.froggers_delay_get_knob(this.host, row),
          badge: " ",
          modSource: this.wasm.froggers_delay_get_row_mod_source(this.host, row),
          modDepth: this.wasm.froggers_delay_get_row_mod_depth(this.host, row),
        });
      } else {
        const namePtr = this.wasm.froggers_row_name(this.host, row);
        rows.push({
          name: this.readCString(namePtr),
          value: this.wasm.froggers_row_value(this.host, row),
          badge: String.fromCharCode(this.wasm.froggers_row_badge(this.host, row)),
          modSource: this.wasm.froggers_get_row_mod_source(this.host, row),
          modDepth: this.wasm.froggers_get_row_mod_depth(this.host, row),
        });
      }
    }
    for (let i = 0; i < 3; i++) {
      morphs.push(this.wasm.froggers_get_vco_display_morph(this.host, i));
    }

    const pairArRows = [];
    if (this.hostPage === 0) {
      for (let i = 0; i < 4; i++) {
        const modSource = this.wasm.froggers_get_audio_pair_ar_mod_source(this.host, i);
        pairArRows.push({
          name: this.readCString(this.wasm.froggers_audio_pair_ar_name(i)),
          value:
            modSource === 255
              ? this.wasm.froggers_get_audio_pair_ar_knob(this.host, i)
              : this.wasm.froggers_get_audio_pair_ar_effective(this.host, i),
          badge: " ",
          modSource,
          modDepth: this.wasm.froggers_get_audio_pair_ar_mod_depth(this.host, i),
        });
      }
    }

    for (let i = 0; i < 7; i++) {
      modLevels.push(this.wasm.froggers_mod_level(this.host, i));
    }

    const scopeSamples = SCOPE_MOD_INDICES.map((modIndex) => this.readScopeSamples(modIndex));
    const modSourceNames = SCOPE_MOD_INDICES.map((modIndex) =>
      this.readCString(this.wasm.froggers_mod_source_name(modIndex))
    );

    this.port.postMessage({
      type: "screen",
      hostPage: this.hostPage,
      wasmPage: this.wasm.froggers_current_page(this.host),
      pageName: HOST_PAGE_NAMES[this.hostPage] ?? `Page ${this.hostPage}`,
      rows,
      pairArRows,
      morphs,
      modLevels,
      scopeSamples,
      modSourceNames,
      audioRunning: this.audioRunning,
      inputPeak: this.inputPeak,
    });
  }

  private zeroOutputs(outL: Float32Array, outR: Float32Array | undefined): void {
    outL.fill(0);
    if (outR) {
      outR.fill(0);
    }
  }

  process(inputs: Float32Array[][], outputs: Float32Array[][]): boolean {
    const input = inputs[0]?.[0];
    const outL = outputs[0]?.[0];
    const outR = outputs[0]?.[1];
    if (!outL) {
      return true;
    }

    if (!this.wasm || !this.host || !this.audioRunning) {
      this.zeroOutputs(outL, outR);
      return true;
    }

    try {
      const n = outL.length;
      const wasm = this.wasm;
      const inPtr = wasm.malloc(n * 4);
      const outLPtr = wasm.malloc(n * 4);
      const outRPtr = outR ? wasm.malloc(n * 4) : 0;
      const heap = this.heapF32();
      const inOff = inPtr >> 2;
      const outLOff = outLPtr >> 2;
      const outROff = outRPtr ? outRPtr >> 2 : 0;

      if (this.externalEnabled && input) {
        let blockPeak = 0;
        for (let i = 0; i < n; i++) {
          const limited = softLimit(EXT_IN_DRIVE * input[i] * EXT_IN_PAD);
          heap[inOff + i] = limited;
          const sample = Math.abs(limited);
          if (sample > blockPeak) {
            blockPeak = sample;
          }
        }
        this.inputPeak = this.inputPeak * 0.65 + blockPeak * 0.35;
      } else {
        heap.fill(0, inOff, inOff + n);
        this.inputPeak *= 0.65;
      }

      wasm.froggers_process_stereo(this.host, inPtr, outLPtr, outRPtr, n);
      outL.set(heap.subarray(outLOff, outLOff + n));
      if (outR && outRPtr) {
        outR.set(heap.subarray(outROff, outROff + n));
      }
      wasm.free(inPtr);
      wasm.free(outLPtr);
      if (outRPtr) {
        wasm.free(outRPtr);
      }

      this.frameCount++;
      if (this.frameCount % 20 === 0) {
        this.postScreen();
      }
      return true;
    } catch (err) {
      if (!this.processErrorPosted) {
        this.processErrorPosted = true;
        const message = err instanceof Error ? err.message : String(err);
        this.port.postMessage({ type: "error", message });
      }
      this.zeroOutputs(outL, outR);
      return true;
    }
  }
}

registerProcessor("froggers-processor", FroggersProcessor);
