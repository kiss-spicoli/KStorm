/**
 * @file ksfloaterlighting.h
 * @author Kissmy Spicoli
 * @brief KStorm Lighting launch list (doc/kstorm2-lighting-UI-design.md, section 10).
 *
 * The floater holds no controls: one button per pane, each with a short
 * status, and a compact (icons only) mode. The environment-state footer was
 * removed on 2026-10-26: the compact launcher has no room for it.
 * For now it always opens compact with its mode toggle hidden; the open mode and
 * KSLightingCompact are kept for later. Opened from KStorm > KStorm Lighting (Alt+Shift+L)
 * or its toolbar button.
 */
#ifndef KS_FLOATERLIGHTING_H
#define KS_FLOATERLIGHTING_H

#include "llenvironment.h"
#include "llfloater.h"

#include <functional>
#include <map>
#include <vector>

class LLButton;
class LLTextBox;

class KSFloaterLighting : public LLFloater
{
public:
    explicit KSFloaterLighting(const LLSD& key);
    ~KSFloaterLighting() override;

    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    // Closing the launcher (close button, Alt+Shift+L, menu, toolbar) hides every docked pane and proxy.
    void onClose(bool app_quitting) override;
    void draw() override;

    // Pane docking: the slot beside the launcher for a pane opened from it, top aligned with
    // the pane's button, on the side of the launcher the pane is on. Returns the launcher and
    // sets slot when pane_rect is within KSLightingPaneSnapRadius of that slot; otherwise null,
    // and the pane moves freely ("broken off"). Called while a pane is dragged, so it also
    // records whether the pane is docked, and on which side (KSLightingDockedPanes).
    static LLFloater* findPaneSnap(LLFloater* pane, const LLRect& pane_rect, LLRect& slot);

    // True for a pane docked beside the launcher (KSLightingDockedPanes) and for proxy headers.
    static bool isDocked(const std::string& name);

private:
    struct Entry
    {
        LLButton* mButton = nullptr;
        LLTextBox* mStatus = nullptr;
        std::string mLabel;
        std::string mFloater;                  // empty: not built yet
        std::string mProxy;                    // header-only stand-in for an oversized AyaneStorm floater
        std::function<std::string()> mStatusText;
    };
    // What docks for an entry: its pane, or the proxy header of an oversized floater.
    static const std::string& dockName(const Entry& entry) { return entry.mProxy.empty() ? entry.mFloater : entry.mProxy; }
    // Docked panes and their side (true = right), plus every proxy (always left).
    std::map<std::string, bool> dockedPanes() const;
    // A button for an oversized AyaneStorm floater: closes it (and its proxy) when it is in
    // front; otherwise opens its proxy header in its slot and the floater itself in front.
    void onProxyButton(const Entry& entry);

    void addEntry(const std::string& key, const std::string& floater,
                  std::function<std::string()> status_text = {},
                  const std::vector<std::string>& watched = {});
    void refreshStatuses();
    void setCompact(bool compact);
    // The slot for an entry's pane of the given size, right or left of the launcher.
    LLRect paneSlot(const Entry& entry, const LLRect& pane_rect, bool on_right) const;
    // Header button: closes the open docked panes, or when none is open, opens every docked
    // pane in its slot from top to bottom, collapsed or expanded as it was when it closed, each
    // in front of the one above it so every header stays visible.
    void onDockToggle();
    // Keeps every open docked pane in its slot (top aligned with its button, against the
    // launcher's edge), so docked panes follow the launcher and any other move or resize.
    void keepDockedPanesInPlace();
    // A button for one of our panes: closes it when it is fully open in front; otherwise opens
    // it fully (expanded, in its slot when docked) and brings it to the front.
    void onPaneButton(const Entry& entry);
    void refreshDockButton();

    std::vector<Entry> mEntries;
    std::vector<boost::signals2::connection> mConnections;
    LLEnvironment::connection_t mEnvironmentConnection;
    LLButton* mModeButton = nullptr;
    LLButton* mDockButton = nullptr;
    S32 mFullWidth = 0;
    bool mCompact = false;
};

#endif // KS_FLOATERLIGHTING_H
