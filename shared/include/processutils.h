#ifndef PROCESSUTILS_H
#define PROCESSUTILS_H

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <memoryapi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

namespace ProcessUtils {

/**
 * @brief Finds the Process ID (PID) of a process by its name.
 * 
 * @param processName The name of the process (e.g., "example.exe").
 * @return The PID of the process.
 * @throws std::runtime_error if a snapshot cannot be created or the process is not found.
 */
DWORD FindPIDByName(const std::string& processName);

/**
 * @brief Finds the base address of a module within a specific process.
 * 
 * @param pid The PID of the process to search.
 * @param moduleName The name of the module (e.g., "example.dll").
 * @return The base address of the module within the process.
 * @throws std::runtime_error if a snapshot cannot be created or the module is not found.
 */
BYTE* FindModuleBaseAddr(const DWORD pid, const std::string& moduleName);

/**
 * @brief Writes to protected process memory, changing memory protections temporarily if necessary.
 * 
 * @param hProcess A handle to the target process. Must have VM_OPERATION and VM_WRITE rights.
 * @param lpBaseAddress The base address in the target process where the data should be written.
 * @param lpBuffer A pointer to the buffer containing the data to write.
 * @param nSize The number of bytes to write.
 * @param newProtect The new memory protection to apply temporarily (e.g., PAGE_EXECUTE_READWRITE).
 * @throws std::runtime_error if changing memory protections or writing to process memory fails.
 */
void WriteProtectedProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, DWORD newProtect);

} // namespace ProcessUtils

#endif // PROCESSUTILS_H
