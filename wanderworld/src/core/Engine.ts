/**
 * Engine — orchestrates renderer, scene, clock, and (later) world/player/input.
 *
 * Phase 0/1 scope: render loop, clock, resize handling, and a placeholder scene.
 * Subsequent phases plug World, Player, Input, UI through clear interfaces.
 */
import * as THREE from 'three';
import Stats from 'stats.js';
import { Renderer } from './Renderer';
import { Scene } from './Scene';
import { Clock } from './Clock';
import type { DeviceProfile } from '../utils/deviceDetect';

export interface EngineOptions {
  canvas: HTMLCanvasElement;
  device: DeviceProfile;
}

export class Engine {
  private readonly canvas: HTMLCanvasElement;
  private readonly device: DeviceProfile;

  private renderer!: Renderer;
  private sceneWrap!: Scene;
  private camera!: THREE.PerspectiveCamera;
  private clock!: Clock;
  private stats: Stats | null = null;

  private rafId = 0;
  private running = false;
  private resizeHandler = (): void => this.onResize();

  constructor(opts: EngineOptions) {
    this.canvas = opts.canvas;
    this.device = opts.device;
  }

  async init(): Promise<void> {
    this.renderer = new Renderer({ canvas: this.canvas, device: this.device });
    this.sceneWrap = new Scene();

    // Temporary fixed camera. Phase 5 will replace this with player-attached FPS camera.
    this.camera = new THREE.PerspectiveCamera(
      70,
      window.innerWidth / window.innerHeight,
      0.1,
      1000,
    );
    this.camera.position.set(8, 6, 12);
    this.camera.lookAt(0, 1, 0);

    this.clock = new Clock();

    if (import.meta.env.DEV) {
      this.stats = new Stats();
      this.stats.dom.style.position = 'fixed';
      this.stats.dom.style.left = '8px';
      this.stats.dom.style.top = '8px';
      document.body.appendChild(this.stats.dom);
    }

    window.addEventListener('resize', this.resizeHandler, { passive: true });
    this.onResize();
  }

  start(): void {
    if (this.running) return;
    this.running = true;
    this.clock.reset();
    this.tick();
  }

  stop(): void {
    this.running = false;
    if (this.rafId) cancelAnimationFrame(this.rafId);
    this.rafId = 0;
  }

  dispose(): void {
    this.stop();
    window.removeEventListener('resize', this.resizeHandler);
    this.renderer.dispose();
    this.sceneWrap.dispose();
  }

  private tick = (): void => {
    if (!this.running) return;
    this.stats?.begin();

    const dt = this.clock.tickSeconds();
    this.sceneWrap.update(dt);
    this.renderer.render(this.sceneWrap.scene, this.camera);

    this.stats?.end();
    this.rafId = requestAnimationFrame(this.tick);
  };

  private onResize(): void {
    const w = window.innerWidth;
    const h = window.innerHeight;
    this.camera.aspect = w / h;
    this.camera.updateProjectionMatrix();
    this.renderer.resize(w, h);
  }
}
