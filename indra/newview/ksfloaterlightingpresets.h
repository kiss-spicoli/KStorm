/**
 * @file ksfloaterlightingpresets.h
 * @author Kissmy Spicoli
 * @brief Lighting Presets pane and its Save as / Rename / Contents dialog
 *        (doc/kstorm2-lighting-UI-design.md, section 12.6).
 */
#ifndef KS_FLOATERLIGHTINGPRESETS_H
#define KS_FLOATERLIGHTINGPRESETS_H

#include "llfloater.h"

#include <boost/signals2.hpp>
#include <string>
#include <vector>

class LLButton;
class LLCheckBoxCtrl;
class LLLineEditor;
class LLScrollListCtrl;
class LLTextBox;

// The list of presets with Apply / Turn off and the editing buttons.
class KSFloaterLightingPresets : public LLFloater
{
public:
    explicit KSFloaterLightingPresets(const LLSD& key);
    ~KSFloaterLightingPresets() override;

    bool postBuild() override;
    void onOpen(const LLSD& key) override;

    // Docks beside the launcher when dragged near this pane's slot (KSFloaterLighting::findPaneSnap).
    LLView* findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type,
                         S32 threshold, S32 padding = 0) override;

    // Selects a preset in the list (after Save current as...).
    void selectPreset(const std::string& name);

private:
    void refreshList();
    void refreshButtons();
    std::string selectedName() const;

    void onApply();
    void onUpdate();
    void onDuplicate();
    void onDelete();
    void openEditor(const std::string& mode);

    LLScrollListCtrl* mList = nullptr;
    LLButton* mApplyButton = nullptr;
    boost::signals2::connection mChangedConnection;
};

// One small dialog for the three name and contents edits; the key holds
// "mode" ("save", "rename" or "contents") and "name" (the preset, except for save).
class KSFloaterLightingPresetEdit : public LLFloater
{
public:
    explicit KSFloaterLightingPresetEdit(const LLSD& key);

    bool postBuild() override;
    void onOpen(const LLSD& key) override;

private:
    void layout(bool show_groups, bool show_text);
    void refreshOK();
    std::vector<std::string> checkedGroups() const;

    void onOK();
    void save(const std::string& name, const std::vector<std::string>& groups);

    std::string mMode;
    std::string mName;
    LLLineEditor* mNameEditor = nullptr;
    LLTextBox* mNote = nullptr;
    LLButton* mOKButton = nullptr;
    std::vector<LLCheckBoxCtrl*> mGroupChecks;
    S32 mFullHeight = 0;
    S32 mGroupsHeight = 0;
};

#endif // KS_FLOATERLIGHTINGPRESETS_H
