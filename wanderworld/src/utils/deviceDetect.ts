/**
 * Device detection — chooses an initial quality preset.
 * Only used at startup; QualityManager later refines based on FPS.
 */

export type QualityPreset = 'low' | 'high' | 'ultra';

export interface DeviceProfile {
  isMobile: boolean;
  isTouch: boolean;
  pixelRatio: number;
  preset: QualityPreset;
  webgl2: boolean;
  gpuRenderer: string | null;
}

export function detectDevice(): DeviceProfile {
  const ua = navigator.userAgent;
  const isMobile = /Android|iPhone|iPad|iPod|Mobile/i.test(ua);
  const isTouch = 'ontouchstart' in window || navigator.maxTouchPoints > 0;
  const pixelRatio = Math.min(window.devicePixelRatio || 1, 2);

  const { webgl2, gpuRenderer } = probeWebGL();

  // Initial preset heuristic. Refined live by QualityManager once FPS samples exist.
  let preset: QualityPreset;
  if (!webgl2) {
    preset = 'low';
  } else if (isMobile) {
    preset = 'high'; // Targeting Poco X6 Pro class. Fallbacks to low if FPS drops.
  } else {
    preset = 'ultra';
  }

  return { isMobile, isTouch, pixelRatio, preset, webgl2, gpuRenderer };
}

function probeWebGL(): { webgl2: boolean; gpuRenderer: string | null } {
  const canvas = document.createElement('canvas');
  const gl = canvas.getContext('webgl2');
  if (!gl) return { webgl2: false, gpuRenderer: null };

  let renderer: string | null = null;
  const dbg = gl.getExtension('WEBGL_debug_renderer_info');
  if (dbg) {
    renderer = gl.getParameter(dbg.UNMASKED_RENDERER_WEBGL) as string;
  }

  // Free the probe context immediately.
  const lose = gl.getExtension('WEBGL_lose_context');
  lose?.loseContext();

  return { webgl2: true, gpuRenderer: renderer };
}
