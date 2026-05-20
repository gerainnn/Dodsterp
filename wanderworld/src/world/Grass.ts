/**
 * Grass — animated cross-quad grass blades using GPU instancing.
 *
 * Each instance gets its own (position, rotation, scale, tint). The vertex
 * shader bends the top vertices with a wind function based on world position
 * and time, so animation costs nothing on the CPU after upload.
 *
 * Memory budget: at TREE preset (12k per chunk) and 9*9 visible chunks the
 * per-grass-kind capacity needs to hold ~1M instances. We keep one global
 * InstancedMesh sized for `GRASS_CAPACITY` and pack chunks into it.
 */
import * as THREE from 'three';
import type { GrassInstance } from './Chunk';

const GRASS_CAPACITY = 200000;

interface ChunkSlot {
  start: number;
  count: number;
}

export class Grass {
  private readonly mesh: THREE.InstancedMesh;
  private readonly material: THREE.ShaderMaterial;
  private readonly geom: THREE.BufferGeometry;
  private readonly tmp = new THREE.Object3D();
  private readonly tmpColor = new THREE.Color();
  private readonly slots = new Map<string, ChunkSlot>();
  private used = 0;

  constructor(scene: THREE.Scene) {
    // Cross-quad geometry: two perpendicular billboards forming an "X" shape.
    this.geom = makeCrossQuad();

    this.material = new THREE.ShaderMaterial({
      uniforms: {
        time: { value: 0 },
        sunDir: { value: new THREE.Vector3(0.4, 0.85, 0.3).normalize() },
        sunColor: { value: new THREE.Color(0xfff0d0) },
        ambientColor: { value: new THREE.Color(0x607080) },
        cameraPos: { value: new THREE.Vector3() },
        fadeStart: { value: 35 },
        fadeEnd: { value: 90 },
      },
      vertexColors: true,
      transparent: false,
      side: THREE.DoubleSide,
      vertexShader: GRASS_VERT,
      fragmentShader: GRASS_FRAG,
    });

    this.mesh = new THREE.InstancedMesh(this.geom, this.material, GRASS_CAPACITY);
    this.mesh.count = 0;
    this.mesh.castShadow = false;
    this.mesh.receiveShadow = false;
    this.mesh.frustumCulled = false;
    this.mesh.name = 'grass';
    // Per-instance color attribute so each blade gets its biome tint.
    const colorAttr = new THREE.InstancedBufferAttribute(new Float32Array(GRASS_CAPACITY * 3), 3);
    this.mesh.geometry.setAttribute('instanceColor', colorAttr);
    scene.add(this.mesh);
  }

  addChunk(cx: number, cz: number, blades: GrassInstance[]): void {
    if (blades.length === 0) return;
    if (this.used + blades.length > GRASS_CAPACITY) return; // drop overflow
    const start = this.used;
    const colorAttr = this.mesh.geometry.attributes.instanceColor as THREE.InstancedBufferAttribute;
    for (let i = 0; i < blades.length; i++) {
      const b = blades[i];
      this.tmp.position.set(b.x, b.y, b.z);
      this.tmp.rotation.set(0, b.ry, 0);
      this.tmp.scale.set(b.s, b.s * (0.7 + Math.random() * 0.3), b.s);
      this.tmp.updateMatrix();
      this.mesh.setMatrixAt(start + i, this.tmp.matrix);
      colorAttr.array[(start + i) * 3 + 0] = b.tint[0];
      colorAttr.array[(start + i) * 3 + 1] = b.tint[1];
      colorAttr.array[(start + i) * 3 + 2] = b.tint[2];
    }
    this.used += blades.length;
    this.slots.set(`${cx},${cz}`, { start, count: blades.length });
    this.mesh.count = this.used;
    this.mesh.instanceMatrix.needsUpdate = true;
    colorAttr.needsUpdate = true;
  }

  removeChunk(cx: number, cz: number): void {
    const k = `${cx},${cz}`;
    const slot = this.slots.get(k);
    if (!slot) return;
    const tailStart = this.used - slot.count;
    const colorAttr = this.mesh.geometry.attributes.instanceColor as THREE.InstancedBufferAttribute;
    if (tailStart > slot.start) {
      const m = new THREE.Matrix4();
      for (let i = 0; i < slot.count; i++) {
        this.mesh.getMatrixAt(tailStart + i, m);
        this.mesh.setMatrixAt(slot.start + i, m);
        colorAttr.array[(slot.start + i) * 3 + 0] = colorAttr.array[(tailStart + i) * 3 + 0];
        colorAttr.array[(slot.start + i) * 3 + 1] = colorAttr.array[(tailStart + i) * 3 + 1];
        colorAttr.array[(slot.start + i) * 3 + 2] = colorAttr.array[(tailStart + i) * 3 + 2];
      }
      // Find the slot owning the tail and remap.
      for (const [, otherSlot] of this.slots) {
        if (otherSlot.start === tailStart) {
          otherSlot.start = slot.start;
          break;
        }
      }
    }
    this.used -= slot.count;
    this.slots.delete(k);
    this.mesh.count = this.used;
    this.mesh.instanceMatrix.needsUpdate = true;
    colorAttr.needsUpdate = true;
    void this.tmpColor; // referenced to silence unused-locals
  }

  /** Push current camera position so the fade ring follows the player. */
  setCamera(cam: THREE.Camera): void {
    this.material.uniforms.cameraPos.value.setFromMatrixPosition(cam.matrixWorld);
  }

  setSun(dir: THREE.Vector3, color: THREE.Color): void {
    this.material.uniforms.sunDir.value.copy(dir).normalize();
    this.material.uniforms.sunColor.value.copy(color);
  }

  update(dt: number): void {
    this.material.uniforms.time.value += dt;
  }

  dispose(): void {
    this.geom.dispose();
    this.material.dispose();
  }
}

/** Build a cross-quad: 2 perpendicular billboards, ~1m tall, base at y=0. */
function makeCrossQuad(): THREE.BufferGeometry {
  const w = 0.6;
  const h = 1.0;
  // Two quads: positions, normals, "isTop" flag in second uv channel.
  // Vertex layout per quad: bl, br, tr, tl
  const positions: number[] = [];
  const normals: number[] = [];
  const uvs: number[] = [];
  const colors: number[] = [];
  const indices: number[] = [];

  const pushQuad = (rotY: number) => {
    const cs = Math.cos(rotY);
    const sn = Math.sin(rotY);
    const verts = [
      [-w / 2, 0, 0, 0, 0],
      [+w / 2, 0, 0, 1, 0],
      [+w / 2, h, 0, 1, 1],
      [-w / 2, h, 0, 0, 1],
    ];
    const base = positions.length / 3;
    for (const v of verts) {
      const x = v[0] * cs;
      const z = v[0] * sn;
      positions.push(x, v[1], z);
      // Normal points outward from quad face.
      normals.push(-sn, 0, cs);
      uvs.push(v[3], v[4]);
      // Per-vertex color is white; biome tint is per-instance.
      colors.push(1, 1, 1);
    }
    indices.push(base, base + 1, base + 2, base, base + 2, base + 3);
  };

  pushQuad(0);
  pushQuad(Math.PI / 2);

  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.BufferAttribute(new Float32Array(positions), 3));
  g.setAttribute('normal', new THREE.BufferAttribute(new Float32Array(normals), 3));
  g.setAttribute('uv', new THREE.BufferAttribute(new Float32Array(uvs), 2));
  g.setAttribute('color', new THREE.BufferAttribute(new Float32Array(colors), 3));
  g.setIndex(indices);
  return g;
}

const GRASS_VERT = /* glsl */ `
  attribute vec3 instanceColor;
  varying vec3 vColor;
  varying vec3 vNormalW;
  varying vec3 vWorldPos;
  varying float vTopFactor;
  varying float vDistance;

  uniform float time;
  uniform vec3 cameraPos;

  void main() {
    vec4 instPos = instanceMatrix * vec4(position, 1.0);
    // World-space position of the blade base + offset (already transformed).
    vec4 worldPos = modelMatrix * instPos;

    // Top factor — 1 at the tip, 0 at the base. UV.y carries this.
    float top = uv.y;
    vTopFactor = top;

    // Wind via low-freq sin in world XZ. Stronger at the top.
    float windPhase = worldPos.x * 0.18 + worldPos.z * 0.22 + time * 1.6;
    float gust = sin(time * 0.7 + worldPos.x * 0.05) * 0.5 + 0.5;
    float bend = sin(windPhase) * 0.18 * top + cos(windPhase * 1.3) * 0.10 * top;
    bend *= 0.7 + gust * 0.6;

    worldPos.x += bend;
    worldPos.z += bend * 0.5;

    vWorldPos = worldPos.xyz;
    vColor = instanceColor;

    // Two-sided lighting — keep normals consistent regardless of facing.
    vec3 worldNormal = normalize(mat3(modelMatrix) * mat3(instanceMatrix) * normal);
    vNormalW = worldNormal;

    vDistance = distance(worldPos.xyz, cameraPos);

    gl_Position = projectionMatrix * viewMatrix * worldPos;
  }
`;

const GRASS_FRAG = /* glsl */ `
  precision highp float;
  varying vec3 vColor;
  varying vec3 vNormalW;
  varying vec3 vWorldPos;
  varying float vTopFactor;
  varying float vDistance;

  uniform vec3 sunDir;
  uniform vec3 sunColor;
  uniform vec3 ambientColor;
  uniform float fadeStart;
  uniform float fadeEnd;

  void main() {
    // Fade out distant grass for performance and to hide popping.
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, vDistance);
    if (fade < 0.02) discard;

    vec3 N = normalize(vNormalW);
    // Soft NdotL — grass blades face arbitrary directions, so we use abs().
    float ndl = abs(dot(N, sunDir));
    vec3 lit = vColor * (ambientColor * 0.6 + sunColor * (0.45 + 0.55 * ndl));

    // Darker at base, brighter at tip — fakes self-shadowing within the blade.
    lit *= mix(0.55, 1.15, vTopFactor);

    gl_FragColor = vec4(lit, fade);
  }
`;
