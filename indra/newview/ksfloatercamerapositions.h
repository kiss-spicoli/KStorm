/**
 * @file ksfloatercamerapositions.h
 * @author Kissmy Spicoli
 * @brief Camera Positioning floater and its Save as / Rename dialog
 *        (doc/kstorm-camera-positions.md).
 */
#ifndef KS_FLOATERCAMERAPOSITIONS_H
#define KS_FLOATERCAMERAPOSITIONS_H

#include "kscamerapositions.h"
#include "llfloater.h"

#include <boost/signals2.hpp>
#include <string>
#include <vector>

class LLButton;
class LLLineEditor;
class LLScrollListCtrl;
class LLTextBox;

// The camera views of every sim, with Apply, the editing buttons and the Lock Camera toggle. Views
// of this sim and connected sims (labeled) can be applied and edited; other sims' (dimmed and
// labeled, hidden while Show disconnected is off) reached by Teleport and deleted.
// Closing it turns Lock Camera off and resets the view. It resizes in height only (the list
// grows); its size is saved.
class KSFloaterCameraPositions : public LLFloater
{
public:
    explicit KSFloaterCameraPositions(const LLSD& key);
    ~KSFloaterCameraPositions() override;

    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    void draw() override;

    // Selects an available view in the list (after Save current as... and Rename...).
    void selectPosition(const std::string& region, const std::string& name);

private:
    void refreshList();
    void refreshButtons();
    const KSCameraPositions::Record* selectedRecord() const;
    // The selected record when it can be applied and edited, else null.
    const KSCameraPositions::Record* selectedAvailable() const;


    void onApply();
    void onUpdate();
    void onDelete();
    void openEditor(const std::string& mode);

    LLScrollListCtrl* mList = nullptr;
    LLButton* mLockButton = nullptr;
    LLTextBox* mDetails = nullptr;
    std::vector<KSCameraPositions::Record> mRecords;   // as listed
    std::string mLastRegion;    // agent region, to refresh the list on change
    std::string mLastConnected; // connected regions (KSCameraPositions::connectedRegions()), likewise
    boost::signals2::connection mChangedConnection;
    boost::signals2::connection mShowConnection;    // KSCameraPositionsShowDisconnected
};

// Name dialog; the key holds "mode" ("save" or "rename") and, for rename, the view's "name",
// "region" and "grid".
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
    KSCameraPositions::Record mRecord;  // the view to rename
    LLLineEditor* mNameEditor = nullptr;
    LLTextBox* mNote = nullptr;
    LLButton* mOKButton = nullptr;
};

#endif // KS_FLOATERCAMERAPOSITIONS_H
