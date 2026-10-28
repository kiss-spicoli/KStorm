/**
 * @file ksfloatercamerapositions.cpp
 * @author Kissmy Spicoli
 * @brief Camera Positioning floater and its Save as / Rename dialog.
 */

#include "llviewerprecompiledheaders.h"

#include "ksfloatercamerapositions.h"

#include "kscamerapositions.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lltextbox.h"
#include "lluicolortable.h"

namespace
{
    const std::string EDITOR = "ks_camera_position_edit";
    const std::string DEGREE = "\xC2\xB0";

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

    // List value of a record: the name for this sim's, unique across sims for the others.
    std::string rowValue(const KSCameraPositions::Record& record)
    {
        return KSCameraPositions::isHere(record) ? record.mName : record.mGrid + "/" + record.mRegion + "/" + record.mName;
    }

    void selectInPane(const std::string& name)
    {
        if (KSFloaterCameraPositions* pane = LLFloaterReg::findTypedInstance<KSFloaterCameraPositions>("ks_camera_positions"))
        {
            pane->selectPosition(name);
        }
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

    mChangedConnection = KSCameraPositions::changedSignal().connect([this]() { refreshList(); });
    refreshList();
    return true;
}

void KSFloaterCameraPositions::onOpen(const LLSD& key)
{
    refreshList();
}

void KSFloaterCameraPositions::onClose(bool app_quitting)
{
    // Lock never outlives the floater that shows it.
    if (!app_quitting)
    {
        KSCameraPositions::unlockAndReset();
    }
    LLFloater::onClose(app_quitting);
}

void KSFloaterCameraPositions::draw()
{
    // Teleports and region crossings change which views are this sim's.
    const std::string region = KSCameraPositions::currentRegion();
    if (region != mLastRegion)
    {
        refreshList();
    }
    LLFloater::draw();
}

void KSFloaterCameraPositions::selectPosition(const std::string& name)
{
    mList->selectByValue(LLSD(name));
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

std::string KSFloaterCameraPositions::selectedName() const
{
    const KSCameraPositions::Record* record = selectedRecord();
    return record && KSCameraPositions::isHere(*record) ? record->mName : std::string();
}

void KSFloaterCameraPositions::refreshList()
{
    // The selection's value from the old list (mRecords), before it is replaced.
    const KSCameraPositions::Record* current = selectedRecord();
    const std::string selected = current ? rowValue(*current) : std::string();
    mLastRegion = KSCameraPositions::currentRegion();

    mRecords = KSCameraPositions::records();
    mList->deleteAllItems();
    const LLColor4 dimmed = LLUIColorTable::instance().getColor("ScrollDisabledColor").get();
    for (const KSCameraPositions::Record& record : mRecords)
    {
        const bool here = KSCameraPositions::isHere(record);
        LLSD row;
        row["value"] = rowValue(record);
        LLSD& name = row["columns"][0];
        name["column"] = "name";
        name["value"] = record.mName;
        name["tool_tip"] = detailsText(record.mView);
        LLSD& sim = row["columns"][1];
        sim["column"] = "sim";
        sim["value"] = here ? getString("here") : record.mRegion;
        LLScrollListItem* item = mList->addElement(row);
        // Other sims' rows are dimmed but selectable, so they can be deleted from anywhere.
        if (item && !here)
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
    const bool here = record && KSCameraPositions::isHere(*record);
    const bool region = !mLastRegion.empty();
    getChild<LLUICtrl>("ks_camera_apply")->setEnabled(here);
    getChild<LLUICtrl>("ks_camera_save")->setEnabled(region);
    getChild<LLUICtrl>("ks_camera_update")->setEnabled(here);
    getChild<LLUICtrl>("ks_camera_rename")->setEnabled(here);
    getChild<LLUICtrl>("ks_camera_delete")->setEnabled(record != nullptr);
    mLockButton->setToggleState(KSCameraPositions::isLocked());
    mLockButton->setEnabled(region || KSCameraPositions::isLocked());
    mDetails->setText(record ? detailsText(record->mView) : std::string());
}

void KSFloaterCameraPositions::onApply()
{
    const std::string name = selectedName();
    if (!name.empty())
    {
        KSCameraPositions::apply(name);
    }
}

void KSFloaterCameraPositions::onUpdate()
{
    const std::string name = selectedName();
    if (!name.empty())
    {
        KSCameraPositions::update(name);
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
    key["name"] = selectedName();
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
    mName = key["name"].asString();
    if (mMode == "rename")
    {
        setTitle(getString("title_rename"));
        mNameEditor->setText(mName);
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
    if (KSCameraPositions::currentRegion().empty())
    {
        ok = false;
        note = "Not in a region.";
    }
    else if (mMode == "save")
    {
        note = "Saves the current camera view for " + KSCameraPositions::currentRegion() + ".";
        if (ok && KSCameraPositions::exists(name))
        {
            note = "A view with this name exists in this sim; saving replaces it.";
        }
    }
    else if (name == mName)
    {
        ok = false;
    }
    else if (ok && KSCameraPositions::exists(name))
    {
        ok = false;
        note = "\"" + name + "\" is taken in this sim.";
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
        if (KSCameraPositions::exists(name))
        {
            confirm("Replace the view \"" + name + "\"?", [name]()
                {
                    if (KSCameraPositions::saveCurrent(name))
                    {
                        selectInPane(name);
                    }
                });
            closeFloater();
            return;
        }
        if (KSCameraPositions::saveCurrent(name))
        {
            selectInPane(name);
            closeFloater();
        }
        else
        {
            mNote->setText(std::string("The view could not be saved."));
        }
    }
    else if (KSCameraPositions::rename(mName, name))
    {
        selectInPane(name);
        closeFloater();
    }
    else
    {
        mNote->setText(std::string("The view could not be renamed."));
    }
}
