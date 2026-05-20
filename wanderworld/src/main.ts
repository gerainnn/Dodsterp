/**
 * Wanderworld — entry point.
 * Boots the engine and hides the loading screen once ready.
 */
import { Engine } from './core/Engine';
import { detectDevice } from './utils/deviceDetect';

async function bootstrap(): Promise<void> {
  const canvas = document.getElementById('scene-canvas') as HTMLCanvasElement | null;
  if (!canvas) {
    throw new Error('scene-canvas element not found');
  }

  const device = detectDevice();
  console.info('[Wanderworld] device profile:', device);

  setLoaderProgress(10, 'Creating renderer…');

  const engine = new Engine({ canvas, device });

  setLoaderProgress(40, 'Building scene…');
  await engine.init();

  setLoaderProgress(90, 'Warming up…');
  // Allow one frame so first paint happens before we hide loader.
  await nextFrame();
  engine.start();

  setLoaderProgress(100, 'Ready');
  hideLoader();

  // Expose for quick debugging in dev.
  if (import.meta.env.DEV) {
    (window as unknown as { __engine?: Engine }).__engine = engine;
  }
}

function setLoaderProgress(percent: number, text: string): void {
  const fill = document.getElementById('loader-fill');
  const label = document.getElementById('loader-text');
  if (fill) fill.style.width = `${Math.min(100, Math.max(0, percent))}%`;
  if (label) label.textContent = text;
}

function hideLoader(): void {
  const el = document.getElementById('loading');
  if (!el) return;
  el.classList.add('hidden');
  // Remove from DOM after transition to free pixels for the canvas.
  setTimeout(() => el.remove(), 500);
}

function nextFrame(): Promise<void> {
  return new Promise((resolve) => requestAnimationFrame(() => resolve()));
}

bootstrap().catch((err) => {
  console.error('[Wanderworld] fatal during bootstrap', err);
  setLoaderProgress(100, `Error: ${err instanceof Error ? err.message : String(err)}`);
});
