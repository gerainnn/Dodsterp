/**
 * Engine — wires Renderer, scene, sky, water, world, player, camera, input,
 * and minimap into a single update loop.
 *
 *   per frame:
 *     dt = clock.tickSeconds()
 *     input = inputManager.consume()
 *     player.update(dt, input, world)
 *     camera.follow(player)
 *     world.update(player.x, player.z, dt) — chunk paging
 *     skySystem.followShadowCamera(player.position)
 *     water.follow(player.position) ; water.update(dt)
 *     grass.setCamera(camera)
 *     minimap.draw(player, world)
 *     renderer.render(scene, camera)
 */
import * as THREE from 'three';
import Stats from 'stats.js';
import { Renderer } from './Renderer';
import { Clock } from './Clock';
import { World } from '../world/World';
import { Player } from '../player/Player';
import { CameraController } from '../player/CameraController';
import { InputManager } from '../input/InputManager';
import { SkySystem } from '../render/SkySystem';
import { Water } from '../render/Water';
import { Minimap } from '../ui/Minimap';
import type { DeviceProfile } from '../utils/deviceDetect';
import { CHUNK_SIZE } from '../world/Chunk';

export interface EngineOptions {
  canvas: HTMLCanvasElement;
  device: DeviceProfile;
  joystickZone: HTMLElement;
  hudRoot: HTMLElement;
  /** Optional fixed seed; otherwise a random one is generated. */
  seed?: number;
}

export class Engine {
  readonly seed: number;
  private readonly opts: EngineOptions;
  private readonly clock = new Clock();

  private renderer!: Renderer;
  private scene!: THREE.Scene;
  private camera!: CameraController;
  private skySystem!: SkySystem;
  private water!: Water;
  private world!: World;
  private player!: Player;
  private input!: InputManager;
  private minimap!: Minimap;
  private stats: Stats | null = null;

  private rafId = 0;
  private running = false;
  private resizeHandler = (): void => this.onResize();
  private knownChunks = new Set<string>();

  constructor(opts: EngineOptions) {
    this.opts = opts;
    this.seed = opts.seed ?? randomSeed();
  }

  async init(): Promise<void> {
    this.renderer = new Renderer({ canvas: this.opts.canvas, device: this.opts.device });

    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0xa0c4ff);
    this.scene.fog = new THREE.Fog(0xa0c4ff, 60, 480);

    this.camera = new CameraController(window.innerWidth, window.innerHeight, 72);

    this.skySystem = new SkySystem(this.scene);
    this.water = new Water(this.scene);
    this.water.setSun(this.skySystem.sunDirection, this.skySystem.sunColor);

    const renderDistance =
      this.opts.device.preset === 'ultra' ? 12 :
      this.opts.device.preset === 'high'  ? 8  :
      5;

    this.world = new World({
      seed: this.seed,
      renderDistance,
      scene: this.scene,
    });

    this.player = new Player();
    // Find a non-water spawn near the origin.
    const spawn = pickSpawn(this.world);
    this.player.setGroundPosition(this.world, spawn.x, spawn.z);
    // Eagerly load the spawn chunk so the player isn't suspended in mid-air.
    this.world.ensureChunkAt(spawn.x, spawn.z);
    this.player.setGroundPosition(this.world, spawn.x, spawn.z);

    this.minimap = new Minimap(this.opts.hudRoot);
    this.minimap.onTeleport(({ worldX, worldZ }) => {
      const ax = this.player.position.x + worldX;
      const az = this.player.position.z + worldZ;
      this.world.ensureChunkAt(ax, az);
      this.player.setGroundPosition(this.world, ax, az);
    });

    this.input = new InputManager({
      joystickZone: this.opts.joystickZone,
      swipeTarget: this.opts.canvas,
      uiBlockers: [this.opts.joystickZone, this.minimap.element],
    });

    if (import.meta.env.DEV) {
      this.stats = new Stats();
      this.stats.dom.style.cssText = 'position:fixed;left:8px;top:8px;z-index:100;';
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
    this.input.dispose();
    this.renderer.dispose();
  }

  private tick = (): void => {
    if (!this.running) return;
    this.stats?.begin();

    const dt = this.clock.tickSeconds();
    const snapshot = this.input.consume();
    this.player.update(dt, snapshot, this.world);
    this.camera.follow(this.player);

    this.world.update(this.player.position.x, this.player.position.z, dt);
    this.syncMinimapTiles();

    this.skySystem.followShadowCamera(this.player.position);
    this.water.follow(this.player.position);
    this.water.update(dt);
    this.water.setSun(this.skySystem.sunDirection, this.skySystem.sunColor);

    this.world.setCamera(this.camera.camera);
    this.world.setSun(this.skySystem.sunDirection, this.skySystem.sunColor);

    this.minimap.draw(this.player, this.world);

    this.renderer.render(this.scene, this.camera.camera);

    this.stats?.end();
    this.rafId = requestAnimationFrame(this.tick);
  };

  /** Reconcile the minimap tile cache with currently loaded chunks. */
  private syncMinimapTiles(): void {
    const seen = new Set<string>();
    for (const c of this.world.iterChunks()) {
      const k = `${c.cx},${c.cz}`;
      seen.add(k);
      if (!this.knownChunks.has(k)) {
        this.minimap.cacheChunkTile(c.cx, c.cz, c.minimapTile);
      }
    }
    for (const k of this.knownChunks) {
      if (!seen.has(k)) {
        const [cx, cz] = k.split(',').map(Number);
        this.minimap.releaseChunkTile(cx, cz);
      }
    }
    this.knownChunks = seen;
  }

  private onResize(): void {
    const w = window.innerWidth;
    const h = window.innerHeight;
    this.camera.resize(w, h);
    this.renderer.resize(w, h);
  }
}

function pickSpawn(world: World): { x: number; z: number } {
  // Walk outward from the origin in steps of CHUNK_SIZE until we find dry land.
  for (let r = 0; r < 16; r++) {
    for (let i = 0; i < 8; i++) {
      const a = (i / 8) * Math.PI * 2;
      const x = Math.cos(a) * r * CHUNK_SIZE;
      const z = Math.sin(a) * r * CHUNK_SIZE;
      const sample = world.terrain.sample(x, z);
      if (sample.height > 0.5 && sample.biome !== 'lake') return { x, z };
    }
  }
  return { x: 0, z: 0 };
}

function randomSeed(): number {
  return Math.floor(Math.random() * 0xffffffff) >>> 0;
}
