#include "patcher.h"
#include "camera.h"
#include <winuser.h>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <thread>
#include <atomic>

#define WRITEBACK_RATE_MS 33
#define DEBOUNCE_PERIOD_MS 33
namespace {
    constexpr float WRITEBACK_RATE_S = WRITEBACK_RATE_MS / 1000.0f;
}

std::atomic<bool> running = true;
std::atomic<bool> freecamEnabled = false;

void processInput(Patcher& p, Camera& cam) {
    bool current_cam_toggle = freecamEnabled.load();
    while (running) {
        if (GetAsyncKeyState(VK_F8) & 0x8000) {
            bool new_state = !current_cam_toggle; 
            current_cam_toggle = new_state;
            try {
                if (current_cam_toggle) {
                    p.patch();
                    cam.syncFromGame();
                } else {
                    p.unpatch();
                }
            } catch (std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
            }
            freecamEnabled.store(new_state);
        }
        if (current_cam_toggle) {
            if (GetAsyncKeyState('W') & 0x8000) cam.moveForward(WRITEBACK_RATE_S);
            if (GetAsyncKeyState('A') & 0x8000) cam.moveLeft(WRITEBACK_RATE_S);
            if (GetAsyncKeyState('S') & 0x8000) cam.moveBackward(WRITEBACK_RATE_S);
            if (GetAsyncKeyState('D') & 0x8000) cam.moveRight(WRITEBACK_RATE_S);
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) cam.moveUp(WRITEBACK_RATE_S);
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) cam.moveDown(WRITEBACK_RATE_S);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(DEBOUNCE_PERIOD_MS));
        
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
        std::this_thread::sleep_for(std::chrono::milliseconds(WRITEBACK_RATE_MS));
    }

    input_thread.join();

    return 0;
}