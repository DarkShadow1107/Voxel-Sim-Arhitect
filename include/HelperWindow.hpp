#pragma once
// ---------------------------------------------------------------------------
//  HelperWindow — standalone dockable help / reference panel.
//
//  TDD §4 spec: "HelperWindow.h / .cpp — Decoupled from GUIManager.
//  Provides a searchable, tabbed reference window for controls,
//  editor panels, tips, and version info."
//
//  Usage:
//    // In GUIManager init (no explicit init call needed)
//    m_helperWindow.open();       // programmatically open
//    m_helperWindow.show();       // call each frame; noop when closed
// ---------------------------------------------------------------------------

class HelperWindow {
public:
    // Request the window to open (e.g. called from the Help menu item).
    void open()  { m_open = true; }

    // Call every frame. Renders the window when open; noop otherwise.
    void show();

    bool isOpen() const { return m_open; }

private:
    bool m_open = false;

    // Active tab index (Controls / Editor / Tips / About)
    int  m_activeTab = 0;

    // Live search filter applied across all entries
    char m_search[128] = "";
};
