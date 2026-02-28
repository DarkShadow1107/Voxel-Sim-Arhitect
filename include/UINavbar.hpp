#pragma once
// ---------------------------------------------------------------------------
// UINavbar.hpp — Enhanced panel descriptor and group types for the redesigned
// top navbar / window-tab-bar in GUIManager.
// ---------------------------------------------------------------------------

#include "UIToolbar.hpp"  // base PanelDesc

#include <cstddef>

// A named group of panels that share a colour theme in the toolbar.
struct PanelGroup {
    const char* name;          // group label (e.g. "WORLD", "DESIGN")
    float       r, g, b;       // group colour
    PanelDesc*  panels;        // pointer to array
    int         count;         // number of panels in array
};
