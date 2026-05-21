#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace inject {

// Ручной маппинг DLL в чужой процесс. Минует системный загрузчик —
// нет регистрации в PEB->Ldr, нет уведомлений LdrRegisterDllNotification.
//
// pid       — PID целевого процесса (cs2.exe)
// dll_path  — абсолютный путь к dodsterp.dll
//
// На успех — true. Деталь ошибки кладётся в last_error().
bool manual_map(DWORD pid, const std::wstring& dll_path);

const char* last_error();

} // namespace inject
