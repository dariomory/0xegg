#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include "InputDevice.h"
#include "Action.h"
#include "Logger.h"

class Egg {
public:
    // No macro-declared runtime can exceed this, regardless of what it requests.
    static constexpr int kHardMaxRuntimeMs = 30 * 60 * 1000;

    Egg(IMouseInput* mouse, IKeyboardInput* keyboard) : mouse_(mouse), keyboard_(keyboard) {}

    void RepeatLeftClick(int count, int delayMs) {
        for (int i = 0; i < count; i++) {
            mouse_->LeftClick();
            if (!WaitOrAbort(delayMs)) return;
        }
    }

    void RepeatRightClick(int count, int delayMs) {
        for (int i = 0; i < count; i++) {
            mouse_->RightClick();
            if (!WaitOrAbort(delayMs)) return;
        }
    }

    void RepeatMoveTo(int x, int y, int count, int delayMs) {
        for (int i = 0; i < count; i++) {
            mouse_->MoveTo(x, y);
            if (!WaitOrAbort(delayMs)) return;
        }
    }

    void RepeatKeyPress(char key, int count, int delayMs) {
        for (int i = 0; i < count; i++) {
            keyboard_->PressChar(key);
            if (!WaitOrAbort(delayMs)) return;
        }
    }

    void ReadPositions(int count, int delayMs) {
        int x, y;
        for (int i = 0; i < count; i++) {
            mouse_->GetPosition(x, y);
            printf("X, Y: (%i, %i)\n", x, y);
            if (!WaitOrAbort(delayMs)) return;
        }
    }

    // Checked for abort and max runtime between every step, since a sequence can
    // run much longer than a single action. maxRuntimeMs is clamped to kHardMaxRuntimeMs
    // so a macro can't declare its way past the engine's own ceiling.
    void RunSequence(const std::vector<Action>& actions, int repeatCount, int maxRuntimeMs = kHardMaxRuntimeMs) {
        int  runtimeCapMs = (std::min)(maxRuntimeMs, kHardMaxRuntimeMs);
        auto startedAt    = std::chrono::steady_clock::now();

        for (int rep = 0; rep < repeatCount; rep++) {
            for (const Action& action : actions) {
                if (RuntimeExceeded(startedAt, runtimeCapMs)) return;

                switch (action.type) {
                case ActionType::MoveTo:
                    mouse_->MoveTo(action.x, action.y);
                    logger_.Log("MoveTo " + std::to_string(action.x) + "," + std::to_string(action.y));
                    break;
                case ActionType::LeftClick:
                    mouse_->LeftClick();
                    logger_.Log("LeftClick");
                    break;
                case ActionType::RightClick:
                    mouse_->RightClick();
                    logger_.Log("RightClick");
                    break;
                case ActionType::Type:
                    logger_.Log("Type \"" + action.text + "\"");
                    for (char c : action.text) {
                        keyboard_->PressChar(c);
                        mouse_->Wait(RateLimited(action.ms));
                        if (CheckAbort() || RuntimeExceeded(startedAt, runtimeCapMs)) return;
                    }
                    continue; // abort/runtime already checked per character
                case ActionType::Wait:
                    mouse_->Wait(RateLimited(action.ms));
                    logger_.Log("Wait " + std::to_string(action.ms) + "ms");
                    break;
                }
                if (CheckAbort()) return;
            }
        }
    }

private:
    // Floor under any requested delay, so a macro can't flood input with near-zero waits.
    static constexpr int kMinDelayMs = 5;

    IMouseInput*    mouse_;
    IKeyboardInput* keyboard_;
    Logger          logger_;

    bool CheckAbort() {
        if (keyboard_->AbortRequested()) {
            std::cout << "Stopped.\n";
            logger_.Log("Stopped: aborted");
            return true;
        }
        return false;
    }

    bool WaitOrAbort(int delayMs) {
        mouse_->Wait(delayMs);
        return !CheckAbort();
    }

    int RateLimited(int ms) {
        return (std::max)(ms, kMinDelayMs);
    }

    bool RuntimeExceeded(const std::chrono::steady_clock::time_point& startedAt, int capMs) {
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startedAt).count();
        if (elapsedMs < capMs) return false;

        std::cout << "Stopped: max runtime exceeded.\n";
        logger_.Log("Stopped: max runtime exceeded");
        return true;
    }
};
