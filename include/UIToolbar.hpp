#pragma once

// ---------------------------------------------------------------------------
// UIToolbar.hpp — shared panel descriptor used by the quick-launch toolbar
// and the window tab bar in GUIManager.
// ---------------------------------------------------------------------------

struct PanelDesc {
    const char* label;    // short display name (quick-launch button + tab chip)
    const char* winName;  // ImGui window title string (used by SetWindowFocus)
    bool*       flag;     // pointer to the show-bool in main loop
    float r, g, b;        // category base colour (0-1)
};
