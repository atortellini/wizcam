#include <windows.h>
#include <iostream>
#include <atomic>
#include <mutex>

class Camera {
    private:
    uintptr_t cambaseaddr;
    HANDLE hProcess;

    std::mutex camlock;
    std::atomic<bool> dirtycam;

    struct GameCamera {
        float x, y, z;
        float pitch, yaw, roll;
    } localCameraData;

    /* 
     * TODO: GameCamera struct is used as a buffer so need to ensure my local struct is the 
     * same size, alligned properly, same members, etc. as the game's cam struct 
     */
    public:
    Camera(HANDLE process, uintptr_t baseaddr) : hProcess(process), cambaseaddr(baseaddr) {
        syncFromGame();
    }

    /* Reads all fields of game cam from process' memory */
    void syncFromGame() {
        struct GameCamera tmpCameraData;
        if (!ReadProcessMemory(hProcess, (void *)cambaseaddr, &tmpCameraData, sizeof(tmpCameraData), NULL)) {
            std::cerr << "Failed to read process memory." << std::endl;
        }
        camlock.lock();
        localCameraData = tmpCameraData;
        dirtycam.store(false);
        camlock.unlock();
    }

    /* Writes all fields of local game cam to process' memory */
    void syncToGame() {
        if (dirtycam.load()) {
            camlock.lock();
            struct GameCamera tmpCameraData = localCameraData;
            dirtycam.store(false);
            camlock.unlock();
            if (!WriteProcessMemory(hProcess, (void *)cambaseaddr, &tmpCameraData, sizeof(tmpCameraData), NULL)) {
                std::cerr << "Failed to write process memory." << std::endl;
            }
        }
    }
    /* 
     * Very basic/rudimentary movement system 
     * TODO: Implement better movement of camera using its forward vector 
     */
    void moveX(float dx) {
        camlock.lock();
        localCameraData.x += dx;
        dirtycam.store(true);
        camlock.unlock();
        
    }

    void moveY(float dy) {
        camlock.lock();
        localCameraData.y += dy;
        dirtycam.store(true);
        camlock.unlock();
    }

    void moveZ(float dz) {
        camlock.lock();
        localCameraData.z += dz;
        dirtycam.store(true);
        camlock.unlock();
    }

    void movePitch(float dpitch) {
        camlock.lock();
        localCameraData.pitch += dpitch;
        dirtycam.store(true);
        camlock.unlock();
    }

    void moveYaw(float dyaw) {
        camlock.lock();
        localCameraData.yaw += dyaw;
        dirtycam.store(true);
        camlock.unlock();
    }

    void moveRoll(float droll) {
        camlock.lock();
        localCameraData.roll += droll;
        dirtycam.store(true);
        camlock.unlock();
    }

    /* Function for debugging */
    void printState() const {
        camlock.lock();
        printf("Camera position: (%f, %f, %f)\n", localCameraData.x, localCameraData.y, localCameraData.z);
        printf("Camera rotation: (Pitch: %f, Yaw: %f, Roll: %f)\n", localCameraData.pitch, localCameraData.yaw, localCameraData.roll);
        camlock.unlock();
    }
    
}