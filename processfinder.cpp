#include <tl32help.h>
#include <psapi.h>

static DWORD FindPIDByName(const std::wstring& processName) {
    HANDLE snapshot;
    if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create process snapshot.\n";
        return -1;
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
    
    return -1;

}