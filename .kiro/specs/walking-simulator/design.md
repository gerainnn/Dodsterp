# Design — Walking Simulator (Real World Edition)

> Цель: выжать максимум из WebGL2 на Poco X6 Pro и десктопе. Рендер выглядит как high-end mobile game / arch-viz.

## 1. Технологический стек

### Ядро
| Технология | Версия | Зачем |
|---|---|---|
| **TypeScript** | 5.4+ | Типобезопасность для большой кодовой базы |
| **Vite** | 5+ | Быстрый dev-сервер, ESM, оптимальный билд |
| **Three.js** | r163+ | Рендер-движок (зрелый, огромный ecosystem) |
| **simplex-noise** | 4+ | Шум для рельефа и биомов |
| **nipplejs** | 0.10+ | Готовый виртуальный джойстик |
| **stats.js** | (dev only) | FPS-монитор |

### Почему Three.js, а не Babylon.js / PlayCanvas
- Самый зрелый ecosystem (тысячи примеров)
- Меньше абстракций → больше контроля над шейдерами
- Меньше бандл (≈ 150 KB gzip vs 350+ у Babylon)
- Все нужные постпроцессоры есть в `three/examples/jsm/postprocessing`

### Почему НЕ WebGPU в v1
- Поддержка в Three.js (`WebGPURenderer`) ещё experimental
- Safari iOS = WebGL2 только (WebGPU за флагом)
- Откладываем на v2; архитектура учитывает миграцию.

## 2. Архитектура (модули)

```
src/
├── main.ts                      # Точка входа, инициализация
├── core/
│   ├── Engine.ts                # Главный game loop, оркестрация модулей
│   ├── Renderer.ts              # Three.js renderer + post-processing
│   ├── Scene.ts                 # Сцена, освещение, небо
│   ├── Clock.ts                 # delta time, fixed timestep для физики
│   └── QualityManager.ts        # Адаптивное качество, пресеты
├── world/
│   ├── World.ts                 # Управление чанками
│   ├── Chunk.ts                 # Один чанк (геометрия + объекты)
│   ├── ChunkLoader.ts           # Async загрузка/выгрузка чанков
│   ├── TerrainGenerator.ts      # Heightmap из шума
│   ├── BiomeMap.ts              # Температура/влажность → биом
│   ├── biomes/
│   │   ├── BiomeRegistry.ts     # Все биомы
│   │   ├── ForestBiome.ts
│   │   ├── PineBiome.ts
│   │   ├── MountainBiome.ts
│   │   ├── LakeBiome.ts
│   │   ├── PlainsBiome.ts
│   │   └── TundraBiome.ts
│   └── scatter/
│       ├── VegetationScatter.ts # Размещение деревьев/травы
│       ├── InstancedForest.ts   # GPU-instanced деревья
│       └── GrassField.ts        # GPU-instanced трава с ветром
├── render/
│   ├── materials/
│   │   ├── TerrainMaterial.ts   # Splatmap blend для биомов
│   │   ├── WaterMaterial.ts     # Реалистичная вода
│   │   ├── GrassMaterial.ts     # Trava с wind shader
│   │   └── SkyMaterial.ts       # Atmospheric sky
│   ├── postfx/
│   │   ├── PostFXComposer.ts    # Effect Composer
│   │   ├── SSAOPass.ts
│   │   ├── BloomPass.ts
│   │   ├── TAAPass.ts
│   │   └── FSR1Pass.ts          # Upscale 1080p → native
│   ├── lighting/
│   │   ├── SunLight.ts          # Directional + cascaded shadows
│   │   ├── HDRIEnvironment.ts   # IBL из HDRI
│   │   └── Atmosphere.ts        # Rayleigh scattering sky
│   └── clouds/
│       └── VolumetricClouds.ts  # Raymarched clouds в куполе
├── player/
│   ├── Player.ts                # Позиция, скорость, состояние
│   ├── PlayerController.ts      # Объединяет input + physics
│   ├── Camera.ts                # FPS-камера
│   └── Physics.ts               # Гравитация, коллизия с террейном
├── input/
│   ├── InputManager.ts          # Унифицированный API
│   ├── TouchInput.ts            # Свайп камеры
│   ├── JoystickInput.ts         # nipplejs wrapper
│   └── KeyboardMouseInput.ts    # Десктоп фолбек
├── ui/
│   ├── HUD.ts                   # Контейнер для всего UI
│   ├── Joystick.ts              # DOM элемент джойстика
│   ├── Minimap.ts               # Canvas2D миникарта + телепорт
│   ├── LoadingScreen.ts         # Прогрессбар на старте
│   ├── SettingsPanel.ts         # Свитчер качества
│   └── Crosshair.ts             # Точка в центре
├── assets/
│   ├── AssetLoader.ts           # GLTF + текстуры + HDRI
│   └── manifest.ts              # Список всех ассетов
└── utils/
    ├── math.ts                  # Хелперы (lerp, smoothstep)
    ├── rng.ts                   # Seedable RNG (mulberry32)
    └── deviceDetect.ts          # Mobile/desktop, GPU tier
```

**Принципы:**
- **Один модуль = одна ответственность**
- `Engine.ts` — единственный, кто знает обо всех модулях
- Модули общаются через явные API, не через global state
- Все хот-пути (game loop, генерация чанков) — без аллокаций (object pools)

## 3. Генерация мира

### 3.1 Чанки

```ts
const CHUNK_SIZE = 64;        // юнитов
const CHUNK_VERTS = 65;       // вершин на сторону (CHUNK_SIZE+1)
const VERTEX_SPACING = 1.0;   // 1 юнит = 1 метр
```

**Координаты**: чанк `(cx, cz)` покрывает `[cx*64, cx*64+64) × [cz*64, cz*64+64)`.

**Жизненный цикл**:
1. Каждый кадр определяем чанк игрока `(pcx, pcz)`
2. В радиусе `renderDistance` все чанки должны быть `loaded`
3. Все вне `renderDistance + 1` → `unloaded` (диспозим геометрию)
4. Загрузка асинхронна: одна генерация в кадр (избегаем фриза)
5. Очередь приоритезируется по дистанции от игрока

### 3.2 Heightmap

Multi-octave Simplex noise:

```ts
function heightAt(x: number, z: number, biome: BiomeData): number {
  let h = 0;
  let amp = biome.amplitude;     // горы=40, поляна=4
  let freq = biome.frequency;    // 0.005..0.02
  for (let o = 0; o < 5; o++) {
    h += simplex2(x * freq, z * freq) * amp;
    amp *= 0.5;
    freq *= 2.0;
  }
  return h + biome.baseHeight;
}
```

**Сглаживание границ биомов**: на каждой вершине берём 4 ближайших биома и weighted-average высоту → нет резких швов.

### 3.3 Биомы

**Вход**: `temperature` (−1..1) + `humidity` (0..1) — два независимых noise-поля с большой длиной волны (≈ 1024 юнита).

**Whittaker-таблица**:
```
        cold    temperate    hot
dry     tundra  plains       (нет — fallback plains)
mid     pine    forest       plains
wet     pine    forest       lake-area
```

Каждый биом определяет:
- `groundTexture` (grass / sand / rock / snow)
- `treeType` + `density`
- `grassDensity`
- `colorTint` для terrain material
- `amplitude`, `frequency` для рельефа

### 3.4 Размещение растительности

**Blue noise (Poisson disk sampling)** для деревьев — равномерно, но без сетки. Реализация: precomputed 2D blue-noise texture 256×256, читаем по `(x mod 256, z mod 256)` → положение + jitter.

**Трава**: instanced billboards, регулярная сетка с jitter, плотность из биома, 50k инстансов на чанк (High preset).

**Камни/цветы**: тоже blue noise, разная плотность.

## 4. Рендеринг — детали

### 4.1 Освещение

**Sun (DirectionalLight)**:
- Угол: 50° над горизонтом, азимут 135° (золотой час)
- Intensity: 3.0
- Color: тёплый `#FFE5B4`
- **Cascaded Shadow Maps**: 3 каскада на 30/100/300 метрах, 2048² (High)

**HDRI Environment**:
- HDR равнопанорама из PolyHaven (`kloppenheim_06_puresky_2k.hdr` или `qwantani_dusk_2_2k.hdr`)
- Используется как IBL для PBR (envMap у всех материалов)
- И как skybox (после prefilter)

**Ambient**: только из IBL, нет дешёвого `AmbientLight`.

### 4.2 Terrain Material

Кастомный shader (`MeshStandardMaterial` extended):
- 4 текстурных слоя (grass, dirt, rock, snow)
- Splatmap blending по биому + наклону + высоте
- Triplanar mapping на крутых склонах (нет растяжения)
- PBR: albedo + normal + roughness + AO для каждого слоя
- Текстуры из PolyHaven (`forest_floor_diff_2k`, `rock_face_03_diff_2k` и т.п.)

### 4.3 Вода

Кастомный shader (на основе `Water2` из three/examples, но улучшенный):
- **Planar reflection**: вторая камера, отзеркаленная по плоскости, рендер в render target
- **Refraction**: depth-aware, по depth buffer узнаём глубину под пикселем
- **Normal map**: 2 слоя движущихся нормалей (разные скорости/направления)
- **Fresnel**: Schlick approximation для смешения reflection/refraction
- **Foam**: на берегах (где depth < threshold) и вокруг объектов
- **Caustics**: опционально, если есть GPU budget

### 4.4 Облака

**Volumetric Clouds Shader** (упрощённый Horizon Zero Dawn approach):
- Sky dome (большая полусфера)
- В fragment shader — raymarching через 3D noise (Perlin-Worley)
- 32–64 степа (адаптивно)
- Half-resolution рендер + upscale (TAA закроет шум)
- Покрытие/плотность управляются uniform'ами (для будущей погоды)

На Low preset — billboards (статичные текстуры облаков на дальнем плане).

### 4.5 Post-processing pipeline

EffectComposer chain:
1. **RenderPass** — основной рендер сцены в RT
2. **SSAOPass** (half-res) — мягкое ambient occlusion
3. **BloomPass** — UnrealBloomPass, threshold=0.9, radius=0.4
4. **TAAPass** — temporal anti-aliasing (важно из-за травы)
5. **ToneMappingPass** — ACES Filmic
6. **FSR1Pass** — upscale до native screen res
7. **OutputPass**

### 4.6 Трава с ветром

Vertex shader displacement:
```glsl
vec3 windOffset = vec3(
  sin(time * windSpeed + worldPos.x * 0.1) * 0.3,
  0.0,
  cos(time * windSpeed + worldPos.z * 0.1) * 0.3
) * vertexHeight;  // только верхушки качаются

gl_Position = projection * modelView * vec4(localPos + windOffset, 1.0);
```

Сетка 5-вершинного билборда (cross-quad) → outdoor 60 FPS на 50k инстансах.

## 5. Управление

### 5.1 Джойстик (slot bottom-left)

`nipplejs` создаёт overlay div. Состояние:
```ts
{ x: -1..1, y: -1..1, active: boolean }
```

В `Player.update()`:
```ts
const forward = camera.getForward(); // y=0
const right = camera.getRight();
const move = forward.multiplyScalar(joystick.y).add(right.multiplyScalar(joystick.x));
position.add(move.multiplyScalar(speed * dt));
```

### 5.2 Свайп камеры

`PointerEvents` на canvas с фильтром «не на UI элементе»:
- `pointerdown` → начало свайпа
- `pointermove` → `dx, dy` → `yaw -= dx * sensitivity`, `pitch -= dy * sensitivity`
- Multi-touch: первый палец — камера, второй (вне UI) — игнор

Pitch clamp: `[-89°, 89°]`.

### 5.3 Десктоп

- WASD → как джойстик
- Mouse + PointerLock API → как свайп
- Esc → unlock

### 5.4 Физика

**Простая, не Cannon/Rapier** (избегаем тяжёлый бандл):
- Игрок = AABB 0.6 × 1.7 × 0.6
- Гравитация `-9.8 m/s²`
- Каждый кадр: запрос `terrain.heightAt(x, z)` → если `player.y < ground + 1.7` → `player.y = ground + 1.7, vy = 0`
- Без коллизий с деревьями (это walking sim, проходим сквозь — простой компромисс) **либо** простой AABB-чек по ближайшим деревьям (по выбору)

## 6. Миникарта

### 6.1 Подход

**Canvas2D**, не отдельная камера (3-rd render pass — слишком дорого). Рисуем по chunkData:

```ts
for each chunk in radius:
  for each vertex in chunk:
    canvas.fillRect(px, py, 1, 1, biomeColor[vertex])
```

Кешируется per-chunk (рисуем один раз при загрузке чанка), затем blit на основной canvas миникарты при каждом обновлении.

### 6.2 Маркер игрока

Треугольник в центре миникарты, повёрнутый по `camera.yaw`. Миникарта не вращается — север сверху (классика).

### 6.3 Тап-телепорт

```ts
minimap.onTap = (px, py) => {
  const worldX = player.x + (px - mapCenterX) / scale;
  const worldZ = player.z + (py - mapCenterY) / scale;
  const y = terrain.heightAt(worldX, worldZ) + 1.7;
  fadeToBlack(300ms).then(() => {
    player.position.set(worldX, y, worldZ);
    chunkLoader.requestUrgent(worldX, worldZ);
    fadeToClear(300ms);
  });
};
```

При телепорте — приоритетная синхронная загрузка чанка под игроком (1 кадр заморозки приемлем).

## 7. Ассеты

Все CC0 / открытые лицензии:

| Тип | Источник | Примеры |
|---|---|---|
| HDRI | [PolyHaven](https://polyhaven.com/hdris) | `kloppenheim_06_puresky` |
| Текстуры | PolyHaven, AmbientCG | `forest_floor`, `rocks_ground_02`, `snow_03` |
| Деревья | [Quaternius Ultimate Nature](https://quaternius.com/) | Pine, Oak, Birch, Dead — GLTF с LOD |
| Камни | Quaternius | Rock_1..5 |
| Трава | Свой billboard atlas | 4 варианта в одной 512² текстуре |
| Цветы | Quaternius / свои билборды | |

Бюджет ассетов:
- HDRI: 2 KB EXR = ~2 MB (лучше) или JPG = 500 KB
- Деревья: 5 моделей × ~500 KB GLTF compressed = 2.5 MB
- Текстуры: 8 × ~1 MB = 8 MB (KTX2/Basis compressed)
- Итого: ≤ **15 MB** на старте, ленивая дозагрузка для редких биомов.

Compression: **KTX2 + Basis Universal** для всех текстур (нативно поддерживается Three.js через `KTX2Loader`). Деревья — **DRACO + meshopt**.

## 8. Loading flow

```
1. HTML загружен → показываем LoadingScreen (CSS-only, без JS-тяжести)
2. Vite chunk main.ts loaded → progressbar 5%
3. AssetLoader.preload() — параллельно:
   - HDRI                              → 30%
   - Все terrain текстуры               → 50%
   - Базовые tree GLTFs                 → 70%
   - Grass atlas                        → 80%
4. Engine.init() — создаёт сцену, прогревает шейдеры (рендерим offscreen сферу с каждым материалом)  → 95%
5. Первый чанк сгенерирован под игроком → 100%
6. Fade out loading screen, fade in мир (300ms)
```

## 9. Адаптивное качество

`QualityManager`:
- На старте: `deviceDetect()` → `mobile_high` / `mobile_low` / `desktop` → пресет
- Каждые 2 секунды: усреднённый FPS
- Если `avg < 45` 3 раза подряд → понижаем пресет, показываем тост «Качество снижено для плавности»
- Юзер может в Settings зафиксировать пресет вручную

## 10. Дерево решений «откатов» при отсутствии WebGL2

WebGL2 нужен для:
- `texture2DArray` (terrain splatmap)
- `EXT_color_buffer_float` (post-processing)
- Cascaded shadows (multi-render-target)

Если нет → показываем экран «Браузер не поддерживается, попробуйте Chrome/Safari последней версии». Не делаем WebGL1 фолбек (overhead не оправдан).

## 11. Как будем понимать, что готово

| Метрика | Цель |
|---|---|
| FPS на Poco X6 Pro (High) | ≥ 58 в среднем |
| First paint | < 2s |
| Полная загрузка | < 5s на 4G |
| Memory peak | < 700 MB |
| Бесшовное пересечение чанка | 0 фризов > 16ms |
| Тап на миникарте → видно мир | < 600ms |

## 12. Будущие фичи (за рамками v1, но архитектура готова)

- Цикл день/ночь (просто меняем sun angle + HDRI blend)
- Погода (дождь, туман — particles + post fog)
- Звук (Howler.js, ambient layer + footsteps по биому)
- Сохранение позиции в LocalStorage
- Шеринг seed'а через URL (`?seed=12345`)
- WebGPU рендерер (отдельный путь рендера)
- Кооп / multiplayer (WebRTC + общий seed)
