/**
 * Biomes — declarative registry. Used by terrain generator (height profile),
 * scatter (vegetation density), grass (density/color), and minimap (color).
 */

export type BiomeId = 'forest' | 'pine' | 'mountains' | 'lake' | 'plains' | 'tundra' | 'beach';

export type TreeKind = 'oak' | 'pine' | 'birch' | 'dead' | 'none';

export interface BiomeData {
  id: BiomeId;
  name: string;
  /** Base height (above water level=0). */
  baseHeight: number;
  /** Heightmap amplitude in metres for the dominant noise layer. */
  amplitude: number;
  /** Heightmap frequency for the dominant noise layer. */
  frequency: number;
  /** Albedo tint as 0..1 RGB. */
  color: [number, number, number];
  /** Roughness tint. Currently unused but reserved for PBR upgrade. */
  roughness: number;
  treeKind: TreeKind;
  /** Trees per chunk at this biome (max). */
  treeDensity: number;
  /** Grass blades per chunk at this biome (max). */
  grassDensity: number;
  /** Hex color used on the 2D minimap. */
  minimapColor: string;
}

const C = (r: number, g: number, b: number): [number, number, number] => [r, g, b];

export const BIOMES: Record<BiomeId, BiomeData> = {
  forest: {
    id: 'forest',
    name: 'Лиственный лес',
    baseHeight: 2,
    amplitude: 5,
    frequency: 0.022,
    color: C(0.32, 0.55, 0.22),
    roughness: 0.95,
    treeKind: 'oak',
    treeDensity: 90,
    grassDensity: 18000,
    minimapColor: '#3d7a26',
  },
  pine: {
    id: 'pine',
    name: 'Хвойный лес',
    baseHeight: 4,
    amplitude: 7,
    frequency: 0.02,
    color: C(0.22, 0.42, 0.25),
    roughness: 0.95,
    treeKind: 'pine',
    treeDensity: 130,
    grassDensity: 9000,
    minimapColor: '#2a5a35',
  },
  mountains: {
    id: 'mountains',
    name: 'Горы',
    baseHeight: 14,
    amplitude: 24,
    frequency: 0.022,
    color: C(0.55, 0.5, 0.45),
    roughness: 0.97,
    treeKind: 'pine',
    treeDensity: 25,
    grassDensity: 4000,
    minimapColor: '#7a6a5a',
  },
  lake: {
    id: 'lake',
    name: 'Озеро',
    baseHeight: -2.5,
    amplitude: 1.4,
    frequency: 0.04,
    color: C(0.7, 0.66, 0.5),
    roughness: 0.9,
    treeKind: 'none',
    treeDensity: 0,
    grassDensity: 0,
    minimapColor: '#3a6aaa',
  },
  plains: {
    id: 'plains',
    name: 'Степь',
    baseHeight: 1,
    amplitude: 2.4,
    frequency: 0.015,
    color: C(0.55, 0.62, 0.32),
    roughness: 0.95,
    treeKind: 'oak',
    treeDensity: 8,
    grassDensity: 16000,
    minimapColor: '#7a9a3a',
  },
  tundra: {
    id: 'tundra',
    name: 'Тундра',
    baseHeight: 6,
    amplitude: 6,
    frequency: 0.022,
    color: C(0.86, 0.9, 0.96),
    roughness: 0.92,
    treeKind: 'dead',
    treeDensity: 18,
    grassDensity: 3000,
    minimapColor: '#bdc8d0',
  },
  beach: {
    id: 'beach',
    name: 'Пляж',
    baseHeight: 0.4,
    amplitude: 0.6,
    frequency: 0.03,
    color: C(0.92, 0.86, 0.65),
    roughness: 0.85,
    treeKind: 'none',
    treeDensity: 0,
    grassDensity: 1200,
    minimapColor: '#e9d68a',
  },
};

export const ALL_BIOME_IDS: BiomeId[] = Object.keys(BIOMES) as BiomeId[];
