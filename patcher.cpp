#include "patcher.h"
#include "processutils.h"

namespace {
    constexpr BYTE NOP = 0x90;

    constexpr uintptr_t BASE_OFFSET = 0x34e1908;
    constexpr uintptr_t OFFSET_0 = 0x80;
    constexpr uintptr_t OFFSET_1 = 0x1e8;
    constexpr uintptr_t OFFSET_2 = 0x288;
    constexpr uintptr_t OFFSET_3 = 0x80;
    constexpr uintptr_t OFFSET_4 = 0x9a8;
    constexpr uintptr_t OFFSET_5 = 0x180;
    constexpr uintptr_t OFFSET_6 = 0x6c;

    constexpr uintptr_t POINTER_OFFSET_CHAIN[] = { BASE_OFFSET, OFFSET_0, OFFSET_1, OFFSET_2, OFFSET_3, OFFSET_4, OFFSET_5, OFFSET_6 };
}

Patcher::Patcher() {
    std::string procName = "WizardGraphicalClient.exe";
    std::string modName = "WizardGraphicalClient.exe";

    DWORD pid;
    BYTE *baseaddr;
    try {
        pid = ProcessUtils::FindPIDByName(procName);
        baseaddr = ProcessUtils::FindModuleBaseAddr(pid, modName);
    } catch (std::runtime_error& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        exit(1);
    }

    HANDLE hProcess;
    if (!(hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid))) {
            std::cerr << "Runtime Error: Couldn't open handle to process." << std::endl;
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
    uintptr_t tmp = gameBaseAddr;

    for (uintptr_t offset : POINTER_OFFSET_CHAIN) {
        tmp += offset;
        if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(tmp), reinterpret_cast<LPVOID>(&tmp), sizeof(tmp), NULL)) {
            std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << ". Offset 0x" << offset << std::dec << std::endl;
            exit(1);
        }
    }

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
        if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPVOID>(instr.orig_instr), instr.bytes)) {
            std::cerr << "Error: Could not read process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
            exit(1);
        }
    }
   
}

Patcher::~Patcher() {
    if (gameProcess) {
        CloseHandle(gameProcess);
    }
}

bool Patcher::patch() { /* Might want to halt all threads before writing to .text pages and flushing instruction caches after. */
    BYTE patch[MAX_ENCODED_INSTRUCTION_LENGTH] = { NOP, NOP, NOP, NOP, NOP };
    try {
        for (auto& instr : instructionAddresses) {
            ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPVOID>(patch), instr.bytes, PAGE_EXECUTE_READWRITE); {
            }
        }
    } catch (std::runtime_error& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return false;
    }

    /* FlushInstructionCache args:
     * hProcess      - gameProcess
     * lpBaseAddress - smallest instruction address modified from instructionAddresses arr
     * dwSize        - (max_instruction_address - min_instruction_address) + max_instruction_address.bytes 
     */
    if (!FlushInstructionCache(gameProcess, reinterpret_cast<LPCVOID>(instructionAddresses[3].offset), reinterpret_cast<LPCVOID>((instructionAddresses[1].offset - instructionAddresses[3].offset) + instructionAddresses[1].bytes))) {
        std::cerr << "Runtime Error: Call to FlushInstructionCache failed" << std::endl;
    }
    return true;
}

bool Patcher::unpatch() {
    try {
        for (auto& instr : instructionAddresses) {
            ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPVOID>(instr.orig_instr), instr.bytes, PAGE_EXECUTE_READWRITE);
        }
    } catch (std::runtime_error& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return false;
    }

     /* FlushInstructionCache args:
     * hProcess      - gameProcess
     * lpBaseAddress - smallest instruction address modified from instructionAddresses arr
     * dwSize        - (max_instruction_address - min_instruction_address) + max_instruction_address.bytes 
     */
    if (!FlushInstructionCache(gameProcess, reinterpret_cast<LPCVOID>(instructionAddresses[3].offset), reinterpret_cast<LPCVOID>((instructionAddresses[1].offset - instructionAddresses[3].offset) + instructionAddresses[1].bytes))) {
        std::cerr << "Runtime Error: Call to FlushInstructionCache failed" << std::endl;
    }
    return true;
}

void Patcher::retrieveCamData(void *buff, size_t nSize) {
    if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(camBaseAddr), buff, nSize)) {
            std::cerr << "Error: Could not read camera data in process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
    }
}

void Patcher::setCamData(void *buff, size_t nSize) {
    if (!ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(camBaseAddr), buff, nSize, PAGE_EXECUTE_READWRITE)) {
        std::cerr << "Error: Could not write camera data in process memory at address 0x" << std::hex << tmp << std::dec << std::endl;
    }
}