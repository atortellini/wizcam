#include "patcher.h"
#include "camera.h"
#include <winuser.h>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <thread>
#include <atomic>

std::atomic<bool> running = true;
std::atomic<bool> freecamEnabled = false;

void processInput(Patcher& p, Camera& cam) {
    bool current_cam_toggle = freecamEnabled.load();
    while (running) {
        if (GetAsyncKeyState(VK_F8) & 0x8000) {
            bool new_state = !current_cam_toggle;
            freecamEnabled.store(new_state); 
            current_cam_toggle = new_state;
            try {
                if (current_cam_toggle) {
                    p.patch();
                } else {
                    p.unpatch();
                }
            } catch (std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
            }
        }
        if (current_cam_toggle) {
            if (GetAsyncKeyState('W') & 0x8000) cam.moveX(CAM_MOVE_SPEED);
            if (GetAsyncKeyState('A') & 0x8000) cam.moveY(-CAM_MOVE_SPEED);
            if (GetAsyncKeyState('S') & 0x8000) cam.moveX(-CAM_MOVE_SPEED);
            if (GetAsyncKeyState('D') & 0x8000) cam.moveY(CAM_MOVE_SPEED);
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) cam.moveZ(CAM_MOVE_SPEED);
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) cam.moveZ(-CAM_MOVE_SPEED);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        
    }
    std::this_thread::yield();
}


int main(void) {
    Patcher p;
    Camera cam(p);

    try {
        p.init();
        cam.syncFromGame();
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::thread input_thread(processInput, std::ref(p), std::ref(cam));

    while (true) {
        if (freecamEnabled.load()) {
            cam.syncToGame();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    input_thread.join();

    return 0;
}