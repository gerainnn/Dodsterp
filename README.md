# Dodsterp - Telegram Notification Bot

Telegram бот для отправки уведомлений.

## Описание

Проект находится в стадии активной разработки. 

**Планируемый функционал:**
- Отправка уведомлений пользователям
- Управление подписками
- Интеграция с SMS-шлюзами
- Настраиваемые шаблоны сообщений

## Технологии

- Python 3.10+
- aiogram 3.x
- aiohttp

## Установка

```bash
# Клонировать репозиторий
git clone https://github.com/gerainnn/Dodsterp.git
cd Dodsterp

# Создать виртуальное окружение
python -m venv venv
source venv/bin/activate

# Установить зависимости
pip install -r requirements.txt

# Создать .env файл
cp .env.example .env
# Добавить BOT_TOKEN в .env

# Запустить бота
python bot.py
```

## Структура проекта

```
Dodsterp/
├── bot.py           # Основной файл бота
├── config.py        # Конфигурация
├── keyboards.py     # Клавиатуры
├── sms_services.py  # SMS-сервисы (в разработке)
└── requirements.txt # Зависимости
```

## Статус

🚧 В разработке

## TODO

- [ ] Интеграция с SMS-провайдерами
- [ ] Система подписок
- [ ] База данных
- [ ] Админ-панель
- [ ] Документация API
