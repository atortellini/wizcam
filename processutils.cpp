#include "processutils.h"

namespace ProcessUtils {

    DWORD FindPIDByName(const std::string& processName) {
        HANDLE snapshot;
        if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create snapshot of all processes.\n";
            return 0;
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

        return 0;

    }

    BYTE* FindModuleBaseAddr(const DWORD pid, const std::string& moduleName) {
        HANDLE snapshot;
        if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid)) == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create snapshot of process modules.\n";
            return NULL;
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
        return NULL;
    }

    /* To use this function, open process handle must already have access rights of VM_OPERATION and VM_WRITE */
    BOOL WriteProtectedProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, DWORD newProtect) {
        DWORD OldProtect;
        if (!VirtualProtectEx(hProcess, lpBaseAddress, 1, newProtect, &OldProtect)) {
            std::cerr << "Failed to change protections on page.\n";
            return FALSE;
        }
    
        BOOL write_err = FALSE;

        if (!WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, NULL)) {
            std::cerr << "Failed to write to process memory.\n";
            write_err = TRUE;
        }

        if (!VirtualProtectEx(hProcess, lpBaseAddress, 1, OldProtect, &OldProtect)) {
            std::cerr << "Failed to change back protections on page.\n"; // Not sure how this would happen but in this scenario the memory write would have already occurred.
        }
        if (write_err) return FALSE;
        return TRUE;

    }
}