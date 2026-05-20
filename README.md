# Wanderworld

Photorealistic walking simulator in the browser. Procedurally generated infinite world with realistic biomes, mobile-first controls, and a tap-to-teleport minimap.

> Status: **Phase 1 / 17** — bootstrap and minimal 3D scene. See [`.kiro/specs/walking-simulator/tasks.md`](.kiro/specs/walking-simulator/tasks.md) for the full plan.

## Stack

- TypeScript (strict)
- Vite 5
- Three.js r163
- nipplejs, simplex-noise

## Local dev

```bash
cd wanderworld
npm install
npm run dev
```

Open the LAN URL on your phone (Vite logs it: `http://192.168.x.x:5173`). The site is full-screen and uses touch controls.

## Build

```bash
npm run build      # → wanderworld/dist
npm run preview    # serves the production build
```

## Spec

The full design lives in `.kiro/specs/walking-simulator/`:

- `requirements.md` — what we're building, perf targets, accept criteria
- `design.md` — architecture, modules, render pipeline
- `tasks.md` — phased implementation plan
