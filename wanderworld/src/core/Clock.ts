/**
 * Clock — wall-clock delta time provider with a hard cap to avoid
 * giant deltas after tab is restored from background.
 */
export class Clock {
  private last = 0;
  private readonly maxDelta = 0.1; // seconds

  reset(): void {
    this.last = performance.now();
  }

  /** Returns delta time in seconds since the last call (clamped). */
  tickSeconds(): number {
    const now = performance.now();
    if (this.last === 0) this.last = now;
    const dt = Math.min((now - this.last) / 1000, this.maxDelta);
    this.last = now;
    return dt;
  }
}
