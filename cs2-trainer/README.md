# dodsterp.dll (cs2-trainer)

Internal-режим. Маппится в `cs2.exe` ручным маппером из `launcher.exe`,
хукает `IDXGISwapChain::Present`, рисует поверх игры ImGui-меню.

## Что в этом коммите

- Полный визуальный скелет меню — без логики, но с рабочим стилем
- Хук Present (vtable swap через MinHook) + Resize buffers
- Кастомная WndProc, отъедает ввод когда меню видимо
- Кастомная палитра, рисованный title bar + sidebar + status bar

## Хоткеи

- `INSERT` — toggle меню
- `END`    — (зарезервировано) выгрузка трейнера

## Зависимости (тянутся через FetchContent)

- [Dear ImGui v1.91.5](https://github.com/ocornut/imgui)
- [MinHook v1.3.3](https://github.com/TsudaKageyu/minhook)

## Сборка

```bash
cd cs2-trainer
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Артефакт: `build/Release/dodsterp.dll`. Кладём рядом с `launcher.exe`.

## Структура

```
src/
├── dllmain.cpp           # bootstrap thread → install_hook
└── overlay/
    ├── hook.{hpp,cpp}    # IDXGISwapChain::Present hook + WndProc
    ├── renderer.{hpp,cpp}# ImGui_ImplWin32 + ImGui_ImplDX11 init
    ├── theme.{hpp,cpp}   # палитра + ImGuiStyle
    └── menu.{hpp,cpp}    # layout: title / sidebar / content / status
```

## Что добавится потом

- `sdk/` — entity walker под Source 2 schema
- `offsets/` — JSON-loader из a2x/cs2-dumper
- `features/esp.cpp` — отрисовка боксов/healthbar/имени, world-to-screen
- `features/aim.cpp` — silent через хук CSGOInput::CreateMove (или внутренний эквивалент в Source 2)
- `features/misc.cpp` — bhop, no-flash, watermark

## Грабли

- Меню рисуется в game render thread → не блокировать, не ждать.
- При resize окна CS2 пересоздаёт swapchain → у нас отдельный hook на ResizeBuffers, пересоздаём RTV.
- При первом `Present` после маппинга `swapchain->GetDesc(&sd)` может вернуть старый HWND, если игра ещё не докрутила свой initial frame. Поэтому в bootstrap-потоке стоит `Sleep(1500)` перед `install_hook()`.
- `imgui.ini` отключён (`io.IniFilename = nullptr`) — не сорим в каталог CS2.
