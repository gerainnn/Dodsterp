"""
Kiosk lock app.

- Fullscreen, topmost, no decorations
- User cannot close window, Alt+F4 / Esc / WM_DELETE blocked
- User must type the correct password to unlock
- Secret hotkey (default: Ctrl+Shift+F12) force-closes the app

Run:
    python main.py
    python main.py --password mypass
    python main.py --password mypass --hotkey Control-Shift-F11
"""
from __future__ import annotations

import argparse
import sys
import time
import tkinter as tk
from tkinter import ttk


DEFAULT_PASSWORD = "ебать"
DEFAULT_HOTKEY = "<Control-Shift-KeyPress-F12>"
MAX_FAILS_BEFORE_LOCK = 5
LOCK_SECONDS = 5


class KioskApp:
    def __init__(self, password: str, hotkey: str) -> None:
        self.password = password
        self.hotkey = hotkey
        self.fails = 0
        self.locked_until = 0.0

        self.root = tk.Tk()
        self.root.title("LOCKED")
        self._configure_window()
        self._build_ui()
        self._bind_blockers()
        self._bind_secret_hotkey()

    # --- window setup ---------------------------------------------------
    def _configure_window(self) -> None:
        self.root.attributes("-fullscreen", True)
        self.root.attributes("-topmost", True)
        try:
            self.root.overrideredirect(True)  # no window decorations
        except tk.TclError:
            pass
        self.root.configure(bg="#0a0a0a")
        # block window close via WM
        self.root.protocol("WM_DELETE_WINDOW", lambda: None)
        # keep focus
        self.root.after(200, self._force_focus_loop)

    def _force_focus_loop(self) -> None:
        try:
            self.root.lift()
            self.root.attributes("-topmost", True)
            self.root.focus_force()
            self.entry.focus_set()
        except tk.TclError:
            return
        self.root.after(500, self._force_focus_loop)

    # --- ui -------------------------------------------------------------
    def _build_ui(self) -> None:
        screen_w = self.root.winfo_screenwidth()
        screen_h = self.root.winfo_screenheight()

        wrap = tk.Frame(self.root, bg="#0a0a0a")
        wrap.place(x=0, y=0, width=screen_w, height=screen_h)

        title = tk.Label(
            wrap,
            text="СИСТЕМА ЗАБЛОКИРОВАНА",
            fg="#ff2b2b",
            bg="#0a0a0a",
            font=("Consolas", 48, "bold"),
        )
        title.pack(pady=(screen_h // 4, 20))

        sub = tk.Label(
            wrap,
            text="Введи пароль чтобы продолжить",
            fg="#dddddd",
            bg="#0a0a0a",
            font=("Consolas", 20),
        )
        sub.pack(pady=10)

        self.entry = tk.Entry(
            wrap,
            show="*",
            font=("Consolas", 28),
            justify="center",
            width=20,
            bg="#1a1a1a",
            fg="#ffffff",
            insertbackground="#ffffff",
            relief="flat",
        )
        self.entry.pack(pady=20, ipady=8)
        self.entry.bind("<Return>", lambda _e: self._try_unlock())

        self.btn = tk.Button(
            wrap,
            text="РАЗБЛОКИРОВАТЬ",
            command=self._try_unlock,
            font=("Consolas", 16, "bold"),
            bg="#222222",
            fg="#ffffff",
            activebackground="#333333",
            activeforeground="#ffffff",
            relief="flat",
            padx=20,
            pady=10,
            cursor="hand2",
        )
        self.btn.pack(pady=10)

        self.status = tk.Label(
            wrap,
            text="",
            fg="#ff8c8c",
            bg="#0a0a0a",
            font=("Consolas", 14),
        )
        self.status.pack(pady=20)

    # --- input blockers -------------------------------------------------
    def _bind_blockers(self) -> None:
        # disable common escape paths
        for seq in (
            "<Escape>",
            "<Alt-F4>",
            "<Alt-Tab>",
            "<Control-w>",
            "<Control-q>",
            "<F11>",
        ):
            self.root.bind_all(seq, self._block)

    @staticmethod
    def _block(_event):
        return "break"

    # --- secret hotkey --------------------------------------------------
    def _bind_secret_hotkey(self) -> None:
        try:
            self.root.bind_all(self.hotkey, self._force_exit)
        except tk.TclError as e:
            print(f"[!] bad hotkey {self.hotkey!r}: {e}", file=sys.stderr)

    def _force_exit(self, _event=None):
        try:
            self.root.destroy()
        finally:
            sys.exit(0)

    # --- unlock logic ---------------------------------------------------
    def _try_unlock(self) -> None:
        now = time.time()
        if now < self.locked_until:
            left = int(self.locked_until - now) + 1
            self.status.config(text=f"Слишком много попыток. Жди {left}с")
            self.entry.delete(0, tk.END)
            return

        value = self.entry.get()
        self.entry.delete(0, tk.END)

        if value == self.password:
            self.status.config(text="OK", fg="#7CFC00")
            self.root.after(150, self._force_exit)
            return

        self.fails += 1
        if self.fails >= MAX_FAILS_BEFORE_LOCK:
            self.locked_until = now + LOCK_SECONDS
            self.fails = 0
            self.status.config(
                text=f"Заблокировано на {LOCK_SECONDS}с", fg="#ff2b2b"
            )
        else:
            left = MAX_FAILS_BEFORE_LOCK - self.fails
            self.status.config(
                text=f"Неверный пароль. Осталось попыток: {left}",
                fg="#ff8c8c",
            )

    # --- run ------------------------------------------------------------
    def run(self) -> None:
        self.root.mainloop()


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Kiosk lock app")
    p.add_argument(
        "--password",
        default=DEFAULT_PASSWORD,
        help=f"Unlock password (default: {DEFAULT_PASSWORD!r})",
    )
    p.add_argument(
        "--hotkey",
        default=DEFAULT_HOTKEY,
        help=(
            "Tkinter-style emergency hotkey, e.g. "
            "'<Control-Shift-KeyPress-F12>'"
        ),
    )
    return p.parse_args()


def main() -> None:
    args = parse_args()
    KioskApp(password=args.password, hotkey=args.hotkey).run()


if __name__ == "__main__":
    main()
