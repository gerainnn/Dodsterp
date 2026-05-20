/**
 * Minimap — top-right canvas showing biome colors of nearby chunks plus a
 * direction-aware player marker. Tap teleports the player.
 *
 * Each chunk produces an `ImageData` tile (CHUNK_VERTS+1 px square) when it
 * loads. We composite the tiles around the player every frame at low cost
 * (~5 ms on Poco X6 Pro).
 */
import { CHUNK_SIZE } from '../world/Chunk';
import type { World } from '../world/World';
import type { Player } from '../player/Player';

const SIZE_PX = 200;
const VIEW_RADIUS_CHUNKS = 8; // ~512m radius shown
const PIXELS_PER_METRE = SIZE_PX / (VIEW_RADIUS_CHUNKS * 2 * CHUNK_SIZE);

export interface MinimapTeleportEvent {
  worldX: number;
  worldZ: number;
}

export class Minimap {
  readonly element: HTMLDivElement;
  private readonly canvas: HTMLCanvasElement;
  private readonly ctx: CanvasRenderingContext2D;
  /** Subscribers to teleport requests. */
  private onTeleportCb: ((e: MinimapTeleportEvent) => void) | null = null;
  /** Cached `OffscreenCanvas` for chunk tiles to avoid re-uploading every frame. */
  private readonly tileCache = new Map<string, HTMLCanvasElement>();

  constructor(parent: HTMLElement) {
    this.element = document.createElement('div');
    this.element.id = 'minimap';
    this.element.style.cssText = `
      position: fixed;
      top: 14px;
      right: 14px;
      width: ${SIZE_PX}px;
      height: ${SIZE_PX}px;
      border-radius: 12px;
      overflow: hidden;
      background: #0f1530;
      box-shadow: 0 6px 18px rgba(0, 0, 0, 0.4), inset 0 0 0 1px rgba(255, 255, 255, 0.18);
      touch-action: none;
      cursor: pointer;
    `;
    this.canvas = document.createElement('canvas');
    this.canvas.width = SIZE_PX;
    this.canvas.height = SIZE_PX;
    this.canvas.style.cssText = 'width:100%;height:100%;display:block;image-rendering:pixelated;';
    const ctx = this.canvas.getContext('2d');
    if (!ctx) throw new Error('Minimap 2D context unavailable');
    this.ctx = ctx;
    this.element.appendChild(this.canvas);
    parent.appendChild(this.element);

    // Click/tap → teleport.
    this.element.addEventListener('click', (e: MouseEvent) => {
      const rect = this.element.getBoundingClientRect();
      const px = e.clientX - rect.left;
      const py = e.clientY - rect.top;
      this.handleTap(px, py);
    });
  }

  onTeleport(cb: (e: MinimapTeleportEvent) => void): void {
    this.onTeleportCb = cb;
  }

  /** Cache the per-chunk thumbnail so the main draw is fast. */
  cacheChunkTile(cx: number, cz: number, tile: ImageData): void {
    const c = document.createElement('canvas');
    c.width = tile.width;
    c.height = tile.height;
    const ctx = c.getContext('2d');
    if (!ctx) return;
    ctx.putImageData(tile, 0, 0);
    this.tileCache.set(`${cx},${cz}`, c);
  }

  releaseChunkTile(cx: number, cz: number): void {
    this.tileCache.delete(`${cx},${cz}`);
  }

  /** Player-centred composite. Call once per frame after world.update(). */
  draw(player: Player, _world: World): void {
    const ctx = this.ctx;
    ctx.fillStyle = '#0f1530';
    ctx.fillRect(0, 0, SIZE_PX, SIZE_PX);

    const cxC = Math.floor(player.position.x / CHUNK_SIZE);
    const czC = Math.floor(player.position.z / CHUNK_SIZE);
    const tilePx = CHUNK_SIZE * PIXELS_PER_METRE;

    // Player is centred. Tile (cx, cz) gets drawn so that the world point
    // (cx*64, cz*64) maps to a screen pixel.
    const half = SIZE_PX / 2;
    for (let dz = -VIEW_RADIUS_CHUNKS; dz <= VIEW_RADIUS_CHUNKS; dz++) {
      for (let dx = -VIEW_RADIUS_CHUNKS; dx <= VIEW_RADIUS_CHUNKS; dx++) {
        const cx = cxC + dx;
        const cz = czC + dz;
        const tile = this.tileCache.get(`${cx},${cz}`);
        if (!tile) continue;
        const worldOriginX = cx * CHUNK_SIZE;
        const worldOriginZ = cz * CHUNK_SIZE;
        const sx = (worldOriginX - player.position.x) * PIXELS_PER_METRE + half;
        const sy = (worldOriginZ - player.position.z) * PIXELS_PER_METRE + half;
        ctx.drawImage(tile, Math.floor(sx), Math.floor(sy), tilePx + 1, tilePx + 1);
      }
    }

    // Player marker — small triangle pointing in player.yaw direction.
    ctx.save();
    ctx.translate(half, half);
    ctx.rotate(-player.yaw); // canvas Y is "south", so flip the angle.
    ctx.beginPath();
    ctx.moveTo(0, -8);
    ctx.lineTo(5.5, 5);
    ctx.lineTo(-5.5, 5);
    ctx.closePath();
    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = '#0c0f24';
    ctx.lineWidth = 1.5;
    ctx.fill();
    ctx.stroke();
    ctx.restore();

    // Crosshair / centre dot.
    ctx.fillStyle = 'rgba(255,255,255,0.5)';
    ctx.fillRect(half - 0.5, half - 0.5, 1, 1);

    // Outer border ring (drawn on top).
    ctx.strokeStyle = 'rgba(255,255,255,0.2)';
    ctx.lineWidth = 1;
    ctx.strokeRect(0.5, 0.5, SIZE_PX - 1, SIZE_PX - 1);
  }

  private handleTap(px: number, py: number): void {
    if (!this.onTeleportCb) return;
    const half = SIZE_PX / 2;
    const dx = (px - half) / PIXELS_PER_METRE;
    const dz = (py - half) / PIXELS_PER_METRE;
    // Caller adds player position to get absolute world coordinates.
    this.onTeleportCb({ worldX: dx, worldZ: dz });
  }
}
