#include "patcher.h"
#include "processutils.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

namespace {
    constexpr BYTE NOP = 0x90;

    /**
     * First cam struct field pointerchain:
     * BA:   ["WizardGraphicalClient.exe" + 0x34e1908] = a0
     * OFF0: [a0 + 0x080] = a1
     * OFF1: [a1 + 0x1e8] = a2
     * OFF2: [a2 + 0x288] = a3
     * OFF3: [a3 + 0x080] = a4
     * OFF4: [a4 + 0x9a8] = a5
     * OFF5: [a5 + 0x180] = a6
     * OFF6:  a6 + 0x06c  = a7 
     */
    constexpr uintptr_t CAM_BASE_OFFSET = 0x34d89f8;
    constexpr uintptr_t CAM_OFFSET_0 = 0x80;
    constexpr uintptr_t CAM_OFFSET_1 = 0x1e8;
    constexpr uintptr_t CAM_OFFSET_2 = 0x288;
    constexpr uintptr_t CAM_OFFSET_3 = 0x150;
    constexpr uintptr_t CAM_OFFSET_4 = 0xe0;
    constexpr uintptr_t CAM_OFFSET_5 = 0x348;
    constexpr uintptr_t CAM_OFFSET_6 = 0x6c;

    constexpr uintptr_t POINTER_OFFSET_CHAIN[] = { CAM_BASE_OFFSET, CAM_OFFSET_0, CAM_OFFSET_1, CAM_OFFSET_2, CAM_OFFSET_3, CAM_OFFSET_4, CAM_OFFSET_5, CAM_OFFSET_6 };

    /**
     * INSTRUCTION MODIFYING X/Z:
     * ["WizardGraphicalClient.exe" + 0x182c97d]; 5 bytes  NORMAL UPDATES
     * ["WizardGraphicalClient.exe" + 0x182c969]; 5 bytes  PLAYER COLLISION
     * ["WizardGraphicalClient.exe" + 0x182cda7]; 5 bytes  HOLDING LEFT MOUSE BUTTON

     * INSTRUCTION MODIFYING Y:
     * NORMAL UPDATES X/Z     + 8; 3 bytes                 NORMAL UPDATES
     * PLAYER COLLISION X/Z   + 8; 3 bytes                 PLAYER COLLISION
     * HOLDING LEFT MOUSE X/Z + 5; 3 bytes                 HOLDING LEFT MOUSE

     * INSTRUCTION MODIFYING PITCH:
     * ["WizardGraphicalClient.exe" + 0x182c828]; 5 Bytes  RESETS PITCH

     * INSTRUCTION MODIFYING ROLL:
     * N/A

     * INSTRUCTION MODIFYING YAW:
     * ["WizardGraphicalClient.exe" + 0x182c74f]; 8 Bytes  RESETS YAW
     */

    constexpr size_t     INSTR_XZ_SIZE = 5;
    constexpr size_t      INSTR_Y_SIZE = 3;
    constexpr size_t    INSTR_PCH_SIZE = 5;
    constexpr size_t    INSTR_YAW_SIZE = 8;

    constexpr uintptr_t OFF_INSTR_NORM_XZ = 0x182c97d;
    constexpr uintptr_t OFF_INSTR_COLL_XZ = 0x182c969;
    constexpr uintptr_t OFF_INSTR_HOLD_XZ = 0x182cda7;
    
    constexpr uintptr_t OFF_INSTR_NORM_Y  = OFF_INSTR_NORM_XZ + INSTR_XZ_SIZE + 3;
    constexpr uintptr_t OFF_INSTR_COLL_Y  = OFF_INSTR_COLL_XZ + INSTR_XZ_SIZE + 3;
    constexpr uintptr_t OFF_INSTR_HOLD_Y  = OFF_INSTR_HOLD_XZ + INSTR_XZ_SIZE;

    constexpr uintptr_t OFF_INSTR_RST_PCH = 0x182c828;
    constexpr uintptr_t OFF_INSTR_RST_YAW = 0x182c74f;
}

Patcher::Patcher() {
    
   instructionAddresses[0] = { OFF_INSTR_NORM_XZ, INSTR_XZ_SIZE };
   instructionAddresses[1] = { OFF_INSTR_COLL_XZ, INSTR_XZ_SIZE };
   instructionAddresses[2] = { OFF_INSTR_HOLD_XZ, INSTR_XZ_SIZE };
   instructionAddresses[3] = {  OFF_INSTR_NORM_Y, INSTR_Y_SIZE };
   instructionAddresses[4] = {  OFF_INSTR_COLL_Y, INSTR_Y_SIZE };
   instructionAddresses[5] = {  OFF_INSTR_HOLD_Y, INSTR_Y_SIZE };
   instructionAddresses[6] = { OFF_INSTR_RST_PCH, INSTR_PCH_SIZE };
   instructionAddresses[7] = { OFF_INSTR_RST_YAW, INSTR_YAW_SIZE };

   initialized = false;
}

Patcher::~Patcher() {
    if ((gameProcess) && (gameProcess != INVALID_HANDLE_VALUE)) {
        CloseHandle(gameProcess);
    }
}

void Patcher::init() {
    if (initialized) {
        throw std::runtime_error("Patcher already initialized.");
    }
    initialized = true;

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
    
    gamePID = pid;
    gameProcess = hProcess;
    gameBaseAddr = reinterpret_cast<uintptr_t>(baseaddr);

   
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
            oss << "Patcher constructor failed: " << "Failed traversing pointer chain.\n" << "Could not read process memory at address 0x" << std::hex << tmp << ". Offset 0x" << POINTER_OFFSET_CHAIN[i];
            throw std::runtime_error(oss.str());
        }
    }

    tmp += POINTER_OFFSET_CHAIN[std::size(POINTER_OFFSET_CHAIN)-1];

    camBaseAddr = tmp;


    /* Reading the original instructions into instruction_t structs so patches can be reverted */
    for (auto& instr : instructionAddresses) {
        if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPVOID>(instr.orig_instr), instr.bytes, NULL)) {
            std::ostringstream oss;
            oss << "Patcher constructor failed: " << "Could not read instructions in process memory at address 0x" << std::hex << gameBaseAddr + instr.offset;
            throw std::runtime_error(oss.str());
        }
    }
}

void Patcher::patch() { /* Might want to halt all threads before writing to .text pages and flushing instruction caches after. */
    BYTE patch[MAX_ENCODED_INSTRUCTION_LENGTH] = { NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP };

    try {
        ProcessUtils::HaltAllProcessThreads(gamePID);

        for (const auto& instr : instructionAddresses) {
            ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPCVOID>(patch), instr.bytes, PAGE_EXECUTE_READWRITE);
        }

        /* FlushInstructionCache args:
        * hProcess      - gameProcess
        * lpBaseAddress - smallest instruction address modified from instructionAddresses arr
        * dwSize        - (max_instruction_address - min_instruction_address) + max_instruction_address.bytes 
        */
        if (!FlushInstructionCache(gameProcess, reinterpret_cast<LPCVOID>(instructionAddresses[7].offset), (instructionAddresses[5].offset - instructionAddresses[7].offset) + instructionAddresses[5].bytes )) {
            throw std::runtime_error("Failed to flush instruction cache.");
        }
        ProcessUtils::ResumeAllProcessThreads(gamePID);
    } catch (std::runtime_error& e) {
        std::ostringstream oss;
        oss << "Applying patch failed: " << e.what();
        throw std::runtime_error(oss.str());
    }
}

void Patcher::unpatch() {
    try {
        ProcessUtils::HaltAllProcessThreads(gamePID);
        for (const auto& instr : instructionAddresses) {
            ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPVOID>(gameBaseAddr + instr.offset), reinterpret_cast<LPCVOID>(instr.orig_instr), instr.bytes, PAGE_EXECUTE_READWRITE);
        }
    
        /** 
         * FlushInstructionCache args:
         * hProcess      - gameProcess
         * lpBaseAddress - smallest instruction address modified from instructionAddresses arr
         * dwSize        - (max_instruction_address - min_instruction_address) + max_instruction_address.bytes 
         */
        if (!FlushInstructionCache(gameProcess, reinterpret_cast<LPCVOID>(instructionAddresses[7].offset), (instructionAddresses[5].offset - instructionAddresses[7].offset) + instructionAddresses[5].bytes )) {
            throw std::runtime_error("Failed to flush instruction cache.");
        }
        ProcessUtils::ResumeAllProcessThreads(gamePID);
    } catch (std::runtime_error& e) {
        std::ostringstream oss;
        oss << "Removing patch failed: " << e.what();
        throw std::runtime_error(oss.str());
    }
}

void Patcher::retrieveCamCoordinates(void *buff, size_t nSize) const {
    if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(camBaseAddr), buff, 12, NULL)) {
        std::ostringstream oss;
        oss << "Failed to retrieve camera data: " << "Could not read camera data in process memory at address 0x" << std::hex << camBaseAddr;
        throw std::runtime_error(oss.str());
    }
}

void Patcher::retrieveCamPitchYaw(void *buff, size_t nSize) const {
    if (!ReadProcessMemory(gameProcess, reinterpret_cast<LPCVOID>(camBaseAddr) + 12, buff, 12, NULL)) {
        std::ostringstream oss;
        oss << "Failed to retrieve camera data: " << "Could not read camera data in process memory at address 0x" << std::hex << camBaseAddr;
        throw std::runtime_error(oss.str());
    }
}

void Patcher::setCamData(const void *buff, size_t nSize) const {
    try {
        ProcessUtils::WriteProtectedProcessMemory(gameProcess, reinterpret_cast<LPVOID>(camBaseAddr), buff, 12, PAGE_READWRITE); // This memory region isn't protected anyways so calling protected write is unnecessary.
    } catch (std::runtime_error& e) {
        std::ostringstream oss;
        oss << "Failed to set camera data: " << e.what();
        throw std::runtime_error(oss.str());
    }
}