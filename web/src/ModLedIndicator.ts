import { modLedDisplayBrightness } from "./hostDisplay.generated";

export class ModLedIndicator {
  readonly element: HTMLDivElement;
  private readonly ledEl: HTMLDivElement;

  constructor(label: string) {
    this.element = document.createElement("div");
    this.element.className = "mod-led-cell";
    const labelEl = document.createElement("span");
    labelEl.className = "mod-label";
    labelEl.textContent = label;
    this.ledEl = document.createElement("div");
    this.ledEl.className = "mod-led";
    this.element.appendChild(labelEl);
    this.element.appendChild(this.ledEl);
    this.setLevel(0, false);
  }

  setLevel(level: number, active: boolean): void {
    const brightness = modLedDisplayBrightness(level, active);
    const formatted = brightness.toFixed(3);
    this.ledEl.dataset.brightness = formatted;
    this.ledEl.style.setProperty("--mod-led-brightness", formatted);
  }

  setLabel(label: string): void {
    const labelEl = this.element.querySelector(".mod-label");
    if (labelEl) {
      labelEl.textContent = label;
    }
  }
}
