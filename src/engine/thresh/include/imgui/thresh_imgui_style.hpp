/**
 * THRΞSH Editor — ImGui Theme
 *
 * Apply with: thresh::editor::ApplyTheme();
 * Fonts must be loaded separately — see SetupFonts().
 *
 * Designed for:
 *   - Minimal rounding (2px)
 *   - Borders on panels only, frameless inputs
 *   - Dark-first palette from brand guide
 */

#pragma once

#include <imgui.h>

namespace thresh::editor {

// ── Brand palette ──────────────────────────────────────────────────────
namespace palette {
    // Background stack
    constexpr ImVec4 BgDeep      = {0.024f, 0.031f, 0.047f, 1.0f};  // #06080C
    constexpr ImVec4 BgPanel     = {0.039f, 0.055f, 0.078f, 1.0f};  // #0A0E14
    constexpr ImVec4 BgSurface   = {0.059f, 0.082f, 0.125f, 1.0f};  // #0F1520
    constexpr ImVec4 BgElevated  = {0.078f, 0.110f, 0.157f, 1.0f};  // #141C28

    // Accent — Cyan
    constexpr ImVec4 Cyan        = {0.000f, 0.941f, 1.000f, 1.0f};  // #00F0FF
    constexpr ImVec4 CyanMuted   = {0.000f, 0.533f, 0.604f, 1.0f};  // #0088AA (approx)
    constexpr ImVec4 CyanSubtle  = {0.000f, 0.941f, 1.000f, 0.08f};
    constexpr ImVec4 CyanHover   = {0.000f, 0.941f, 1.000f, 0.12f};
    constexpr ImVec4 CyanActive  = {0.000f, 0.941f, 1.000f, 0.18f};

    // Accent — Magenta (error/alert)
    constexpr ImVec4 Magenta     = {1.000f, 0.176f, 0.416f, 1.0f};  // #FF2D6A

    // Accent — Amber (warning)
    constexpr ImVec4 Amber       = {1.000f, 0.722f, 0.000f, 1.0f};  // #FFB800

    // Accent — Green (success/active)
    constexpr ImVec4 Green       = {0.176f, 1.000f, 0.416f, 1.0f};  // #2DFF6A

    // Text
    constexpr ImVec4 TextPrimary   = {0.784f, 0.816f, 0.863f, 1.0f};  // #C8D0DC
    constexpr ImVec4 TextSecondary = {0.416f, 0.478f, 0.565f, 1.0f};  // #6A7A90
    constexpr ImVec4 TextDim       = {0.227f, 0.290f, 0.361f, 1.0f};  // #3A4A5C
    constexpr ImVec4 TextBright    = {0.910f, 0.929f, 0.961f, 1.0f};  // #E8EDF5

    // Borders
    constexpr ImVec4 BorderPanel = {0.000f, 0.941f, 1.000f, 0.06f};
    constexpr ImVec4 BorderActive = {0.000f, 0.941f, 1.000f, 0.25f};

    // Transparent
    constexpr ImVec4 Transparent = {0.0f, 0.0f, 0.0f, 0.0f};

} // namespace palette

// ── Theme application ──────────────────────────────────────────────────
inline void ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    using namespace palette;

    // ── Style variables ──

    // Rounding — minimal, 2px
    style.WindowRounding    = 2.0f;
    style.ChildRounding     = 2.0f;
    style.FrameRounding     = 2.0f;
    style.PopupRounding     = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 2.0f;

    // Borders — panels only, frameless inputs
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;  // Frameless inputs
    style.TabBorderSize     = 0.0f;

    // Spacing & padding
    style.WindowPadding     = {8.0f, 8.0f};
    style.FramePadding      = {6.0f, 3.0f};
    style.CellPadding       = {4.0f, 2.0f};
    style.ItemSpacing       = {6.0f, 4.0f};
    style.ItemInnerSpacing  = {4.0f, 4.0f};
    style.IndentSpacing     = 14.0f;
    style.ScrollbarSize     = 10.0f;
    style.GrabMinSize       = 8.0f;

    // Misc
    style.WindowTitleAlign    = {0.0f, 0.5f};
    style.SeparatorTextAlign  = {0.0f, 0.5f};
    style.WindowMenuButtonPosition = ImGuiDir_None; // Hide the collapse button

    // ── Colors ──

    // Text
    colors[ImGuiCol_Text]                  = TextPrimary;
    colors[ImGuiCol_TextDisabled]          = TextDim;

    // Windows
    colors[ImGuiCol_WindowBg]              = BgPanel;
    colors[ImGuiCol_ChildBg]               = Transparent;
    colors[ImGuiCol_PopupBg]               = BgElevated;

    // Borders
    colors[ImGuiCol_Border]                = BorderPanel;
    colors[ImGuiCol_BorderShadow]          = Transparent;

    // Frame (input backgrounds) — frameless feel
    colors[ImGuiCol_FrameBg]               = BgSurface;
    colors[ImGuiCol_FrameBgHovered]        = BgElevated;
    colors[ImGuiCol_FrameBgActive]         = BgElevated;

    // Title bar
    colors[ImGuiCol_TitleBg]               = BgDeep;
    colors[ImGuiCol_TitleBgActive]         = BgPanel;
    colors[ImGuiCol_TitleBgCollapsed]      = BgDeep;

    // Menu bar
    colors[ImGuiCol_MenuBarBg]             = BgPanel;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]           = Transparent;
    colors[ImGuiCol_ScrollbarGrab]         = BgElevated;
    colors[ImGuiCol_ScrollbarGrabHovered]  = TextDim;
    colors[ImGuiCol_ScrollbarGrabActive]   = TextSecondary;

    // Checkmark
    colors[ImGuiCol_CheckMark]             = Cyan;

    // Slider
    colors[ImGuiCol_SliderGrab]            = CyanMuted;
    colors[ImGuiCol_SliderGrabActive]      = Cyan;

    // Button
    colors[ImGuiCol_Button]                = CyanSubtle;
    colors[ImGuiCol_ButtonHovered]         = CyanHover;
    colors[ImGuiCol_ButtonActive]          = CyanActive;

    // Header (tree nodes, collapsing headers, selectable)
    colors[ImGuiCol_Header]                = CyanSubtle;
    colors[ImGuiCol_HeaderHovered]         = CyanHover;
    colors[ImGuiCol_HeaderActive]          = CyanActive;

    // Separator
    colors[ImGuiCol_Separator]             = BorderPanel;
    colors[ImGuiCol_SeparatorHovered]      = CyanMuted;
    colors[ImGuiCol_SeparatorActive]       = Cyan;

    // Resize grip
    colors[ImGuiCol_ResizeGrip]            = CyanSubtle;
    colors[ImGuiCol_ResizeGripHovered]     = CyanHover;
    colors[ImGuiCol_ResizeGripActive]      = CyanActive;

    // Tabs
    colors[ImGuiCol_Tab]                   = BgDeep;
    colors[ImGuiCol_TabHovered]            = CyanHover;
    colors[ImGuiCol_TabSelected]           = BgPanel;
    colors[ImGuiCol_TabSelectedOverline]   = CyanMuted;
    colors[ImGuiCol_TabDimmed]             = BgDeep;
    colors[ImGuiCol_TabDimmedSelected]     = BgPanel;
    colors[ImGuiCol_TabDimmedSelectedOverline] = {0.0f, 0.0f, 0.0f, 0.0f};

    // Docking
    colors[ImGuiCol_DockingPreview]        = CyanMuted;
    colors[ImGuiCol_DockingEmptyBg]        = BgDeep;

    // Plot
    colors[ImGuiCol_PlotLines]             = CyanMuted;
    colors[ImGuiCol_PlotLinesHovered]      = Cyan;
    colors[ImGuiCol_PlotHistogram]         = CyanMuted;
    colors[ImGuiCol_PlotHistogramHovered]  = Cyan;

    // Table
    colors[ImGuiCol_TableHeaderBg]         = BgSurface;
    colors[ImGuiCol_TableBorderStrong]     = BorderPanel;
    colors[ImGuiCol_TableBorderLight]      = {0.0f, 0.941f, 1.000f, 0.03f};
    colors[ImGuiCol_TableRowBg]            = Transparent;
    colors[ImGuiCol_TableRowBgAlt]         = {1.0f, 1.0f, 1.0f, 0.01f};

    // Text input
    colors[ImGuiCol_TextSelectedBg]        = CyanActive;

    // Drag-drop
    colors[ImGuiCol_DragDropTarget]        = Cyan;

    // Nav
    colors[ImGuiCol_NavCursor]             = Cyan;
    colors[ImGuiCol_NavWindowingHighlight] = {1.0f, 1.0f, 1.0f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]     = {0.0f, 0.0f, 0.0f, 0.50f};

    // Modal dim
    colors[ImGuiCol_ModalWindowDimBg]      = {0.0f, 0.0f, 0.0f, 0.60f};
}

// ── Font setup ─────────────────────────────────────────────────────────
//
// Call during initialization, before the first frame.
// Adjust paths to match your asset pipeline.
//
// Example:
//   thresh::editor::SetupFonts();
//
inline void SetupFonts() {
    ImGuiIO& io = ImGui::GetIO();

    // UI body — Rajdhani Medium, 15px
    // Good readability at small sizes, angular enough to feel technical
    io.Fonts->AddFontFromFileTTF(
        "res://fonts/Rajdhani-Medium.ttf", 15.0f
    );

    // Monospace — Share Tech Mono, 13px
    // Console, data fields, labels
    // Store the pointer if you need to push/pop it:
    //   ImFont* fontMono = io.Fonts->AddFontFromFileTTF(...);
    io.Fonts->AddFontFromFileTTF(
        "res://fonts/ShareTechMono-Regular.ttf", 13.0f
    );

    // Display — Orbitron Bold, 18px
    // Panel headers, splash, branding
    io.Fonts->AddFontFromFileTTF(
        "res://fonts/Orbitron-Bold.ttf", 18.0f
    );

    // Build the font atlas
    io.Fonts->Build();
}

} // namespace thresh::editor