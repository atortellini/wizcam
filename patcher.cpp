#include "patcher.h"
#include "processutils.h"


Patcher::Patcher() {
    std::string procName = "WizardGraphicalClient.exe";
    std::string modName = "WizardGraphicalClient.exe";

    DWORD pid = ProcessUtils::FindPIDByName(procName);
    if (!pid) {
        std::cerr << "Process couldn't be found.\n";
        exit(1);
    }

    BYTE *baseaddr = ProcessUtils::FindModuleBaseAddr(pid, modName);
    if (!baseaddr) {
        std::cerr << "Couldn't find module in process address space.\n";
        exit(1);
    }

    HANDLE hProcess;
    if (!(hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid))) {
            std::cerr << "Couldn't open handle to process.\n";
            exit(1);
    }
    
    gameProcess = hProcess;
    gameBaseAddr = baseaddr;

     /* First cam struct field pointerchain:
    * BA:   ["WizardGraphicalClient.exe" + 0x34e1908] = a0
    * OFF0: [a0 + 0x80] = a1
    * OFF1: [a1 + 0x1e8] = a2
    * OFF2: [a2 + 0x288] = a3
    * OFF3: [a3 + 0x80] = a4
    * OFF4: [a4 + 0x9a8] = a5
    * OFF5: [a5 + 0x180] = a6
    * OFF6: [a6 + 0x6c] = a7 
    */

   /* Traversing the pointer chain to retieve the first field of wiz's camera struct */
    uintptr_t tmp;
    tmp = gameBaseAddr + 0x34e1908;
    if (!ReadProcessMemory(gameProcess, (void *)tmp, &tmp, sizeof(tmp), NULL)) {
        std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
        exit(1);
    }
    tmp += 0x80;
    if (!ReadProcessMemory(gameProcess, (void *)tmp, &tmp, sizeof(tmp), NULL)) {
        std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
        exit(1);
    }
    tmp += 0x1e8;
    if (!ReadProcessMemory(gameProcess, (void *)tmp, &tmp, sizeof(tmp), NULL)) {
        std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
        exit(1);
    }
    tmp += 0x288;
    if (!ReadProcessMemory(gameProcess, (void *)tmp, &tmp, sizeof(tmp), NULL)) {
        std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
        exit(1);
    }
    tmp += 0x80;
    if (!ReadProcessMemory(gameProcess, (void *)tmp, &tmp, sizeof(tmp), NULL)) {
        std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
        exit(1);
    }
    tmp += 0x9a8;
    if (!ReadProcessMemory(gameProcess, (void *)tmp, &tmp, sizeof(tmp), NULL)) {
        std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
        exit(1);
    }
    tmp += 0x180;
    if (!ReadProcessMemory(gameProcess, (void *)tmp, &tmp, sizeof(tmp), NULL)) {
        std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
        exit(1);
    }
    tmp += 0x6c; /* TODO: NOT SURE IF THIS LAST ADDRESS IS THE ADDRESS OF THE CAM STRUCT OR IF IT CONTAINS THE ADDRESS OF THE CAM STRUCT */

    camBaseAddr = tmp;

    /*  INSTRUCTION MODIFYING X/Y:
    * ["WizardGraphicalClient.exe" + 0x18298bd]; 5 bytes

    * INSTRUCTION MODIFYING Z:
    * ["WizardGraphicalClient.exe" + 0x18298c5]; 3 bytes

    * INSTRUCTION MODIFYING PITCH:
    * ["WizardGraphicalClient.exe" + 0x1829768]; 5 Bytes

    * INSTRUCTION MODIFYING ROLL:
    * N/A

    * INSTRUCTION MODIFYING YAW:
    * ["WizardGraphicalClient.exe" + 0x182968f]; 5 Bytes
    */
   instructionAddresses[0].offset = 0x18298bd;
   instructionAddresses[1].offset = 0x18298c5;
   instructionAddresses[2].offset = 0x1829768;
   instructionAddresses[3].offset = 0x182968f;

   instructionAddresses[0].bytes = 5;
   instructionAddresses[1].bytes = 3;
   instructionAddresses[2].bytes = 5;
   instructionAddresses[3].bytes = 5;


    /* Reading the original instructions into instruction_t structs so patches can be reverted */
    for (auto& instr : instructionAddresses) {
        if (!ReadProcessMemory(gameProcess, (void *)(gameBaseAddr + instr.offset), instr.orig_instr, instr.bytes)) {
            std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
            exit(1);
        }
    }
   
}

bool Patcher::patch() { /* Might want to halt all threads before writing to .text pages and flushing instruction caches after. */
    BYTE patch[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    for (auto& instr : instructionAddresses) {
        if (!ProcessUtils::WriteProtectedProcessMemory(gameProcess, (void *)(gameBaseAddr + instr.offset), patch, instr.bytes, PAGE_EXECUTE_READWRITE)) {
            std::cerr << "Error: Could not patch instructions in process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
            return false;
        }
    }
    return true;
}

bool Patcher::unpatch() {
    for (auto& instr : instructionAddresses) {
        if (!ProcessUtils::WriteProtectedProcessMemory(gameProcess, (void *)(gameBaseAddr + instr.offset), instr.orig_instr, instr.bytes, PAGE_EXECUTE_READWRITE)) {
            std::cerr << "Error: Could not revert patched instructions in process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
            return false;
        }
    }
    return true;
}

void Patcher::retrieveCamData(void *buff, size_t nSize) {
    if (!ReadProcessMemory(gameProcess, (void *)camBaseAddr, buff, nSize)) {
            std::cerr << "Error: Could not read camera data in process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
    }
}

void Patcher::setCamData(void *buff, size_t nSize) {
    if (!ProcessUtils::WriteProtectedProcessMemory(gameProcess, (void *)camBaseAddr, buff, nSize, PAGE_EXECUTE_READWRITE)) {
        std::cerr << "Error: Could not write camera data in process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
    }
}