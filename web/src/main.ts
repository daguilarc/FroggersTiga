const wasmUrl = `${import.meta.env.BASE_URL}froggers.wasm`;
const processorUrl = `${import.meta.env.BASE_URL}froggers-processor.js`;
import { CvScopeCanvas } from "./CvScopeCanvas";
import { ModLedIndicator } from "./ModLedIndicator";
import { coreKnobLabel, pairArKnobLabel } from "./paramDisplayNames";
import { RotaryKnob } from "./RotaryKnob";

const HOST_PAGE_COUNT = 6;
const CORE_KNOB_COUNT = 8;
const PAIR_AR_KNOB_COUNT = 4;
const TOTAL_KNOB_COUNT = CORE_KNOB_COUNT + PAIR_AR_KNOB_COUNT;
const PAGE_NAMES = ["Audio", "Random S&H", "Reverb", "Filter", "Drive", "Delay"];

type ModBaySpec = { modIndex: number; kind: "scope" | "led" };

const MOD_BAY_SPEC: ModBaySpec[] = [
  { modIndex: 0, kind: "scope" },
  { modIndex: 4, kind: "scope" },
  { modIndex: 5, kind: "led" },
  { modIndex: 6, kind: "led" },
];

type ModBayIndicator =
  | { kind: "scope"; modIndex: number; scope: CvScopeCanvas }
  | { kind: "led"; modIndex: number; led: ModLedIndicator };

const modBayIndicators: ModBayIndicator[] = MOD_BAY_SPEC.map((spec) =>
  spec.kind === "scope"
    ? { kind: "scope", modIndex: spec.modIndex, scope: new CvScopeCanvas("", "continuous") }
    : { kind: "led", modIndex: spec.modIndex, led: new ModLedIndicator("") }
);

interface AssignableModOption {
  index: number;
  label: string;
}

let assignableModOptions: AssignableModOption[] = [];

const PAGE_BLURBS: Record<number, string> = {
  0: "Three VCOs, cross-coupling, phase modulation, and pair-sum AR.",
  1: "Random CV — press Rand Resample to step.",
  2: "Reverb mix, size, decay, stereo width, and diffusion.",
  3: "Comb offset, peak EQ, comb filter, and Crispy.",
  4: "Drive, SRR, XOR grit, and fuzz.",
  5: "Stereo delay — send, feedback, width, detune, mod.",
};

const DELAY_HINTS: Record<number, string> = {
  0: "~0–2 s",
};

interface ScreenRow {
  name: string;
  value: number;
  badge: string;
  modSource: number;
  modDepth: number;
}

const playBtn = document.getElementById("play-btn") as HTMLButtonElement;
const stopBtn = document.getElementById("stop-btn") as HTMLButtonElement;
const externalBtn = document.getElementById("external-btn") as HTMLButtonElement;
const externalMidiBtn = document.getElementById("external-midi-btn") as HTMLButtonElement;
const statusEl = document.getElementById("status") as HTMLSpanElement;
const knobsEl = document.getElementById("knobs") as HTMLDivElement;
const modBayEl = document.getElementById("mod-bay") as HTMLDivElement;
const modBayToggle = document.getElementById("mod-bay-toggle") as HTMLButtonElement;
const externalMeterEl = document.getElementById("external-meter") as HTMLDivElement;
const externalMeterFillEl = document.getElementById("external-meter-fill") as HTMLSpanElement;
const pageChromeEl = document.getElementById("page-chrome") as HTMLElement;
const pageChromeTitle = document.getElementById("page-chrome-title") as HTMLHeadingElement;
const pageChromeBlurb = document.getElementById("page-chrome-blurb") as HTMLParagraphElement;
const pageRandKnobsBtn = document.getElementById("page-rand-knobs") as HTMLButtonElement;
const pageRandModBtn = document.getElementById("page-rand-mod") as HTMLButtonElement;
const pagePrev = document.getElementById("page-prev") as HTMLButtonElement;
const pageNext = document.getElementById("page-next") as HTMLButtonElement;
const pagePillsEl = document.getElementById("page-pills") as HTMLElement;
const fieldLayoutEl = document.getElementById("field-layout") as HTMLDivElement;
const appHeaderTrigger = document.getElementById("app-header-trigger") as HTMLButtonElement;
const appHelpMenu = document.getElementById("app-help-menu") as HTMLDivElement;
const helpModal = document.getElementById("help-modal") as HTMLDivElement;
const helpModalTitle = document.getElementById("help-modal-title") as HTMLHeadingElement;
const helpModalBody = document.getElementById("help-modal-body") as HTMLPreElement;
const helpModalClose = document.getElementById("help-modal-close") as HTMLButtonElement;

const helpDocCache = new Map<string, string>();
const HELP_DOC_PATHS: Record<string, { title: string; path: string }> = {
  manual: { title: "Manual", path: `${import.meta.env.BASE_URL}sim-manual.md` },
  "quick-dict": { title: "Quick Dict", path: `${import.meta.env.BASE_URL}quick-dict.md` },
  license: { title: "License", path: `${import.meta.env.BASE_URL}license.md` },
};

let workletNode: AudioWorkletNode | null = null;
let mediaStream: MediaStream | null = null;
let micSource: MediaStreamAudioSourceNode | null = null;
let midiAccess: MIDIAccess | null = null;
const midiInputHandlers = new Map<MIDIInput, (event: MIDIMessageEvent) => void>();
let audioContext: AudioContext | null = null;
let externalEnabled = false;
let externalMidiEnabled = false;
let audioRunning = false;
let transportIntentPlaying = false;
let engineReady = false;
let outputGain: GainNode | null = null;
let hostPage = 0;
let modBayExpanded = sessionStorage.getItem("modBayExpanded") !== "false";
let swipeStartX = 0;
let swipeStartY = 0;

const INPUT_PEAK_SILENT = 1e-4;
const SILENT_SCREEN_TICKS = 17;

const knobDragging = new Array<boolean>(TOTAL_KNOB_COUNT).fill(false);
let lastScreenRows: ScreenRow[] = [];
let lastPairArRows: ScreenRow[] = [];
let lastMorphs = [0, 0, 0];
let lastWasmPage = 0;
let silentScreenTicks = 0;
let inputSilentHint = false;
const rotaryKnobs: RotaryKnob[] = [];
const modSelects: HTMLSelectElement[] = [];
const knobMainLabels: HTMLLabelElement[] = [];
const knobHintLabels: HTMLSpanElement[] = [];
const knobCols: HTMLDivElement[] = [];
const vcoMorphBtns: HTMLButtonElement[] = [];
const pagePillButtons: HTMLButtonElement[] = [];

function send(msg: object): void {
  workletNode?.port.postMessage(msg);
}

function isDelayPage(): boolean {
  return hostPage === 5;
}

function isPairArKnob(index: number): boolean {
  return index >= CORE_KNOB_COUNT;
}

function pairArCellIndex(index: number): number {
  return index - CORE_KNOB_COUNT;
}

function modSelectIndex(modSource: number): number {
  if (modSource === 255) {
    return 0;
  }
  const optionIndex = assignableModOptions.findIndex((option) => option.index === modSource);
  return optionIndex >= 0 ? optionIndex + 1 : 0;
}

function populateModSelects(options: AssignableModOption[]): void {
  assignableModOptions = options;
  for (let i = 0; i < modSelects.length; i++) {
    const select = modSelects[i];
    const row = isPairArKnob(i)
      ? lastPairArRows[pairArCellIndex(i)]
      : lastScreenRows[i];
    const currentMod = row?.modSource ?? 255;
    select.innerHTML = '<option value="255">None</option>';
    for (const option of options) {
      const el = document.createElement("option");
      el.value = String(option.index);
      el.textContent = option.label;
      select.appendChild(el);
    }
    select.selectedIndex = modSelectIndex(currentMod);
  }
}

function applyModBayAvailability(): void {
  for (const indicator of modBayIndicators) {
    if (indicator.modIndex !== 0) {
      continue;
    }
    const el = indicator.kind === "scope" ? indicator.scope.element : indicator.led.element;
    el.classList.toggle("mod-disabled", !externalMidiEnabled);
  }
}

function setWebCcPairEnabled(pairIndex: number, enabled: boolean): void {
  send({ type: "setCcPairEnabled", pairIndex, enabled });
}

function evalWaveMorph(phase: number, morph: number): number {
  const sine = Math.sin(phase * 2 * Math.PI);
  const saw = 2 * phase - 1;
  const square = phase < 0.5 ? 1 : -1;
  if (morph <= 0.5) {
    const t = morph * 2;
    return sine * (1 - t) + saw * t;
  }
  const t = (morph - 0.5) * 2;
  return saw * (1 - t) + square * t;
}

function waveSvg(morph: number): string {
  const points: string[] = [];
  for (let i = 0; i <= 24; i++) {
    const t = i / 24;
    const x = 2 + t * 24;
    const y = 14 - evalWaveMorph(t, morph) * 9;
    points.push(`${i === 0 ? "M" : "L"}${x.toFixed(1)} ${y.toFixed(1)}`);
  }
  return `<svg class="wave-icon" width="28" height="28" viewBox="0 0 28 28" aria-hidden="true"><path d="${points.join(" ")}" fill="none" stroke="currentColor" stroke-width="2"/></svg>`;
}

function initModBay(): void {
  modBayEl.innerHTML = "";
  for (const entry of modBayIndicators) {
    modBayEl.appendChild(entry.kind === "scope" ? entry.scope.element : entry.led.element);
  }
}

function renderModBay(
  scopeSamples: number[][] | undefined,
  levels: number[],
  running: boolean
): void {
  for (let i = 0; i < modBayIndicators.length; i++) {
    const entry = modBayIndicators[i];
    const level = levels[entry.modIndex] ?? 0;
    if (entry.kind === "scope") {
      const block = scopeSamples?.[i];
      if (block && block.length > 0) {
        entry.scope.pushBlock(block);
      } else {
        entry.scope.pushSample(level);
      }
      entry.scope.setIdle(!running);
      entry.scope.draw();
      continue;
    }
    entry.led.setLevel(running ? level : 0);
  }
  modBayEl.classList.toggle("collapsed", !modBayExpanded);
  modBayToggle.setAttribute("aria-expanded", String(modBayExpanded));
  modBayToggle.textContent = modBayExpanded ? "Mod sources ▾" : "Mod sources ▸";
}

function renderVcoMorphButtons(wasmPage: number): void {
  lastWasmPage = wasmPage;
  const show = hostPage === 0;
  for (let i = 0; i < vcoMorphBtns.length; i++) {
    const btn = vcoMorphBtns[i];
    btn.hidden = !show;
    if (show) {
      btn.innerHTML = waveSvg(lastMorphs[i] ?? 0);
    }
  }
}

function renderInputMeter(peak: number, active: boolean): void {
  externalMeterEl.dataset.active = active ? "true" : "false";
  const width = active ? Math.min(100, Math.max(0, peak * 100)) : 0;
  externalMeterFillEl.style.width = `${width}%`;
}

type MobileAudioSessionMode = "playback" | "reset" | "externalOn" | "externalOff";

function isMobileWeb(): boolean {
  return (
    window.matchMedia("(max-width: 720px)").matches ||
    /Android|iPhone|iPad|iPod/i.test(navigator.userAgent)
  );
}

function applyMobileAudioSession(mode: MobileAudioSessionMode): void {
  if (!isMobileWeb() || !("audioSession" in navigator)) {
    return;
  }
  const session = (navigator as Navigator & { audioSession: { type: string } }).audioSession;
  try {
    if (mode === "externalOff") {
      session.type = "playback";
      session.type = "auto";
      return;
    }
    const sessionType: Record<Exclude<MobileAudioSessionMode, "externalOff">, string> = {
      playback: "playback",
      reset: "auto",
      externalOn: "play-and-record",
    };
    session.type = sessionType[mode];
  } catch {
    // Audio Session API is experimental on some mobile browsers
  }
}

function playingStatusBase(): string {
  if (!audioRunning || !audioContext) {
    return "";
  }
  const extLabel = externalEnabled ? "external on" : "external off";
  const midiLabel = externalMidiEnabled ? "midi on" : "midi off";
  return `Playing — ${extLabel} — ${midiLabel} — ${audioContext.sampleRate | 0} Hz`;
}

function mobileExternalRoutingHint(): string {
  if (!isMobileWeb() || !externalEnabled || !audioRunning) {
    return "";
  }
  return " — without headphones, iOS may use the earpiece; plug in headphones or turn External off for speaker";
}

function applyPlayingStatus(): void {
  const base = playingStatusBase();
  if (!base) {
    return;
  }
  const silentSuffix = inputSilentHint ? " — input silent (check mic permission / level)" : "";
  statusEl.textContent = `${base}${silentSuffix}${mobileExternalRoutingHint()}`;
}

function layoutKnobCols(): void {
  knobsEl.innerHTML = "";
  const visibleCount = hostPage === 0 ? TOTAL_KNOB_COUNT : CORE_KNOB_COUNT;
  for (let i = 0; i < visibleCount; i++) {
    knobsEl.appendChild(knobCols[i]);
  }
}

function syncTransportUi(): void {
  playBtn.disabled = audioRunning;
  stopBtn.disabled = !audioRunning;
  if (audioRunning && audioContext) {
    applyPlayingStatus();
  }
  if (!audioRunning || !externalEnabled) {
    renderInputMeter(0, false);
    silentScreenTicks = 0;
    inputSilentHint = false;
  }
}

function renderPageChrome(): void {
  pageChromeTitle.textContent = `${PAGE_NAMES[hostPage]} (${hostPage + 1}/${HOST_PAGE_COUNT})`;
  pageChromeBlurb.textContent = PAGE_BLURBS[hostPage] ?? "";
  for (let i = 0; i < pagePillButtons.length; i++) {
    pagePillButtons[i].classList.toggle("active", i === hostPage);
  }
}

function applyKnobLabelsFromRows(rows: ScreenRow[], pairArRows: ScreenRow[]): void {
  const wasmCore = rows.length === CORE_KNOB_COUNT ? rows : null;
  const wasmPairAr = pairArRows.length === PAIR_AR_KNOB_COUNT ? pairArRows : null;
  for (let i = 0; i < CORE_KNOB_COUNT; i++) {
    const wasmName = wasmCore?.[i]?.name;
    knobMainLabels[i].textContent = wasmName || coreKnobLabel(hostPage, i);
    knobHintLabels[i].textContent = hostPage === 5 ? (DELAY_HINTS[i] ?? "") : "";
    knobHintLabels[i].style.display = "block";
  }
  for (let i = 0; i < PAIR_AR_KNOB_COUNT; i++) {
    const colIndex = CORE_KNOB_COUNT + i;
    const wasmName = wasmPairAr?.[i]?.name;
    knobMainLabels[colIndex].textContent = wasmName || pairArKnobLabel(i);
    knobHintLabels[colIndex].textContent = "";
    knobHintLabels[colIndex].style.display = hostPage === 0 ? "block" : "none";
  }
}

function applyModSourceLabels(names: string[]): void {
  for (let i = 0; i < modBayIndicators.length; i++) {
    const label = names[i] ?? "";
    const indicator = modBayIndicators[i];
    if (indicator.kind === "scope") {
      indicator.scope.setLabel(label);
    } else {
      indicator.led.setLabel(label);
    }
  }
}

function setHostPage(page: number): void {
  hostPage = ((page % HOST_PAGE_COUNT) + HOST_PAGE_COUNT) % HOST_PAGE_COUNT;
  renderPageChrome();
  layoutKnobCols();
  applyKnobLabelsFromRows([], []);
  renderVcoMorphButtons(lastWasmPage);
  if (workletNode) {
    send({ type: "hostPage", page: hostPage });
  }
}

function changeHostPage(delta: number): void {
  hostPage = ((hostPage + delta) % HOST_PAGE_COUNT + HOST_PAGE_COUNT) % HOST_PAGE_COUNT;
  renderPageChrome();
  layoutKnobCols();
  applyKnobLabelsFromRows([], []);
  renderVcoMorphButtons(lastWasmPage);
  if (workletNode) {
    send({ type: "hostPageDelta", delta });
  }
}

function updateEngineDependentUi(): void {
  pageRandKnobsBtn.disabled = !engineReady;
  pageRandModBtn.disabled = !engineReady;
}

function requireEngineForAction(): boolean {
  if (engineReady) {
    return true;
  }
  statusEl.textContent = "Click Play first";
  return false;
}

function syncKnobUi(rows: ScreenRow[], pairArRows: ScreenRow[]): void {
  applyKnobLabelsFromRows(rows, pairArRows);
  for (let i = 0; i < CORE_KNOB_COUNT; i++) {
    const row = rows[i];
    const modIdx = modSelectIndex(row.modSource);
    if (modSelects[i] && modSelects[i].selectedIndex !== modIdx) {
      modSelects[i].selectedIndex = modIdx;
    }
    if (rotaryKnobs[i] && !knobDragging[i]) {
      rotaryKnobs[i].setValue(row.value);
    }
  }
  if (hostPage !== 0) {
    return;
  }
  for (let i = 0; i < PAIR_AR_KNOB_COUNT; i++) {
    const colIndex = CORE_KNOB_COUNT + i;
    const row = pairArRows[i];
    if (!row) {
      continue;
    }
    const modIdx = modSelectIndex(row.modSource);
    if (modSelects[colIndex] && modSelects[colIndex].selectedIndex !== modIdx) {
      modSelects[colIndex].selectedIndex = modIdx;
    }
    if (rotaryKnobs[colIndex] && !knobDragging[colIndex]) {
      rotaryKnobs[colIndex].setValue(row.value);
    }
  }
}

function onScreenUpdate(data: Record<string, unknown>): void {
  const nextPage = data.hostPage as number;
  if (nextPage !== hostPage) {
    hostPage = nextPage;
    renderPageChrome();
  }
  const rows = data.rows as ScreenRow[];
  const pairArRows = (data.pairArRows as ScreenRow[] | undefined) ?? [];
  lastScreenRows = rows;
  lastPairArRows = pairArRows;
  const modSourceNames = data.modSourceNames as string[] | undefined;
  if (modSourceNames && modSourceNames.length >= modBayIndicators.length) {
    applyModSourceLabels(modSourceNames);
  }
  const morphs = data.morphs as number[] | undefined;
  if (morphs && morphs.length >= 3) {
    lastMorphs = morphs.slice(0, 3);
  }
  const wasmPage = data.wasmPage as number;
  renderModBay(
    data.scopeSamples as number[][] | undefined,
    data.modLevels as number[],
    audioRunning
  );
  syncKnobUi(rows, pairArRows);
  renderVcoMorphButtons(wasmPage);
  layoutKnobCols();

  const meterActive = externalEnabled && audioRunning;
  const inputPeak = meterActive ? Math.max(0, Number(data.inputPeak ?? 0)) : 0;
  renderInputMeter(inputPeak, meterActive);
  if (meterActive) {
    if (inputPeak < INPUT_PEAK_SILENT) {
      silentScreenTicks++;
    } else {
      silentScreenTicks = 0;
    }
    const nextHint = silentScreenTicks >= SILENT_SCREEN_TICKS;
    if (nextHint !== inputSilentHint) {
      inputSilentHint = nextHint;
      applyPlayingStatus();
    }
  }
}

for (let i = 0; i < PAGE_NAMES.length; i++) {
  const pill = document.createElement("button");
  pill.type = "button";
  pill.className = "page-pill";
  pill.textContent = PAGE_NAMES[i];
  pill.addEventListener("click", () => setHostPage(i));
  pagePillsEl.appendChild(pill);
  pagePillButtons.push(pill);
}

for (let i = 0; i < TOTAL_KNOB_COUNT; i++) {
  const col = document.createElement("div");
  col.className = "knob-col";
  if (isPairArKnob(i)) {
    col.dataset.pairAr = "true";
  }

  const mainLabel = document.createElement("label");
  mainLabel.className = "knob-label-main";
  mainLabel.textContent = "";
  col.appendChild(mainLabel);
  knobMainLabels.push(mainLabel);

  if (i < 3) {
    const morphBtn = document.createElement("button");
    morphBtn.type = "button";
    morphBtn.className = "vco-morph-btn";
    morphBtn.hidden = true;
    morphBtn.title = "Click to cycle wave morph (sine ↔ saw ↔ square)";
    const vcoIndex = i;
    morphBtn.addEventListener("click", () => {
      if (!requireEngineForAction()) {
        return;
      }
      const v = lastMorphs[vcoIndex] ?? 0;
      let next = 0;
      if (v < 0.25) {
        next = 0.5;
      } else if (v < 0.75) {
        next = 1;
      }
      lastMorphs[vcoIndex] = next;
      morphBtn.innerHTML = waveSvg(next);
      send({ type: "cycleVcoMorph", index: vcoIndex });
    });
    col.appendChild(morphBtn);
    vcoMorphBtns.push(morphBtn);
  } else {
    const morphSlot = document.createElement("span");
    morphSlot.className = "knob-morph-slot";
    morphSlot.setAttribute("aria-hidden", "true");
    col.appendChild(morphSlot);
  }

  const hintLabel = document.createElement("span");
  hintLabel.className = "knob-hint";
  knobHintLabels.push(hintLabel);

  const knob = new RotaryKnob(
    (v) => {
      const modIdx = Number(modSelects[i]?.value ?? 255);
      if (isPairArKnob(i)) {
        const pairIndex = pairArCellIndex(i);
        if (modIdx !== 255) {
          send({ type: "pairArModDepth", index: pairIndex, depth: v });
        } else {
          send({ type: "pairArKnob", index: pairIndex, value: v });
        }
        return;
      }
      if (isDelayPage()) {
        if (modIdx !== 255) {
          send({ type: "delayModDepth", row: i, depth: v });
        } else {
          send({ type: "delayKnob", row: i, value: v });
        }
        return;
      }
      if (modIdx !== 255) {
        send({ type: "modDepth", row: i, depth: v });
      } else {
        send({ type: "knob", index: i, value: v });
      }
    },
    (dragging) => {
      knobDragging[i] = dragging;
    },
    () => {
      const modIdx = Number(modSelects[i]?.value ?? 255);
      if (modIdx === 255) {
        return;
      }
      if (isPairArKnob(i)) {
        const row = lastPairArRows[pairArCellIndex(i)];
        if (!row || row.modSource !== modIdx) {
          return;
        }
        knob.setValue(row.modDepth);
        return;
      }
      const row = lastScreenRows[i];
      if (!row || row.modSource !== modIdx) {
        return;
      }
      knob.setValue(row.modDepth);
    }
  );
  knob.setValue(0.5);
  const knobRow = document.createElement("div");
  knobRow.className = "knob-row";
  knobRow.appendChild(knob.element);
  col.appendChild(knobRow);
  col.appendChild(hintLabel);

  const modLabel = document.createElement("span");
  modLabel.className = "mod-source-label";
  modLabel.textContent = "Mod source";
  col.appendChild(modLabel);

  const modSelect = document.createElement("select");
  modSelect.className = "mod-select";
  modSelect.innerHTML = '<option value="255">None</option>';
  modSelect.addEventListener("change", () => {
    const modIndex = Number(modSelect.value);
    if (isPairArKnob(i)) {
      send({ type: "pairArModSource", index: pairArCellIndex(i), modIndex });
      return;
    }
    if (isDelayPage()) {
      send({ type: "delayModSource", row: i, modIndex });
    } else {
      send({ type: "modSource", row: i, modIndex });
    }
  });
  col.appendChild(modSelect);
  modSelects.push(modSelect);

  rotaryKnobs.push(knob);
  knobCols.push(col);
}

layoutKnobCols();

modBayToggle.addEventListener("click", () => {
  modBayExpanded = !modBayExpanded;
  sessionStorage.setItem("modBayExpanded", String(modBayExpanded));
  modBayEl.classList.toggle("collapsed", !modBayExpanded);
  modBayToggle.setAttribute("aria-expanded", String(modBayExpanded));
  modBayToggle.textContent = modBayExpanded ? "Mod sources ▾" : "Mod sources ▸";
});

pagePrev.addEventListener("click", () => changeHostPage(-1));
pageNext.addEventListener("click", () => changeHostPage(1));

pageRandKnobsBtn.addEventListener("click", () => {
  if (!requireEngineForAction()) {
    return;
  }
  if (isDelayPage()) {
    send({ type: "delayRandomizeKnobs" });
    return;
  }
  send({ type: "randomizePage", page: hostPage });
});

pageRandModBtn.addEventListener("click", () => {
  if (!requireEngineForAction()) {
    return;
  }
  if (isDelayPage()) {
    send({ type: "delayRandomizeMod" });
    return;
  }
  send({ type: "randomizePageMod", page: hostPage });
});

document.getElementById("rand-morphs")?.addEventListener("click", () => {
  if (!requireEngineForAction()) {
    return;
  }
  send({ type: "randomizeMorphs" });
});
document.getElementById("rand-all")?.addEventListener("click", () => {
  send({ type: "randomizeAll" });
});
document.getElementById("rand-mod")?.addEventListener("click", () => {
  send({ type: "randomizeMod" });
});
document.getElementById("marbles-btn")?.addEventListener("click", () => {
  send({ type: "marbles" });
});

function closeHelpMenu(): void {
  appHelpMenu.hidden = true;
  appHeaderTrigger.setAttribute("aria-expanded", "false");
}

async function openHelpDoc(key: string): Promise<void> {
  const entry = HELP_DOC_PATHS[key];
  if (!entry) {
    return;
  }
  closeHelpMenu();
  let text = helpDocCache.get(key);
  if (text === undefined) {
    const response = await fetch(entry.path);
    text = response.ok ? await response.text() : `Could not load ${entry.path}`;
    helpDocCache.set(key, text);
  }
  helpModalTitle.textContent = entry.title;
  helpModalBody.textContent = text;
  helpModal.hidden = false;
}

function closeHelpModal(): void {
  helpModal.hidden = true;
}

appHeaderTrigger.addEventListener("click", (event) => {
  event.stopPropagation();
  const open = appHelpMenu.hidden;
  appHelpMenu.hidden = !open;
  appHeaderTrigger.setAttribute("aria-expanded", String(open));
});

appHelpMenu.querySelectorAll<HTMLButtonElement>("button[data-help]").forEach((btn) => {
  btn.addEventListener("click", () => {
    void openHelpDoc(btn.dataset.help ?? "");
  });
});

helpModalClose.addEventListener("click", closeHelpModal);
helpModal.addEventListener("click", (event) => {
  if (event.target === helpModal) {
    closeHelpModal();
  }
});

document.addEventListener("keydown", (event) => {
  if (event.key === "Escape") {
    closeHelpMenu();
    closeHelpModal();
  }
});

document.addEventListener("click", (event) => {
  if (
    !appHelpMenu.hidden &&
    !appHelpMenu.contains(event.target as Node) &&
    !appHeaderTrigger.contains(event.target as Node)
  ) {
    closeHelpMenu();
  }
});

fieldLayoutEl.addEventListener(
  "pointerdown",
  (e) => {
    swipeStartX = e.clientX;
    swipeStartY = e.clientY;
  },
  { passive: true }
);

fieldLayoutEl.addEventListener("pointerup", (e) => {
  if (knobDragging.some(Boolean)) {
    return;
  }
  const dx = e.clientX - swipeStartX;
  const dy = e.clientY - swipeStartY;
  if (Math.abs(dx) < 60 || Math.abs(dx) < Math.abs(dy)) {
    return;
  }
  changeHostPage(dx < 0 ? 1 : -1);
});

document.addEventListener("keydown", (e) => {
  if (e.key === "[") {
    changeHostPage(-1);
  } else if (e.key === "]") {
    changeHostPage(1);
  } else if (e.key === "m" || e.key === "M") {
    send({ type: "marbles" });
  }
});

function applyExternalUi(enabled: boolean): void {
  externalEnabled = enabled;
  externalBtn.textContent = enabled ? "External Audio: On" : "External Audio: Off";
  externalBtn.classList.toggle("active", enabled);
  send({ type: "external", enabled });
}

function externalErrorMessage(err: unknown): string {
  if (!window.isSecureContext) {
    return "HTTPS required for external audio — allow mic over HTTPS, then click External Audio again";
  }
  if (err instanceof DOMException || err instanceof Error) {
    if (err.name === "NotAllowedError") {
      return "Microphone blocked — allow mic for this site in browser settings, then click External Audio again";
    }
    if (err.name === "NotFoundError") {
      return "No microphone found — connect a mic and click External Audio again";
    }
    return `External error: ${err.message}`;
  }
  return `External error: ${String(err)}`;
}

async function micPermissionDenied(): Promise<boolean> {
  if (!navigator.permissions?.query) {
    return false;
  }
  try {
    const result = await navigator.permissions.query({ name: "microphone" as PermissionName });
    return result.state === "denied";
  } catch {
    return false;
  }
}

function disconnectExternalStream(): void {
  if (micSource) {
    micSource.disconnect();
    micSource = null;
  }
  if (mediaStream) {
    mediaStream.getTracks().forEach((t) => t.stop());
    mediaStream = null;
  }
  applyMobileAudioSession("externalOff");
}

function disconnectExternalMidi(): void {
  for (const [input] of midiInputHandlers) {
    input.onmidimessage = null;
  }
  midiInputHandlers.clear();
  if (midiAccess) {
    midiAccess.onstatechange = null;
  }
  midiAccess = null;
}

function handleWebMidiMessage(event: MIDIMessageEvent): void {
  const data = event.data;
  if (!data || data.length < 3) {
    return;
  }
  const status = data[0];
  if ((status & 0xf0) !== 0xb0) {
    return;
  }
  send({
    type: "midiCc",
    channel: status & 0x0f,
    cc: data[1],
    value: data[2],
  });
}

function externalMidiErrorMessage(err: unknown): string {
  if (!window.isSecureContext) {
    return "HTTPS required for External MIDI — use HTTPS, then click External MIDI again";
  }
  if (typeof navigator.requestMIDIAccess !== "function") {
    return "Web MIDI not supported in this browser";
  }
  if (err instanceof DOMException || err instanceof Error) {
    if (err.name === "NotAllowedError") {
      return "MIDI blocked — allow MIDI for this site in browser settings, then click External MIDI again";
    }
    return `External MIDI error: ${err.message}`;
  }
  return `External MIDI error: ${String(err)}`;
}

function applyExternalMidiUi(enabled: boolean): void {
  externalMidiEnabled = enabled;
  externalMidiBtn.textContent = enabled ? "External MIDI: On" : "External MIDI: Off";
  externalMidiBtn.classList.toggle("active", enabled);
}

function attachWebMidiInputs(access: MIDIAccess): void {
  for (const input of access.inputs.values()) {
    const handler = (event: MIDIMessageEvent) => handleWebMidiMessage(event);
    input.onmidimessage = handler;
    midiInputHandlers.set(input, handler);
  }
  access.onstatechange = () => {
    if (!externalMidiEnabled || !midiAccess) {
      return;
    }
    for (const [input] of midiInputHandlers) {
      input.onmidimessage = null;
    }
    midiInputHandlers.clear();
    for (const input of midiAccess.inputs.values()) {
      const handler = (event: MIDIMessageEvent) => handleWebMidiMessage(event);
      input.onmidimessage = handler;
      midiInputHandlers.set(input, handler);
    }
  };
}

async function setExternalEnabled(enabled: boolean): Promise<void> {
  if (!enabled) {
    disconnectExternalStream();
    applyExternalUi(false);
    silentScreenTicks = 0;
    inputSilentHint = false;
    renderInputMeter(0, false);
    if (audioRunning) {
      applyPlayingStatus();
    }
    return;
  }

  if (!window.isSecureContext || !navigator.mediaDevices?.getUserMedia) {
    statusEl.textContent = externalErrorMessage(new DOMException("Insecure context", "SecurityError"));
    return;
  }

  unlockAudioContext();

  try {
    await initWorklet();
  } catch (err) {
    statusEl.textContent = `Engine error: ${err instanceof Error ? err.message : String(err)}`;
    return;
  }

  if (!audioContext || !workletNode) {
    statusEl.textContent = "Engine not ready — wait for Engine ready, then click External Audio again";
    return;
  }

  if (await micPermissionDenied()) {
    statusEl.textContent = externalErrorMessage(new DOMException("Permission denied", "NotAllowedError"));
    return;
  }

  try {
    applyMobileAudioSession("reset");
    const mobileMic = isMobileWeb();
    mediaStream = await navigator.mediaDevices.getUserMedia({
      audio: {
        echoCancellation: mobileMic,
        noiseSuppression: mobileMic,
        autoGainControl: false,
      },
    });
    micSource = audioContext.createMediaStreamSource(mediaStream);
    micSource.connect(workletNode);
    applyMobileAudioSession("externalOn");
    applyExternalUi(true);
    if (audioRunning) {
      applyPlayingStatus();
    }
  } catch (err) {
    disconnectExternalStream();
    applyExternalUi(false);
    statusEl.textContent = externalErrorMessage(err);
  }
}

externalBtn.addEventListener("click", () => {
  void setExternalEnabled(!externalEnabled);
});

async function setExternalMidiEnabled(enabled: boolean): Promise<void> {
  if (!enabled) {
    disconnectExternalMidi();
    setWebCcPairEnabled(0, false);
    applyExternalMidiUi(false);
    applyModBayAvailability();
    if (audioRunning) {
      applyPlayingStatus();
    }
    return;
  }

  if (!window.isSecureContext || typeof navigator.requestMIDIAccess !== "function") {
    statusEl.textContent = externalMidiErrorMessage(new Error("Web MIDI unavailable"));
    return;
  }

  unlockAudioContext();
  try {
    await initWorklet();
  } catch (err) {
    statusEl.textContent = `Engine error: ${err instanceof Error ? err.message : String(err)}`;
    return;
  }

  try {
    midiAccess = await navigator.requestMIDIAccess({ sysex: false });
    attachWebMidiInputs(midiAccess);
    setWebCcPairEnabled(1, false);
    setWebCcPairEnabled(0, true);
    applyExternalMidiUi(true);
    applyModBayAvailability();
    if (audioRunning) {
      applyPlayingStatus();
    }
  } catch (err) {
    disconnectExternalMidi();
    applyExternalMidiUi(false);
    statusEl.textContent = externalMidiErrorMessage(err);
  }
}

externalMidiBtn.addEventListener("click", () => {
  void setExternalMidiEnabled(!externalMidiEnabled);
});

function handleWorkletMessage(event: MessageEvent): void {
  const data = event.data;
  if (data.type === "screen") {
    onScreenUpdate(data);
    return;
  }
  if (data.type === "error") {
    transportIntentPlaying = false;
    engineReady = false;
    updateEngineDependentUi();
    stopAudio();
    statusEl.textContent = `Error: ${data.message} — click Play to retry`;
    syncTransportUi();
    return;
  }
  if (data.type === "modAvailabilityChanged") {
    const options = data.assignableModOptions as AssignableModOption[] | undefined;
    if (options) {
      populateModSelects(options);
    }
    applyModBayAvailability();
    return;
  }
  if (data.type === "ready") {
    engineReady = true;
    updateEngineDependentUi();
    const modSourceNames = data.modSourceNames as string[] | undefined;
    if (modSourceNames && modSourceNames.length >= modBayIndicators.length) {
      applyModSourceLabels(modSourceNames);
    }
    const options = data.assignableModOptions as AssignableModOption[] | undefined;
    if (options && options.length > 0) {
      populateModSelects(options);
    }
    setWebCcPairEnabled(1, false);
    applyModBayAvailability();
    if (audioContext) {
      send({ type: "setSampleRate", sampleRate: audioContext.sampleRate });
    }
    syncTransportUi();
    if (!audioRunning) {
      statusEl.textContent = "Stopped — click Play";
    }
  }
}

function connectWorkletOutput(): void {
  if (!workletNode || !outputGain) {
    return;
  }
  workletNode.disconnect();
  workletNode.connect(outputGain);
}

function setupAudioContextStateChange(): void {
  if (!audioContext) {
    return;
  }
  audioContext.onstatechange = () => {
    if (!audioContext) {
      return;
    }
    if (audioContext.state === "suspended" && transportIntentPlaying) {
      audioRunning = false;
      statusEl.textContent = "Audio suspended — click Play";
      syncTransportUi();
      return;
    }
    if (audioContext.state === "running" && transportIntentPlaying && workletNode && outputGain) {
      connectWorkletOutput();
      send({ type: "setRunning", running: true });
      audioRunning = true;
      syncTransportUi();
      applyPlayingStatus();
    }
  };
}

function unlockAudioContext(): void {
  if (!audioContext) {
    audioContext = new AudioContext();
    setupAudioContextStateChange();
  }
  if (audioContext.state === "suspended") {
    void audioContext.resume();
  }
}

async function ensureAudioContextRunning(): Promise<boolean> {
  if (!audioContext) {
    return false;
  }
  if (audioContext.state === "suspended") {
    await audioContext.resume();
  }
  return audioContext.state === "running";
}

async function initWorklet(): Promise<void> {
  if (workletNode) {
    return;
  }
  if (!audioContext) {
    throw new Error("AudioContext not initialized");
  }
  statusEl.textContent = "Loading engine...";
  playBtn.disabled = true;
  const [wasmModule] = await Promise.all([
    fetch(wasmUrl).then(async (response) => {
      if (!response.ok) {
        throw new Error(`WASM fetch failed: ${response.status}`);
      }
      return WebAssembly.compile(await response.arrayBuffer());
    }),
    audioContext.audioWorklet.addModule(processorUrl),
  ]);
  workletNode = new AudioWorkletNode(audioContext, "froggers-processor", {
    processorOptions: { wasmModule },
    numberOfInputs: 1,
    numberOfOutputs: 1,
    outputChannelCount: [2],
  });
  workletNode.port.onmessage = handleWorkletMessage;
  outputGain = audioContext.createGain();
  outputGain.gain.value = 1.0;
  outputGain.connect(audioContext.destination);
}

async function startAudio(): Promise<void> {
  if (audioRunning && audioContext?.state === "running") {
    return;
  }
  playBtn.disabled = true;
  unlockAudioContext();
  transportIntentPlaying = true;
  try {
    if (!workletNode) {
      await initWorklet();
    }
    if (!workletNode || !audioContext || !outputGain) {
      transportIntentPlaying = false;
      syncTransportUi();
      return;
    }
    if (!(await ensureAudioContextRunning())) {
      audioRunning = false;
      syncTransportUi();
      statusEl.textContent = "Audio suspended — click Play";
      return;
    }
    connectWorkletOutput();
    send({ type: "setRunning", running: true });
    send({ type: "external", enabled: externalEnabled });
    audioRunning = true;
    if (!externalEnabled) {
      applyMobileAudioSession("playback");
    }
    syncTransportUi();
    if (externalEnabled) {
      await setExternalEnabled(true);
    }
    if (externalMidiEnabled) {
      await setExternalMidiEnabled(true);
    }
  } catch (err) {
    statusEl.textContent = `Audio error: ${err instanceof Error ? err.message : String(err)} — click Play to retry`;
    transportIntentPlaying = false;
    audioRunning = false;
    syncTransportUi();
  }
}

function stopAudio(): void {
  send({ type: "setRunning", running: false });
  transportIntentPlaying = false;
  audioRunning = false;
  disconnectExternalStream();
  externalEnabled = false;
  externalBtn.textContent = "External Audio: Off";
  externalBtn.classList.remove("active");
  send({ type: "external", enabled: false });
  disconnectExternalMidi();
  setWebCcPairEnabled(0, false);
  externalMidiEnabled = false;
  externalMidiBtn.textContent = "External MIDI: Off";
  externalMidiBtn.classList.remove("active");
  if (workletNode) {
    workletNode.disconnect();
  }
  syncTransportUi();
  silentScreenTicks = 0;
  inputSilentHint = false;
  renderInputMeter(0, false);
  statusEl.textContent = engineReady ? "Stopped — click Play" : "Click Play to start";
  renderModBay(undefined, [0, 0, 0, 0, 0, 0, 0], false);
}

playBtn.addEventListener("click", () => {
  void startAudio();
});
stopBtn.addEventListener("click", () => {
  stopAudio();
});

initModBay();
renderPageChrome();
applyKnobLabelsFromRows(lastScreenRows, lastPairArRows);
renderVcoMorphButtons(0);
renderInputMeter(0, false);
renderModBay(undefined, [0, 0, 0, 0, 0, 0, 0], false);
updateEngineDependentUi();
syncTransportUi();

if (typeof navigator.requestMIDIAccess !== "function") {
  externalMidiBtn.disabled = true;
  externalMidiBtn.title = "Web MIDI not supported in this browser";
}

document.addEventListener(
  "touchstart",
  () => {
    unlockAudioContext();
  },
  { once: true, passive: true }
);
