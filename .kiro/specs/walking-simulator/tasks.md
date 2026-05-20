# Tasks — Walking Simulator (Real World Edition)

> Каждая фаза = независимый, проверяемый инкремент. После каждой фазы можно открыть в браузере и увидеть прогресс.

## Phase 0 — Bootstrap (≈ 30 мин)

- [ ] **0.1** Инициализировать проект: `npm create vite@latest walking-sim -- --template vanilla-ts`
- [ ] **0.2** Установить зависимости: `three`, `simplex-noise`, `nipplejs`, dev: `@types/three`, `stats.js`
- [ ] **0.3** Настроить `tsconfig.json` (strict, paths)
- [ ] **0.4** Настроить `vite.config.ts` (base path для Pages, asset handling, optimizeDeps для three)
- [ ] **0.5** Базовый `index.html`: full-screen canvas, viewport meta для мобилок (`user-scalable=no`), preload HDRI
- [ ] **0.6** Создать структуру папок согласно design.md §2

**Acceptance**: `npm run dev` открывает чёрный экран без ошибок.

---

## Phase 1 — Минимальный 3D мир (≈ 1.5 часа)

- [ ] **1.1** `Renderer.ts`: настроить `WebGLRenderer` (antialias, sRGB, ACESFilmic tonemap, shadowMap.enabled)
- [ ] **1.2** `Engine.ts`: главный requestAnimationFrame loop с delta time
- [ ] **1.3** `Scene.ts`: пустая сцена + базовое skyblue фоновое окружение
- [ ] **1.4** Простая `PerspectiveCamera` с фиксированным положением
- [ ] **1.5** Один `DirectionalLight` (sun) + `HemisphereLight` (для тестов)
- [ ] **1.6** Плоский plane 100×100 как заглушка terrain
- [ ] **1.7** Один тестовый куб для проверки масштаба
- [ ] **1.8** `stats.js` в углу для FPS

**Acceptance**: открываем — синее небо, серый пол, белый куб, FPS показан.

---

## Phase 2 — Procedural terrain (≈ 3 часа)

- [ ] **2.1** `utils/rng.ts`: seedable RNG (mulberry32)
- [ ] **2.2** `TerrainGenerator.ts`: функция `heightAt(x, z, seed) → number` через multi-octave Simplex
- [ ] **2.3** `Chunk.ts`: класс с PlaneGeometry 65×65 вершин, `applyHeightmap()` (заполняет y координаты), `computeVertexNormals()`
- [ ] **2.4** Базовый `MeshStandardMaterial` с зелёным albedo на чанк
- [ ] **2.5** `World.ts`: статичная сетка 5×5 чанков вокруг (0,0)
- [ ] **2.6** Игрок в центре, можно посмотреть мышью (временный OrbitControls для разработки)

**Acceptance**: видим зелёные холмы, бесшовные между чанками, без T-junction артефактов.

---

## Phase 3 — Биомы и палитра (≈ 2 часа)

- [ ] **3.1** `BiomeMap.ts`: noise температуры + влажности с большой длиной волны
- [ ] **3.2** `BiomeRegistry.ts`: 6 биомов из requirements §FR-2 с конфигами
- [ ] **3.3** `Chunk.applyHeightmap()` использует биом каждой вершины (амплитуда/частота)
- [ ] **3.4** Сглаживание границ (4 nearest biomes weighted average)
- [ ] **3.5** Vertex colors на terrain меше из biome.colorTint (быстрый MVP до splatmap)
- [ ] **3.6** `MeshStandardMaterial` с `vertexColors: true`

**Acceptance**: видим разные зоны разного цвета, плавные переходы.

---

## Phase 4 — Динамическая загрузка чанков (≈ 2 часа)

- [ ] **4.1** `ChunkLoader.ts`: очередь генерации, max 1 чанк в кадр
- [ ] **4.2** `World.update(playerPos)`: вычисляет нужные чанки, добавляет в очередь / убирает лишние
- [ ] **4.3** Корректная утилизация: `geometry.dispose()`, `material.dispose()` на выгрузке
- [ ] **4.4** Object pool для геометрий (переиспользуем буферы)
- [ ] **4.5** Тест: ходить по плоскому миру, проверить отсутствие лагов и утечек памяти

**Acceptance**: ходим в любую сторону, чанки подгружаются плавно, в DevTools Performance — нет фризов > 16ms.

---

## Phase 5 — Игрок и управление (мобайл-first) (≈ 3 часа)

- [ ] **5.1** `Player.ts`: позиция, скорость, yaw/pitch
- [ ] **5.2** `Physics.ts`: гравитация, привязка к terrain.heightAt
- [ ] **5.3** `Camera.ts`: FPS-камера, follow Player.position + offset
- [ ] **5.4** `JoystickInput.ts`: инициализация nipplejs в bottom-left, экспорт `{x, y}`
- [ ] **5.5** `TouchInput.ts`: PointerEvents на canvas, dx/dy → yaw/pitch (с фильтром «не на UI»)
- [ ] **5.6** `KeyboardMouseInput.ts`: WASD + PointerLock для десктопа
- [ ] **5.7** `InputManager.ts`: автодетект устройства, активирует нужные модули
- [ ] **5.8** `PlayerController.ts`: объединяет input + physics, обновляет Player
- [ ] **5.9** Удалить временный OrbitControls
- [ ] **5.10** Тест на телефоне через Vite network mode

**Acceptance**: на телефоне джойстик двигает, свайп крутит камеру, на десктопе WASD+мышь работают.

---

## Phase 6 — Реалистичное освещение (≈ 3 часа)

- [ ] **6.1** Скачать HDRI из PolyHaven (`kloppenheim_06_puresky_2k.hdr`)
- [ ] **6.2** `HDRIEnvironment.ts`: `RGBELoader` + `PMREMGenerator` → `scene.environment`
- [ ] **6.3** Skybox: equirectangular на большой сфере (или `scene.background = pmremTexture`)
- [ ] **6.4** `SunLight.ts`: настроить угол под HDRI, intensity, color
- [ ] **6.5** Cascaded shadow maps: реализовать через `CSM` (есть готовый в `three/examples/jsm/csm`)
- [ ] **6.6** 3 каскада, 2048², far=300m
- [ ] **6.7** Корректный shadow bias (избегаем shadow acne и peter-panning)
- [ ] **6.8** Все материалы: `envMapIntensity = 1.0`

**Acceptance**: terrain выглядит освещённым реалистично, тени мягкие, нет артефактов.

---

## Phase 7 — PBR Terrain Material (≈ 4 часа)

- [ ] **7.1** Скачать текстуры из PolyHaven (forest_floor, sand, rock, snow, grass) — 2K, KTX2
- [ ] **7.2** `KTX2Loader` setup
- [ ] **7.3** `TerrainMaterial.ts`: extend MeshStandardMaterial через `onBeforeCompile`
- [ ] **7.4** Splatmap blending в fragment shader (4 слоя по биомам/высоте/наклону)
- [ ] **7.5** Triplanar mapping для крутых склонов (slope > 45°)
- [ ] **7.6** Normal map per-layer
- [ ] **7.7** Tiled UVs с разной частотой для каждого слоя

**Acceptance**: terrain выглядит детализированно вблизи, без растяжения на склонах.

---

## Phase 8 — Растительность: деревья (≈ 4 часа)

- [ ] **8.1** Скачать tree pack от Quaternius (Pine, Oak, Birch, Dead, Bush)
- [ ] **8.2** Конвертировать в DRACO+meshopt GLTF
- [ ] **8.3** Сделать LOD (3 уровня: full, mid, billboard) — вручную в Blender или скриптом
- [ ] **8.4** `AssetLoader.ts`: GLTF + DRACO + meshopt loaders
- [ ] **8.5** `VegetationScatter.ts`: blue noise sampling, по плотности биома
- [ ] **8.6** `InstancedForest.ts`: `InstancedMesh` per tree type + LOD switch
- [ ] **8.7** Frustum culling вручную для instancing (built-in не работает на InstancedMesh)
- [ ] **8.8** Тени от деревьев (но cascaded shadows, не для всех)

**Acceptance**: чанки засажены деревьями, плотность зависит от биома, FPS держится 60.

---

## Phase 9 — Растительность: трава с ветром (≈ 3 часа)

- [ ] **9.1** Сделать grass billboard atlas (cross-quad mesh, 4 текстуры в атласе)
- [ ] **9.2** `GrassMaterial.ts`: custom shader с wind vertex displacement
- [ ] **9.3** `GrassField.ts`: InstancedMesh, 50k инстансов на чанк, jittered grid
- [ ] **9.4** Density по биому (густо в forest, редко в desert/tundra)
- [ ] **9.5** Альфа-тест в фрагменте (без alpha blend → правильная сортировка не нужна)
- [ ] **9.6** Distance fade — трава исчезает дальше 50m

**Acceptance**: трава качается на ветру, плотная, не лагает.

---

## Phase 10 — Реалистичная вода (≈ 4 часа)

- [ ] **10.1** Определить уровень воды (`waterLevel = 0`), видим где terrain ниже
- [ ] **10.2** Большой плоский Plane на waterLevel в каждом «мокром» чанке (или один глобальный с маской)
- [ ] **10.3** `WaterMaterial.ts`: planar reflection (вторая камера + RT)
- [ ] **10.4** Refraction depth-aware (sample depth buffer)
- [ ] **10.5** 2 нормали движущиеся в разных направлениях
- [ ] **10.6** Fresnel mix
- [ ] **10.7** Foam shoreline (где depth < 0.5)
- [ ] **10.8** Reflection RT на half-resolution для производительности

**Acceptance**: вода красиво отражает небо/деревья, у берега виден песок/камни через прозрачность.

---

## Phase 11 — Облака (≈ 3 часа)

- [ ] **11.1** `VolumetricClouds.ts`: shader-based на sky dome
- [ ] **11.2** Generate 3D Perlin-Worley noise texture (offline, сохранить как KTX2 3D)
- [ ] **11.3** Raymarching в fragment shader, 32 степа на high
- [ ] **11.4** Освещение облаков от sun (Henyey-Greenstein)
- [ ] **11.5** Half-resolution рендер + upscale
- [ ] **11.6** Low preset fallback: 4 billboard облака

**Acceptance**: на небе видны объёмные облака, в рассветном свете подсвечены красиво.

---

## Phase 12 — Post-processing (≈ 2 часа)

- [ ] **12.1** `PostFXComposer.ts`: EffectComposer setup
- [ ] **12.2** SSAOPass (half-res, blur)
- [ ] **12.3** UnrealBloomPass (threshold 0.9, strength 0.4)
- [ ] **12.4** TAAPass — настроить jitter под половину пикселя
- [ ] **12.5** ToneMappingPass (ACES Filmic, exposure 1.0)
- [ ] **12.6** FSR1Pass — простая реализация для 1080p → native
- [ ] **12.7** Профайлинг: каждый pass должен стоить < 2ms на Poco

**Acceptance**: картинка стала «киношной», без алиасинга, мягкое AO в углах.

---

## Phase 13 — Миникарта + телепорт (≈ 3 часа)

- [ ] **13.1** `Minimap.ts`: HTML canvas 200×200 в правом верхнем углу
- [ ] **13.2** Per-chunk minimap cache: рендерим 32×32 миниатюру из biome colors при загрузке чанка
- [ ] **13.3** Composite cached chunks вокруг игрока на main minimap canvas
- [ ] **13.4** Player marker — треугольник в центре, поворот по yaw
- [ ] **13.5** Tap handler: pixel → world coords
- [ ] **13.6** Teleport: fade out (CSS overlay 300ms) → переместить → загрузить чанк urgent → fade in
- [ ] **13.7** Запретить телепорт в воду (если depth > 0.5 — телепортирует на ближайший берег)

**Acceptance**: миникарта показывает биомы вокруг, тап → плавная телепортация.

---

## Phase 14 — UI / Loading / Settings (≈ 2 часа)

- [ ] **14.1** `LoadingScreen.ts`: full-screen overlay с progressbar
- [ ] **14.2** `AssetLoader` сообщает прогресс
- [ ] **14.3** Shader warmup (рендерим offscreen pre-compile)
- [ ] **14.4** `SettingsPanel.ts`: модалка с пресетами Low/High/Ultra + Reset position
- [ ] **14.5** Кнопка settings в углу
- [ ] **14.6** Crosshair (точка в центре)
- [ ] **14.7** Mobile fullscreen API + lock orientation landscape

**Acceptance**: загрузка с прогрессом, можно поменять качество, fullscreen на телефоне работает.

---

## Phase 15 — Адаптивное качество (≈ 1.5 часа)

- [ ] **15.1** `QualityManager.ts`: 3 пресета
- [ ] **15.2** `deviceDetect.ts`: User agent + GPU tier (через `getParameter(UNMASKED_RENDERER_WEBGL)`)
- [ ] **15.3** FPS monitor: rolling average over 2s
- [ ] **15.4** Если avg FPS < 45 три раза подряд → понизить пресет
- [ ] **15.5** Toast notification «Качество снижено»
- [ ] **15.6** Применение пресета на лету (renderer.setPixelRatio, render distance, post pass enable/disable)

**Acceptance**: на слабом девайсе автоматом откатывается до Low; вручную можно зафиксировать.

---

## Phase 16 — Полировка (≈ 3 часа)

- [ ] **16.1** Проверка на iPhone Safari (часто отличается от Chrome)
- [ ] **16.2** Тест на Poco X6 Pro: профилирование, подгонка high preset до стабильных 60
- [ ] **16.3** Memory leak check: ходить 10 минут, должно быть стабильно
- [ ] **16.4** Корректная обработка resize / orientation change
- [ ] **16.5** Меню паузы (опционально)
- [ ] **16.6** Логотип / тайтл на загрузке
- [ ] **16.7** Favicon

---

## Phase 17 — Деплой (≈ 30 мин)

- [ ] **17.1** GitHub repo `walking-sim` (создаём в начале или в конце)
- [ ] **17.2** GitHub Action: build на push, deploy в `gh-pages`
- [ ] **17.3** Vite `base: '/walking-sim/'` для GitHub Pages
- [ ] **17.4** Прислать пользователю ссылку
- [ ] **17.5** Тест на реальном Poco X6 Pro по public URL

**Acceptance**: пользователь открывает ссылку на телефоне → всё работает.

---

## Estimated total

≈ **40–45 часов** активной работы на всё. Реальная сессия: я делаю инкрементально, ты тестируешь после каждой фазы и даёшь фидбек.

## Порядок: что я предлагаю делать первым

Сначала **Phase 0 → 5** — это уже даст играбельный мир (без красоты, но с механикой). Ты сможешь зайти на сайт с телефона, ходить, крутить камеру.

Потом **Phase 6 → 12** — навешиваем красоту слоями, после каждого слоя картинка становится лучше.

Потом **Phase 13 → 17** — миникарта, UI, деплой.

## Чекпоинты для тебя

После каждой фазы я запушу в Git ветку и дам ссылку. Ты:
1. Зайдёшь на телефоне
2. Скажешь «ок» или «вот тут поправь»
3. Идём дальше

---

## Решения (зафиксировано)

| Вопрос | Решение |
|---|---|
| **Имя проекта** | `wanderworld` |
| **Деплой** | GitHub Pages (gh-pages branch, авто через GitHub Actions) |
| **Seed мира** | Рандомный при каждом запуске + поддержка `?seed=` в URL для шеринга |
| **Стартовый биом** | Случайный (где сгенерировался — там и появился игрок) |
| **Коллизия с деревьями** | Да, AABB-чек по ближайшим деревьям |
| **Звук** | Отложен на v2 (не критично) |
