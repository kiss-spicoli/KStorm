/**
 * @file ksfloatercamerapositions.cpp
 * @author Kissmy Spicoli
 * @brief Camera Positioning floater and its Save as / Rename dialog.
 */

#include "llviewerprecompiledheaders.h"

#include "ksfloatercamerapositions.h"

#include "kscamerapositions.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lltextbox.h"
#include "lluicolortable.h"
#include "llviewercontrol.h"
#include "llviewerinput.h"

namespace
{
    const std::string EDITOR = "ks_camera_position_edit";
    const std::string DEGREE = "\xC2\xB0";

    // Same ramp as the Cameratools and Camera Controls roll buttons (llfloatercamera.cpp): a click
    // nudges, and holding builds to full speed.
    const F32 ROLL_NUDGE_TIME = 0.25f;  // seconds
    const F32 ROLL_NUDGE_RATE = 0.05f;  // fraction of normal speed

    void rollCamera(const LLButton* button, bool left)
    {
        const F32 time = button->getHeldDownTime();
        const F32 rate = time < ROLL_NUDGE_TIME ? ROLL_NUDGE_RATE + time * (1.f - ROLL_NUDGE_RATE) / ROLL_NUDGE_TIME : 1.f;
        gAgentCamera.unlockView();
        if (left)
        {
            gAgentCamera.setRollLeftKey(rate);
        }
        else
        {
            gAgentCamera.setRollRightKey(rate);
        }
    }

    // Asks before something that cannot be undone; on Yes runs action.
    void confirm(const std::string& message, std::function<void()> action)
    {
        LLSD args;
        args["MESSAGE"] = message;
        LLNotificationsUtil::add("GenericAlertYesCancel", args, LLSD(),
            [action](const LLSD& notification, const LLSD& response)
            {
                if (LLNotificationsUtil::getSelectedOption(notification, response) == 0)
                {
                    action();
                }
            });
    }

    std::string detailsText(const KSCameraPositions::View& view)
    {
        return llformat("x %.1f  y %.1f  z %.1f\nyaw %.0f%s  pitch %.0f%s  roll %.0f%s  zoom %.2fx",
            view.mPosition.mV[VX], view.mPosition.mV[VY], view.mPosition.mV[VZ],
            view.mYaw, DEGREE.c_str(), view.mPitch, DEGREE.c_str(), view.mRoll, DEGREE.c_str(),
            KSCameraPositions::zoomFactor(view.mViewAngle));
    }

    const std::string SHOW_DISCONNECTED = "KSCameraPositionsShowDisconnected";
    // The Camera Controls floater Ctrl+K toggles: the small one while FSUseSmallCameraFloater is on
    // (as FSToggleCameraFloater in llviewermenu.cpp).
    std::string cameraControls()
    {
        static LLCachedControl<bool> use_small(gSavedSettings, "FSUseSmallCameraFloater");
        return use_small ? "fs_camera_small" : "camera";
    }

    // List value of a record: unique across regions and grids.
    std::string rowValue(const KSCameraPositions::Record& record)
    {
        return record.mGrid + "/" + record.mRegion + "/" + record.mName;
    }

    void selectInPane(const std::string& region, const std::string& name)
    {
        if (KSFloaterCameraPositions* pane = LLFloaterReg::findTypedInstance<KSFloaterCameraPositions>("ks_camera_positions"))
        {
            pane->selectPosition(region, name);
        }
    }

    // Says why something did not happen.
    void alert(const std::string& message)
    {
        LLSD args;
        args["MESSAGE"] = message;
        LLNotificationsUtil::add("GenericAlert", args);
    }
}

// ---------------------------------------------------------------------------
// Floater

KSFloaterCameraPositions::KSFloaterCameraPositions(const LLSD& key)
    : LLFloater(key)
{
}

KSFloaterCameraPositions::~KSFloaterCameraPositions()
{
    mChangedConnection.disconnect();
    mShowConnection.disconnect();
}

bool KSFloaterCameraPositions::postBuild()
{
    mList = getChild<LLScrollListCtrl>("ks_camera_list");
    mList->setCommitOnSelectionChange(true);
    mList->setCommitCallback([this](LLUICtrl*, const LLSD&) { refreshButtons(); });
    mList->setDoubleClickCallback([this]() { onApply(); });
    mDetails = getChild<LLTextBox>("ks_camera_details");

    getChild<LLUICtrl>("ks_camera_apply")->setCommitCallback([this](LLUICtrl*, const LLSD&) { onApply(); });
    getChild<LLUICtrl>("ks_camera_save")->setCommitCallback([this](LLUICtrl*, const LLSD&) { openEditor("save"); });
    getChild<LLUICtrl>("ks_camera_update")->setCommitCallback([this](LLUICtrl*, const LLSD&) { onUpdate(); });
    getChild<LLUICtrl>("ks_camera_rename")->setCommitCallback([this](LLUICtrl*, const LLSD&) { openEditor("rename"); });
    getChild<LLUICtrl>("ks_camera_delete")->setCommitCallback([this](LLUICtrl*, const LLSD&) { onDelete(); });
    mLockButton = getChild<LLButton>("ks_camera_lock");
    mLockButton->setCommitCallback([](LLUICtrl*, const LLSD&)
        {
            KSCameraPositions::setLocked(!KSCameraPositions::isLocked());
        });
    mControlsButton = getChild<LLButton>("ks_camera_controls");
    mControlsButton->setCommitCallback([](LLUICtrl*, const LLSD&)
        {
            // Opens or closes, like Ctrl+K: clicking here takes focus from the floater, so the
            // toolbar's bring-to-front step would never close it.
            LLFloaterReg::toggleInstance(cameraControls());
        });
    mRollLeftButton = getChild<LLButton>("ks_camera_roll_left");
    mRollLeftButton->setCommitCallback([this](LLUICtrl*, const LLSD&) { rollCamera(mRollLeftButton, true); });
    mRollLeftButton->setHeldDownCallback([this](LLUICtrl*, const LLSD&) { rollCamera(mRollLeftButton, true); });
    mRollRightButton = getChild<LLButton>("ks_camera_roll_right");
    mRollRightButton->setCommitCallback([this](LLUICtrl*, const LLSD&) { rollCamera(mRollRightButton, false); });
    mRollRightButton->setHeldDownCallback([this](LLUICtrl*, const LLSD&) { rollCamera(mRollRightButton, false); });
    refreshRollHint();

    mChangedConnection = KSCameraPositions::changedSignal().connect([this]() { refreshList(); });
    // The Show disconnected check box binds the setting (control_name); the list follows it.
    if (LLControlVariable* control = gSavedSettings.getControl(SHOW_DISCONNECTED))
    {
        mShowConnection = control->getSignal()->connect([this](LLControlVariable*, const LLSD&, const LLSD&) { refreshList(); });
    }
    // KStorm panes grow taller, never wider: only the top and bottom edges drag (no side bars or
    // corner handles). save_rect keeps the height across closing and sessions.
    enableResizeCtrls(true, false, true);
    refreshList();
    return true;
}

void KSFloaterCameraPositions::onOpen(const LLSD& key)
{
    refreshRollHint();
    refreshList();
}

// Shows the current bindings, since they can be changed in Preferences > Controls. Roll has the
// same keys in every camera mode by default; third person is the mode a scene is shot in.
void KSFloaterCameraPositions::refreshRollHint()
{
    const auto bindingText = [this](const std::string& control)
        {
            const std::string keys = gViewerInput.getKeyBindingAsString("third_person", control);
            return keys.empty() ? getString("roll_no_key") : keys;
        };
    LLStringUtil::format_map_t args;
    args["[LEFT]"] = bindingText("roll_left");
    args["[RIGHT]"] = bindingText("roll_right");
    LLTextBox* hint = getChild<LLTextBox>("ks_camera_roll_hint");
    hint->setValue(getString("roll_hint", args));

    // Key names vary in length, so the buttons sit against the text wherever it ends.
    constexpr S32 BUTTON_GAP = 4;
    const LLRect hint_rect = hint->getRect();
    const S32 half_width = llmin(hint->getTextPixelWidth(), hint_rect.getWidth()) / 2;
    const auto moveButton = [](LLButton* button, S32 left)
        {
            LLRect rect = button->getRect();
            rect.setOriginAndSize(left, rect.mBottom, rect.getWidth(), rect.getHeight());
            button->setRect(rect);
        };
    moveButton(mRollLeftButton, hint_rect.getCenterX() - half_width - BUTTON_GAP - mRollLeftButton->getRect().getWidth());
    moveButton(mRollRightButton, hint_rect.getCenterX() + half_width + BUTTON_GAP);
}

void KSFloaterCameraPositions::onClose(bool app_quitting)
{
    // Lock Camera never outlives the floater that shows it.
    if (!app_quitting)
    {
        KSCameraPositions::unlockAndReset();
    }
    LLFloater::onClose(app_quitting);
}

void KSFloaterCameraPositions::draw()
{
    // Teleports, region crossings and neighbours connecting or dropping change which views are
    // this sim's and which can be applied.
    if (KSCameraPositions::currentRegion() != mLastRegion || KSCameraPositions::connectedRegions() != mLastConnected)
    {
        refreshList();
    }
    // The Camera Controls toggle shows pressed while that floater is open, however it was opened.
    mControlsButton->setToggleState(LLFloaterReg::instanceVisible(cameraControls()));
    LLFloater::draw();
}

void KSFloaterCameraPositions::selectPosition(const std::string& region, const std::string& name)
{
    for (const KSCameraPositions::Record& record : mRecords)
    {
        if (record.mRegion == region && record.mName == name && KSCameraPositions::isAvailable(record))
        {
            mList->selectByValue(LLSD(rowValue(record)));
            break;
        }
    }
    refreshButtons();
}

const KSCameraPositions::Record* KSFloaterCameraPositions::selectedRecord() const
{
    const LLScrollListItem* item = mList->getFirstSelected();
    if (!item)
    {
        return nullptr;
    }
    const std::string value = item->getValue().asString();
    for (const KSCameraPositions::Record& record : mRecords)
    {
        if (rowValue(record) == value)
        {
            return &record;
        }
    }
    return nullptr;
}

const KSCameraPositions::Record* KSFloaterCameraPositions::selectedAvailable() const
{
    const KSCameraPositions::Record* record = selectedRecord();
    return record && KSCameraPositions::isAvailable(*record) ? record : nullptr;
}

void KSFloaterCameraPositions::refreshList()
{
    // The selection's value from the old list (mRecords), before it is replaced.
    const KSCameraPositions::Record* current = selectedRecord();
    const std::string selected = current ? rowValue(*current) : std::string();
    mLastRegion = KSCameraPositions::currentRegion();
    mLastConnected = KSCameraPositions::connectedRegions();

    // Only the listed records are kept: selection and buttons work on mRecords.
    const bool show_disconnected = gSavedSettings.getBOOL(SHOW_DISCONNECTED);
    mRecords.clear();
    for (const KSCameraPositions::Record& record : KSCameraPositions::records())
    {
        if (show_disconnected || KSCameraPositions::isAvailable(record))
        {
            mRecords.push_back(record);
        }
    }
    mList->deleteAllItems();
    const LLColor4 dimmed = LLUIColorTable::instance().getColor("ScrollDisabledColor").get();
    for (const KSCameraPositions::Record& record : mRecords)
    {
        const bool here = KSCameraPositions::isHere(record);
        const bool connected = !here && KSCameraPositions::isConnected(record);
        LLSD row;
        row["value"] = rowValue(record);
        // No tooltip on the name: it would cover the names below (the details line shows the values).
        LLSD& name = row["columns"][0];
        name["column"] = "name";
        name["value"] = record.mName;
        LLSD& sim = row["columns"][1];
        sim["column"] = "sim";
        if (here)
        {
            sim["value"] = getString("here");
        }
        else if (connected)
        {
            LLStringUtil::format_map_t args;
            args["[SIM]"] = record.mRegion;
            sim["value"] = getString("connected", args);
            sim["tool_tip"] = getString("connected_tip", args);
        }
        else
        {
            sim["value"] = record.mRegion;
        }
        LLScrollListItem* item = mList->addElement(row);
        // Unconnected sims' rows are dimmed but selectable: they can be applied (by their stored
        // origin) and deleted from anywhere.
        if (item && !here && !connected)
        {
            for (S32 i = 0; i < item->getNumColumns(); ++i)
            {
                item->getColumn(i)->setColor(dimmed);
            }
        }
    }
    if (selected.empty() || !mList->selectByValue(LLSD(selected)))
    {
        mList->selectFirstItem();
    }
    refreshButtons();
}

void KSFloaterCameraPositions::refreshButtons()
{
    const KSCameraPositions::Record* record = selectedRecord();
    const bool available = selectedAvailable() != nullptr;
    const bool region = !mLastRegion.empty();
    // Apply on this sim's and connected sims' views; Teleport on the others (same grid).
    LLButton* apply = getChild<LLButton>("ks_camera_apply");
    const bool teleport = record && !available;
    apply->setLabel(getString(teleport ? "teleport_label" : "apply_label"));
    apply->setToolTip(getString(teleport ? "teleport_tip" : "apply_tip"));
    apply->setEnabled(available || (record && KSCameraPositions::canTeleport(*record)));
    getChild<LLUICtrl>("ks_camera_save")->setEnabled(region);
    getChild<LLUICtrl>("ks_camera_update")->setEnabled(available);
    getChild<LLUICtrl>("ks_camera_rename")->setEnabled(available);
    getChild<LLUICtrl>("ks_camera_delete")->setEnabled(record != nullptr);
    mLockButton->setToggleState(KSCameraPositions::isLocked());
    mLockButton->setEnabled(region || KSCameraPositions::isLocked());
    // Lock Camera blocks the roll keys too (KSCameraPositions::blocksAction()).
    mRollLeftButton->setEnabled(!KSCameraPositions::isLocked());
    mRollRightButton->setEnabled(!KSCameraPositions::isLocked());
    mDetails->setText(record ? detailsText(record->mView) : std::string());
}

void KSFloaterCameraPositions::onApply()
{
    const KSCameraPositions::Record* selected = selectedRecord();
    if (!selected)
    {
        return;
    }
    if (KSCameraPositions::isAvailable(*selected))
    {
        KSCameraPositions::apply(*selected);
        return;
    }
    // A disconnected sim's view: teleport there, and the view is applied after landing.
    if (KSCameraPositions::canTeleport(*selected))
    {
        const KSCameraPositions::Record record = *selected;
        confirm("Teleport to " + record.mRegion + " and move the camera to \"" + record.mName + "\"?",
            [record]() { KSCameraPositions::teleportTo(record); });
    }
}

void KSFloaterCameraPositions::onUpdate()
{
    const KSCameraPositions::Record* selected = selectedAvailable();
    if (!selected)
    {
        return;
    }
    const KSCameraPositions::Record record = *selected;
    const std::string region = KSCameraPositions::cameraRegion();
    if (KSCameraPositions::update(record))
    {
        selectPosition(region, record.mName);
    }
    else if (region != record.mRegion && KSCameraPositions::exists(record.mName, region))
    {
        // The camera is over another sim now, where a different view has this name.
        alert("The camera is over " + region + ", which already has a view named \"" + record.mName
            + "\". Rename one of them first.");
    }
}

void KSFloaterCameraPositions::onDelete()
{
    const KSCameraPositions::Record* selected = selectedRecord();
    if (!selected)
    {
        return;
    }
    const KSCameraPositions::Record record = *selected;
    std::string message = "Delete the view \"" + record.mName + "\"";
    if (!KSCameraPositions::isHere(record))
    {
        message += " of " + record.mRegion;
    }
    confirm(message + "? This cannot be undone.", [record]() { KSCameraPositions::remove(record); });
}

void KSFloaterCameraPositions::openEditor(const std::string& mode)
{
    LLSD key;
    key["mode"] = mode;
    if (const KSCameraPositions::Record* record = selectedAvailable())
    {
        key["name"] = record->mName;
        key["region"] = record->mRegion;
        key["grid"] = record->mGrid;
    }
    LLFloaterReg::showInstance(EDITOR, key, true);
}

// ---------------------------------------------------------------------------
// Dialog

KSFloaterCameraPositionEdit::KSFloaterCameraPositionEdit(const LLSD& key)
    : LLFloater(key)
{
}

bool KSFloaterCameraPositionEdit::postBuild()
{
    mNameEditor = getChild<LLLineEditor>("ks_edit_name");
    mNameEditor->setKeystrokeCallback([this](LLLineEditor*, void*) { refreshOK(); }, nullptr);
    mNote = getChild<LLTextBox>("ks_edit_note");
    mOKButton = getChild<LLButton>("ks_edit_ok");
    mOKButton->setCommitCallback([this](LLUICtrl*, const LLSD&) { onOK(); });
    getChild<LLUICtrl>("ks_edit_cancel")->setCommitCallback([this](LLUICtrl*, const LLSD&) { closeFloater(); });
    return true;
}

void KSFloaterCameraPositionEdit::onOpen(const LLSD& key)
{
    mMode = key["mode"].asString() == "rename" ? "rename" : "save";
    mRecord.mName = key["name"].asString();
    mRecord.mRegion = key["region"].asString();
    mRecord.mGrid = key["grid"].asString();
    if (mMode == "rename")
    {
        setTitle(getString("title_rename"));
        mNameEditor->setText(mRecord.mName);
        mOKButton->setLabel(std::string("Rename"));
    }
    else
    {
        setTitle(getString("title_save"));
        mNameEditor->setText(LLStringUtil::null);
        mOKButton->setLabel(std::string("Save"));
    }
    refreshOK();
    mNameEditor->setFocus(true);
}

void KSFloaterCameraPositionEdit::refreshOK()
{
    std::string name = mNameEditor->getText();
    LLStringUtil::trim(name);
    std::string note;
    bool ok = !name.empty();
    // A view belongs to the sim the camera is over, which can be a neighbour of yours.
    const std::string region = KSCameraPositions::cameraRegion();
    if (region.empty())
    {
        ok = false;
        note = "Not in a region.";
    }
    else if (mMode == "save")
    {
        note = "Saves the current camera view for " + region + ", the sim the camera is over.";
        if (ok && KSCameraPositions::exists(name, region))
        {
            note = "A view with this name exists in " + region + "; saving replaces it.";
        }
    }
    else if (name == mRecord.mName)
    {
        ok = false;
    }
    else if (ok && KSCameraPositions::exists(name, mRecord.mRegion))
    {
        ok = false;
        note = "\"" + name + "\" is taken in " + mRecord.mRegion + ".";
    }
    mNote->setText(note);
    mOKButton->setEnabled(ok);
}

void KSFloaterCameraPositionEdit::onOK()
{
    std::string name = mNameEditor->getText();
    LLStringUtil::trim(name);
    if (mMode == "save")
    {
        const std::string region = KSCameraPositions::cameraRegion();
        if (KSCameraPositions::exists(name, region))
        {
            confirm("Replace the view \"" + name + "\" of " + region + "?", [name]()
                {
                    // Saved for the sim the camera is over when Yes is pressed.
                    if (KSCameraPositions::saveCurrent(name))
                    {
                        selectInPane(KSCameraPositions::cameraRegion(), name);
                    }
                });
            closeFloater();
            return;
        }
        if (KSCameraPositions::saveCurrent(name))
        {
            selectInPane(region, name);
            closeFloater();
        }
        else
        {
            mNote->setText(std::string("The view could not be saved."));
        }
    }
    else if (KSCameraPositions::rename(mRecord, name))
    {
        selectInPane(mRecord.mRegion, name);
        closeFloater();
    }
    else
    {
        mNote->setText(std::string("The view could not be renamed."));
    }
}
