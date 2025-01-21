#include "camera.h"

#include <iostream>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

Camera::Camera(Patcher& p) : patcher(p), camspeed(40.) {}

void Camera::syncFromGame() {
    struct GameCamera tmpCameraData;
    try { 
        patcher.retrieveCamData(&tmpCameraData, sizeof(tmpCameraData));
    } catch (std::runtime_error& e) {
        std::ostringstream oss;
        oss << "Camera failed to sync: " << e.what();
        throw std::runtime_error(oss.str());
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
        try {
            patcher.setCamData(&tmpCameraData, sizeof(tmpCameraData));
        } catch (std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

/* https://gamedev.stackexchange.com/questions/190054/how-to-calculate-the-forward-up-right-vectors-using-the-rotation-angles */
glm::vec3 Camera::calculateForwardVector() { 
    float cyaw, cpitch;
    camlock.lock();
    cyaw = localCameraData.yaw;
    cpitch = localCameraData.pitch;
    camlock.unlock();
    glm::vec3 forward;
    forward.x = cos(glm::radians(cpitch)) * sin(glm::radians(cyaw));
    forward.y = sin(glm::radians(cpitch));
    forward.z = cos(glm::radians(cpitch)) * cos(glm::radians(cyaw));
    return glm::normalize(forward);
}

glm::vec3 Camera::calculateRightVector() {
    float cyaw;
    camlock.lock();
    cyaw = localCameraData.yaw
    camlock.unlock();
    glm::vec3 right;
    right.x = cos(glm::radians(cyaw));
    right.y = 0;
    right.z = -sin(glm::radians(cyaw));
    return glm::normalize(right);
}

void Camera::moveForward(float dt) {
    glm::vec3 forward = calculateForwardVector();
    float dx = dt * camspeed * forward.x;
    float dy = dt * camspeed * forward.y;
    float dz = dt * camspeed * forward.z;
    camlock.lock();
    localCameraData.x += dx;
    localCameraData.y += dy;
    localCameraData.z += dz;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveBackward(float dt) {
    glm::vec3 forward = calculateForwardVector();
    float dx = dt * camspeed * forward.x;
    float dy = dt * camspeed * forward.y;
    float dz = dt * camspeed * forward.z;
    camlock.lock();
    localCameraData.x -= dx;
    localCameraData.y -= dy;
    localCameraData.z -= dz;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveLeft(float dt) { /* y coordinate is ignored */
    glm::vec3 right = calculateRightVector();
    float dx = dt * camspeed * right.x;
    float dz = dt * camspeed * right.z;
    camlock.lock();
    localCameraData.x -= dx;
    localCameraData.z -= dz;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveRight(float dt) { /* y coordinate is ignored */
    glm::vec3 right = calculateRightVector();
    float dx = dt * camspeed * right.x;
    float dz = dt * camspeed * right.z;
    camlock.lock();
    localCameraData.x += dx;
    localCameraData.z += dz;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveUp(float dt) {
    camlock.lock();
    localCameraData.z += dt * camspeed;
    dirtycam.store(true);
    camlock.unlock();
}

void Camera::moveDown(float dt) {
    camlock.lock();
    localCameraData.z -= dt * camspeed;
    dirtycam.store(true);
    camlock.unlock();
}



/*   BEING HANDLED BY GAME NATIVELY
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
*/
void Camera::printState() {
    camlock.lock();
    printf("Camera position: (%f, %f, %f)\n", localCameraData.x, localCameraData.y, localCameraData.z);
    /* printf("Camera rotation: (Pitch: %f, Yaw: %f, Roll: %f)\n", localCameraData.pitch, localCameraData.yaw, localCameraData.roll); */
    camlock.unlock();
}
