/**
 * TerrainGenerator — single source of truth for `(x, z) -> { height, biome }`.
 *
 * Layered noise:
 *   continentalness (very low freq) — controls macro-layout (lake/land/mountain).
 *   temperature, humidity (low freq) — picks biome inside the "land" band.
 *   detail noise (multi-octave Simplex) — adds local roughness.
 *
 * Designed to be called millions of times during chunk generation, so
 * everything is allocation-free and uses cached noise instances.
 */
import { createNoise2D, type NoiseFunction2D } from 'simplex-noise';
import { mulberry32 } from '../utils/rng';
import { smoothstep } from '../utils/math';
import { BIOMES, type BiomeId } from './biomes';

export const WATER_LEVEL = 0;

export interface TerrainSample {
  height: number;
  biome: BiomeId;
}

export class TerrainGenerator {
  readonly seed: number;

  private readonly continentalness: NoiseFunction2D;
  private readonly temperature: NoiseFunction2D;
  private readonly humidity: NoiseFunction2D;
  private readonly detail: NoiseFunction2D;
  private readonly detailB: NoiseFunction2D;

  /** Reusable buffer for `sample()` to avoid allocating result objects. */
  private readonly out: TerrainSample = { height: 0, biome: 'plains' };

  constructor(seed: number) {
    this.seed = seed;
    const rng = mulberry32(seed);
    this.continentalness = createNoise2D(rng);
    this.temperature = createNoise2D(rng);
    this.humidity = createNoise2D(rng);
    this.detail = createNoise2D(rng);
    this.detailB = createNoise2D(rng);
  }

  /** Cheap 1-call lookup — preferred for large batches (chunk meshes). */
  sample(x: number, z: number): TerrainSample {
    const cont = this.continentalness(x * 0.0014, z * 0.0014);
    const temp = this.temperature(x * 0.0009, z * 0.0009);
    const hum = this.humidity(x * 0.0011, z * 0.0011);

    let biome: BiomeId;
    if (cont < -0.42) biome = 'lake';
    else if (cont > 0.5) biome = 'mountains';
    else if (cont < -0.3) biome = 'beach';
    else if (temp < -0.25) biome = 'tundra';
    else if (hum < -0.2) biome = 'plains';
    else if (temp > 0.15 && hum > -0.05) biome = 'forest';
    else biome = 'pine';

    const data = BIOMES[biome];

    let height = data.baseHeight;
    // Macro shaping — slight inland gradient.
    height += cont * 4;

    // Multi-octave detail.
    let amp = data.amplitude;
    let freq = data.frequency;
    for (let o = 0; o < 4; o++) {
      height += this.detail(x * freq, z * freq) * amp;
      amp *= 0.5;
      freq *= 2;
    }

    // Sharp ridged noise for mountains — more dramatic peaks.
    if (biome === 'mountains') {
      const r = 1 - Math.abs(this.detailB(x * 0.018, z * 0.018));
      height += r * r * 18 * smoothstep(0.5, 0.85, cont);
    }

    // Lakes are flat below water level; small wobble for a natural bottom.
    if (biome === 'lake') {
      height = -2.5 - Math.abs(this.detail(x * 0.04, z * 0.04)) * 1.3;
    }

    // Beach: forced just above water for a believable shoreline.
    if (biome === 'beach') {
      height = 0.3 + this.detailB(x * 0.06, z * 0.06) * 0.6;
    }

    this.out.height = height;
    this.out.biome = biome;
    return this.out;
  }

  heightAt(x: number, z: number): number {
    return this.sample(x, z).height;
  }
}
