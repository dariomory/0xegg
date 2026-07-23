#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <windows.h>
#include "Action.h"

// Captures live clicks, mouse movement, and keystrokes via low-level Win32
// hooks into a vector<Action>, as a recording fallback for users who don't want to
// hand-write or describe a macro. Only one instance can record at a time
// (SetWindowsHookEx callbacks must be free functions, so a single static
// pointer routes hook events to whichever instance is currently active).
//
// Known limitations: the click that stops recording is itself captured as
// the final action (the hook fires before the app's own button handles the
// click); and only plain unmodified characters translate reliably (accents/
// dead keys are not handled).
class InputRecorder {
public:
    ~InputRecorder() { Stop(); }

    void Start() {
        if (recording_) return;
        actions_.clear();
        pendingText_.clear();
        lastEventTime_      = std::chrono::steady_clock::now();
        lastMoveSampleTime_ = lastEventTime_;

        activeInstance_ = this;
        mouseHook_    = SetWindowsHookExW(WH_MOUSE_LL, &MouseProc, GetModuleHandleW(nullptr), 0);
        keyboardHook_ = SetWindowsHookExW(WH_KEYBOARD_LL, &KeyboardProc, GetModuleHandleW(nullptr), 0);
        recording_ = true;
    }

    void Stop() {
        if (!recording_) return;
        FlushPendingText();
        if (mouseHook_)    { UnhookWindowsHookEx(mouseHook_);    mouseHook_    = nullptr; }
        if (keyboardHook_) { UnhookWindowsHookEx(keyboardHook_); keyboardHook_ = nullptr; }
        activeInstance_ = nullptr;
        recording_ = false;
    }

    bool   IsRecording() const { return recording_; }
    size_t ActionCount() const { return actions_.size(); }

    std::vector<Action> TakeActions() {
        std::vector<Action> result = std::move(actions_);
        actions_.clear();
        return result;
    }

private:
    static InputRecorder* activeInstance_;

    bool  recording_    = false;
    HHOOK mouseHook_    = nullptr;
    HHOOK keyboardHook_ = nullptr;

    static constexpr int kMoveSampleIntervalMs = 50; // throttle: ~20 samples/sec, not every pixel

    std::vector<Action>                   actions_;
    std::string                           pendingText_;
    std::chrono::steady_clock::time_point lastEventTime_;
    std::chrono::steady_clock::time_point lastMoveSampleTime_;

    // Emits a Wait action for the gap since the last event, so replay reproduces timing.
    void RecordGap() {
        auto now = std::chrono::steady_clock::now();
        int  gapMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEventTime_).count());
        lastEventTime_ = now;

        if (gapMs > 20) {
            Action wait;
            wait.type = ActionType::Wait;
            wait.ms   = gapMs;
            actions_.push_back(wait);
        }
    }

    void FlushPendingText() {
        if (pendingText_.empty()) return;
        Action type;
        type.type = ActionType::Type;
        type.text = pendingText_;
        type.ms   = 30;
        actions_.push_back(type);
        pendingText_.clear();
    }

    void OnClick(ActionType clickType, int x, int y) {
        FlushPendingText();
        RecordGap();

        Action move;
        move.type = ActionType::MoveTo;
        move.x = x;
        move.y = y;
        actions_.push_back(move);

        Action click;
        click.type = clickType;
        actions_.push_back(click);
    }

    void OnChar(char c) {
        RecordGap();
        pendingText_ += c;
    }

    // Throttled: recording every WM_MOUSEMOVE would produce an unusably large
    // macro, so only a periodic sample of the path is kept.
    void OnMove(int x, int y) {
        auto now = std::chrono::steady_clock::now();
        int  sinceLastMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMoveSampleTime_).count());
        if (sinceLastMs < kMoveSampleIntervalMs) return;
        lastMoveSampleTime_ = now;

        FlushPendingText();
        RecordGap();

        Action move;
        move.type = ActionType::MoveTo;
        move.x = x;
        move.y = y;
        actions_.push_back(move);
    }

    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && activeInstance_) {
            auto* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            if (wParam == WM_LBUTTONDOWN) {
                activeInstance_->OnClick(ActionType::LeftClick, info->pt.x, info->pt.y);
            } else if (wParam == WM_RBUTTONDOWN) {
                activeInstance_->OnClick(ActionType::RightClick, info->pt.x, info->pt.y);
            } else if (wParam == WM_MOUSEMOVE) {
                activeInstance_->OnMove(info->pt.x, info->pt.y);
            }
        }
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && activeInstance_ && wParam == WM_KEYDOWN) {
            auto* info = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            BYTE  keyState[256];
            GetKeyboardState(keyState);
            WCHAR buf[2] = {};
            int   result = ToUnicode(info->vkCode, info->scanCode, keyState, buf, 2, 0);
            if (result == 1 && buf[0] < 128) {
                activeInstance_->OnChar(static_cast<char>(buf[0]));
            }
        }
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }
};

inline InputRecorder* InputRecorder::activeInstance_ = nullptr;
