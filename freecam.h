#pragma once

#include <windows.h>
#include <iostream>
#include <atomic>
#include <mutex>

class Camera {
    private:
    Patcher& patcher;
    std::mutex camlock;
    std::atomic<bool> dirtycam;

    /* 
     * TODO: GameCamera struct is used as a buffer so need to ensure my local struct is the 
     * same size, alligned properly, same members, etc. as the game's cam struct 
     */
    struct GameCamera {
        float x, y, z;
        float pitch, roll, yaw;
    } localCameraData;
    
    public:
    Camera(Patcher& p);

    /* Reads all fields of game cam from process' memory */
    void syncFromGame();
    /* Writes all fields of local game cam to process' memory */
    void syncToGame();

     /* 
     * Very basic/rudimentary movement system 
     * TODO: Implement better movement of camera using its forward vector 
     */
    void moveX(float dx);
    void moveY(float dy);
    void moveZ(float dz);
    void movePitch(float dpitch);
    void moveRoll(float droll);
    void moveYaw(float dyaw);

    /* Function for debugging */
    void printState() const;
}