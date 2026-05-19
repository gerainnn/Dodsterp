from aiogram.types import InlineKeyboardMarkup, InlineKeyboardButton

def get_main_menu():
    """Главное меню бота"""
    keyboard = InlineKeyboardMarkup(
        inline_keyboard=[
            [InlineKeyboardButton(text="🚀 Начать спам", callback_data="start_spam")],
            [InlineKeyboardButton(text="ℹ️ Инструкция", callback_data="instruction")],
            [InlineKeyboardButton(text="⚙️ Настройки", callback_data="settings")],
        ]
    )
    return keyboard

def get_speed_menu():
    """Меню выбора скорости"""
    keyboard = InlineKeyboardMarkup(
        inline_keyboard=[
            [InlineKeyboardButton(text="🐢 Медленно (1 смс/сек)", callback_data="speed_slow")],
            [InlineKeyboardButton(text="🚗 Средне (3 смс/сек)", callback_data="speed_medium")],
            [InlineKeyboardButton(text="🚀 Быстро (5 смс/сек)", callback_data="speed_fast")],
            [InlineKeyboardButton(text="⚡ Максимально (10 смс/сек)", callback_data="speed_ultra")],
            [InlineKeyboardButton(text="◀️ Назад", callback_data="back_main")],
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
