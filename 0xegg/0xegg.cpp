#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include "InputDevice.h"
#include "WindowsInput.h"
#include "Humanizer.h"
#include "Action.h"
#include "Egg.h"
#include "MacroLoader.h"

#ifndef EGG_VERSION
#define EGG_VERSION "dev"
#endif

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
    std::cout << "6) Sequence\n";
    std::cout << "7) Load macro\n";
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

std::string AskHumanizationPreset() {
    std::cout << "Humanization:\n";
    std::cout << "0) Off    - exact, constant-speed, no jitter\n";
    std::cout << "1) Subtle - light jitter and timing variance\n";
    std::cout << "2) Human  - curved movement, jitter, variable timing\n";
    std::cout << "Enter number:\n> ";
    int choice = 0;
    std::cin >> choice;
    if (choice == 1) return "subtle";
    if (choice == 2) return "human";
    return "off";
}

// Stand-in for the JSON macro loader planned in issue #4 — both produce the
// same std::vector<Action> for Egg::RunSequence to execute.
std::vector<Action> BuildSequence() {
    std::vector<Action> actions;
    std::cout << "Build a sequence. Add steps one at a time; 0 finishes and runs it.\n";

    while (true) {
        std::cout << "\n1) Move to X,Y\n2) Left click\n3) Right click\n4) Type text\n5) Wait ms\n0) Done\n> ";
        int step = 0;
        std::cin >> step;

        if (step == 0) break;

        Action action;
        char   yn = 'n';
        switch (step) {
        case 1:
            action.type = ActionType::MoveTo;
            std::cout << "X: ";  std::cin >> action.x;
            std::cout << "Y: ";  std::cin >> action.y;
            break;
        case 2:
            action.type = ActionType::LeftClick;
            break;
        case 3:
            action.type = ActionType::RightClick;
            break;
        case 4:
            action.type = ActionType::Type;
            std::cin.ignore();
            std::cout << "Text: ";  std::getline(std::cin, action.text);
            std::cout << "Delay between keystrokes (ms): ";  std::cin >> action.ms;
            std::cout << "Press Enter afterwards? (y/n): ";  std::cin >> yn;
            if (yn == 'y' || yn == 'Y') action.text += '\r';
            break;
        case 5:
            action.type = ActionType::Wait;
            std::cout << "Wait (ms): ";  std::cin >> action.ms;
            break;
        default:
            std::cout << "Invalid step.\n";
            continue;
        }

        actions.push_back(action);
        std::cout << "Added. " << actions.size() << " step(s) so far.\n";
    }

    return actions;
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

    PrintSplash();

    // Always wrapped: with the "off" preset every Humanizer override is a plain
    // passthrough, so this adds no behavior change unless a preset is chosen.
    HumanizationConfig config = HumanizationPreset(AskHumanizationPreset());
    Humanizer humanizer(&winMouse, &winKeyboard, config);
    Egg       clicker(&humanizer, &humanizer);

    int method   = 0;
    int count    = 0;
    int delayMs  = 0;

    while (true) {
        method = AskMethod();

        if (method == 0) break;

        if (method == 6) {
            std::vector<Action> sequence = BuildSequence();
            if (sequence.empty()) {
                std::cout << "Empty sequence, nothing to run.\n";
                continue;
            }

            int repeatCount = 1;
            std::cout << "Repeat sequence how many times? ";  std::cin >> repeatCount;

            CountDown(&winMouse);
            std::cout << "Press ESC to stop.\n";
            clicker.RunSequence(sequence, repeatCount);
            continue;
        }

        if (method == 7) {
            std::cout << "Macro file path: ";
            std::string path;
            std::cin >> path;

            try {
                Macro macro = LoadMacroFromFile(path);
                if (macro.actions.empty()) {
                    std::cout << "Macro has no actions.\n";
                    continue;
                }

                HumanizationConfig macroConfig = HumanizationPreset(macro.humanization);
                Humanizer          macroHumanizer(&winMouse, &winKeyboard, macroConfig);
                Egg                macroClicker(&macroHumanizer, &macroHumanizer);

                std::cout << "Loaded \"" << macro.name << "\": " << macro.actions.size()
                          << " action(s), repeat " << macro.repeat << "x, humanization: "
                          << macro.humanization << "\n";

                CountDown(&winMouse);
                std::cout << "Press ESC to stop.\n";
                macroClicker.RunSequence(macro.actions, macro.repeat, macro.maxRuntimeMs);
            } catch (const std::exception& e) {
                std::cout << "Failed to load macro: " << e.what() << "\n";
            }
            continue;
        }

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
