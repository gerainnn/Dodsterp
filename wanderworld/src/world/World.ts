/**
 * World — manages chunk lifecycle, vegetation/grass batching, and
 * exposes terrain queries to the player.
 *
 * Single update loop:
 *   1. Compute the player's current chunk.
 *   2. Mark all chunks within renderDistance as "wanted".
 *   3. Generate at most one new chunk per frame to avoid hitches.
 *   4. Dispose chunks outside renderDistance + 1.
 */
import * as THREE from 'three';
import { CHUNK_SIZE, CHUNK_VERTS, type ChunkData, generateChunk, disposeChunk, bilinearHeight } from './Chunk';
import { TerrainGenerator } from './TerrainGenerator';
import { mulberry32, hash2 } from '../utils/rng';
import { Vegetation } from './Vegetation';
import { Grass } from './Grass';

export interface WorldOptions {
  seed: number;
  renderDistance: number; // in chunks
  scene: THREE.Scene;
}

export class World {
  readonly seed: number;
  readonly terrain: TerrainGenerator;
  readonly scene: THREE.Scene;

  private renderDistance: number;
  private readonly chunks = new Map<string, ChunkData>();
  private readonly pending = new Set<string>();
  private readonly vegetation: Vegetation;
  private readonly grass: Grass;

  constructor(opts: WorldOptions) {
    this.seed = opts.seed;
    this.terrain = new TerrainGenerator(opts.seed);
    this.scene = opts.scene;
    this.renderDistance = opts.renderDistance;
    this.vegetation = new Vegetation(opts.scene);
    this.grass = new Grass(opts.scene);
  }

  /** Public terrain height query for player physics. Bilinear. */
  heightAt(wx: number, wz: number): number {
    const cx = Math.floor(wx / CHUNK_SIZE);
    const cz = Math.floor(wz / CHUNK_SIZE);
    const c = this.chunks.get(key(cx, cz));
    if (c) {
      const lx = wx - cx * CHUNK_SIZE;
      const lz = wz - cz * CHUNK_SIZE;
      return bilinearHeight(c.heights, lx, lz, CHUNK_VERTS + 1);
    }
    // Fall back to direct generator query (slower but OK for occasional misses).
    return this.terrain.heightAt(wx, wz);
  }

  /** Trees within a radius of the world point — used for collisions. */
  *treesNear(wx: number, wz: number, radius: number) {
    const cx = Math.floor(wx / CHUNK_SIZE);
    const cz = Math.floor(wz / CHUNK_SIZE);
    const r2 = radius * radius;
    for (let dz = -1; dz <= 1; dz++) {
      for (let dx = -1; dx <= 1; dx++) {
        const c = this.chunks.get(key(cx + dx, cz + dz));
        if (!c) continue;
        for (const t of c.trees) {
          const ex = t.x - wx;
          const ez = t.z - wz;
          if (ex * ex + ez * ez < r2) yield t;
        }
      }
    }
  }

  /** Iterate currently loaded chunks (for minimap rendering). */
  iterChunks(): IterableIterator<ChunkData> {
    return this.chunks.values();
  }

  setRenderDistance(rd: number): void {
    this.renderDistance = rd;
  }

  /** Push the active camera so grass can fade with distance. */
  setCamera(cam: THREE.Camera): void {
    this.grass.setCamera(cam);
  }

  /** Forward sun direction/colour to the grass shader. */
  setSun(dir: THREE.Vector3, color: THREE.Color): void {
    this.grass.setSun(dir, color);
  }

  /** Drive chunk loading. Call once per frame with the player's world XZ. */
  update(playerX: number, playerZ: number, dt: number): void {
    const pcx = Math.floor(playerX / CHUNK_SIZE);
    const pcz = Math.floor(playerZ / CHUNK_SIZE);
    const r = this.renderDistance;

    // Find the closest missing chunk to enqueue.
    let bestKey: string | null = null;
    let bestCx = 0;
    let bestCz = 0;
    let bestDist = Infinity;
    for (let dz = -r; dz <= r; dz++) {
      for (let dx = -r; dx <= r; dx++) {
        const cx = pcx + dx;
        const cz = pcz + dz;
        const k = key(cx, cz);
        if (this.chunks.has(k) || this.pending.has(k)) continue;
        const d = dx * dx + dz * dz;
        if (d < bestDist) {
          bestDist = d;
          bestKey = k;
          bestCx = cx;
          bestCz = cz;
        }
      }
    }
    if (bestKey) {
      this.pending.add(bestKey);
      this.generateAndAdd(bestCx, bestCz);
    }

    // Unload anything outside (r + 1).
    const cull = r + 1;
    for (const [k, c] of this.chunks) {
      if (Math.abs(c.cx - pcx) > cull || Math.abs(c.cz - pcz) > cull) {
        this.scene.remove(c.terrainMesh);
        disposeChunk(c);
        this.vegetation.removeChunk(c.cx, c.cz);
        this.grass.removeChunk(c.cx, c.cz);
        this.chunks.delete(k);
      }
    }

    this.grass.update(dt);
  }

  /** Synchronously load a single chunk under (wx, wz). Used after teleport. */
  ensureChunkAt(wx: number, wz: number): void {
    const cx = Math.floor(wx / CHUNK_SIZE);
    const cz = Math.floor(wz / CHUNK_SIZE);
    const k = key(cx, cz);
    if (!this.chunks.has(k) && !this.pending.has(k)) {
      this.pending.add(k);
      this.generateAndAdd(cx, cz);
    }
  }

  private generateAndAdd(cx: number, cz: number): void {
    const k = key(cx, cz);
    const rng = mulberry32(hash2(cx ^ this.seed, cz ^ (this.seed >>> 1)));
    const data = generateChunk(cx, cz, this.terrain, rng);
    this.scene.add(data.terrainMesh);
    this.vegetation.addChunk(cx, cz, data.trees);
    this.grass.addChunk(cx, cz, data.grass);
    this.chunks.set(k, data);
    this.pending.delete(k);
  }

  /** Estimated total bounds in metres for camera frustum / fog tuning. */
  get viewRadius(): number {
    return this.renderDistance * CHUNK_SIZE;
  }
}

function key(cx: number, cz: number): string {
  return `${cx},${cz}`;
}
