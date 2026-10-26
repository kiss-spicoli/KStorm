/**
 * @file ksfloaterlighting.h
 * @author Kissmy Spicoli
 * @brief KStorm Lighting launch list (doc/kstorm2-lighting-UI-design.md, section 10).
 *
 * The floater holds no controls: one button per pane, each with a short
 * status, a footer for the environment state, and a compact (icons only) mode
 * saved in KSLightingCompact. Opened from KStorm > KStorm Lighting (Alt+Shift+L)
 * or its toolbar button.
 */
#ifndef KS_FLOATERLIGHTING_H
#define KS_FLOATERLIGHTING_H

#include "llenvironment.h"
#include "llfloater.h"

#include <functional>
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
    void draw() override;

private:
    struct Entry
    {
        LLButton* mButton = nullptr;
        LLTextBox* mStatus = nullptr;
        std::string mLabel;
        std::string mFloater;                  // empty: not built yet
        std::function<std::string()> mStatusText;
    };

    void addEntry(const std::string& key, const std::string& floater,
                  std::function<std::string()> status_text = {},
                  const std::vector<std::string>& watched = {});
    void refreshStatuses();
    void refreshFooter();
    void setCompact(bool compact);
    void onResetEnvironment();

    std::vector<Entry> mEntries;
    std::vector<boost::signals2::connection> mConnections;
    LLEnvironment::connection_t mEnvironmentConnection;
    LLButton* mModeButton = nullptr;
    LLTextBox* mFooter = nullptr;
    LLButton* mFooterReset = nullptr;
    S32 mFullWidth = 0;
    bool mCompact = false;
};

#endif // KS_FLOATERLIGHTING_H
