#include <tl32help.h>
#include <psapi.h>
#include <memoryapi.h>
#include <iostream>


#define RDATA_OFF 0x271b000
#define CAM_MAX_OZOOM_OFF0
#define CAM_MAX_OZOOM_OFF1
#define NEW_CAM_MAX_ZOOM_LIMIT 8000f;

static DWORD FindPIDByName(const std::wstring& processName) {
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

static BYTE* FindModuleBaseAddr(const DWORD pid, const std::wstring& moduleName) {
    HANDLE snapshot;
    if ((CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid)) == INVALID_HANDLE_VALUE) {
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
static BOOL WriteProtectedProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize) {
    DWORD OldProtect;
    if (!VirtualProtectEx(hProcess, lpBaseAddress, 1, PAGE_READWRITE, &OldProtect)) {
        std::cerr << "Failed to change protections on page.\n";
        return FALSE;
    }
    SIZE_T TotalBytesWritten = 0;
    SIZE_T NumberOfBytesWritten = 0;
    BOOL write_err = FALSE:

    while (TotalBytesWritten < nSize) {
        if(!WriteProcessMemory(hProcess, lpBaseAddress + TotalBytesWritten, lpBuffer + TotalBytesWritten, nSize - TotalBytesWritten, &NumberOfBytesWritten)) {
            std::cerr << "Failed to write to process memory.\n";
            write_err = TRUE;
            break;
        }
        TotalBytesWritten += NumberOfBytesWritten;
    }

    if (!VirtualProtectEx(hProcess, lpBaseAddress, 1, OldProtect, &OldProtect)) {
        std::cerr << "Failed to change back protections on page.\n"; // Not sure how this would happen but in this scenario the memory write would have already occurred.
    }
    if (write_err) return FALSE;
    return TRUE;

}


int main(void) {
    std::wstring procName = L"WizardGraphicalClient.exe";
    std::wstring modName = L"WizardGraphicalClient.exe";

    DWORD pid = FindPIDByName(procName);
    if (!pid) {
        std::cerr << "Process couldn't be found.\n";
        return 1;
    }

    BYTE *baseaddr = FindModuleBaseAddr(pid, modName);
    if (!baseaddr) {
        std::cerr << "Couldn't find module in process address space.\n";
        return 1;
    }

    BYTE *rdata_base = baseaddr + RDATA_OFF;
    BYTE *max_zoom_out0 = rdata_base + CAM_MAX_OZOOM_OFF0;
    BYTE *max_zoom_out1 = rdata_base + CAM_MAX_OZOOM_OFF1;

    FLOAT new_max = NEW_CAM_MAX_ZOOM_LIMIT;

    HANDLE hProcess;
    if (!(hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid))) {
        std::cerr << "Couldn't open handle to process.\n";
        return 1;
    }
    if (!WriteProtectedProcessMemory(hProcess, max_zoom_out0, &new_max, sizeof(FLOAT))) {
        CloseHandle(hProcess);
        return 1;
    }
    if (!WriteProtectedProcessMemory(hProcess, max_zoom_out1, &new_max, sizeof(FLOAT))) {
        CloseHandle(hProcess);
        return 1;
    }

    CloseHandle(hProcess);
    return 0;
    
}
