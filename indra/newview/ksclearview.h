/**
 * @file ksclearview.h
 * @author Kissmy Spicoli
 * @brief KStorm Clear View: one switch (KSClearView) that leaves only the world on
 *        screen, for screen shots taken outside the viewer.
 *
 * It hides what a snapshot without UI and HUD hides: all 2D UI (menus, toolbars,
 * floaters, chat, notifications, the navigation and status bars), avatar name tags
 * and hover text, beacons, and HUD attachments. The world view takes the whole window.
 * Turning it off restores only what it hid, so a HUD or UI that was already hidden
 * stays hidden.
 *
 * KStorm > KS Clear View (Alt+Shift+U) flips the switch. Menu shortcuts work while
 * the menus are hidden and while a floater or the chat bar has keyboard focus. The
 * switch is never saved: every session starts with the UI shown.
 */
#ifndef KS_CLEARVIEW_H
#define KS_CLEARVIEW_H

#include "llsd.h"

namespace KSClearView
{
    // KSClearView changed (listener connected in settings_setup_listeners()).
    void onSettingChanged(const LLSD& new_value);
}

#endif // KS_CLEARVIEW_H
