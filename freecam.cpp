#include <windows.h>
#include <iostream>

class Camera {
    private:
    uintptr_t cambaseaddr;
    HANDLE hProcess;

    struct GameCamera {
        float x, y, z;
        float pitch, yaw, roll;
    } localCameraData;

    public:
    Camera(HANDLE process, uintptr_t baseaddr) : hProcess(process), cambaseaddr(baseaddr) {
        syncFromGame();
    }

    /* Reads all fields of game cam from process' memory */
    void syncFromGame() {
        if (!ReadProcessMemory(hProcess, (void *)cambaseaddr, &localCameraData, sizeof(localCameraData), NULL)) {
            std::cerr << "Failed to read process memory." << std::endl;
        }
    }

    /* Writes all fields of local game cam to process' memory */
    void syncToGame() {
        if (!WriteProcessMemory(hProcess, (void *)cambaseaddr, &localCameraData, sizeof(localCameraData), NULL)) {
            std::cerr << "Failed to write process memory." << std::endl;
        }
    }
    /* Very basic/rudimentary movement system 
     * TODO: Implement better movement of camera using its forward vector */
    void moveX(float dx) {
        localCameraData.x += dx;
    }

    void moveY(float dy) {
        localCameraData.y += dy;
    }

    void moveZ(float dz) {
        localCameraData.z += dz;
    }

    void movePitch(float dpitch) {
        localCameraData.pitch += dpitch;
    }

    void moveYaw(float dyaw) {
        localCameraData.yaw += dyaw;
    }

    void moveRoll(float droll) {
        localCameraData.roll += droll;
    }

    /* Function for debugging */
    void printState() const {
        printf("Camera position: (%f, %f, %f)\n", localCameraData.x, localCameraData.y, localCameraData.z);
        printf("Camera rotation: (Pitch: %f, Yaw: %f, Roll: %f)\n", localCameraData.pitch, localCameraData.yaw, localCameraData.roll);
    }
    
}