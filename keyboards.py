"""
Keyboards Module
Модуль клавиатур для навигации
"""
from aiogram.types import InlineKeyboardMarkup, InlineKeyboardButton


def get_main_menu():
    """Главное меню бота"""
    keyboard = InlineKeyboardMarkup(
        inline_keyboard=[
            [InlineKeyboardButton(text="ℹ️ О боте", callback_data="about")],
            [InlineKeyboardButton(text="⚙️ Настройки", callback_data="settings")],
        ]
    )
    return keyboard


def get_back_button():
    """Кнопка назад в главное меню"""
    keyboard = InlineKeyboardMarkup(
        inline_keyboard=[
            [InlineKeyboardButton(text="◀️ Назад", callback_data="back_main")],
        ]
    )
    return keyboard
