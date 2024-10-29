#pragma once
#include <Windows.h>

DWORD GetProcessIdByName(const wchar_t* processName);
HMODULE GetRemoteModuleHandle(DWORD pid, const wchar_t* moduleName);
