/**
 * Vegetation — renders all trees in the world using one InstancedMesh per
 * tree kind. Templates are built procedurally (no external GLTF assets) to
 * keep the bundle tiny and the pipeline self-contained.
 *
 * For each chunk we get a list of TreeInstance; we copy per-instance
 * matrices into a shared InstancedMesh capacity buffer. Adding/removing
 * a chunk just rewrites the matrix slice and bumps `instanceMatrix.needsUpdate`.
 */
import * as THREE from 'three';
import type { TreeInstance } from './Chunk';

interface ChunkSlot {
  start: number;
  count: number;
}

interface KindBucket {
  template: THREE.BufferGeometry;
  material: THREE.Material;
  mesh: THREE.InstancedMesh;
  capacity: number;
  used: number;
  /** Per-chunk slot ranges. */
  chunkSlots: Map<string, ChunkSlot>;
}

const TREE_CAPACITY = 4096; // total tree instances per kind (across all chunks)

export class Vegetation {
  private readonly buckets: Record<TreeInstance['kind'], KindBucket>;
  private readonly tmp = new THREE.Object3D();

  constructor(scene: THREE.Scene) {
    this.buckets = {
      oak: makeBucket(scene, buildOak(), 'oak'),
      pine: makeBucket(scene, buildPine(), 'pine'),
      birch: makeBucket(scene, buildBirch(), 'birch'),
      dead: makeBucket(scene, buildDead(), 'dead'),
    };
  }

  addChunk(cx: number, cz: number, trees: TreeInstance[]): void {
    if (trees.length === 0) return;
    // Group by kind.
    const byKind: Record<string, TreeInstance[]> = { oak: [], pine: [], birch: [], dead: [] };
    for (const t of trees) byKind[t.kind].push(t);

    for (const kind of ['oak', 'pine', 'birch', 'dead'] as const) {
      const list = byKind[kind];
      if (list.length === 0) continue;
      const bucket = this.buckets[kind];
      // Compact write at the end of the bucket.
      if (bucket.used + list.length > bucket.capacity) {
        // Bucket full; drop further trees of this kind. In v1 this is unlikely
        // because we only keep ~9*9 chunks at high preset (~1296 trees max).
        continue;
      }
      const start = bucket.used;
      for (let i = 0; i < list.length; i++) {
        const t = list[i];
        this.tmp.position.set(t.x, t.y, t.z);
        this.tmp.rotation.set(0, t.ry, 0);
        this.tmp.scale.set(t.s, t.s, t.s);
        this.tmp.updateMatrix();
        bucket.mesh.setMatrixAt(start + i, this.tmp.matrix);
      }
      bucket.used += list.length;
      bucket.chunkSlots.set(`${cx},${cz}`, { start, count: list.length });
      bucket.mesh.count = bucket.used;
      bucket.mesh.instanceMatrix.needsUpdate = true;
    }
  }

  removeChunk(cx: number, cz: number): void {
    // We don't shrink the buffer — we move the last block into the freed slot.
    const k = `${cx},${cz}`;
    for (const kind of Object.keys(this.buckets) as (keyof typeof this.buckets)[]) {
      const bucket = this.buckets[kind];
      const slot = bucket.chunkSlots.get(k);
      if (!slot) continue;
      const tailStart = bucket.used - slot.count;
      if (tailStart > slot.start) {
        // Move tail block into the freed slot.
        const m = new THREE.Matrix4();
        for (let i = 0; i < slot.count; i++) {
          bucket.mesh.getMatrixAt(tailStart + i, m);
          bucket.mesh.setMatrixAt(slot.start + i, m);
        }
        // Find the chunk that owned the tail and remap its slot.
        for (const [, otherSlot] of bucket.chunkSlots) {
          if (otherSlot.start === tailStart) {
            otherSlot.start = slot.start;
            break;
          }
        }
      }
      bucket.used -= slot.count;
      bucket.chunkSlots.delete(k);
      bucket.mesh.count = bucket.used;
      bucket.mesh.instanceMatrix.needsUpdate = true;
    }
  }

  dispose(): void {
    for (const kind of Object.keys(this.buckets) as (keyof typeof this.buckets)[]) {
      const b = this.buckets[kind];
      b.template.dispose();
      b.material.dispose();
    }
  }
}

function makeBucket(scene: THREE.Scene, geom: THREE.BufferGeometry, kind: string): KindBucket {
  const mat = new THREE.MeshStandardMaterial({
    vertexColors: true,
    roughness: 0.95,
    metalness: 0,
  });
  const mesh = new THREE.InstancedMesh(geom, mat, TREE_CAPACITY);
  mesh.count = 0;
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  mesh.frustumCulled = false; // it's one giant mesh, leave culling off
  mesh.name = `trees-${kind}`;
  scene.add(mesh);
  return { template: geom, material: mat, mesh, capacity: TREE_CAPACITY, used: 0, chunkSlots: new Map() };
}

// ----------------------------------------------------------------------------
// Procedural tree builders. Each returns a single merged BufferGeometry with
// vertex colors so InstancedMesh + MeshStandardMaterial gets correct colors.
// ----------------------------------------------------------------------------

function buildOak(): THREE.BufferGeometry {
  const trunk = new THREE.CylinderGeometry(0.18, 0.28, 2.2, 8, 1);
  trunk.translate(0, 1.1, 0);
  paintGeometry(trunk, [0.32, 0.22, 0.14]);

  const foliage = new THREE.IcosahedronGeometry(1.6, 1);
  foliage.translate(0, 3.0, 0);
  paintGeometry(foliage, [0.22, 0.45, 0.18]);

  const foliage2 = new THREE.IcosahedronGeometry(1.1, 1);
  foliage2.translate(1.0, 2.6, 0.4);
  paintGeometry(foliage2, [0.26, 0.5, 0.2]);

  const foliage3 = new THREE.IcosahedronGeometry(1.0, 1);
  foliage3.translate(-0.9, 2.7, -0.3);
  paintGeometry(foliage3, [0.2, 0.42, 0.16]);

  return mergeAndDispose([trunk, foliage, foliage2, foliage3]);
}

function buildPine(): THREE.BufferGeometry {
  const trunk = new THREE.CylinderGeometry(0.16, 0.24, 4.0, 8, 1);
  trunk.translate(0, 2.0, 0);
  paintGeometry(trunk, [0.28, 0.18, 0.1]);

  const cones: THREE.BufferGeometry[] = [trunk];
  let radius = 1.6;
  let y = 2.0;
  for (let i = 0; i < 4; i++) {
    const c = new THREE.ConeGeometry(radius, 1.7, 8, 1);
    c.translate(0, y, 0);
    paintGeometry(c, [0.16, 0.36 - i * 0.02, 0.18]);
    cones.push(c);
    radius *= 0.78;
    y += 1.0;
  }
  return mergeAndDispose(cones);
}

function buildBirch(): THREE.BufferGeometry {
  const trunk = new THREE.CylinderGeometry(0.14, 0.2, 3.4, 8, 1);
  trunk.translate(0, 1.7, 0);
  paintGeometry(trunk, [0.92, 0.92, 0.9]);

  const foliage = new THREE.IcosahedronGeometry(1.2, 1);
  foliage.translate(0, 3.5, 0);
  paintGeometry(foliage, [0.5, 0.7, 0.28]);

  const foliage2 = new THREE.IcosahedronGeometry(0.9, 1);
  foliage2.translate(0.8, 3.2, 0.2);
  paintGeometry(foliage2, [0.55, 0.72, 0.3]);

  return mergeAndDispose([trunk, foliage, foliage2]);
}

function buildDead(): THREE.BufferGeometry {
  const trunk = new THREE.CylinderGeometry(0.14, 0.22, 3.0, 8, 1);
  trunk.translate(0, 1.5, 0);
  paintGeometry(trunk, [0.4, 0.34, 0.26]);

  // Two upward branches.
  const b1 = new THREE.CylinderGeometry(0.06, 0.1, 1.5, 5, 1);
  b1.rotateZ(0.6);
  b1.translate(0.6, 2.4, 0);
  paintGeometry(b1, [0.42, 0.36, 0.28]);

  const b2 = new THREE.CylinderGeometry(0.06, 0.1, 1.4, 5, 1);
  b2.rotateZ(-0.5);
  b2.translate(-0.55, 2.3, 0.1);
  paintGeometry(b2, [0.42, 0.36, 0.28]);

  return mergeAndDispose([trunk, b1, b2]);
}

/** Add a `color` BufferAttribute to a geometry, painted uniformly. */
function paintGeometry(g: THREE.BufferGeometry, rgb: [number, number, number]): void {
  const count = (g.attributes.position as THREE.BufferAttribute).count;
  const colors = new Float32Array(count * 3);
  for (let i = 0; i < count; i++) {
    // Tiny per-vertex jitter for organic look.
    const j = (Math.sin(i * 12.345) * 43758.5453) % 1;
    const f = 0.92 + ((j + 1) % 1) * 0.16;
    colors[i * 3 + 0] = rgb[0] * f;
    colors[i * 3 + 1] = rgb[1] * f;
    colors[i * 3 + 2] = rgb[2] * f;
  }
  g.setAttribute('color', new THREE.BufferAttribute(colors, 3));
}

/** Merge a list of geometries into one and dispose the inputs. */
function mergeAndDispose(parts: THREE.BufferGeometry[]): THREE.BufferGeometry {
  // Simple in-house merge to avoid pulling BufferGeometryUtils.
  const stride = 3;
  let totalVerts = 0;
  let totalIndices = 0;
  for (const p of parts) {
    p.deleteAttribute('uv');
    p.deleteAttribute('uv1');
    p.deleteAttribute('uv2');
    if (!p.index) p.computeVertexNormals();
    if (!p.index) {
      // Convert to indexed for compactness — Three's primitives are usually indexed.
      // Fallback: keep non-indexed by leaving null.
    }
    totalVerts += (p.attributes.position as THREE.BufferAttribute).count;
    totalIndices += p.index ? p.index.count : (p.attributes.position as THREE.BufferAttribute).count;
  }
  const positions = new Float32Array(totalVerts * stride);
  const normals = new Float32Array(totalVerts * stride);
  const colors = new Float32Array(totalVerts * stride);
  const indices = new Uint32Array(totalIndices);

  let posOff = 0;
  let normOff = 0;
  let colOff = 0;
  let idxOff = 0;
  let baseVert = 0;
  for (const p of parts) {
    const pos = p.attributes.position as THREE.BufferAttribute;
    const nrm = p.attributes.normal as THREE.BufferAttribute | undefined;
    const col = p.attributes.color as THREE.BufferAttribute | undefined;
    positions.set(pos.array as Float32Array, posOff);
    posOff += pos.count * stride;
    if (nrm) {
      normals.set(nrm.array as Float32Array, normOff);
    }
    normOff += pos.count * stride;
    if (col) {
      colors.set(col.array as Float32Array, colOff);
    }
    colOff += pos.count * stride;
    if (p.index) {
      const arr = p.index.array as Uint32Array | Uint16Array;
      for (let i = 0; i < arr.length; i++) indices[idxOff + i] = arr[i] + baseVert;
      idxOff += arr.length;
    } else {
      for (let i = 0; i < pos.count; i++) indices[idxOff + i] = baseVert + i;
      idxOff += pos.count;
    }
    baseVert += pos.count;
    p.dispose();
  }

  const out = new THREE.BufferGeometry();
  out.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  out.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
  out.setAttribute('color', new THREE.BufferAttribute(colors, 3));
  out.setIndex(new THREE.BufferAttribute(indices, 1));
  out.computeBoundingSphere();
  return out;
}
