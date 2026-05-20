/**
 * Water — a single global infinite plane at y=0 with a custom shader.
 * Cheap (no real reflection passes) but visually rich:
 *   - 3 stacked sin waves shape an analytical normal field
 *   - Schlick Fresnel mixes a procedural sky tint with deep-water color
 *   - Blinn-Phong specular highlight for sun glint
 *   - Vertical fog matches the world's atmospheric tint
 *
 * Realistic enough that you immediately read the surface as water.
 */
import * as THREE from 'three';
import { WATER_LEVEL } from '../world/TerrainGenerator';

export class Water {
  readonly mesh: THREE.Mesh;
  readonly material: THREE.ShaderMaterial;

  constructor(scene: THREE.Scene) {
    const geom = new THREE.PlaneGeometry(4000, 4000, 1, 1);
    geom.rotateX(-Math.PI / 2);
    geom.translate(0, WATER_LEVEL, 0);

    this.material = new THREE.ShaderMaterial({
      uniforms: {
        time: { value: 0 },
        sunDir: { value: new THREE.Vector3(0.4, 0.8, 0.3).normalize() },
        sunColor: { value: new THREE.Color(0xfff0d0) },
        skyColor: { value: new THREE.Color(0xa0c4ff) },
        deepColor: { value: new THREE.Color(0x064a64) },
        shallowColor: { value: new THREE.Color(0x29a8c8) },
        fogColor: { value: new THREE.Color(0xa0c4ff) },
        fogDensity: { value: 0.005 },
      },
      transparent: true,
      depthWrite: false,
      vertexShader: WATER_VERT,
      fragmentShader: WATER_FRAG,
    });

    this.mesh = new THREE.Mesh(geom, this.material);
    this.mesh.name = 'water';
    this.mesh.frustumCulled = false;
    this.mesh.renderOrder = 1; // draw after terrain, before transparent UI
    this.mesh.receiveShadow = false;
    scene.add(this.mesh);
  }

  setSun(dir: THREE.Vector3, color: THREE.Color): void {
    this.material.uniforms.sunDir.value.copy(dir).normalize();
    this.material.uniforms.sunColor.value.copy(color);
  }

  setSky(color: THREE.Color): void {
    this.material.uniforms.skyColor.value.copy(color);
    this.material.uniforms.fogColor.value.copy(color);
  }

  follow(target: THREE.Vector3): void {
    // Keep the giant water plane centred on the player so we don't run out of plane.
    this.mesh.position.x = target.x;
    this.mesh.position.z = target.z;
  }

  update(dt: number): void {
    this.material.uniforms.time.value += dt;
  }

  dispose(): void {
    this.mesh.geometry.dispose();
    this.material.dispose();
  }
}

const WATER_VERT = /* glsl */ `
  varying vec3 vWorldPos;
  void main() {
    vec4 wp = modelMatrix * vec4(position, 1.0);
    vWorldPos = wp.xyz;
    gl_Position = projectionMatrix * viewMatrix * wp;
  }
`;

const WATER_FRAG = /* glsl */ `
  precision highp float;
  varying vec3 vWorldPos;

  uniform float time;
  uniform vec3 sunDir;
  uniform vec3 sunColor;
  uniform vec3 skyColor;
  uniform vec3 deepColor;
  uniform vec3 shallowColor;
  uniform vec3 fogColor;
  uniform float fogDensity;

  // Analytical 3-wave normal field. Cheap but believable.
  vec3 waveNormal(vec3 p, float t) {
    float a = sin(p.x * 0.42 + p.z * 0.17 + t * 1.1);
    float b = sin(p.x * 0.08 - p.z * 0.31 + t * 0.7);
    float c = sin(p.x * 0.22 + p.z * 0.45 + t * 1.6);
    float d = cos(p.x * 0.6  - p.z * 0.3  + t * 2.2);

    // Approximate gradient via finite differences of the wave function.
    float h = 0.6 * a + 0.4 * b + 0.3 * c + 0.15 * d;
    float dx =
      0.42 * 0.6 * cos(p.x * 0.42 + p.z * 0.17 + t * 1.1) +
      0.08 * 0.4 * cos(p.x * 0.08 - p.z * 0.31 + t * 0.7) +
      0.22 * 0.3 * cos(p.x * 0.22 + p.z * 0.45 + t * 1.6) +
     -0.6  * 0.15 * sin(p.x * 0.6  - p.z * 0.3  + t * 2.2);
    float dz =
      0.17 * 0.6 * cos(p.x * 0.42 + p.z * 0.17 + t * 1.1) +
     -0.31 * 0.4 * cos(p.x * 0.08 - p.z * 0.31 + t * 0.7) +
      0.45 * 0.3 * cos(p.x * 0.22 + p.z * 0.45 + t * 1.6) +
     -0.3  * 0.15 * sin(p.x * 0.6  - p.z * 0.3  + t * 2.2);
    return normalize(vec3(-dx * 0.3, 1.0, -dz * 0.3));
  }

  void main() {
    vec3 N = waveNormal(vWorldPos, time);
    vec3 V = normalize(cameraPosition - vWorldPos);

    // Fresnel — water gets reflective at grazing angles.
    float cosTheta = clamp(dot(N, V), 0.0, 1.0);
    float fresnel = pow(1.0 - cosTheta, 5.0);
    fresnel = mix(0.04, 1.0, fresnel);

    // Reflected sky lookup: tilt N up slightly to get a "warm horizon" feel.
    vec3 R = reflect(-V, N);
    vec3 reflCol = mix(skyColor * 0.85, skyColor * 1.15, clamp(R.y * 0.5 + 0.5, 0.0, 1.0));

    // Body color: shallower near grazing, deep at near-vertical view.
    vec3 body = mix(shallowColor, deepColor, clamp(1.0 - cosTheta, 0.0, 1.0));

    vec3 col = mix(body, reflCol, fresnel);

    // Sun specular (Blinn-Phong) — shines on the wave tops.
    vec3 H = normalize(sunDir + V);
    float spec = pow(max(dot(N, H), 0.0), 220.0);
    col += sunColor * spec * 1.6 * fresnel;

    // Atmospheric fog — fade to skyColor with distance.
    float d = length(cameraPosition - vWorldPos);
    float fog = 1.0 - exp(-fogDensity * d);
    col = mix(col, fogColor, clamp(fog, 0.0, 1.0));

    // Slightly translucent so very shallow water shows ground hints (without depth read).
    float alpha = mix(0.78, 0.96, fresnel);
    gl_FragColor = vec4(col, alpha);
  }
`;
