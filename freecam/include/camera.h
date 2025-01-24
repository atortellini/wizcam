#pragma once

#include "patcher.h"

#include <atomic>
#include <mutex>
#include "vec3.hpp"

class Camera {
    private:
    Patcher& patcher;
    std::mutex camlock;
    std::atomic<bool> dirtycam;
    float camspeed;

    struct GameCamera {
        float x, z, y;
        float pitch, roll, yaw;
    } localCameraData;
    
    glm::vec3 calculateForwardVector();
    glm::vec3 calculateRightVector();
    void syncFromGameYaw();
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
    void moveForward(float dt);
    void moveBackward(float dt);
    void moveLeft(float dt);
    void moveRight(float dt);
    void moveUp(float dt);
    void moveDown(float dt);
    /* HANDLED BY GAME NATIVELY NOW
    void movePitch(float dpitch);
    void moveRoll(float droll);
    void moveYaw(float dyaw);
    */
    /* Function for debugging */
    void printState();
};