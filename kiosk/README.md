# Kiosk Lock

Полноэкранная блокировка на чистом Python (tkinter). Без зависимостей.

## Запуск

```bash
python kiosk/main.py
```

С кастомным паролем и хоткеем:

```bash
python kiosk/main.py --password secret --hotkey "<Control-Shift-KeyPress-F11>"
```

## Дефолты

- Пароль: `ебать`
- Аварийный хоткей: `Ctrl + Shift + F12`
- После 5 неверных попыток — таймаут 5 секунд

## Что блокируется

- Кнопка закрытия окна, `Alt+F4`, `Esc`, `Ctrl+W`, `Ctrl+Q`, `F11`, `Alt+Tab` (в рамках tkinter)
- Окно держится поверх всех и постоянно перехватывает фокус

## Что НЕ блокируется (ограничения юзерспейса)

- `Ctrl+Alt+Del` (Windows) — это уровень ОС
- Клавиша `Win` / системные хоткеи менеджера окон
- Переключение TTY (`Ctrl+Alt+F1..F7`) на Linux
- Принудительное завершение через `kill` из соседнего терминала

Для реального киоска нужны админские средства (групповые политики Windows,
systemd kiosk session на Linux и т.п.).

## Формат хоткея

Tkinter-стиль: `<Control-Shift-KeyPress-F12>`, `<Alt-Shift-KeyPress-Q>` и т.д.
