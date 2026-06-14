const LED_ON_THRESHOLD = 0.55;

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
  }

  setLevel(level: number): void {
    const clamped = Math.min(Math.max(level, 0), 1);
    this.ledEl.dataset.on = clamped > LED_ON_THRESHOLD ? "true" : "false";
  }

  setLabel(label: string): void {
    const labelEl = this.element.querySelector(".mod-label");
    if (labelEl) {
      labelEl.textContent = label;
    }
  }
}
