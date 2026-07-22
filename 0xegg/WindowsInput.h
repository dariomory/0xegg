#pragma once

#include <windows.h>
#include "InputDevice.h"

class WindowsMouse : public IMouseInput {
public:
    void LeftClick() override {
        INPUT input = { 0 };
        input.type          = INPUT_MOUSE;
        input.mi.dwFlags    = MOUSEEVENTF_LEFTDOWN;
        ::SendInput(1, &input, sizeof(input));

        ::ZeroMemory(&input, sizeof(input));
        input.type          = INPUT_MOUSE;
        input.mi.dwFlags    = MOUSEEVENTF_LEFTUP;
        ::SendInput(1, &input, sizeof(input));
    }

    void RightClick() override {
        INPUT input = { 0 };
        input.type          = INPUT_MOUSE;
        input.mi.dwFlags    = MOUSEEVENTF_RIGHTDOWN;
        ::SendInput(1, &input, sizeof(input));

        ::ZeroMemory(&input, sizeof(input));
        input.type          = INPUT_MOUSE;
        input.mi.dwFlags    = MOUSEEVENTF_RIGHTUP;
        ::SendInput(1, &input, sizeof(input));
    }

    void MoveTo(int x, int y) override {
        double screenW = ::GetSystemMetrics(SM_CXSCREEN);
        double screenH = ::GetSystemMetrics(SM_CYSCREEN);

        INPUT input = { 0 };
        input.type          = INPUT_MOUSE;
        input.mi.dwFlags    = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
        input.mi.dx         = static_cast<LONG>(x * (65535.0 / screenW));
        input.mi.dy         = static_cast<LONG>(y * (65535.0 / screenH));
        ::SendInput(1, &input, sizeof(input));
    }

    void GetPosition(int& x, int& y) override {
        POINT p;
        ::GetCursorPos(&p);
        x = p.x;
        y = p.y;
    }

    void Wait(int ms) override {
        ::Sleep(ms);
    }
};

class WindowsKeyboard : public IKeyboardInput {
public:
    // Chars needing a modifier (e.g. 'A') map to the unshifted key; the shift bit VkKeyScanA returns is ignored.
    void PressChar(char c) override {
        SHORT vk = ::VkKeyScanA(c);
        if (vk == -1) return;

        INPUT input = { 0 };
        input.type          = INPUT_KEYBOARD;
        input.ki.wVk        = static_cast<WORD>(vk & 0xFF);
        ::SendInput(1, &input, sizeof(input));

        input.ki.dwFlags    = KEYEVENTF_KEYUP;
        ::SendInput(1, &input, sizeof(input));
    }

    // Polls physical key state rather than console input, so it works while another window has focus.
    bool AbortRequested() override {
        return (::GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    }
};
