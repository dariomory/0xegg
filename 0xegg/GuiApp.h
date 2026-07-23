#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <filesystem>
#include <d3d11.h>
#include <commdlg.h>
#include "imgui.h"
#include "WindowsInput.h"
#include "Humanizer.h"
#include "Egg.h"
#include "Action.h"
#include "MacroLoader.h"
#include "InputRecorder.h"
#include "IconTexture.h"

static constexpr int   kIconResourceId  = 2;
static constexpr float kTitleBarHeight  = 48.0f;
static constexpr float kStatusBarHeight = 40.0f;
static constexpr ImGuiWindowFlags kPanelFlags =
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;

class GuiApp {
public:
    explicit GuiApp(ID3D11Device* device) {
        icon_.LoadFromResource(device, kIconResourceId);
        std::filesystem::create_directories("macros");
        RefreshLibrary();
    }

    // Blocks until any in-flight run finishes (or the user aborts it with ESC)
    // rather than leaving a detached thread referencing a destroyed GuiApp.
    ~GuiApp() {
        recorder_.Stop();
        if (runThread_.joinable()) runThread_.join();
    }

    void Draw() {
        DrawTitleBar();
        DrawChatSidebar();
        DrawLibraryPanel();
        DrawRecorderPanel();
        if (showActions_) DrawActionsPanel();
        DrawStatusBar();
    }

    bool WantsClose() const { return wantsClose_; }

    // One-shot: true at most once per minimize-button click, so the caller
    // doesn't re-issue SW_MINIMIZE every frame while minimized.
    bool ConsumeMinimizeRequest() {
        bool result = wantsMinimize_;
        wantsMinimize_ = false;
        return result;
    }

    // Client-space rect spanning the minimize+close buttons, so WndProc's
    // WM_NCHITTEST can exclude them from the draggable title-bar region.
    void GetCloseButtonRect(ImVec2& outMin, ImVec2& outMax) const {
        outMin = closeButtonMin_;
        outMax = closeButtonMax_;
    }

private:
    WindowsMouse    winMouse_;
    WindowsKeyboard winKeyboard_;
    IconTexture     icon_;

    bool   wantsClose_    = false;
    bool   wantsMinimize_ = false;
    bool   showActions_   = false;
    ImVec2 closeButtonMin_{0, 0};
    ImVec2 closeButtonMax_{0, 0};

    std::thread       runThread_;
    std::atomic<bool> running_{false};

    int         humanizationIndex_ = 0;
    const char* humanizationNames_[3] = { "Off", "Subtle", "Human" };

    int  clickCount_    = 5;
    int  clickDelayMs_  = 200;
    int  moveX_ = 0, moveY_ = 0;
    int  moveCount_     = 1;
    int  moveDelayMs_   = 200;
    char keyChar_[2]    = "a";
    int  keyCount_      = 5;
    int  keyDelayMs_    = 200;

    std::vector<Action> sequence_;
    int  sequenceRepeat_ = 1;
    int  seqStepType_    = 0;
    int  seqX_ = 0, seqY_ = 0;
    char seqText_[256]   = "";
    int  seqMs_          = 200;
    bool seqPressEnter_  = false;

    std::vector<std::string> macroFiles_;
    std::string loadPathBuf_;
    std::string lastError_;

    InputRecorder recorder_;
    char          recordNameBuf_[128] = "recorded";

    std::string HumanizationName() const {
        return humanizationIndex_ == 1 ? "subtle" : humanizationIndex_ == 2 ? "human" : "off";
    }

    bool RunGuard() {
        if (running_.load()) return false;
        if (runThread_.joinable()) runThread_.join();
        return true;
    }

    void RunInBackground(std::function<void(Egg&)> work) {
        if (!RunGuard()) return;
        running_.store(true);
        HumanizationConfig config = HumanizationPreset(HumanizationName());
        runThread_ = std::thread([this, work, config]() {
            Humanizer humanizer(&winMouse_, &winKeyboard_, config);
            Egg       egg(&humanizer, &humanizer);
            work(egg);
            running_.store(false);
        });
    }

    // GetOpenFileNameW returns the path in the system codepage; LoadMacroFromFile's
    // std::ifstream expects the same, so this converts via CP_ACP, not CP_UTF8.
    std::string OpenMacroFilePicker() {
        wchar_t path[MAX_PATH] = L"";
        OPENFILENAMEW ofn   = { sizeof(ofn) };
        ofn.lpstrFilter     = L"Macro JSON (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile       = path;
        ofn.nMaxFile        = MAX_PATH;
        ofn.lpstrInitialDir = L"macros";
        ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (!GetOpenFileNameW(&ofn)) return "";

        int size = WideCharToMultiByte(CP_ACP, 0, path, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return "";
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_ACP, 0, path, -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    void RunMacroFile(const std::string& path) {
        try {
            Macro macro = LoadMacroFromFile(path);
            if (macro.actions.empty()) {
                lastError_ = "Macro has no actions.";
                return;
            }
            lastError_ = "";
            if (!RunGuard()) return;
            running_.store(true);
            runThread_ = std::thread([this, macro]() {
                HumanizationConfig config = HumanizationPreset(macro.humanization);
                Humanizer          humanizer(&winMouse_, &winKeyboard_, config);
                Egg                egg(&humanizer, &humanizer);
                egg.RunSequence(macro.actions, macro.repeat, macro.maxRuntimeMs);
                running_.store(false);
            });
        } catch (const std::exception& e) {
            lastError_ = e.what();
        }
    }

    void RefreshLibrary() {
        macroFiles_.clear();
        for (auto& entry : std::filesystem::directory_iterator("macros")) {
            if (entry.path().extension() == ".json") macroFiles_.push_back(entry.path().string());
        }
    }

    // Pinned, non-movable strip standing in for the native title bar removed in
    // gui_main.cpp (WS_POPUP has none). Its own drag/resize is handled there via
    // WM_NCHITTEST, using GetCloseButtonRect() to keep the close button clickable.
    void DrawTitleBar() {
        float width = ImGui::GetIO().DisplaySize.x;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, kTitleBarHeight));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        // More opaque/saturated than the panels below, so this reads as a distinct bar.
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.88f, 0.88f, 0.88f, 0.98f));
        ImGui::Begin("##TitleBar", nullptr, flags);

        const float iconSize   = 22.0f;
        const float buttonSize = 28.0f;
        const float sidePad    = 16.0f;
        const float buttonGap  = 6.0f;

        ImGui::SetCursorPos(ImVec2(sidePad, (kTitleBarHeight - iconSize) * 0.5f));
        if (icon_.IsLoaded()) ImGui::Image(icon_.TexRef(), ImVec2(iconSize, iconSize));

        float textX = sidePad + (icon_.IsLoaded() ? iconSize + 10.0f : 0.0f);
        ImGui::SetCursorPos(ImVec2(textX, (kTitleBarHeight - ImGui::GetTextLineHeight()) * 0.5f));
        ImGui::Text("0xegg");

        float closeX = width - buttonSize - sidePad;
        float minX   = closeX - buttonSize - buttonGap;
        float btnY   = (kTitleBarHeight - buttonSize) * 0.5f;

        ImGui::SetCursorPos(ImVec2(minX, btnY));
        if (ImGui::Button("-", ImVec2(buttonSize, buttonSize))) wantsMinimize_ = true;
        ImVec2 controlsMin = ImGui::GetItemRectMin();

        ImGui::SetCursorPos(ImVec2(closeX, btnY));
        if (ImGui::Button("X", ImVec2(buttonSize, buttonSize))) wantsClose_ = true;
        ImVec2 controlsMax = ImGui::GetItemRectMax();

        // Combined rect spanning both buttons, excluded from the draggable region.
        closeButtonMin_ = controlsMin;
        closeButtonMax_ = controlsMax;

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    // Pinned to the bottom, mirroring the title bar's fixed-strip approach.
    // Always-visible cursor readout plus the toggle for the Actions overlay,
    // which now floats on demand instead of occupying a permanent column.
    void DrawStatusBar() {
        float width  = ImGui::GetIO().DisplaySize.x;
        float height = ImGui::GetIO().DisplaySize.y;

        ImGui::SetNextWindowPos(ImVec2(0, height - kStatusBarHeight));
        ImGui::SetNextWindowSize(ImVec2(width, kStatusBarHeight));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.88f, 0.88f, 0.88f, 0.98f));
        ImGui::Begin("##StatusBar", nullptr, flags);

        int mx, my;
        winMouse_.GetPosition(mx, my);
        ImGui::SetCursorPosY((kStatusBarHeight - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::Text("Cursor: (%d, %d)", mx, my);

        const float buttonWidth = 90.0f;
        ImGui::SameLine(width - buttonWidth - 32.0f);
        ImGui::SetCursorPosY((kStatusBarHeight - ImGui::GetFrameHeight()) * 0.5f);
        if (ImGui::Button(showActions_ ? "Hide Actions" : "Actions", ImVec2(buttonWidth, 0))) {
            showActions_ = !showActions_;
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    void DrawActionsPanel() {
        ImGui::SetNextWindowPos(ImVec2(10, 56), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(430, 660), ImGuiCond_Always);
        ImGui::Begin("Actions", nullptr, kPanelFlags);

        ImGui::Combo("Humanization", &humanizationIndex_, humanizationNames_, 3);
        if (running_.load()) {
            ImGui::TextColored(ImVec4(1, 0.7f, 0.2f, 1), "Running... press ESC to stop.");
        }
        ImGui::Separator();

        ImGui::BeginDisabled(running_.load());

        ImGui::Text("Click");
        ImGui::InputInt("Count##click", &clickCount_);
        ImGui::InputInt("Delay ms##click", &clickDelayMs_);
        if (ImGui::Button("Left Click")) {
            int c = clickCount_, d = clickDelayMs_;
            RunInBackground([c, d](Egg& egg) { egg.RepeatLeftClick(c, d); });
        }
        ImGui::SameLine();
        if (ImGui::Button("Right Click")) {
            int c = clickCount_, d = clickDelayMs_;
            RunInBackground([c, d](Egg& egg) { egg.RepeatRightClick(c, d); });
        }

        ImGui::Separator();
        ImGui::Text("Move To");
        ImGui::InputInt("X##move", &moveX_);
        ImGui::InputInt("Y##move", &moveY_);
        ImGui::InputInt("Count##move", &moveCount_);
        ImGui::InputInt("Delay ms##move", &moveDelayMs_);
        if (ImGui::Button("Move")) {
            int x = moveX_, y = moveY_, c = moveCount_, d = moveDelayMs_;
            RunInBackground([x, y, c, d](Egg& egg) { egg.RepeatMoveTo(x, y, c, d); });
        }

        ImGui::Separator();
        ImGui::Text("Key Press");
        ImGui::InputText("Key##press", keyChar_, sizeof(keyChar_));
        ImGui::InputInt("Count##key", &keyCount_);
        ImGui::InputInt("Delay ms##key", &keyDelayMs_);
        if (ImGui::Button("Press Key") && keyChar_[0] != '\0') {
            char k = keyChar_[0];
            int  c = keyCount_, d = keyDelayMs_;
            RunInBackground([k, c, d](Egg& egg) { egg.RepeatKeyPress(k, c, d); });
        }

        ImGui::Separator();
        ImGui::Text("Sequence (%zu step(s))", sequence_.size());
        const char* stepTypes[] = { "Move", "Left Click", "Right Click", "Type", "Wait" };
        ImGui::Combo("Step type", &seqStepType_, stepTypes, 5);
        if (seqStepType_ == 0) {
            ImGui::InputInt("X##seq", &seqX_);
            ImGui::InputInt("Y##seq", &seqY_);
        }
        if (seqStepType_ == 3) {
            ImGui::InputText("Text##seq", seqText_, sizeof(seqText_));
            ImGui::InputInt("Ms/char##seq", &seqMs_);
            ImGui::Checkbox("Press Enter##seq", &seqPressEnter_);
        }
        if (seqStepType_ == 4) ImGui::InputInt("Wait ms##seq", &seqMs_);

        if (ImGui::Button("Add Step")) {
            Action a;
            switch (seqStepType_) {
            case 0: a.type = ActionType::MoveTo; a.x = seqX_; a.y = seqY_; break;
            case 1: a.type = ActionType::LeftClick; break;
            case 2: a.type = ActionType::RightClick; break;
            case 3:
                a.type = ActionType::Type;
                a.text = seqText_;
                a.ms   = seqMs_;
                if (seqPressEnter_) a.text += '\r';
                break;
            case 4: a.type = ActionType::Wait; a.ms = seqMs_; break;
            }
            sequence_.push_back(a);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Steps")) sequence_.clear();

        ImGui::InputInt("Repeat##seq", &sequenceRepeat_);
        if (ImGui::Button("Run Sequence") && !sequence_.empty()) {
            std::vector<Action> steps  = sequence_;
            int                 repeat = sequenceRepeat_;
            RunInBackground([steps, repeat](Egg& egg) { egg.RunSequence(steps, repeat); });
        }

        ImGui::EndDisabled();
        ImGui::End();
    }

    void DrawLibraryPanel() {
        ImGui::SetNextWindowPos(ImVec2(450, 56), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(430, 280), ImGuiCond_Always);
        ImGui::Begin("Macro Library", nullptr, kPanelFlags);

        ImGui::BeginDisabled(running_.load());

        if (!loadPathBuf_.empty()) {
            ImGui::TextDisabled("%s", loadPathBuf_.c_str());
        }
        if (ImGui::Button("Browse & Run...")) {
            std::string picked = OpenMacroFilePicker();
            if (!picked.empty()) {
                loadPathBuf_ = picked;
                RunMacroFile(picked);
            }
        }

        if (!lastError_.empty()) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Error: %s", lastError_.c_str());
        }

        ImGui::Separator();

        bool invalidated = false;
        for (auto& file : macroFiles_) {
            ImGui::PushID(file.c_str());
            ImGui::Text("%s", file.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Run")) RunMacroFile(file);
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) {
                std::filesystem::remove(file);
                invalidated = true;
            }
            ImGui::PopID();
            if (invalidated) break; // vector below is about to be repopulated
        }
        if (invalidated) RefreshLibrary();

        ImGui::EndDisabled();
        ImGui::End();
    }

    void DrawRecorderPanel() {
        ImGui::SetNextWindowPos(ImVec2(450, 346), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(430, 370), ImGuiCond_Always);
        ImGui::Begin("Recorder", nullptr, kPanelFlags);

        ImGui::TextWrapped(
            "Records clicks and keystrokes system-wide. The click that stops "
            "recording is captured too - delete it from the saved file if needed.");

        if (!recorder_.IsRecording()) {
            ImGui::BeginDisabled(running_.load());
            if (ImGui::Button("Start Recording")) recorder_.Start();
            ImGui::EndDisabled();
        } else {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Recording... (%zu actions)", recorder_.ActionCount());
            if (ImGui::Button("Stop Recording")) recorder_.Stop();
        }

        if (!recorder_.IsRecording() && recorder_.ActionCount() > 0) {
            ImGui::InputText("Save as", recordNameBuf_, sizeof(recordNameBuf_));
            if (ImGui::Button("Save to Library") && recordNameBuf_[0] != '\0') {
                Macro macro;
                macro.name    = recordNameBuf_;
                macro.actions = recorder_.TakeActions();
                try {
                    SaveMacroToFile(macro, "macros/" + std::string(recordNameBuf_) + ".json");
                    RefreshLibrary();
                } catch (const std::exception& e) {
                    lastError_ = e.what();
                }
            }
        }

        ImGui::End();
    }

    void DrawChatSidebar() {
        ImGui::SetNextWindowPos(ImVec2(10, 56), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(430, 660), ImGuiCond_Always);
        ImGui::Begin("AI Assistant", nullptr, kPanelFlags);

        static char chatBuf[256] = "";
        ImGui::BeginDisabled(true);
        ImGui::InputText("##chat", chatBuf, sizeof(chatBuf));
        ImGui::SameLine();
        ImGui::Button("Send");
        ImGui::EndDisabled();
        ImGui::TextDisabled("Coming soon.");

        ImGui::End();
    }
};
