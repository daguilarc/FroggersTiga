export type CvScopeMode = "continuous" | "stepHold";

export class CvScopeCanvas {
  readonly element: HTMLDivElement;
  private readonly canvas: HTMLCanvasElement;
  private readonly ctx: CanvasRenderingContext2D;
  private readonly mode: CvScopeMode;
  private readonly samples: Float32Array;
  private writeIndex = 0;
  private hasSamples = false;
  private lastLevel = 0.5;
  private idle = false;

  constructor(label: string, mode: CvScopeMode, bufferSize = 96) {
    this.mode = mode;
    this.samples = new Float32Array(bufferSize);
    this.element = document.createElement("div");
    this.element.className = "mod-scope-cell";
    const labelEl = document.createElement("span");
    labelEl.className = "mod-label";
    labelEl.textContent = label;
    this.canvas = document.createElement("canvas");
    this.canvas.className = "mod-scope-canvas";
    this.canvas.width = 120;
    this.canvas.height = 40;
    const ctx = this.canvas.getContext("2d");
    if (!ctx) {
      throw new Error("2d context unavailable");
    }
    this.ctx = ctx;
    this.element.appendChild(labelEl);
    this.element.appendChild(this.canvas);
  }

  setIdle(idle: boolean): void {
    this.idle = idle;
    this.element.classList.toggle("mod-scope-idle", idle);
    this.draw();
  }

  pushSample(value: number): void {
    const clamped = Math.min(Math.max(value, 0), 1);
    this.samples[this.writeIndex] = clamped;
    this.writeIndex = (this.writeIndex + 1) % this.samples.length;
    this.hasSamples = true;
    this.lastLevel = clamped;
  }

  pushBlock(values: number[]): void {
    for (const value of values) {
      this.pushSample(value);
    }
  }

  draw(): void {
    const { width, height } = this.canvas;
    const g = this.ctx;
    g.clearRect(0, 0, width, height);
    g.fillStyle = "#111";
    g.fillRect(0, 0, width, height);
    const alpha = this.idle ? 0.35 : 1.0;
    g.globalAlpha = alpha;

    if (this.mode === "stepHold" || !this.hasSamples) {
      const y = height - this.lastLevel * (height - 4) - 2;
      g.fillStyle = "#c9a227";
      g.fillRect(2, y, width - 4, 2);
      g.globalAlpha = 1;
      return;
    }

    g.strokeStyle = "#c9a227";
    g.lineWidth = 1.5;
    g.beginPath();
    const count = this.samples.length;
    for (let i = 0; i < count; i++) {
      const idx = (this.writeIndex + i) % count;
      const x = 2 + (i / (count - 1)) * (width - 4);
      const y = height - this.samples[idx] * (height - 4) - 2;
      if (i === 0) {
        g.moveTo(x, y);
      } else {
        g.lineTo(x, y);
      }
    }
    g.stroke();
    g.globalAlpha = 1;
  }
}
