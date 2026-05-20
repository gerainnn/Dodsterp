/**
 * Chunk — owns terrain mesh, vegetation instances, grass, and minimap thumb
 * for one CHUNK_SIZE x CHUNK_SIZE region of the world.
 *
 * Built once, then disposed when out of range.
 */
import * as THREE from 'three';
import { BIOMES, type BiomeId } from './biomes';
import { TerrainGenerator, WATER_LEVEL } from './TerrainGenerator';

export const CHUNK_SIZE = 64; // metres
export const CHUNK_VERTS = 64; // segments per side -> 65x65 vertices

export interface TreeInstance {
  /** World x. */
  x: number;
  /** World y (ground). */
  y: number;
  /** World z. */
  z: number;
  /** Uniform scale multiplier. */
  s: number;
  /** Rotation around Y in radians. */
  ry: number;
  /** Tree kind dictates which mesh template to pick. */
  kind: 'oak' | 'pine' | 'birch' | 'dead';
}

export interface GrassInstance {
  x: number;
  y: number;
  z: number;
  s: number;
  ry: number;
  /** Biome tint: 0..1 RGB. */
  tint: [number, number, number];
}

export interface ChunkData {
  cx: number;
  cz: number;
  /** Terrain mesh, ready to be added to scene. */
  terrainMesh: THREE.Mesh;
  /** Heightfield (CHUNK_VERTS+1)^2 floats — used for player physics queries. */
  heights: Float32Array;
  /** Per-vertex biome ids, parallel to heights. */
  biomes: BiomeId[];
  trees: TreeInstance[];
  grass: GrassInstance[];
  /** Pre-rendered minimap tile (CHUNK_VERTS+1 px square). */
  minimapTile: ImageData;
  /** True if any vertex has terrain below the water level — water mesh needed. */
  hasWater: boolean;
}

/**
 * Generate a single chunk. Heavy work; call once per chunk.
 *
 * @param cx Chunk x coordinate (chunk grid, not world).
 * @param cz Chunk z coordinate.
 */
export function generateChunk(
  cx: number,
  cz: number,
  terrain: TerrainGenerator,
  rngForChunk: () => number,
): ChunkData {
  const N = CHUNK_VERTS + 1; // vertex count per side
  const totalVerts = N * N;

  const heights = new Float32Array(totalVerts);
  const biomes: BiomeId[] = new Array(totalVerts);
  const colorArr = new Float32Array(totalVerts * 3);

  let hasWater = false;
  let minH = Infinity;
  let maxH = -Infinity;

  // Sample terrain at each vertex.
  const baseX = cx * CHUNK_SIZE;
  const baseZ = cz * CHUNK_SIZE;
  for (let j = 0; j < N; j++) {
    for (let i = 0; i < N; i++) {
      const idx = j * N + i;
      const wx = baseX + i;
      const wz = baseZ + j;
      const s = terrain.sample(wx, wz);
      heights[idx] = s.height;
      biomes[idx] = s.biome;
      const c = BIOMES[s.biome].color;
      // Slight per-vertex hue variance for organic look.
      const noise = (Math.sin((wx * 12.9898 + wz * 78.233) * 0.0001) * 43758.5453) % 1;
      const jitter = 0.94 + ((noise + 1) % 1) * 0.12;
      colorArr[idx * 3 + 0] = c[0] * jitter;
      colorArr[idx * 3 + 1] = c[1] * jitter;
      colorArr[idx * 3 + 2] = c[2] * jitter;

      if (s.height < WATER_LEVEL) hasWater = true;
      if (s.height < minH) minH = s.height;
      if (s.height > maxH) maxH = s.height;
    }
  }

  // Build geometry.
  const geom = new THREE.PlaneGeometry(CHUNK_SIZE, CHUNK_SIZE, CHUNK_VERTS, CHUNK_VERTS);
  geom.rotateX(-Math.PI / 2);
  // Translate so the chunk origin (cx*64, 0, cz*64) is at vertex (i=0, j=0).
  geom.translate(CHUNK_SIZE / 2, 0, CHUNK_SIZE / 2);

  const positions = geom.attributes.position as THREE.BufferAttribute;
  // PlaneGeometry order: row-major across X, then Z. After rotateX, the
  // mapping (i, j) -> position index is straightforward: i is X column, j is Z row.
  for (let j = 0; j < N; j++) {
    for (let i = 0; i < N; i++) {
      const idx = j * N + i;
      positions.setY(idx, heights[idx]);
    }
  }
  positions.needsUpdate = true;
  geom.computeVertexNormals();

  geom.setAttribute('color', new THREE.BufferAttribute(colorArr, 3));

  const material = new THREE.MeshStandardMaterial({
    vertexColors: true,
    roughness: 0.95,
    metalness: 0,
    flatShading: false,
  });

  const mesh = new THREE.Mesh(geom, material);
  mesh.position.set(baseX, 0, baseZ);
  mesh.receiveShadow = true;
  mesh.castShadow = false; // terrain doesn't shadow itself meaningfully; cheaper this way
  mesh.matrixAutoUpdate = false;
  mesh.updateMatrix();

  // Build minimap tile (NxN ImageData).
  const minimap = new ImageData(N, N);
  for (let j = 0; j < N; j++) {
    for (let i = 0; i < N; i++) {
      const idx = j * N + i;
      const h = heights[idx];
      let hex: string;
      if (h < WATER_LEVEL) hex = '#3a6aaa';
      else hex = BIOMES[biomes[idx]].minimapColor;
      const r = parseInt(hex.slice(1, 3), 16);
      const g = parseInt(hex.slice(3, 5), 16);
      const b = parseInt(hex.slice(5, 7), 16);
      // Subtle altitude shading for visual depth.
      const shade = Math.max(0.6, Math.min(1, (h - minH) / Math.max(1, maxH - minH) * 0.6 + 0.7));
      const o = idx * 4;
      minimap.data[o + 0] = (r * shade) | 0;
      minimap.data[o + 1] = (g * shade) | 0;
      minimap.data[o + 2] = (b * shade) | 0;
      minimap.data[o + 3] = 255;
    }
  }

  // Scatter trees and grass.
  const trees = scatterTrees(cx, cz, biomes, heights, rngForChunk);
  const grass = scatterGrass(cx, cz, biomes, heights, rngForChunk);

  return {
    cx,
    cz,
    terrainMesh: mesh,
    heights,
    biomes,
    trees,
    grass,
    minimapTile: minimap,
    hasWater,
  };
}

/** Scatter trees with stratified sampling (jittered cells). */
function scatterTrees(
  cx: number,
  cz: number,
  biomes: BiomeId[],
  heights: Float32Array,
  rng: () => number,
): TreeInstance[] {
  const N = CHUNK_VERTS + 1;
  const trees: TreeInstance[] = [];
  // Cell-based scatter: each cell rolls a tree at its biome's max density.
  // Use a uniform 16x16 attempt grid (256 cells) and accept by biome density.
  const ATTEMPTS_PER_SIDE = 16;
  const cellSize = CHUNK_SIZE / ATTEMPTS_PER_SIDE;

  for (let cj = 0; cj < ATTEMPTS_PER_SIDE; cj++) {
    for (let ci = 0; ci < ATTEMPTS_PER_SIDE; ci++) {
      const lx = (ci + rng()) * cellSize;
      const lz = (cj + rng()) * cellSize;
      // Look up biome at the nearest vertex.
      const i = Math.min(N - 1, Math.max(0, Math.round(lx)));
      const j = Math.min(N - 1, Math.max(0, Math.round(lz)));
      const idx = j * N + i;
      const biome = biomes[idx];
      const data = BIOMES[biome];
      if (data.treeKind === 'none') continue;
      // Density encoded as max-trees-per-chunk, normalised by attempt count.
      const acceptProb = data.treeDensity / (ATTEMPTS_PER_SIDE * ATTEMPTS_PER_SIDE);
      if (rng() > acceptProb) continue;
      const y = bilinearHeight(heights, lx, lz, N);
      if (y < WATER_LEVEL + 0.2) continue; // skip submerged
      // Avoid steep slopes for trees.
      const slope = approxSlope(heights, lx, lz, N);
      if (slope > 0.7) continue;
      trees.push({
        x: cx * CHUNK_SIZE + lx,
        y,
        z: cz * CHUNK_SIZE + lz,
        s: 0.85 + rng() * 0.5,
        ry: rng() * Math.PI * 2,
        kind: data.treeKind,
      });
    }
  }
  return trees;
}

/** Scatter grass — one quad per cell of a fine grid. */
function scatterGrass(
  cx: number,
  cz: number,
  biomes: BiomeId[],
  heights: Float32Array,
  rng: () => number,
): GrassInstance[] {
  const N = CHUNK_VERTS + 1;
  // For perf, cap grass per chunk at 12k regardless of biome's nominal density.
  // We choose a grid size based on the densest biome present.
  let densitySum = 0;
  for (let k = 0; k < biomes.length; k++) densitySum += BIOMES[biomes[k]].grassDensity;
  const avgDensity = densitySum / biomes.length;
  if (avgDensity < 200) return [];

  const TARGET = Math.min(12000, Math.round(avgDensity));
  const SIDE = Math.min(120, Math.round(Math.sqrt(TARGET))); // 120^2 = 14400 max
  const cellSize = CHUNK_SIZE / SIDE;
  const grass: GrassInstance[] = [];

  for (let cj = 0; cj < SIDE; cj++) {
    for (let ci = 0; ci < SIDE; ci++) {
      const lx = (ci + rng() * 0.95 + 0.025) * cellSize;
      const lz = (cj + rng() * 0.95 + 0.025) * cellSize;
      const i = Math.min(N - 1, Math.max(0, Math.round(lx)));
      const j = Math.min(N - 1, Math.max(0, Math.round(lz)));
      const idx = j * N + i;
      const biome = biomes[idx];
      const data = BIOMES[biome];
      if (data.grassDensity < 200) continue;
      // Accept rate scales with biome density relative to densest (forest=18000).
      const accept = data.grassDensity / 18000;
      if (rng() > accept) continue;
      const y = bilinearHeight(heights, lx, lz, N);
      if (y < WATER_LEVEL + 0.25) continue;
      const slope = approxSlope(heights, lx, lz, N);
      if (slope > 0.6) continue;
      grass.push({
        x: cx * CHUNK_SIZE + lx,
        y,
        z: cz * CHUNK_SIZE + lz,
        s: 0.7 + rng() * 0.6,
        ry: rng() * Math.PI * 2,
        tint: [data.color[0] * 1.25, data.color[1] * 1.2, data.color[2] * 1.05],
      });
    }
  }
  return grass;
}

/** Bilinear sample of the heightfield given chunk-local coordinates. */
export function bilinearHeight(
  heights: Float32Array,
  lx: number,
  lz: number,
  N: number,
): number {
  const x = Math.max(0, Math.min(N - 1.0001, lx));
  const z = Math.max(0, Math.min(N - 1.0001, lz));
  const i0 = Math.floor(x);
  const j0 = Math.floor(z);
  const tx = x - i0;
  const tz = z - j0;
  const h00 = heights[j0 * N + i0];
  const h10 = heights[j0 * N + i0 + 1];
  const h01 = heights[(j0 + 1) * N + i0];
  const h11 = heights[(j0 + 1) * N + i0 + 1];
  const a = h00 * (1 - tx) + h10 * tx;
  const b = h01 * (1 - tx) + h11 * tx;
  return a * (1 - tz) + b * tz;
}

/** Cheap slope estimate (max gradient over 1m). */
function approxSlope(heights: Float32Array, lx: number, lz: number, N: number): number {
  const i = Math.min(N - 2, Math.max(1, Math.round(lx)));
  const j = Math.min(N - 2, Math.max(1, Math.round(lz)));
  const c = heights[j * N + i];
  const dx = heights[j * N + i + 1] - c;
  const dz = heights[(j + 1) * N + i] - c;
  return Math.sqrt(dx * dx + dz * dz);
}

export function disposeChunk(c: ChunkData): void {
  c.terrainMesh.geometry.dispose();
  (c.terrainMesh.material as THREE.Material).dispose();
}
