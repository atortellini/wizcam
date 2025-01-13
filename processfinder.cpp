#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <memoryapi.h>
#include <iostream>
#include "processutils.h"


#define RDATA_OFF 0x271b000
#define CAM_MAX_OZOOM_OFF0 0x4849a4
#define CAM_MAX_OZOOM_OFF1 0x485dd0

int main(void) {

    std::string procName = "WizardGraphicalClient.exe";
    std::string modName = "WizardGraphicalClient.exe";

    DWORD pid = ProcessUtils::FindPIDByName(procName);
    if (!pid) {
        std::cerr << "Process couldn't be found.\n";
        return 1;
    }

    BYTE *baseaddr = ProcessUtils::FindModuleBaseAddr(pid, modName);
    if (!baseaddr) {
        std::cerr << "Couldn't find module in process address space.\n";
        return 1;
    }

    BYTE *rdata_base = baseaddr + RDATA_OFF;
    BYTE *max_zoom_out0 = rdata_base + CAM_MAX_OZOOM_OFF0;
    BYTE *max_zoom_out1 = rdata_base + CAM_MAX_OZOOM_OFF1;

    FLOAT new_max;
    while (TRUE) {
        std::cout << "New max zoom value: ";
        std::cin >> new_max;
        HANDLE hProcess;
        if (!(hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid))) {
            std::cerr << "Couldn't open handle to process.\n";
            return 1;
        }
        if (!ProcessUtils::WriteProtectedProcessMemory(hProcess, max_zoom_out0, &new_max, sizeof(FLOAT))) {
            CloseHandle(hProcess);
            return 1;
        }
        if (!ProcessUtils::WriteProtectedProcessMemory(hProcess, max_zoom_out1, &new_max, sizeof(FLOAT))) {
            CloseHandle(hProcess);
            return 1;
        }

        CloseHandle(hProcess);
    }
    
    return 0;
}

