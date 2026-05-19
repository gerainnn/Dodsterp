# TODO - Задачи для доработки

> Этот файл содержит список задач для следующего разработчика.

## Критические задачи 🔴

### 1. Реализация SMS-шлюзов
**Файл:** `sms_services.py`

**Что нужно сделать:**
- Добавить реальные API endpoints для отправки SMS
- Интегрировать SMS-шлюзы (SMS.ru, SMSC.ru и др.)
- Обработать HTTP ответы от сервисов
- Добавить retry логику при временных ошибках

**Пример сервисов для исследования:**
- SMS.ru
- SMSC.ru
- Bytehand
- SmsAero

```python
# Пример реализации для SMS.ru
async def _send_via_sms_ru(self, phone: str) -> bool:
    url = "https://sms.ru/sms/send"
    params = {
        "api_id": "YOUR_API_ID",
        "to": phone,
        "msg": "Ваш код подтверждения: 1234"
    }
    async with self.session.get(url, params=params) as response:
        data = await response.json()
        return data.get("status") == "OK"
```

### 2. База данных
**Что нужно:**
- SQLite или PostgreSQL
- Хранить: user_id, баланс, лимиты, история отправок

**Структура таблиц:**
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    telegram_id BIGINT UNIQUE,
    username TEXT,
    balance INTEGER DEFAULT 0,
    is_admin BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP
);

CREATE TABLE send_sessions (
    id INTEGER PRIMARY KEY,
    user_id INTEGER,
    phone TEXT,
    sms_count INTEGER,
    status TEXT,
    created_at TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

### 3. Система авторизации
**Задачи:**
- Проверка `user_id` при каждом запросе
- Белый список пользователей
- Админы с расширенными правами

## Важные задачи 🟡

### 4. Админ-панель
**Команды:**
- `/admin` - вход в админку
- `/add_user <id>` - добавить пользователя
- `/remove_user <id>` - удалить пользователя
- `/stats` - общая статистика
- `/broadcast <message>` - рассылка всем пользователям

### 5. Улучшение UX
**Что добавить:**
- Кнопка "Стоп" во время отправки
- Красивые прогресс-бары
- Подтверждение перед стартом
- История последних 10 запросов

### 6. Безопасность
**Задачи:**
- Rate limiting (не более X запросов в минуту)
- Проверка на ботов
- Логирование всех действий
- Защита от SQL injection

## Желательные задачи 🟢

### 7. Docker
**Создать:**
- `Dockerfile`
- `docker-compose.yml`
- `.dockerignore`

```dockerfile
FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY . .
CMD ["python", "bot.py"]
```

### 8. Тесты
**Что тестировать:**
- Валидацию номеров телефонов
- FSM состояния
- Обработчики команд

### 9. Мониторинг и логирование
**Добавить:**
- Логирование в файл с ротацией
- Sentry для отлова ошибок
- Метрики Prometheus/Grafana (опционально)

### 10. Multi-service балансировка
**Идея:**
- Автоматический выбор рабочего шлюза
- Fallback на резервные
- Round-robin балансировка

## Архитектурные решения

### Почему aiogram 3.x?
- Асинхронный, подходит для высокой нагрузки
- FSM из коробки
- Удобная работа с inline кнопками
- Хорошая документация

### Почему нет БД сейчас?
- Для MVP достаточно in-memory хранения
- SQLite добавит ~50 строк кода
- Можно использовать `aiosqlite` для async

### Переменные окружения
Добавить в `.env`:
```
BOT_TOKEN=...
DATABASE_URL=sqlite:///bot.db
ADMIN_IDS=123456789,987654321
LOG_LEVEL=INFO
```

## Советы следующему разработчику

1. **Начни с малого** - сначала сделай один рабочий SMS шлюз
2. **Тестируй на себе** - используй свой номер для тестов
3. **Логи - твои друзья** - включи DEBUG уровень для разработки
4. **Читай документацию aiogram** - https://docs.aiogram.dev/
5. **Не забывай про async/await** - весь код асинхронный

## Полезные ссылки

- [aiogram документация](https://docs.aiogram.dev/en/latest/)
- [Python asyncio](https://docs.python.org/3/library/asyncio.html)
- [SMS.ru API](https://sms.ru/api)
- [Telegram Bot API](https://core.telegram.org/bots/api)

---

**Удачи! 🍀**
