/**
 * @file ksfloatercamerapositions.h
 * @author Kissmy Spicoli
 * @brief Camera Positioning floater and its Save as / Rename dialog
 *        (doc/kstorm-camera-positions.md).
 */
#ifndef KS_FLOATERCAMERAPOSITIONS_H
#define KS_FLOATERCAMERAPOSITIONS_H

#include "llfloater.h"

#include <boost/signals2.hpp>
#include <string>

class LLButton;
class LLLineEditor;
class LLScrollListCtrl;
class LLTextBox;

// The camera views of every sim (other sims' disabled and labeled), with Apply, the editing
// buttons and the Lock toggle. Closing it turns Lock off and resets the view.
class KSFloaterCameraPositions : public LLFloater
{
public:
    explicit KSFloaterCameraPositions(const LLSD& key);
    ~KSFloaterCameraPositions() override;

    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    void draw() override;

    // Selects a view of this sim in the list (after Save current as... and Rename...).
    void selectPosition(const std::string& name);

private:
    void refreshList();
    void refreshButtons();
    std::string selectedName() const;

    void onApply();
    void onUpdate();
    void onDelete();
    void openEditor(const std::string& mode);

    LLScrollListCtrl* mList = nullptr;
    LLButton* mLockButton = nullptr;
    LLTextBox* mDetails = nullptr;
    std::string mLastRegion;    // agent region, to refresh the list on change
    boost::signals2::connection mChangedConnection;
};

// Name dialog; the key holds "mode" ("save" or "rename") and "name" (the view, for rename).
class KSFloaterCameraPositionEdit : public LLFloater
{
public:
    explicit KSFloaterCameraPositionEdit(const LLSD& key);

    bool postBuild() override;
    void onOpen(const LLSD& key) override;

private:
    void refreshOK();
    void onOK();

    std::string mMode;
    std::string mName;
    LLLineEditor* mNameEditor = nullptr;
    LLTextBox* mNote = nullptr;
    LLButton* mOKButton = nullptr;
};

#endif // KS_FLOATERCAMERAPOSITIONS_H
