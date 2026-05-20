/**
 * CameraController — sticks a perspective camera to the player and applies
 * yaw/pitch as a Y/X Euler rotation. Owns the camera so the engine just asks
 * for it.
 */
import * as THREE from 'three';
import type { Player } from './Player';

export class CameraController {
  readonly camera: THREE.PerspectiveCamera;
  /** Vertical eye offset above the player position. */
  private eyeOffset = 0;

  constructor(width: number, height: number, fov = 70) {
    this.camera = new THREE.PerspectiveCamera(fov, width / height, 0.1, 2000);
    this.camera.rotation.order = 'YXZ';
  }

  resize(width: number, height: number): void {
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
  }

  follow(player: Player): void {
    this.camera.position.copy(player.position);
    this.camera.position.y += this.eyeOffset;
    this.camera.rotation.set(player.pitch, player.yaw, 0, 'YXZ');
  }
}
