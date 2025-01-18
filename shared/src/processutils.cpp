#include "processutils.h"

#include <tlhelp32.h>
#include <psapi.h>
#include <memoryapi.h>
#include <processthreadsapi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

namespace ProcessUtils {

    DWORD FindPIDByName(const std::string& processName) {
        HANDLE snapshot;
        if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to create snapshot of all processes.");
        }

        DWORD pid;
        PROCESSENTRY32 proc;
        proc.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot, &proc)) {
            do {
                if (processName == proc.szExeFile) {
                    pid = proc.th32ProcessID;
                    CloseHandle(snapshot);
                    return pid;
                }
            } while (Process32Next(snapshot, &proc));
        }
        CloseHandle(snapshot);
        std::ostringstream oss;
        oss << "Failed to find " << processName << " in snapshot of all processes."; 
        throw std::runtime_error(oss.str());
    }

    BYTE* FindModuleBaseAddr(const DWORD pid, const std::string& moduleName) {
        HANDLE snapshot;
        if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid)) == INVALID_HANDLE_VALUE) {
            std::ostringstream oss;
            oss << "Failed to create snapshot of " << pid <<  "'s modules.";
            throw std::runtime_error(oss.str());
        }
        
        BYTE *base;
        MODULEENTRY32 mod;
        mod.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(snapshot, &mod)) {
            do {
                if (moduleName == mod.szModule) {
                    base = mod.modBaseAddr;
                    CloseHandle(snapshot);
                    return base;
                }
            } while (Module32Next(snapshot, &mod));
        }
        
        CloseHandle(snapshot);
        std::ostringstream oss;
        oss << "Failed to find " << moduleName << " in snapshot of all modules mapped within process " << pid << ".";
        throw std::runtime_error(oss.str());
    }

    /* To use this function, open process handle must already have access rights of VM_OPERATION and VM_WRITE */
    void WriteProtectedProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, DWORD newProtect) {
        DWORD OldProtect;
        if (!VirtualProtectEx(hProcess, lpBaseAddress, nSize, newProtect, &OldProtect)) {
            std::ostringstream oss;
            oss << "Failed to change protections on the memory page(s) containing addresses " << std::hex << reinterpret_cast<uintptr_t>(lpBaseAddress) << "-" << (reinterpret_cast<uintptr_t>(lpBaseAddress) + nSize);
            throw std::runtime_error(oss.str());
        }

        if (!WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, NULL)) {
            std::ostringstream oss;
            oss << "Failed to write " << nSize << " bytes to process memory at address " << std::hex << reinterpret_cast<uintptr_t>(lpBaseAddress);
            throw std::runtime_error(oss.str());
        }

        if (!VirtualProtectEx(hProcess, lpBaseAddress, nSize, OldProtect, &OldProtect)) {
            std::ostringstream oss;
            oss << "Failed to change back protections on the memory page(s) containing addresses " << std::hex << reinterpret_cast<uintptr_t>(lpBaseAddress) << "-" << (reinterpret_cast<uintptr_t>(lpBaseAddress) + nSize);
            throw std::runtime_error(oss.str()); // Not sure how this would happen but in this scenario the memory write would have already occurred.
        }
    }

    void HaltAllProcessThreads(const DWORD pid) {
        HANDLE snapshot;
        if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)) == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to create snapshot of all threads.");
        }

        THREADENTRY32 thr;
        thr.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(snapshot, &thr)) {
            do {
                if (thr.th32OwnerProcessID == pid) {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, thr.th32ThreadID);
                    if (hThread) {
                        SuspendThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(snapshot, &thr));
        }

        CloseHandle(snapshot);
    }

    void ResumseAllProcessThreads(const DWORD pid) {
        HANDLE snapshot;
        if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)) == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to create snapshot of all threads.");
        }

        THREADENTRY32 thr;
        thr.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(snapshot, &thr)) {
            do {
                if (thr.th32OwnerProcessID == pid) {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, thr.th32ThreadID);
                    if (hThread) {
                        ResumeThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(snapshot, &thr));
        }

        CloseHandle(snapshot);
    }
}