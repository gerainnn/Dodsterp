/**
 * SkySystem — atmospheric sky (Hosek-Wilkie analytical model from
 * three/examples/jsm/objects/Sky.js) plus a single sun light.
 *
 * Provides a coherent sun direction so other systems (water, grass) can
 * shade consistently.
 */
import * as THREE from 'three';
import { Sky } from 'three/examples/jsm/objects/Sky.js';

export class SkySystem {
  readonly sky: Sky;
  readonly sun: THREE.DirectionalLight;
  readonly hemi: THREE.HemisphereLight;
  readonly sunDirection = new THREE.Vector3();
  readonly sunColor = new THREE.Color();

  constructor(scene: THREE.Scene) {
    this.sky = new Sky();
    this.sky.scale.setScalar(20000);
    const u = this.sky.material.uniforms;
    u.turbidity.value = 4.0;
    u.rayleigh.value = 1.4;
    u.mieCoefficient.value = 0.005;
    u.mieDirectionalG.value = 0.85;
    scene.add(this.sky);

    // Sun light. Position is set by setSunByElevation().
    this.sun = new THREE.DirectionalLight(0xfff0d0, 3.2);
    this.sun.castShadow = true;
    this.sun.shadow.mapSize.set(2048, 2048);
    const cam = this.sun.shadow.camera as THREE.OrthographicCamera;
    cam.near = 0.5;
    cam.far = 250;
    cam.left = -90;
    cam.right = 90;
    cam.top = 90;
    cam.bottom = -90;
    this.sun.shadow.bias = -0.0008;
    this.sun.shadow.normalBias = 0.04;
    scene.add(this.sun);
    scene.add(this.sun.target);

    // Soft ambient from sky/ground hemisphere.
    this.hemi = new THREE.HemisphereLight(0xbfd6ff, 0x4a4030, 0.35);
    scene.add(this.hemi);

    // Default to a "late golden hour" mood.
    this.setSunByElevation(28, 145);
  }

  /**
   * Set the sun direction by elevation (degrees above horizon) and
   * azimuth (degrees clockwise from -Z).
   */
  setSunByElevation(elevationDeg: number, azimuthDeg: number): void {
    const phi = THREE.MathUtils.degToRad(90 - elevationDeg);
    const theta = THREE.MathUtils.degToRad(azimuthDeg);
    const sun = new THREE.Vector3();
    sun.setFromSphericalCoords(1, phi, theta);
    this.sky.material.uniforms.sunPosition.value.copy(sun);

    this.sunDirection.copy(sun).normalize();
    // Position the directional light "far" along the sun direction.
    this.sun.position.copy(this.sunDirection).multiplyScalar(150);
    this.sun.target.position.set(0, 0, 0);
    this.sun.target.updateMatrixWorld();

    // Approximate sun color: warmer at low elevation.
    const t = Math.max(0, Math.min(1, elevationDeg / 60));
    this.sunColor.setRGB(1.0, 0.85 + t * 0.12, 0.7 + t * 0.25);
    this.sun.color.copy(this.sunColor);
  }

  /** Move the shadow camera to follow the player. */
  followShadowCamera(target: THREE.Vector3): void {
    const off = this.sunDirection.clone().multiplyScalar(150);
    this.sun.position.copy(target).add(off);
    this.sun.target.position.copy(target);
    this.sun.target.updateMatrixWorld();
  }

  dispose(): void {
    this.sky.material.dispose();
  }
}
