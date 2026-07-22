#pragma once

#include <iostream>
#include <vector>
#include "InputDevice.h"
#include "Action.h"

class Egg {
public:
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

    // Checked for abort between every step, since a sequence can run much longer than a single action.
    void RunSequence(const std::vector<Action>& actions, int repeatCount) {
        for (int rep = 0; rep < repeatCount; rep++) {
            for (const Action& action : actions) {
                switch (action.type) {
                case ActionType::MoveTo:
                    mouse_->MoveTo(action.x, action.y);
                    break;
                case ActionType::LeftClick:
                    mouse_->LeftClick();
                    break;
                case ActionType::RightClick:
                    mouse_->RightClick();
                    break;
                case ActionType::Type:
                    for (char c : action.text) {
                        keyboard_->PressChar(c);
                        mouse_->Wait(action.ms);
                        if (CheckAbort()) return;
                    }
                    continue; // abort already checked per character
                case ActionType::Wait:
                    mouse_->Wait(action.ms);
                    break;
                }
                if (CheckAbort()) return;
            }
        }
    }

private:
    IMouseInput*    mouse_;
    IKeyboardInput* keyboard_;

    bool CheckAbort() {
        if (keyboard_->AbortRequested()) {
            std::cout << "Stopped.\n";
            return true;
        }
        return false;
    }

    bool WaitOrAbort(int delayMs) {
        mouse_->Wait(delayMs);
        return !CheckAbort();
    }
};
