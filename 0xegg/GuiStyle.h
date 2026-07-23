#pragma once

#include "imgui.h"

// A light, minimal "frosted glass" look: white and near-white translucent
// panels, rounded corners, thin borders. Purely grayscale - no color accents.
// Not real OS-level blur - just ImGui's own transparency and color system.
inline void ApplyGlassStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;

    style.WindowPadding = ImVec2(14, 14);
    style.FramePadding  = ImVec2(8, 6);
    style.ItemSpacing    = ImVec2(8, 8);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]                  = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    c[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    c[ImGuiCol_WindowBg]              = ImVec4(0.98f, 0.98f, 0.98f, 0.92f);
    c[ImGuiCol_ChildBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    c[ImGuiCol_PopupBg]               = ImVec4(0.99f, 0.99f, 0.99f, 0.98f);
    c[ImGuiCol_Border]                = ImVec4(0.85f, 0.85f, 0.85f, 0.60f);
    c[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    c[ImGuiCol_FrameBg]               = ImVec4(0.93f, 0.93f, 0.93f, 0.85f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.88f, 0.88f, 0.88f, 0.90f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 0.95f);

    c[ImGuiCol_TitleBg]               = ImVec4(0.95f, 0.95f, 0.95f, 0.95f);
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.90f, 0.90f, 0.90f, 0.98f);
    c[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.95f, 0.95f, 0.95f, 0.75f);
    c[ImGuiCol_MenuBarBg]             = ImVec4(0.95f, 0.95f, 0.95f, 0.95f);

    c[ImGuiCol_ScrollbarBg]           = ImVec4(0.95f, 0.95f, 0.95f, 0.60f);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.80f, 0.80f, 0.80f, 0.80f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.70f, 0.70f, 0.70f, 0.90f);
    c[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.60f, 0.60f, 0.60f, 0.95f);

    c[ImGuiCol_CheckMark]             = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    c[ImGuiCol_SliderGrab]            = ImVec4(0.55f, 0.55f, 0.55f, 0.90f);
    c[ImGuiCol_SliderGrabActive]      = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);

    c[ImGuiCol_Button]                = ImVec4(0.92f, 0.92f, 0.92f, 0.90f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.85f, 0.85f, 0.85f, 0.95f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);

    c[ImGuiCol_Header]                = ImVec4(0.88f, 0.88f, 0.88f, 0.80f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.82f, 0.82f, 0.82f, 0.90f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.72f, 0.72f, 0.72f, 1.00f);

    c[ImGuiCol_Separator]             = ImVec4(0.85f, 0.85f, 0.85f, 0.60f);
    c[ImGuiCol_SeparatorHovered]      = ImVec4(0.65f, 0.65f, 0.65f, 0.70f);
    c[ImGuiCol_SeparatorActive]       = ImVec4(0.50f, 0.50f, 0.50f, 0.90f);

    c[ImGuiCol_ResizeGrip]            = ImVec4(0.85f, 0.85f, 0.85f, 0.30f);
    c[ImGuiCol_ResizeGripHovered]     = ImVec4(0.70f, 0.70f, 0.70f, 0.60f);
    c[ImGuiCol_ResizeGripActive]      = ImVec4(0.55f, 0.55f, 0.55f, 0.90f);

    c[ImGuiCol_Tab]                   = ImVec4(0.92f, 0.92f, 0.92f, 0.80f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.85f, 0.85f, 0.85f, 0.95f);
    c[ImGuiCol_TabSelected]           = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    c[ImGuiCol_TabDimmed]             = ImVec4(0.93f, 0.93f, 0.93f, 0.60f);
    c[ImGuiCol_TabDimmedSelected]     = ImVec4(0.85f, 0.85f, 0.85f, 0.90f);

    c[ImGuiCol_TextSelectedBg]        = ImVec4(0.50f, 0.50f, 0.50f, 0.30f);
    c[ImGuiCol_TextLink]              = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
}
