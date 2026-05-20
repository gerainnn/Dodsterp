/**
 * Wanderworld — entry point.
 * Boots the engine, hides the loader, exposes seed in the URL for sharing.
 */
import { Engine } from './core/Engine';
import { detectDevice } from './utils/deviceDetect';

async function bootstrap(): Promise<void> {
  const canvas = document.getElementById('scene-canvas') as HTMLCanvasElement | null;
  const hudRoot = document.getElementById('hud') as HTMLElement | null;
  const joystickZone = document.getElementById('joystick-zone') as HTMLElement | null;
  if (!canvas || !hudRoot || !joystickZone) {
    throw new Error('Required DOM mounts missing');
  }

  const device = detectDevice();
  console.info('[Wanderworld] device profile', device);

  setLoaderProgress(15, 'Booting engine…');

  // Optional ?seed= URL parameter for reproducible worlds.
  const params = new URLSearchParams(window.location.search);
  const seedParam = params.get('seed');
  const seed = seedParam !== null ? Number.parseInt(seedParam, 10) >>> 0 : undefined;

  const engine = new Engine({ canvas, device, joystickZone, hudRoot, seed });

  setLoaderProgress(45, 'Carving terrain…');
  await engine.init();

  setLoaderProgress(85, 'Planting forests…');
  await nextFrame();
  engine.start();

  // Mirror seed into the URL so users can refresh / share their world.
  const u = new URL(window.location.href);
  u.searchParams.set('seed', String(engine.seed));
  window.history.replaceState(null, '', u.toString());

  setLoaderProgress(100, 'Ready');
  setTimeout(hideLoader, 200);

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
  setTimeout(() => el.remove(), 600);
}

function nextFrame(): Promise<void> {
  return new Promise((resolve) => requestAnimationFrame(() => resolve()));
}

bootstrap().catch((err) => {
  console.error('[Wanderworld] fatal during bootstrap', err);
  setLoaderProgress(100, `Error: ${err instanceof Error ? err.message : String(err)}`);
});
