/**
 * InputManager — single source of truth for player input.
 *
 * Sources:
 *   - Virtual joystick (nipplejs) anchored to a DOM zone — preferred on touch.
 *   - Touch swipe on the canvas — drives camera look.
 *   - Keyboard (WASD/Shift) + mouse PointerLock — desktop fallback.
 *
 * The engine consumes a per-frame `InputSnapshot` and the manager resets the
 * "delta" channels (look) right after.
 */
import nipplejs, { type JoystickManager } from 'nipplejs';

export interface InputSnapshot {
  /** Strafe: -1..1, +1 = right. */
  moveX: number;
  /** Forward: -1..1, +1 = forward. */
  moveZ: number;
  /** Yaw delta (radians) accumulated since last frame. */
  lookDX: number;
  /** Pitch delta (radians) accumulated since last frame. */
  lookDY: number;
  run: boolean;
}

export interface InputManagerOptions {
  joystickZone: HTMLElement;
  /** Canvas / overlay to listen for swipe gestures on. */
  swipeTarget: HTMLElement;
  /** Element rectangles to ignore when starting a swipe (e.g. minimap, joystick zone). */
  uiBlockers: HTMLElement[];
}

const TOUCH_LOOK_SENSITIVITY = 0.0035; // radians per pixel
const MOUSE_LOOK_SENSITIVITY = 0.0022;

export class InputManager {
  private moveX = 0;
  private moveZ = 0;
  private lookDX = 0;
  private lookDY = 0;
  private run = false;

  private joystick: JoystickManager | null = null;
  private readonly opts: InputManagerOptions;
  private readonly keysDown = new Set<string>();
  private activeSwipePointer: number | null = null;
  private lastX = 0;
  private lastY = 0;
  private mouseLocked = false;
  private snapshot: InputSnapshot = {
    moveX: 0,
    moveZ: 0,
    lookDX: 0,
    lookDY: 0,
    run: false,
  };

  constructor(opts: InputManagerOptions) {
    this.opts = opts;
    this.attachJoystick();
    this.attachSwipe();
    this.attachKeyboard();
    this.attachMouse();
  }

  /** Read accumulated input for this frame, then clear deltas. */
  consume(): InputSnapshot {
    // Combine joystick (touch) + WASD (desktop) — the larger wins per axis.
    const wasdX = (this.keysDown.has('KeyD') ? 1 : 0) - (this.keysDown.has('KeyA') ? 1 : 0);
    const wasdZ = (this.keysDown.has('KeyW') ? 1 : 0) - (this.keysDown.has('KeyS') ? 1 : 0);
    const finalX = clamp(this.moveX + wasdX, -1, 1);
    const finalZ = clamp(this.moveZ + wasdZ, -1, 1);
    this.snapshot.moveX = finalX;
    this.snapshot.moveZ = finalZ;
    this.snapshot.lookDX = this.lookDX;
    this.snapshot.lookDY = this.lookDY;
    this.snapshot.run = this.run || this.keysDown.has('ShiftLeft') || this.keysDown.has('ShiftRight');
    this.lookDX = 0;
    this.lookDY = 0;
    return this.snapshot;
  }

  dispose(): void {
    this.joystick?.destroy();
  }

  // ----- Sub-source attachments -----

  private attachJoystick(): void {
    this.joystick = nipplejs.create({
      zone: this.opts.joystickZone,
      mode: 'static',
      position: { left: '50%', top: '50%' },
      color: 'rgba(255, 255, 255, 0.85)',
      size: 110,
      restOpacity: 0.55,
      fadeTime: 100,
    });
    this.joystick.on('move', (_e, data) => {
      if (data.vector) {
        this.moveX = clamp(data.vector.x, -1, 1);
        this.moveZ = clamp(data.vector.y, -1, 1); // up on stick = forward
      }
      this.run = data.distance != null && data.distance > 45;
    });
    this.joystick.on('end', () => {
      this.moveX = 0;
      this.moveZ = 0;
      this.run = false;
    });
  }

  private isInsideBlocker(clientX: number, clientY: number): boolean {
    for (const el of this.opts.uiBlockers) {
      const r = el.getBoundingClientRect();
      if (clientX >= r.left && clientX <= r.right && clientY >= r.top && clientY <= r.bottom) {
        return true;
      }
    }
    return false;
  }

  private attachSwipe(): void {
    const t = this.opts.swipeTarget;
    t.addEventListener('pointerdown', (e: PointerEvent) => {
      if (e.pointerType === 'mouse') return; // mouse is handled by pointer lock path
      if (this.activeSwipePointer !== null) return;
      if (this.isInsideBlocker(e.clientX, e.clientY)) return;
      this.activeSwipePointer = e.pointerId;
      this.lastX = e.clientX;
      this.lastY = e.clientY;
      t.setPointerCapture(e.pointerId);
    });
    t.addEventListener('pointermove', (e: PointerEvent) => {
      if (this.activeSwipePointer !== e.pointerId) return;
      const dx = e.clientX - this.lastX;
      const dy = e.clientY - this.lastY;
      this.lastX = e.clientX;
      this.lastY = e.clientY;
      this.lookDX += dx * TOUCH_LOOK_SENSITIVITY;
      this.lookDY += dy * TOUCH_LOOK_SENSITIVITY;
    });
    const release = (e: PointerEvent): void => {
      if (this.activeSwipePointer === e.pointerId) {
        this.activeSwipePointer = null;
        try {
          t.releasePointerCapture(e.pointerId);
        } catch {
          /* may already be released */
        }
      }
    };
    t.addEventListener('pointerup', release);
    t.addEventListener('pointercancel', release);
  }

  private attachKeyboard(): void {
    window.addEventListener('keydown', (e) => {
      this.keysDown.add(e.code);
      // Toggle pointer lock on Esc — handled by the browser itself.
    });
    window.addEventListener('keyup', (e) => {
      this.keysDown.delete(e.code);
    });
  }

  private attachMouse(): void {
    // Click to lock (desktop only). Outside click handler stays inert on touch.
    this.opts.swipeTarget.addEventListener('click', (e: MouseEvent) => {
      if (this.isInsideBlocker(e.clientX, e.clientY)) return;
      const t = this.opts.swipeTarget;
      // Only request pointer lock for mouse-class pointers.
      if ('requestPointerLock' in t && navigator.maxTouchPoints === 0) {
        t.requestPointerLock?.();
      }
    });
    document.addEventListener('pointerlockchange', () => {
      this.mouseLocked = document.pointerLockElement === this.opts.swipeTarget;
    });
    document.addEventListener('mousemove', (e: MouseEvent) => {
      if (!this.mouseLocked) return;
      this.lookDX += e.movementX * MOUSE_LOOK_SENSITIVITY;
      this.lookDY += e.movementY * MOUSE_LOOK_SENSITIVITY;
    });
  }
}

function clamp(v: number, lo: number, hi: number): number {
  return v < lo ? lo : v > hi ? hi : v;
}
