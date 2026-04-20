#include <iostream>
#include <windows.h>

class IMouseInput {
public:
    virtual ~IMouseInput() = default;

    virtual void LeftClick()                          = 0;
    virtual void RightClick()                         = 0;
    virtual void MoveTo(int x, int y)                 = 0;
    virtual void GetPosition(int& x, int& y)          = 0;
    virtual void Wait(int ms)                         = 0;
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

class AutoClicker {
public:
    explicit AutoClicker(IMouseInput* mouse) : mouse_(mouse) {}

    void RepeatLeftClick(int count, int delayMs) {
        for (int i = 0; i < count; i++) {
            mouse_->LeftClick();
            mouse_->Wait(delayMs);
        }
    }

    void RepeatRightClick(int count, int delayMs) {
        for (int i = 0; i < count; i++) {
            mouse_->RightClick();
            mouse_->Wait(delayMs);
        }
    }

    void RepeatMoveTo(int x, int y, int count, int delayMs) {
        for (int i = 0; i < count; i++) {
            mouse_->MoveTo(x, y);
            mouse_->Wait(delayMs);
        }
    }

    void ReadPositions(int count, int delayMs) {
        int x, y;
        for (int i = 0; i < count; i++) {
            CountDown();
            mouse_->GetPosition(x, y);
            printf("X, Y: (%i, %i)\n", x, y);
            mouse_->Wait(delayMs);
        }
    }

private:
    IMouseInput* mouse_;

    void CountDown() {
        std::cout << "Starting in 3 seconds...\n";
        mouse_->Wait(1000);
        std::cout << "2..\n";
        mouse_->Wait(1000);
        std::cout << "1..\n";
        mouse_->Wait(1000);
    }
};

void PrintSplash() {
    std::cout << "----------------\n";
    std::cout << "AUTOCLICKER v0.1\n";
    std::cout << "----------------\n";
}

int AskMethod() {
    int choice = 0;
    std::cout << "0) Exit\n";
    std::cout << "1) Position\n";
    std::cout << "2) Right\n";
    std::cout << "3) Left\n";
    std::cout << "4) Move to\n";
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

void CountDownGlobal(IMouseInput* mouse) {
    std::cout << "Starting in 3 seconds...\n";
    mouse->Wait(1000);
    std::cout << "2..\n";
    mouse->Wait(1000);
    std::cout << "1..\n";
    mouse->Wait(1000);
}

int main() {
    SetConsoleTitle("AutoClicker v0.1");
    const char *actions[5] = {"", "reads", "clicks", "clicks", "moves"};

    WindowsMouse winMouse;
    AutoClicker  clicker(&winMouse);

    PrintSplash();

    int method   = 0;
    int count    = 0;
    int delayMs  = 0;

    while (true) {
        method = AskMethod();

        if (method == 0) break;
        if (method >= 1 && method <= 4) {
            AskClickParams(actions[method], count, delayMs);
            CountDownGlobal(&winMouse);
        }

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
        case 4: {
            int x, y;
            std::cout << "Enter X position: ";  std::cin >> x;
            std::cout << "Enter Y position: ";  std::cin >> y;
            clicker.RepeatMoveTo(x, y, count, delayMs);
            break;
        }
        default:
            std::cout << "Invalid option.\n";
            break;
        }
    }

    return 0;
}