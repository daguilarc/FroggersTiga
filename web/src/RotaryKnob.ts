export class RotaryKnob {
  readonly element: HTMLDivElement;
  private pointerEl: HTMLDivElement;
  private value = 0.5;
  private dragStartY = 0;
  private dragStartValue = 0.5;
  private dragging = false;
  private readonly onChange: (value: number) => void;
  private readonly onDragState: (dragging: boolean) => void;
  private readonly onDragStart?: () => void;

  constructor(
    onChange: (value: number) => void,
    onDragState: (dragging: boolean) => void,
    onDragStart?: () => void
  ) {
    this.onChange = onChange;
    this.onDragState = onDragState;
    this.onDragStart = onDragStart;
    this.element = document.createElement("div");
    this.element.className = "rotary-knob";
    this.pointerEl = document.createElement("div");
    this.pointerEl.className = "rotary-knob-pointer";
    this.element.appendChild(this.pointerEl);
    this.element.addEventListener("pointerdown", this.onPointerDown);
    this.element.addEventListener("pointermove", this.onPointerMove);
    this.element.addEventListener("pointerup", this.onPointerUp);
    this.element.addEventListener("pointercancel", this.onPointerUp);
    this.paint();
  }

  setValue(value: number): void {
    this.value = Math.min(Math.max(value, 0), 1);
    this.paint();
  }

  getValue(): number {
    return this.value;
  }

  private paint(): void {
    const angle = -135 + this.value * 270;
    this.pointerEl.style.transform = `rotate(${angle}deg)`;
  }

  private readonly onPointerDown = (event: PointerEvent): void => {
    this.dragging = true;
    this.dragStartY = event.clientY;
    this.onDragStart?.();
    this.dragStartValue = this.value;
    this.element.setPointerCapture(event.pointerId);
    this.onDragState(true);
  };

  private readonly onPointerMove = (event: PointerEvent): void => {
    if (!this.dragging) {
      return;
    }
    const delta = (this.dragStartY - event.clientY) / 120;
    this.value = Math.min(Math.max(this.dragStartValue + delta, 0), 1);
    this.paint();
    this.onChange(this.value);
  };

  private readonly onPointerUp = (event: PointerEvent): void => {
    if (!this.dragging) {
      return;
    }
    this.dragging = false;
    if (this.element.hasPointerCapture(event.pointerId)) {
      this.element.releasePointerCapture(event.pointerId);
    }
    this.onDragState(false);
  };
}
