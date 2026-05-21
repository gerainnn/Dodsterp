# cs2-trainer

External-режим тренировочный тулинг под CS2. Чтение памяти процесса,
позже — оверлей с ESP и aim-helper. Только оффлайн / ботматчи / личный сервер.

## Архитектура

```
+------------------+        ReadProcessMemory        +-----------------+
|  cs2-trainer.exe |  <----------------------------- |    cs2.exe      |
|  (наш процесс)   |                                 | (Source 2 game) |
|                  |                                 +-----------------+
|  + memory reader |
|  + offsets cache |
|  + entity scan   |        topmost transparent
|  + overlay (D3D11) ---->  window over game
|  + ImGui menu    |
+------------------+
```

Никаких DLL-инъекций. Свой процесс, своё окно.

## Стадии разработки

1. **Stage 1 — Sanity scan (где мы сейчас).**
   Открываем процесс, находим базу `client.dll`, читаем `dwLocalPlayerPawn`,
   печатаем в консоль HP / позицию / тиму. Цель — убедиться что схема памяти
   читается, оффсеты валидны.

2. **Stage 2 — Entity loop.**
   Итерируем `dwEntityList`, фильтруем по `CCSPlayerPawn`, собираем
   массив игроков с health/origin/team/bones.

3. **Stage 3 — Overlay.**
   Прозрачное topmost-окно + D3D11 + ImGui. Рисуем тестовый прямоугольник
   поверх игры. WorldToScreen через view-matrix.

4. **Stage 4 — ESP.**
   Бокс по bounding box, healthbar, имя, дистанция. Отдельно — bone ESP
   через `CSkeletonInstance`.

5. **Stage 5 — Aim helper.**
   Расчёт угла до головы выбранной цели, плавный довод через `SendInput` /
   `mouse_event`. RCS (recoil compensation) опционально.

## Зависимости

- Windows 10/11, MSVC 2022 (или clang-cl)
- CMake >= 3.20
- vcpkg или ручная сборка:
  - [Dear ImGui](https://github.com/ocornut/imgui) (docking branch)
  - [nlohmann/json](https://github.com/nlohmann/json) — парсинг offsets.json
  - DirectX 11 SDK (идёт с Windows SDK)

## Оффсеты

Хардкодить нельзя — каждый патч CS2 ломает. Используем
[a2x/cs2-dumper](https://github.com/a2x/cs2-dumper):

```bash
# отдельно склонировать дампер, собрать, прогнать на запущенном CS2
# результат — output/offsets.json и output/client_dll.json
```

Кладём свежие JSON в `cs2-trainer/data/`, наш код подхватывает их при старте.

## Сборка

```bash
cd cs2-trainer
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Запуск

```bash
# 1. запустил CS2, зашёл в "Practice with bots" / Workshop map
# 2. от админа (нужно для OpenProcess к cs2.exe)
build\Release\cs2-trainer.exe
```

## Грабли

- **VAC.** Оффлайн с ботами VAC-secured серверов не касается, но VAC сканит
  при запуске CS2 в принципе. Учётка — отдельная, не основная.
- **Schema offsets едут.** Раз в неделю — пересобрать дамп.
- **Анти-чит будущего.** Valve может прикрутить usermode-античит как у
  Faceit/Vanguard. Сейчас (2026) — нет.
- **OpenProcess требует прав.** Запускать cs2-trainer из-под админа,
  иначе `PROCESS_VM_READ` обломается.
- **W2S за камерой.** View matrix даёт `clipspace`. Если `w <= 0` — точка
  за камерой, не рисуем.

## Дисклеймер

Никакого онлайн-использования. Никакой публикации бинарей. Личный R&D,
изучение Source 2 internals, тренировка с ботами.
