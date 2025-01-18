#include "camera.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

Camera::Camera(Patcher& p) : patcher(p) {}

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

void Camera::printState() {
    camlock.lock();
    printf("Camera position: (%f, %f, %f)\n", localCameraData.x, localCameraData.y, localCameraData.z);
    printf("Camera rotation: (Pitch: %f, Yaw: %f, Roll: %f)\n", localCameraData.pitch, localCameraData.yaw, localCameraData.roll);
    camlock.unlock();
}
