#include "freecam.h"
/* First cam struct field pointerchain:
 * BA:   ["WizardGraphicalClient.exe" + 0x34e1908] = a0
 * OFF0: [a0 + 0x80] = a1
 * OFF1: [a1 + 0x1e8] = a2
 * OFF2: [a2 + 0x288] = a3
 * OFF3: [a3 + 0x80] = a4
 * OFF4: [a4 + 0x9a8] = a5
 * OFF5: [a5 + 0x180] = a6
 * OFF6: [a6 + 0x6c] = a7 

 * INSTRUCTION MODIFYING X/Y:
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

Camera::Camera(HANDLE process, uintptr_t baseaddr) 
    : hProcess(process), cambaseaddr(baseaddr) {
    syncFromGame();
}

void Camera::syncFromGame() {
    struct GameCamera tmpCameraData;
    if (!ReadProcessMemory(hProcess, (void*)cambaseaddr, &tmpCameraData, sizeof(tmpCameraData), NULL)) {
        std::cerr << "Failed to read process memory." << std::endl;
    }
    camlock.lock();
    localCameraData = tmpCameraData;
    dirtycam.store(false);
    camlock.unlock();
}

void Camera::syncToGame() {
    if (dirtycam.load()) {
        camlock.lock();
        struct GameCamera tmpCameraData = localCameraData;
        dirtycam.store(false);
        camlock.unlock();
        if (!WriteProcessMemory(hProcess, (void*)cambaseaddr, &tmpCameraData, sizeof(tmpCameraData), NULL)) {
            std::cerr << "Failed to write process memory." << std::endl;
        }
    }
}

void Camera::moveX(float dx) {
    camlock.lock();
    localCameraData.x += dx;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveY(float dy) {
    camlock.lock();
    localCameraData.y += dy;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveZ(float dz) {
    camlock.lock();
    localCameraData.z += dz;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::movePitch(float dpitch) {
    camlock.lock();
    localCameraData.pitch += dpitch;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveYaw(float dyaw) {
    camlock.lock();
    localCameraData.yaw += dyaw;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveRoll(float droll) {
    camlock.lock();
    localCameraData.roll += droll;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::printState() const {
    camlock.lock();
    printf("Camera position: (%f, %f, %f)\n", localCameraData.x, localCameraData.y, localCameraData.z);
    printf("Camera rotation: (Pitch: %f, Yaw: %f, Roll: %f)\n", localCameraData.pitch, localCameraData.yaw, localCameraData.roll);
    camlock.unlock();
}
