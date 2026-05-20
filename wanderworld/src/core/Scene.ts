/**
 * Scene — wraps THREE.Scene with placeholder content for Phase 1.
 *
 * Phase 6 replaces sky+lights with HDRI/CSM.
 * Phase 2 replaces ground plane with chunked terrain.
 */
import * as THREE from 'three';

export class Scene {
  readonly scene: THREE.Scene;
  private readonly disposables: Array<{ dispose: () => void }> = [];
  private readonly testCube: THREE.Mesh;

  constructor() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x88b4ff);
    this.scene.fog = new THREE.FogExp2(0x88b4ff, 0.008);

    // Lights — placeholder. Phase 6 replaces with HDRI + CSM.
    const sun = new THREE.DirectionalLight(0xfff0d8, 3.0);
    sun.position.set(40, 80, 30);
    sun.castShadow = true;
    sun.shadow.mapSize.set(2048, 2048);
    sun.shadow.camera.near = 1;
    sun.shadow.camera.far = 200;
    sun.shadow.camera.left = -50;
    sun.shadow.camera.right = 50;
    sun.shadow.camera.top = 50;
    sun.shadow.camera.bottom = -50;
    sun.shadow.bias = -0.0005;
    this.scene.add(sun);

    const hemi = new THREE.HemisphereLight(0xbfd6ff, 0x4a4030, 0.6);
    this.scene.add(hemi);

    // Placeholder ground — replaced in Phase 2 by chunked terrain.
    const groundGeom = new THREE.PlaneGeometry(200, 200);
    const groundMat = new THREE.MeshStandardMaterial({
      color: 0x4a7a3a,
      roughness: 0.95,
      metalness: 0.0,
    });
    const ground = new THREE.Mesh(groundGeom, groundMat);
    ground.rotation.x = -Math.PI / 2;
    ground.receiveShadow = true;
    this.scene.add(ground);
    this.disposables.push(groundGeom, groundMat);

    // Placeholder cube — sanity check for scale and shadows.
    const cubeGeom = new THREE.BoxGeometry(2, 2, 2);
    const cubeMat = new THREE.MeshStandardMaterial({
      color: 0xffffff,
      roughness: 0.4,
      metalness: 0.05,
    });
    this.testCube = new THREE.Mesh(cubeGeom, cubeMat);
    this.testCube.position.set(0, 1, 0);
    this.testCube.castShadow = true;
    this.testCube.receiveShadow = true;
    this.scene.add(this.testCube);
    this.disposables.push(cubeGeom, cubeMat);
  }

  update(dt: number): void {
    // Spin the test cube so we can verify the loop runs at all.
    // Removed as soon as Phase 2 lands real content.
    this.testCube.rotation.y += dt * 0.6;
  }

  dispose(): void {
    for (const d of this.disposables) d.dispose();
  }
}
