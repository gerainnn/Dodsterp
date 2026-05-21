"""
Fullscreen lock window. Закрывается только паролем.
Запуск: python lock.py
"""

import tkinter as tk
from tkinter import font as tkfont
import time
import sys

PASSWORD = "1"
TITLE_TEXT = "СИСТЕМА ЗАБЛОКИРОВАНА"
HINT_TEXT = "Введите пароль для разблокировки"

BG = "#0a0a0a"
FG = "#e8e8e8"
ACCENT = "#ff3344"
DIM = "#555555"


class LockScreen:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("")
        self.root.configure(bg=BG)
        self.attempts = 0
        self.shake_state = 0

        # фуллскрин + поверх всего + без декораций
        self.root.attributes("-fullscreen", True)
        self.root.attributes("-topmost", True)
        try:
            self.root.overrideredirect(True)
        except tk.TclError:
            pass

        # блокируем стандартные способы закрытия
        self.root.protocol("WM_DELETE_WINDOW", self._noop)
        for seq in ("<Escape>", "<Alt-F4>", "<Control-w>",
                    "<Control-q>", "<F11>", "<Super_L>", "<Super_R>"):
            self.root.bind_all(seq, self._block)

        self._build_ui()
        self._tick_clock()
        self._focus_loop()

    def _noop(self):
        pass

    def _block(self, _event):
        return "break"

    def _build_ui(self):
        sw = self.root.winfo_screenwidth()
        sh = self.root.winfo_screenheight()

        # центральный контейнер
        self.frame = tk.Frame(self.root, bg=BG)
        self.frame.place(relx=0.5, rely=0.5, anchor="center")

        title_font = tkfont.Font(family="Consolas", size=42, weight="bold")
        hint_font = tkfont.Font(family="Consolas", size=14)
        entry_font = tkfont.Font(family="Consolas", size=28)
        status_font = tkfont.Font(family="Consolas", size=12)
        clock_font = tkfont.Font(family="Consolas", size=18)

        self.title_lbl = tk.Label(
            self.frame, text=TITLE_TEXT,
            font=title_font, fg=ACCENT, bg=BG
        )
        self.title_lbl.pack(pady=(0, 8))

        self.hint_lbl = tk.Label(
            self.frame, text=HINT_TEXT,
            font=hint_font, fg=DIM, bg=BG
        )
        self.hint_lbl.pack(pady=(0, 30))

        # рамка вокруг поля ввода
        self.entry_frame = tk.Frame(self.frame, bg=ACCENT, padx=2, pady=2)
        self.entry_frame.pack()

        self.entry = tk.Entry(
            self.entry_frame, show="*", font=entry_font,
            justify="center", bg=BG, fg=FG,
            insertbackground=FG, bd=0, width=20
        )
        self.entry.pack(ipady=10, ipadx=10)
        self.entry.bind("<Return>", self._check)

        self.status_lbl = tk.Label(
            self.frame, text="", font=status_font,
            fg=DIM, bg=BG
        )
        self.status_lbl.pack(pady=(20, 0))

        # часы в углу
        self.clock_lbl = tk.Label(
            self.root, text="", font=clock_font,
            fg=DIM, bg=BG
        )
        self.clock_lbl.place(x=sw - 30, y=20, anchor="ne")

        # подпись внизу
        bottom_lbl = tk.Label(
            self.root, text="ACCESS DENIED // AUTH REQUIRED",
            font=("Consolas", 10), fg=DIM, bg=BG
        )
        bottom_lbl.place(relx=0.5, y=sh - 30, anchor="center")

        self.entry.focus_force()

    def _tick_clock(self):
        self.clock_lbl.config(text=time.strftime("%H:%M:%S  //  %d.%m.%Y"))
        self.root.after(1000, self._tick_clock)

    def _focus_loop(self):
        # принудительно держим фокус и поверх всего
        try:
            self.root.attributes("-topmost", True)
            self.entry.focus_force()
        except tk.TclError:
            pass
        self.root.after(500, self._focus_loop)

    def _check(self, _event=None):
        value = self.entry.get()
        if value == PASSWORD:
            self.root.destroy()
            sys.exit(0)
        self.attempts += 1
        self.entry.delete(0, tk.END)
        self.status_lbl.config(
            text=f"НЕВЕРНО. Попыток: {self.attempts}", fg=ACCENT
        )
        self._shake()

    def _shake(self, step=0):
        # тряска формы при ошибке
        offsets = [12, -12, 10, -10, 6, -6, 0]
        if step >= len(offsets):
            return
        x = offsets[step]
        self.frame.place_configure(relx=0.5, rely=0.5, anchor="center", x=x)
        self.root.after(40, lambda: self._shake(step + 1))

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    LockScreen().run()
