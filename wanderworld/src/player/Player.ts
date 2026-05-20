/**
 * Player — first-person walker with simple capsule-vs-tree collision.
 * Owns position, velocity, view angles, and uses the World for gravity and
 * tree pushback.
 */
import * as THREE from 'three';
import { wrapAngle, clamp } from '../utils/math';
import type { World } from '../world/World';
import type { InputSnapshot } from '../input/InputManager';

const HEIGHT = 1.7;
const RADIUS = 0.34;
const WALK_SPEED = 5.2;
const RUN_SPEED = 8.4;
const GRAVITY = 22;
const PITCH_LIMIT = THREE.MathUtils.degToRad(85);

export class Player {
  readonly position = new THREE.Vector3();
  readonly velocity = new THREE.Vector3();
  yaw = 0;
  pitch = 0;
  /** Whether the player is on solid ground this frame. */
  grounded = true;

  private readonly forward = new THREE.Vector3();
  private readonly right = new THREE.Vector3();
  private readonly displacement = new THREE.Vector3();

  /** Place the player on the ground at (x, z). Use after teleport. */
  setGroundPosition(world: World, x: number, z: number): void {
    const y = world.heightAt(x, z) + HEIGHT;
    this.position.set(x, y, z);
    this.velocity.set(0, 0, 0);
  }

  update(dt: number, input: InputSnapshot, world: World): void {
    // ---- Look (yaw + pitch) ----
    this.yaw = wrapAngle(this.yaw - input.lookDX);
    this.pitch = clamp(this.pitch - input.lookDY, -PITCH_LIMIT, PITCH_LIMIT);

    // ---- Movement basis ----
    const cy = Math.cos(this.yaw);
    const sy = Math.sin(this.yaw);
    // Forward = looking in -Z when yaw=0 (matches camera default).
    this.forward.set(-sy, 0, -cy);
    this.right.set(cy, 0, -sy);

    const speed = input.run ? RUN_SPEED : WALK_SPEED;
    // Movement input: x = strafe, z = forward (positive z = forward).
    this.displacement.set(0, 0, 0);
    this.displacement.addScaledVector(this.forward, input.moveZ);
    this.displacement.addScaledVector(this.right, input.moveX);
    if (this.displacement.lengthSq() > 1) this.displacement.normalize();

    this.velocity.x = this.displacement.x * speed;
    this.velocity.z = this.displacement.z * speed;
    this.velocity.y -= GRAVITY * dt;

    // Tentative new position.
    const nx = this.position.x + this.velocity.x * dt;
    const nz = this.position.z + this.velocity.z * dt;
    const ny = this.position.y + this.velocity.y * dt;

    // ---- Tree collision: push player out of any tree within (RADIUS + treeRadius). ----
    let cx = nx;
    let cz = nz;
    for (const tree of world.treesNear(nx, nz, 4)) {
      const dx = cx - tree.x;
      const dz = cz - tree.z;
      const minDist = RADIUS + 0.4 * tree.s;
      const d2 = dx * dx + dz * dz;
      if (d2 < minDist * minDist && d2 > 1e-6) {
        const d = Math.sqrt(d2);
        const push = (minDist - d) / d;
        cx += dx * push;
        cz += dz * push;
      }
    }

    // ---- Ground follow ----
    const groundY = world.heightAt(cx, cz) + HEIGHT;
    let cy2: number;
    if (ny <= groundY) {
      cy2 = groundY;
      this.velocity.y = 0;
      this.grounded = true;
    } else {
      cy2 = ny;
      this.grounded = false;
    }

    this.position.set(cx, cy2, cz);
  }

  get height(): number {
    return HEIGHT;
  }
}
