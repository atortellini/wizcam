#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <memoryapi.h>
#include <iostream>

namespace ProcessUtils {

    /* Find the process ID by the process name. */
    DWORD FindPIDByName(const std::string& processName);

    /* Find the base address of a module in the given process. */
    BYTE *FindModuleBaseAddr(const DWORD pid, const std::string& moduleName);

    /* Write to a process's memory that is protected. */
    BOOL WriteProtectedProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize);
    
}