/**
 * Renderer — wraps THREE.WebGLRenderer with project-wide settings.
 *
 * Phase 1: basic renderer with sRGB output and ACES tonemapping.
 * Later phases attach EffectComposer for post-processing.
 */
import * as THREE from 'three';
import type { DeviceProfile } from '../utils/deviceDetect';

export interface RendererOptions {
  canvas: HTMLCanvasElement;
  device: DeviceProfile;
}

export class Renderer {
  readonly gl: THREE.WebGLRenderer;
  private readonly device: DeviceProfile;

  constructor(opts: RendererOptions) {
    this.device = opts.device;

    if (!this.device.webgl2) {
      throw new Error('WebGL2 is required. Please use a recent Chrome / Safari / Firefox.');
    }

    this.gl = new THREE.WebGLRenderer({
      canvas: opts.canvas,
      antialias: false, // We will use TAA later. AA off saves a lot of GPU on mobile.
      alpha: false,
      powerPreference: 'high-performance',
      stencil: false,
      depth: true,
    });

    this.gl.outputColorSpace = THREE.SRGBColorSpace;
    this.gl.toneMapping = THREE.ACESFilmicToneMapping;
    this.gl.toneMappingExposure = 1.0;
    this.gl.shadowMap.enabled = true;
    this.gl.shadowMap.type = THREE.PCFSoftShadowMap;

    // Cap pixel ratio. On Poco X6 Pro DPR is ~2.75; we render at 1080p logical
    // and rely on FSR1/upscale later (Phase 12).
    this.gl.setPixelRatio(Math.min(this.device.pixelRatio, 1.5));
  }

  resize(width: number, height: number): void {
    this.gl.setSize(width, height, false);
  }

  render(scene: THREE.Scene, camera: THREE.Camera): void {
    this.gl.render(scene, camera);
  }

  dispose(): void {
    this.gl.dispose();
  }
}
