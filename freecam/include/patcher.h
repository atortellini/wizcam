#pragma once

#include <windows.h>
#include <cstdint>

#define MAX_ENCODED_INSTRUCTION_LENGTH 8
#define NUM_INSTRUCTIONS_TO_UPDATE 4

/**
 * @class Patcher
 * @brief Provides functionality for modifying and restoring process memory, particularly for camera manipulation in Wiz.
 */
class Patcher {
private:
    bool initialized;                   ///< Tracks whether the Patcher has been successfully initialized.

    DWORD gamePID;                      ///< Wiz's PID.
    HANDLE gameProcess;                 ///< Handle to Wiz.
    uintptr_t gameBaseAddr;             ///< Base address of the Wiz's module in its process' address space.
    uintptr_t camBaseAddr;              ///< Base address of the camera struct.

    /**
     * @struct instruction_t
     * @brief Represents an instruction to be modified in Wiz. 
     *        These instructions control aspects of Wiz's camera functionality.
     */
    struct instruction_t {
        uintptr_t offset;                                   ///< Offset from the gameBaseAddr to the instruction.
        size_t bytes;                                       ///< Number of bytes the instruction's encoding occupies.
        BYTE orig_instr[MAX_ENCODED_INSTRUCTION_LENGTH];    ///< Original instruction's bytes for restoring.
    } instructionAddresses[NUM_INSTRUCTIONS_TO_UPDATE];     ///< Array of instructions to be modified or restored.

public:
    /**
     * @brief Constructs the Patcher object.
     * 
     * Initializes the `instructionAddresses` array with predefined offsets and sizes. 
     * Does not perform any operations that could fail.
     */
    Patcher();

    /**
     * @brief Destroys the Patcher object and closes the handle to Wiz, if present.
     */
    ~Patcher();

    /**
     * @brief Performs the initialization logic for the Patcher.
     * 
     * - Finds the process ID of Wiz.
     * - Retrieves the base address of Wiz's module in memory.
     * - Opens a handle to the process with required access rights.
     * - Traverses a pointer chain to locate the camera structure.
     * - Reads the original instructions at predefined offsets for restoration purposes.
     * 
     * @throws std::runtime_error if any operation fails (e.g., process not found, memory inaccessible).
     */
    void init();

    /**
     * @brief Applies a patch to modify specified instructions in Wiz.
     * 
     * The patch replaces instruction bytes with NOP instructions.
     * 
     * @throws std::runtime_error if patching fails.
     */
    void patch();

    /**
     * @brief Restores the original instructions in Wiz.
     * 
     * This method reverts the changes made by the `patch()` method by writing 
     * the original instruction bytes back into memory.
     * 
     * @throws std::runtime_error if restoration fails (e.g., memory write error).
     */
    void unpatch();

    /**
     * @brief Retrieves data from Wiz's camera struct.
     * 
     * @param buff Pointer to the buffer where the retrieved data will be stored.
     * @param nSize Size of the buffer (number of bytes to read).
     * @throws std::runtime_error if the memory read operation fails.
     */
    void retrieveCamData(void* buff, size_t nSize) const;

    /**
     * @brief Writes data to Wiz's camera struct.
     * 
     * @param buff Pointer to the buffer containing the data to write.
     * @param nSize Size of the buffer (number of bytes to write).
     * @throws std::runtime_error if the memory write operation fails.
     */
    void setCamData(const void* buff, size_t nSize) const;
};
