"""
Telegram Bot Skeleton
Основной файл бота
"""
import asyncio
import logging
from aiogram import Bot, Dispatcher, types, F
from aiogram.filters import Command
from aiogram.fsm.context import FSMContext
from aiogram.fsm.state import State, StatesGroup
from aiogram.fsm.storage.memory import MemoryStorage

from config import BOT_TOKEN
from keyboards import get_main_menu, get_back_button

# Настройка логирования
logging.basicConfig(level=logging.INFO)

# Инициализация бота
bot = Bot(token=BOT_TOKEN)
dp = Dispatcher(storage=MemoryStorage())


class FormStates(StatesGroup):
    """Состояния формы"""
    waiting_for_input = State()


@dp.message(Command("start"))
async def cmd_start(message: types.Message):
    """Обработчик команды /start"""
    await message.answer(
        "👋 Добро пожаловать!\n\n"
        "Бот находится в разработке.\n"
        "Функционал будет добавлен позже.",
        reply_markup=get_main_menu()
    )


@dp.callback_query(F.data == "back_main")
async def back_to_main(callback: types.CallbackQuery, state: FSMContext):
    """Возврат в главное меню"""
    await state.clear()
    await callback.message.edit_text(
        "👋 Главное меню\n\nВыберите действие:",
        reply_markup=get_main_menu()
    )
    await callback.answer()


@dp.callback_query(F.data == "about")
async def show_about(callback: types.CallbackQuery):
    """Информация о боте"""
    text = (
        "ℹ️ <b>О боте</b>\n\n"
        "Данный бот разрабатывается для отправки уведомлений.\n\n"
        "<b>Планируемый функционал:</b>\n"
        "• Отправка уведомлений\n"
        "• Управление подписками\n"
        "• Настройка шаблонов\n\n"
        "⚠️ Бот находится на стадии разработки."
    )
    await callback.message.edit_text(text, parse_mode="HTML", reply_markup=get_back_button())
    await callback.answer()


@dp.callback_query(F.data == "settings")
async def show_settings(callback: types.CallbackQuery):
    """Показать настройки"""
    text = (
        "⚙️ <b>Настройки</b>\n\n"
        "Раздел в разработке.\n\n"
        "Здесь будут доступны:\n"
        "• Язык интерфейса\n"
        "• Уведомления\n"
        "• Другие параметры"
    )
    await callback.message.edit_text(text, parse_mode="HTML", reply_markup=get_back_button())
    await callback.answer()


async def main():
    """Запуск бота"""
    print("🤖 Бот запускается...")
    await dp.start_polling(bot)


if __name__ == "__main__":
    asyncio.run(main())
