#pragma once

#include <stdint.h>

#define NUM_INSTRUCTIONS_TO_UPDATE 4


class Patcher {
    private:
    HANDLE gameProcess;
    uintptr_t gameBaseAddr;
    uintptr_t camBaseAddr;

    struct instruction_t {
        uintptr_t offset;
        size_t bytes;
        BYTE orig_instr[5];
    } instructionAddresses[NUM_INSTRUCTIONS_TO_UPDATE];

    public:
    Patcher();
    bool patch();
    bool unpatch();
    void retrieveCamData(void *buff, size_t nSize);
    void setCamData(void *buff, size_t nSize);

}