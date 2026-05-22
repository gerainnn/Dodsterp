# cs2_cheat

Internal DLL cheat для CS2 для лабораторного / учебного использования
на оффлайн-матчах с ботами. ESP + healthbar поверх игры через ImGui,
рендеринг через хук `IDXGISwapChain::Present`.

> Предназначено для тестирования на собственной машине **без подключения
> к матчмейкингу Valve и community-серверам**. На VAC-secure серверах
> ловится мгновенно, и это не задача данного проекта.

## Структура

```
cs2_cheat/
├── CMakeLists.txt
├── src/
│   ├── dllmain.cpp     ← DllMain + worker thread
│   ├── globals.hpp     ← глобалы движка (D3D device, hwnd и т.п.)
│   ├── offsets.hpp     ← оффсеты CS2 из cs2-dumper
│   ├── sdk.hpp         ← обёртки над игровыми классами
│   ├── math.hpp/.cpp   ← Vector3/Matrix, world_to_screen
│   ├── hooks.hpp/.cpp  ← DX11 Present / WndProc hooks
│   ├── render.hpp/.cpp ← инициализация ImGui
│   ├── esp.hpp/.cpp    ← логика ESP (поиск pawn-ов, рисование)
│   └── menu.hpp/.cpp   ← ImGui-меню (toggles, цвета)
└── injector/
    ├── CMakeLists.txt
    └── injector.cpp    ← LoadLibrary-injector по имени процесса
```

## Сборка

Требуется Visual Studio 2022 (Build Tools достаточно), CMake 3.20+,
Windows SDK.

```powershell
cd cs2_cheat
cmake -S . -B build -A x64
cmake --build build --config Release
```

На выходе:
- `build/Release/cs2_cheat.dll` — собственно чит
- `build/injector/Release/injector.exe` — инжектор

## Обновление оффсетов

CS2 обновляется часто, оффсеты из `src/offsets.hpp` нужно регулярно
синхронизировать с публичным dumper-ом:

https://github.com/a2x/cs2-dumper

После каждого патча CS2 — заменяй численные значения в `offsets.hpp`
из свежих `client.dll.hpp` / `offsets.hpp` репозитория dumper-а.

## Использование

1. Запусти CS2.
2. В консоли игры (`~`) подключи бота для теста:
   ```
   sv_cheats 1
   map de_dust2
   bot_add_t
   bot_add_ct
   mp_warmup_end
   ```
3. Из обычной cmd:
   ```
   injector.exe cs2.exe path\to\cs2_cheat.dll
   ```
4. INSERT — открывает / закрывает меню.
5. END — выгружает DLL чисто (опционально, можно убить процесс).

## Грабли

- DX11-хук подразумевает что CS2 запущена в DirectX 11 (дефолт). Если
  принудительно запущена через `-vulkan` — нужен Vulkan-хук, в этой
  версии не реализован.
- ImGui захватывает мышь только когда меню открыто. Логика Present
  blocks input через WndProc-hook, но если игра уходит в полноэкранный
  exclusive — ввод может конфликтовать. Запускай в borderless / fullscreen
  windowed.
- При обновлении CS2 классы schema могут переименоваться — поля в
  `offsets.hpp` всегда сверяй именами, а не позиционно.
- DLL детектится загруженным по имени модуля. Для приватных тестов это
  норма. Для anti-cheat-protected окружения нужен manual map / reflective
  load — отдельная задача.
