#pragma once

#include <windows.h>
#include <cstdint>

#define MAX_ENCODED_INSTRUCTION_LENGTH 5
#define NUM_INSTRUCTIONS_TO_UPDATE 4

/**
 * @class Patcher
 * @brief Provides functionality for modifying and restoring process memory, particularly for camera manipulation in Wiz.
 */
class Patcher {
private:
    HANDLE gameProcess;                 ///< Handle to Wiz.
    uintptr_t gameBaseAddr;             ///< Base address of the Wiz's module in its process' address space.
    uintptr_t camBaseAddr;              ///< Base address of the camera struct.

    /**
     * @struct instruction_t
     * @brief Represents an instruction to be modifed in Wiz. The represented instructions are responsible for manipulation Wiz's camera.
     */
    struct instruction_t {
        uintptr_t offset;                                   ///< Offset from the gameBaseAddr to the instruction.
        size_t bytes;                                       ///< Number of bytes the instruction's encoding occupies.
        BYTE orig_instr[MAX_ENCODED_INSTRUCTION_LENGTH];    ///< Original instruction's bytes for restoring.
    } instructionAddresses[NUM_INSTRUCTIONS_TO_UPDATE];     ///< Array of instructions to be modified or restored.

public:
    /**
     * @brief Constructs the Patcher object, initializes a handle to Wiz, locates the camera structure, and stores the instructions responsible for manipulating Wiz's camera.
     * @throws std::runtime_error if the process, camera structure, or corresponding instructions modifying the camera struct cannot be located or accessed.
     */
    Patcher();

    /**
     * @brief Destroys the Patcher object and closes the handle to Wiz, if present.
     */
    ~Patcher();

    /**
     * @brief Applies a patch to modify specified instructions in Wiz.
     * 
     * The patch replaces instruction bytes with NOP instructions.
     * @throws std::runtime_error if patching fails.
     */
    void patch();

    /**
     * @brief Restores the original instructions in Wiz.
     * 
     * Reverts the instructions modified by the `patch` method.
     * @throws std::runtime_error if unpatching fails.
     */
    void unpatch();

    /**
     * @brief Retrieves data from Wiz's camera struct.
     * 
     * @param buff Pointer to the buffer where the retrieved data will be stored.
     * @param nSize Size of the buffer (number of bytes to read).
     * @throws std::runtime_error if the memory read operation fails.
     */
    void retrieveCamData(void* buff, size_t nSize);

    /**
     * @brief Writes data to Wiz's camera struct.
     * 
     * @param buff Pointer to the buffer containing the data to write.
     * @param nSize Size of the buffer (number of bytes to write).
     * @throws std::runtime_error if the memory write operation fails.
     */
    void setCamData(void* buff, size_t nSize);
};
