import asyncio
import logging
from aiogram import Bot, Dispatcher, types, F
from aiogram.filters import Command, StateFilter
from aiogram.fsm.context import FSMContext
from aiogram.fsm.state import State, StatesGroup
from aiogram.fsm.storage.memory import MemoryStorage

from config import BOT_TOKEN
from keyboards import get_main_menu, get_speed_menu, get_back_button
from sms_services import SMSService

# Настройка логирования
logging.basicConfig(level=logging.INFO)

# Инициализация бота
bot = Bot(token=BOT_TOKEN)
dp = Dispatcher(storage=MemoryStorage())

# Сервис отправки SMS
sms_service = SMSService()

# Состояния FSM
class SendStates(StatesGroup):
    waiting_for_phone = State()
    waiting_for_speed = State()
    sending = State()

# Словарь для хранения настроек скорости
SPEED_SETTINGS = {
    "slow": 1,      # 1 смс в секунду
    "medium": 3,    # 3 смс в секунду
    "fast": 5,      # 5 смс в секунду
    "ultra": 10,    # 10 смс в секунду
}

# Хранилище данных пользователя
user_data = {}

@dp.message(Command("start"))
async def cmd_start(message: types.Message):
    """Обработчик команды /start"""
    await message.answer(
        "👋 Добро пожаловать в SMS Bot!\n\n"
        "Этот бот предназначен для отправки SMS на номера РФ (+7)\n\n"
        "⚠️ Выберите действие в меню:",
        reply_markup=get_main_menu()
    )

@dp.callback_query(F.data == "back_main")
async def back_to_main(callback: types.CallbackQuery, state: FSMContext):
    """Возврат в главное меню"""
    await state.clear()
    await callback.message.edit_text(
        "👋 Главное меню",
        reply_markup=get_main_menu()
    )
    await callback.answer()

@dp.callback_query(F.data == "instruction")
async def show_instruction(callback: types.CallbackQuery):
    """Показать инструкцию"""
    text = (
        "📖 <b>Инструкция по использованию</b>\n\n"
        "1️⃣ Нажмите «Начать отправку» в главном меню\n"
        "2️⃣ Введите номер телефона в формате:\n"
        "   • +7XXXXXXXXXX\n"
        "   • 8XXXXXXXXXX\n"
        "   • 7XXXXXXXXXX\n"
        "   • 9XXXXXXXXX (без кода страны)\n\n"
        "3️⃣ Выберите скорость отправки\n"
        "4️⃣ Дождитесь завершения\n\n"
        "⚠️ <b>Поддерживаются только номера РФ (+7)</b>"
    )
    await callback.message.edit_text(text, parse_mode="HTML", reply_markup=get_back_button())
    await callback.answer()

@dp.callback_query(F.data == "settings")
async def show_settings(callback: types.CallbackQuery):
    """Показать настройки"""
    user_id = callback.from_user.id
    current_speed = user_data.get(user_id, {}).get("speed", "medium")
    speed_names = {"slow": "🐢 Медленно", "medium": "🚗 Средне", "fast": "🚀 Быстро", "ultra": "⚡ Максимально"}
    
    text = (
        "⚙️ <b>Настройки</b>\n\n"
        f"Текущая скорость: {speed_names.get(current_speed, 'Не установлена')}\n\n"
        "Выберите скорость отправки:"
    )
    await callback.message.edit_text(text, parse_mode="HTML", reply_markup=get_speed_menu())
    await callback.answer()

@dp.callback_query(F.data.startswith("speed_"))
async def set_speed(callback: types.CallbackQuery, state: FSMContext):
    """Установка скорости"""
    user_id = callback.from_user.id
    speed = callback.data.split("_")[1]
    
    if user_id not in user_data:
        user_data[user_id] = {}
    user_data[user_id]["speed"] = speed
    
    speed_names = {"slow": "🐢 Медленно", "medium": "🚗 Средне", "fast": "🚀 Быстро", "ultra": "⚡ Максимально"}
    
    await callback.answer(f"Скорость установлена: {speed_names.get(speed)}")
    await callback.message.edit_text(
        f"✅ Скорость изменена на: {speed_names.get(speed)}\n\n"
        "Вы в главном меню:",
        reply_markup=get_main_menu()
    )

@dp.callback_query(F.data == "start_send")
async def start_send_process(callback: types.CallbackQuery, state: FSMContext):
    """Начало процесса отправки - запрос номера"""
    await state.set_state(SendStates.waiting_for_phone)
    await callback.message.edit_text(
        "📱 <b>Введите номер телефона</b>\n\n"
        "Поддерживаемые форматы:\n"
        "• +7XXXXXXXXXX\n"
        "• 8XXXXXXXXXX\n"
        "• 7XXXXXXXXXX\n"
        "• 9XXXXXXXXX\n\n"
        "⚠️ Только номера РФ (+7)",
        parse_mode="HTML",
        reply_markup=get_back_button()
    )
    await callback.answer()

@dp.message(SendStates.waiting_for_phone)
async def process_phone(message: types.Message, state: FSMContext):
    """Обработка введенного номера телефона"""
    phone = message.text.strip()
    
    # Валидация номера
    is_valid, processed_phone = SMSService.validate_phone(phone)
    
    if not is_valid:
        await message.answer(
            f"❌ {processed_phone}\n\n"
            "Попробуйте еще раз:",
            reply_markup=get_back_button()
        )
        return
    
    # Сохраняем номер
    await state.update_data(phone=processed_phone)
    
    # Переходим к выбору скорости
    await state.set_state(SendStates.waiting_for_speed)
    
    await message.answer(
        f"✅ Номер принят: <code>{processed_phone}</code>\n\n"
        "Теперь выберите скорость отправки:",
        parse_mode="HTML",
        reply_markup=get_speed_menu()
    )

@dp.callback_query(SendStates.waiting_for_speed, F.data.startswith("speed_"))
async def start_sending(callback: types.CallbackQuery, state: FSMContext):
    """Запуск отправки"""
    user_id = callback.from_user.id
    speed = callback.data.split("_")[1]
    
    data = await state.get_data()
    phone = data.get("phone")
    
    if not phone:
        await callback.answer("❌ Ошибка: номер не найден")
        return
    
    await state.set_state(SendStates.sending)
    
    speed_names = {"slow": "🐢 Медленно", "medium": "🚗 Средне", "fast": "🚀 Быстро", "ultra": "⚡ Максимально"}
    sms_per_sec = SPEED_SETTINGS.get(speed, 3)
    
    # Запускаем отправку
    await callback.message.edit_text(
        f"🚀 <b>Отправка запущена!</b>\n\n"
        f"📱 Номер: <code>{phone}</code>\n"
        f"⚡ Скорость: {speed_names.get(speed)} ({sms_per_sec} смс/сек)\n"
        f"📊 Отправлено: 0\n\n"
        "⏳ Пожалуйста, подождите...",
        parse_mode="HTML"
    )
    
    # Запускаем асинхронную отправку
    asyncio.create_task(send_task(callback.message, phone, sms_per_sec, state))

async def send_task(message: types.Message, phone: str, sms_per_sec: int, state: FSMContext):
    """Задача отправки SMS"""
    total_sent = 0
    total_failed = 0
    delay = 1.0 / sms_per_sec
    
    # Максимальное количество SMS (можно изменить)
    max_sms = 100
    
    for i in range(max_sms):
        # Проверяем, не отменена ли задача
        current_state = await state.get_state()
        if current_state != SendStates.sending:
            break
        
        try:
            result = await sms_service.send_sms(phone)
            total_sent += result["sent"]
            total_failed += result["failed"]
        except Exception as e:
            total_failed += 1
        
        # Обновляем сообщение каждые 10 отправок
        if (i + 1) % 10 == 0:
            try:
                await message.edit_text(
                    f"🚀 <b>Отправка в процессе...</b>\n\n"
                    f"📱 Номер: <code>{phone}</code>\n"
                    f"⚡ Скорость: {sms_per_sec} смс/сек\n"
                    f"📊 Отправлено: {total_sent}\n"
                    f"❌ Ошибок: {total_failed}\n"
                    f"📈 Прогресс: {i + 1}/{max_sms}",
                    parse_mode="HTML"
                )
            except:
                pass
        
        await asyncio.sleep(delay)
    
    # Финальное сообщение
    await state.clear()
    await message.edit_text(
        f"✅ <b>Отправка завершена!</b>\n\n"
        f"📱 Номер: <code>{phone}</code>\n"
        f"📊 Всего отправлено: {total_sent}\n"
        f"❌ Ошибок: {total_failed}\n\n"
        "Вернуться в меню:",
        parse_mode="HTML",
        reply_markup=get_main_menu()
    )

@dp.callback_query(F.data == "stop_send")
async def stop_send(callback: types.CallbackQuery, state: FSMContext):
    """Остановка отправки"""
    await state.clear()
    await callback.message.edit_text(
        "⏹ <b>Отправка остановлена</b>\n\n"
        "Вернуться в меню:",
        parse_mode="HTML",
        reply_markup=get_main_menu()
    )
    await callback.answer("Отправка остановлена")

async def main():
    """Запуск бота"""
    print("🤖 Бот запускается...")
    await dp.start_polling(bot)

if __name__ == "__main__":
    asyncio.run(main())
