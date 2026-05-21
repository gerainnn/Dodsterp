"""
Блокировка экрана. Пароль: 123
Секретная комбинация: Ctrl+Shift+Q (закрывает без пароля)
"""

import tkinter as tk
import sys


PASSWORD = "123"


class ScreenLock:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("")
        self.root.attributes("-fullscreen", True)
        self.root.attributes("-topmost", True)
        self.root.configure(bg="#1a1a1a")

        # Блокируем закрытие
        self.root.protocol("WM_DELETE_WINDOW", lambda: None)
        self.root.bind("<Alt-F4>", lambda e: "break")
        self.root.bind("<Escape>", lambda e: "break")

        # Секретная комбинация Ctrl+Shift+Q
        self.root.bind("<Control-Shift-Q>", self.unlock)
        self.root.bind("<Control-Shift-q>", self.unlock)

        self.build_ui()

    def build_ui(self):
        frame = tk.Frame(self.root, bg="#1a1a1a")
        frame.place(relx=0.5, rely=0.5, anchor="center")

        lbl = tk.Label(
            frame,
            text="🔒 Экран заблокирован",
            font=("Arial", 28, "bold"),
            fg="white",
            bg="#1a1a1a",
        )
        lbl.pack(pady=(0, 30))

        self.entry = tk.Entry(
            frame,
            font=("Arial", 18),
            show="*",
            width=20,
            justify="center",
        )
        self.entry.pack(pady=10)
        self.entry.focus_set()

        btn = tk.Button(
            frame,
            text="Разблокировать",
            font=("Arial", 14),
            command=self.check_password,
            bg="#333333",
            fg="white",
            activebackground="#555555",
            activeforeground="white",
            relief="flat",
            padx=20,
            pady=5,
        )
        btn.pack(pady=15)

        self.msg = tk.Label(
            frame,
            text="",
            font=("Arial", 12),
            fg="red",
            bg="#1a1a1a",
        )
        self.msg.pack()

        # Enter тоже проверяет пароль
        self.entry.bind("<Return>", lambda e: self.check_password())

    def check_password(self):
        if self.entry.get() == PASSWORD:
            self.unlock()
        else:
            self.msg.config(text="Неверный пароль")
            self.entry.delete(0, tk.END)

    def unlock(self, event=None):
        self.root.destroy()
        sys.exit(0)

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    app = ScreenLock()
    app.run()
