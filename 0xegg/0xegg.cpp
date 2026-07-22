#include <iostream>
#include <windows.h>

#ifndef EGG_VERSION
#define EGG_VERSION "dev"
#endif

class IMouseInput {
public:
    virtual ~IMouseInput() = default;

    virtual void LeftClick()                          = 0;
    virtual void RightClick()                         = 0;
    virtual void MoveTo(int x, int y)                 = 0;
    virtual void GetPosition(int& x, int& y)          = 0;
    virtual void Wait(int ms)                         = 0;
};

class IKeyboardInput {
public:
    virtual ~IKeyboardInput() = default;

    virtual void PressChar(char c)                    = 0;
    virtual bool AbortRequested()                     = 0;
};

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
    // Only characters reachable without a modifier are sent; others are ignored.
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

    // Polls the physical key state, so it works while another window has focus.
    bool AbortRequested() override {
        return (::GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    }
};

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

private:
    IMouseInput*    mouse_;
    IKeyboardInput* keyboard_;

    // Returns false once the user asks to stop, ending the current run.
    bool WaitOrAbort(int delayMs) {
        mouse_->Wait(delayMs);
        if (keyboard_->AbortRequested()) {
            std::cout << "Stopped.\n";
            return false;
        }
        return true;
    }
};

void PrintSplash() {
    std::cout << "----------------\n";
    std::cout << "0xegg v" EGG_VERSION "\n";
    std::cout << "----------------\n";
}

int AskMethod() {
    int choice = 0;
    std::cout << "0) Exit\n";
    std::cout << "1) Position\n";
    std::cout << "2) Right\n";
    std::cout << "3) Left\n";
    std::cout << "4) Move to\n";
    std::cout << "5) Key press\n";
    std::cout << "Enter number:\n> ";
    std::cin >> choice;
    return choice;
}

void AskClickParams(const char* type, int& count, int& delayMs) {
    std::cout << "Number of "<< type << " : ";
    std::cin >> count;
    std::cout << "Wait between "<< type << " (ms): ";
    std::cin >> delayMs;
}

void CountDown(IMouseInput* mouse) {
    std::cout << "Starting in 3 seconds...\n";
    mouse->Wait(1000);
    std::cout << "2..\n";
    mouse->Wait(1000);
    std::cout << "1..\n";
    mouse->Wait(1000);
}

int main() {
    SetConsoleTitleA("0xegg v" EGG_VERSION);
    const char *actions[6] = {"", "reads", "clicks", "clicks", "moves", "presses"};

    WindowsMouse    winMouse;
    WindowsKeyboard winKeyboard;
    Egg  clicker(&winMouse, &winKeyboard);

    PrintSplash();

    int method   = 0;
    int count    = 0;
    int delayMs  = 0;

    while (true) {
        method = AskMethod();

        if (method == 0) break;
        if (method < 1 || method > 5) {
            std::cout << "Invalid option.\n";
            continue;
        }

        AskClickParams(actions[method], count, delayMs);

        int  x   = 0;
        int  y   = 0;
        char key = 0;
        if (method == 4) {
            std::cout << "Enter X position: ";  std::cin >> x;
            std::cout << "Enter Y position: ";  std::cin >> y;
        } else if (method == 5) {
            std::cout << "Enter key to press: ";  std::cin >> key;
        }

        CountDown(&winMouse);
        std::cout << "Press ESC to stop.\n";

        switch (method) {
        case 1:
            clicker.ReadPositions(count, delayMs);
            break;
        case 2:
            clicker.RepeatRightClick(count, delayMs);
            break;
        case 3:
            clicker.RepeatLeftClick(count, delayMs);
            break;
        case 4:
            clicker.RepeatMoveTo(x, y, count, delayMs);
            break;
        case 5:
            clicker.RepeatKeyPress(key, count, delayMs);
            break;
        }
    }

    return 0;
}