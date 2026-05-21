# launcher.exe (Dodsterp)

Лаунчер CS2 + ручной маппер `dodsterp.dll`.

## Что делает

1. Ищет `cs2.exe` в process list. Если нет — `ShellExecute(steam://rungameid/730)`,
   ждёт появления процесса (таймаут 240 сек).
2. Дожидается подгрузки `client.dll` в cs2.exe (таймаут 60 сек).
3. Спит 5 сек на стабилизацию рендера.
4. Manual map'ит `dodsterp.dll` (рядом с launcher.exe) в cs2.exe.
5. Затирает PE-header у замапленного образа (анти-сигскан).

## Manual map vs LoadLibraryA — почему вручную

- LoadLibrary регистрирует модуль в `PEB->Ldr` и стреляет
  `LdrRegisterDllNotification` callback'ами. CS2 такие коллбеки слушает
  и кикает неизвестные DLL (либо просто ловит модуль для последующего бан-флага).
- Manual map работает мимо системного загрузчика:
  `VirtualAllocEx` + копирование секций + relocations + imports + TLS + DllMain.
  В PEB-список не попадает.

## Что Loader делает в target

```
relocations  →  imports  →  TLS callbacks  →  DllMain(DLL_PROCESS_ATTACH)
```

Loader полностью PIC, никаких внешних вызовов кроме `LoadLibraryA` / `GetProcAddress`,
указатели на которые передаются через `LoaderData`.

## Сборка

```bash
cd launcher
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Артефакт: `build/Release/launcher.exe`. Положить рядом с `dodsterp.dll`.

## Использование

```
dist/
├── launcher.exe   # ← запустить это
└── dodsterp.dll
```

Если CS2 уже запущен — лаунчер просто маппит DLL.
Если не запущен — поднимает через Steam и потом маппит.

## Грабли

- **Права.** `OpenProcess(PROCESS_ALL_ACCESS)` работает без админа для процессов
  той же сессии и того же integrity level. CS2 запускается как обычный
  пользователь, так что админ не нужен. Если же запущен из админ-стима —
  лаунчер тоже надо под админом.
- **/OPT:NOICF /OPT:NOREF** в линкере — обязательно. Без них компилятор
  может пере-уложить `Loader` и `LoaderEnd`, и `(BYTE*)LoaderEnd - (BYTE*)Loader`
  даст мусор, в худшем случае — отрицательное число.
- **SEH unwind в DLL.** Не регистрируем `RtlAddFunctionTable` для замапленного
  образа. Если в `dodsterp.dll` вылетит исключение и пойдёт раскрутка — крашнет.
  В коде DLL не throw'аем и собираем с `/EHsc` без структурных исключений.
- **Loader return codes (если что-то пошло не так):**
  - `0xDEAD0001` — нулевая база (внутренняя ошибка стаба)
  - `0xDEAD0002` — `LoadLibraryA` для одного из импортов вернул NULL
  - `0xDEAD0003` — `GetProcAddress` для импорта вернул NULL
