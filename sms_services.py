"""
SMS Services Module
Модуль для работы с SMS-шлюзами

TODO: Добавить интеграцию с SMS-провайдерами
"""
from typing import Optional
import aiohttp


class SMSService:
    """
    Класс для работы с SMS-шлюзами
    
    Планируется интеграция с провайдерами:
    - SMS.ru
    - SMSC.ru
    - И другие
    """
    
    def __init__(self):
        self.session: Optional[aiohttp.ClientSession] = None
    
    async def init_session(self):
        """Инициализация HTTP-сессии"""
        if self.session is None or self.session.closed:
            self.session = aiohttp.ClientSession()
    
    async def close_session(self):
        """Закрытие сессии"""
        if self.session and not self.session.closed:
            await self.session.close()
    
    @staticmethod
    def validate_phone(phone: str) -> tuple[bool, str]:
        """
        Валидация номера телефона РФ
        
        Args:
            phone: Номер телефона
            
        Returns:
            tuple: (is_valid, formatted_phone_or_error_message)
        """
        digits = ''.join(filter(str.isdigit, phone))
        
        if digits.startswith('8') and len(digits) == 11:
            return True, f"+7{digits[1:]}"
        elif digits.startswith('7') and len(digits) == 11:
            return True, f"+7{digits}"
        elif len(digits) == 10 and digits.startswith('9'):
            return True, f"+7{digits}"
        elif phone.startswith('+7') and len(digits) == 11:
            return True, phone
        
        return False, "Неверный формат номера"
