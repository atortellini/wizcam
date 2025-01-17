#include "processutils.h"

#include <iostream>
#include <stdexcept>



#define RDATA_OFF 0x271b000
#define CAM_MAX_OZOOM_OFF0 0x4849a4
#define CAM_MAX_OZOOM_OFF1 0x485dd0

int main(void) {

    std::string procName = "WizardGraphicalClient.exe";
    std::string modName = "WizardGraphicalClient.exe";
    HANDLE hProcess = NULL;
    FLOAT new_max;

    try {
        DWORD pid = ProcessUtils::FindPIDByName(procName);
        BYTE *baseaddr = ProcessUtils::FindModuleBaseAddr(pid, modName);
    
        BYTE *rdata_base = baseaddr + RDATA_OFF;
        BYTE *max_zoom_out0 = rdata_base + CAM_MAX_OZOOM_OFF0;
        BYTE *max_zoom_out1 = rdata_base + CAM_MAX_OZOOM_OFF1;

        while (TRUE) {
            std::cout << "New max zoom value: ";
            std::cin >> new_max;
            if (!(hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid))) {
                std::cerr << "Couldn't open handle to process.\n";
                return 1;
            }
            ProcessUtils::WriteProtectedProcessMemory(hProcess, max_zoom_out0, &new_max, sizeof(FLOAT), PAGE_READWRITE);
            ProcessUtils::WriteProtectedProcessMemory(hProcess, max_zoom_out1, &new_max, sizeof(FLOAT), PAGE_READWRITE);

            CloseHandle(hProcess);
            hProcess = NULL;
        }
        
        return 0;
    } catch (std::runtime_error& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        if (hProcess) CloseHandle(hProcess);
        return 1;
    }
}

