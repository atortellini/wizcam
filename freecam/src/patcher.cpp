#include "patcher.h"
#include "processutils.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

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
        std::ostringstream oss;
        oss << "Patcher constructor failed: " << e.what();
        throw std::runtime_error(oss.str());
    }

    HANDLE hProcess;
    if (!(hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid))) {
        std::ostringstream oss;
        oss << "Patcher constructor failed: " << "Failed opening handle to process: " << pid;
        throw std::runtime_error(oss.str());
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

   /**
    * Traversing the pointer chain to retieve the first field of wiz's camera struct 
    * Last offset in the pointer offset chain should not be added and dereferenced
    * as the last offset is the offset of the first field of the cam struct from the
    * base class address stored in the heap.
    */
    uintptr_t tmp = gameBaseAddr;

    for (size_t i=0;i<std::size(POINTER_OFFSET_CHAIN)-1;i++) {
        tmp += POINTER_OFFSET_CHAIN[i];
        if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(tmp), reinterpret_cast<LPVOID>(&tmp), sizeof(tmp), NULL)) {
            std::ostringstream oss;
            oss << "Patcher constructor failed: " << "Failed traversing pointer chain.\n" << "Could not read process memory at address 0x" << std::hex << tmp << ". Offset 0x" << offset;
            throw std::runtime_error(oss.str());
        }
    }

    tmp += POINTER_OFFSET_CHAIN[std::size(POINTER_OFFSET_CHAIN)-1];

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
        if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPVOID>(instr.orig_instr), instr.bytes, NULL)) {
            std::ostringstream oss;
            oss << "Patcher constructor failed: " << "Could not read instructions in process memory at address 0x" << std::hex << gameBaseAddr + instr.offset;
            throw std::runtime_error(oss.str());
        }
    }
   
}

Patcher::~Patcher() {
    if (gameProcess) {
        CloseHandle(gameProcess);
    }
}

void Patcher::patch() { /* Might want to halt all threads before writing to .text pages and flushing instruction caches after. */
    BYTE patch[MAX_ENCODED_INSTRUCTION_LENGTH] = { NOP, NOP, NOP, NOP, NOP };
    try {
        for (const auto& instr : instructionAddresses) {
            ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPVOID>(patch), instr.bytes, PAGE_EXECUTE_READWRITE);
        }
    } catch (std::runtime_error& e) {
        std::ostringstream oss;
        oss << "Applying patch failed: " << e.what();
        throw std::runtime_error(oss.str());
    }

    /* FlushInstructionCache args:
     * hProcess      - gameProcess
     * lpBaseAddress - smallest instruction address modified from instructionAddresses arr
     * dwSize        - (max_instruction_address - min_instruction_address) + max_instruction_address.bytes 
     */
    if (!FlushInstructionCache(gameProcess, reinterpret_cast<LPCVOID>(instructionAddresses[3].offset), reinterpret_cast<LPCVOID>((instructionAddresses[1].offset - instructionAddresses[3].offset) + instructionAddresses[1].bytes))) {
        throw std::runtime_error("Applying patch failed: Failed to flush instruction cache.");
    }
}

void Patcher::unpatch() {
    try {
        for (const auto& instr : instructionAddresses) {
            ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPVOID>(instr.orig_instr), instr.bytes, PAGE_EXECUTE_READWRITE);
        }
    } catch (std::runtime_error& e) {
        std::ostringstream oss;
        oss << "Removing patch failed: " << e.what();
        throw std::runtime_error(oss.str());
    }
     /* FlushInstructionCache args:
     * hProcess      - gameProcess
     * lpBaseAddress - smallest instruction address modified from instructionAddresses arr
     * dwSize        - (max_instruction_address - min_instruction_address) + max_instruction_address.bytes 
     */
    if (!FlushInstructionCache(gameProcess, reinterpret_cast<LPCVOID>(instructionAddresses[3].offset), reinterpret_cast<LPCVOID>((instructionAddresses[1].offset - instructionAddresses[3].offset) + instructionAddresses[1].bytes))) {
        throw std::runtime_error("Removing patch failed: Failed to flush instruction cache.");
    }
}

void Patcher::retrieveCamData(void *buff, size_t nSize) {
    if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(camBaseAddr), buff, nSize)) {
        std::ostringstream oss;
        oss << "Failed to retrieve camera data: " << "Could not read camera data in process memory at address 0x" << std::hex << camBaseAddr;
        throw std::runtime_error(oss.str());
    }
}

void Patcher::setCamData(void *buff, size_t nSize) {
    try {
        ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(camBaseAddr), buff, nSize, PAGE_EXECUTE_READWRITE);
    } catch (std::runtime_error& e) {
        std::ostringstream oss;
        oss << "Failed to set camera data: " << e.what();
        throw std::runtime_error(oss.str());
    }
}