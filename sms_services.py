"""
SMS Services Module - Интеграция с SMS-шлюзами

Этот модуль отвечает за отправку SMS через различные сервисы.
Сейчас содержит заглушки - их нужно заменить на реальные API!

ВАЖНО: Реализуйте методы _send_via_X для реальных сервисов
"""
import aiohttp
import random
from typing import List, Optional

class SMSService:
    """
    Класс для отправки SMS через различные сервисы
    
    Идея: несколько сервисов для отказоустойчивости
    Если один не работает - пробуем следующий
    
    TODO для следующего разработчика:
    1. Добавьте реальные API endpoints
    2. Обработайте ответы от сервисов
    3. Добавьте API ключи в config.py и .env
    """
    
    def __init__(self):
        # Список функций-отправщиков
        # Добавьте сюда свои сервисы
        self.services = [
            self._send_via_1,
            self._send_via_2,
            self._send_via_3,
            # self._send_via_smsru,      # Раскомментируйте при реализации
            # self._send_via_smsc,       # Раскомментируйте при реализации
        ]
        self.session: Optional[aiohttp.ClientSession] = None
    
    async def init_session(self):
        """Инициализация HTTP сессии (вызывается автоматически)"""
        if self.session is None or self.session.closed:
            self.session = aiohttp.ClientSession()
    
    async def close_session(self):
        """Закрытие сессии при завершении работы бота"""
        if self.session and not self.session.closed:
            await self.session.close()
    
    # ==========================================
    # МЕТОДЫ ДЛЯ РЕАЛИЗАЦИИ (сейчас заглушки)
    # ==========================================
    
    async def _send_via_1(self, phone: str) -> bool:
        """
        Сервис 1 - ЗАГЛУШКА, ТРЕБУЕТ РЕАЛИЗАЦИИ
        
        TODO: Заменить на реальный API
        
        Пример для SMS.ru:
            url = "https://sms.ru/sms/send"
            params = {
                "api_id": API_KEY,
                "to": phone,
                "msg": "Ваш код: {random_code}"
            }
            async with self.session.get(url, params=params) as resp:
                data = await resp.json()
                return data["status"] == "OK"
        """
        try:
            # Сейчас просто имитируем успех
            # В реальности здесь должен быть HTTP запрос
            return True
        except Exception as e:
            print(f"Service 1 error: {e}")
            return False
    
    async def _send_via_2(self, phone: str) -> bool:
        """Сервис 2 - ЗАГЛУШКА"""
        try:
            return True
        except Exception as e:
            print(f"Service 2 error: {e}")
            return False
    
    async def _send_via_3(self, phone: str) -> bool:
        """Сервис 3 - ЗАГЛУШКА"""
        try:
            return True
        except Exception as e:
            print(f"Service 3 error: {e}")
            return False
    
    # ==========================================
    # ОСНОВНЫЕ МЕТОДЫ
    # ==========================================
    
    async def send_sms(self, phone: str) -> dict:
        """
        Отправка SMS через все доступные сервисы
        
        Args:
            phone: Номер в формате +7XXXXXXXXXX
            
        Returns:
            dict с результатами:
                - phone: номер
                - sent: количество успешных отправок
                - failed: количество неудач
                - errors: список ошибок
        """
        await self.init_session()
        
        results = {
            "phone": phone,
            "sent": 0,
            "failed": 0,
            "errors": []
        }
        
        # Пробуем каждый сервис
        # TODO: можно добавить логику - прерывать после первого успеха
        for service in self.services:
            try:
                success = await service(phone)
                if success:
                    results["sent"] += 1
                else:
                    results["failed"] += 1
            except Exception as e:
                results["failed"] += 1
                results["errors"].append(str(e))
        
        return results
    
    @staticmethod
    def validate_phone(phone: str) -> tuple[bool, str]:
        """
        Валидация номера телефона РФ (+7)
        
        Поддерживаемые форматы:
            - +7XXXXXXXXXX (11 цифр с +)
            - 8XXXXXXXXXX (11 цифр, начинается с 8)
            - 7XXXXXXXXXX (11 цифр, начинается с 7)
            - 9XXXXXXXXX (10 цифр, начинается с 9)
        
        Args:
            phone: Строка с номером телефона
            
        Returns:
            tuple: (is_valid: bool, formatted_phone_or_error: str)
            
        Examples:
            >>> validate_phone("+79991234567")
            (True, "+79991234567")
            >>> validate_phone("89991234567")
            (True, "+79991234567")
            >>> validate_phone("123")
            (False, "Неверный формат номера...")
        """
        # Убираем все кроме цифр
        digits = ''.join(filter(str.isdigit, phone))
        
        # Проверяем разные форматы
        if digits.startswith('8') and len(digits) == 11:
            # 8XXXXXXXXXX -> +7XXXXXXXXXX
            return True, f"+7{digits[1:]}"
        elif digits.startswith('7') and len(digits) == 11:
            # 7XXXXXXXXXX -> +7XXXXXXXXXX
            return True, f"+7{digits}"
        elif len(digits) == 10 and digits.startswith('9'):
            # 9XXXXXXXXX -> +79XXXXXXXXX
            return True, f"+7{digits}"
        elif phone.startswith('+7') and len(digits) == 11:
            # Уже в правильном формате
            return True, phone
        
        return False, "Неверный формат номера. Используйте номера РФ (+7)"


# ==========================================
# ПРИМЕРЫ РЕАЛИЗАЦИИ ДЛЯ РЕАЛЬНЫХ СЕРВИСОВ
# ==========================================

"""
# SMS.ru - https://sms.ru/api
async def _send_via_smsru(self, phone: str) -> bool:
    url = "https://sms.ru/sms/send"
    params = {
        "api_id": settings.SMSRU_API_KEY,
        "to": phone,
        "msg": f"Код подтверждения: {random.randint(1000, 9999)}",
        "json": 1
    }
    try:
        async with self.session.get(url, params=params) as resp:
            data = await resp.json()
            return data.get("status") == "OK"
    except:
        return False

# SMSC.ru - https://smsc.ru/api/
async def _send_via_smsc(self, phone: str) -> bool:
    from urllib.parse import urlencode
    params = {
        "login": settings.SMSC_LOGIN,
        "psw": settings.SMSC_PASSWORD,
        "phones": phone,
        "mes": f"Код: {random.randint(1000, 9999)}"
    }
    url = f"https://smsc.ru/sys/send.php?{urlencode(params)}"
    try:
        async with self.session.get(url) as resp:
            text = await resp.text()
            return "ERROR" not in text
    except:
        return False
"""
