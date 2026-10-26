/**
 * @file ksfloaterlightingpresets.cpp
 * @author Kissmy Spicoli
 * @brief Lighting Presets pane and its Save as / Rename / Contents dialog.
 */

#include "llviewerprecompiledheaders.h"

#include "ksfloaterlightingpresets.h"

#include "ksenvironment.h"
#include "ksfloaterlighting.h"
#include "kslightingpresets.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lltextbox.h"
#include "lltexteditor.h"

#include <algorithm>

namespace
{
    const std::string EDITOR = "ks_lighting_preset_edit";

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
}

// ---------------------------------------------------------------------------
// Pane

KSFloaterLightingPresets::KSFloaterLightingPresets(const LLSD& key)
    : LLFloater(key)
{
    KSEnvironment::init();
}

KSFloaterLightingPresets::~KSFloaterLightingPresets()
{
    mChangedConnection.disconnect();
}

bool KSFloaterLightingPresets::postBuild()
{
    mList = getChild<LLScrollListCtrl>("ks_preset_list");
    mList->setCommitOnSelectionChange(true);
    mList->setCommitCallback([this](LLUICtrl*, const LLSD&) { refreshButtons(); });
    mList->setDoubleClickCallback([this]() { onApply(); });

    mApplyButton = getChild<LLButton>("ks_preset_apply");
    mApplyButton->setCommitCallback([this](LLUICtrl*, const LLSD&) { onApply(); });
    getChild<LLUICtrl>("ks_preset_save")->setCommitCallback([this](LLUICtrl*, const LLSD&) { openEditor("save"); });
    getChild<LLUICtrl>("ks_preset_update")->setCommitCallback([this](LLUICtrl*, const LLSD&) { onUpdate(); });
    getChild<LLUICtrl>("ks_preset_rename")->setCommitCallback([this](LLUICtrl*, const LLSD&) { openEditor("rename"); });
    getChild<LLUICtrl>("ks_preset_duplicate")->setCommitCallback([this](LLUICtrl*, const LLSD&) { onDuplicate(); });
    getChild<LLUICtrl>("ks_preset_delete")->setCommitCallback([this](LLUICtrl*, const LLSD&) { onDelete(); });
    getChild<LLUICtrl>("ks_preset_contents")->setCommitCallback([this](LLUICtrl*, const LLSD&) { openEditor("contents"); });

    mChangedConnection = KSLightingPresets::changedSignal().connect([this]() { refreshList(); });
    refreshList();
    return true;
}

LLView* KSFloaterLightingPresets::findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type,
                            S32 threshold, S32 padding)
{
    // Called on every drag step with the rect at the dragged position; within the snap radius
    // of this pane's slot beside the launcher, the slot wins over the usual edge snapping.
    LLRect slot;
    if (LLFloater* launcher = KSFloaterLighting::findPaneSnap(this, getRect(), slot))
    {
        new_rect = slot;
        return launcher;
    }
    return LLFloater::findSnapRect(new_rect, mouse_dir, snap_type, threshold, padding);
}

void KSFloaterLightingPresets::onOpen(const LLSD& key)
{
    refreshList();
}

void KSFloaterLightingPresets::selectPreset(const std::string& name)
{
    mList->selectByValue(LLSD(name));
    refreshButtons();
}

std::string KSFloaterLightingPresets::selectedName() const
{
    const LLScrollListItem* item = mList->getFirstSelected();
    return item ? item->getValue().asString() : std::string();
}

void KSFloaterLightingPresets::refreshList()
{
    std::string selected = selectedName();
    const std::string& active = KSLightingPresets::active();
    if (selected.empty())
    {
        selected = active.empty() ? KSLightingPresets::KS_REFERENCE : active;
    }

    mList->deleteAllItems();
    for (const std::string& name : KSLightingPresets::names())
    {
        LLSD row;
        row["value"] = name;
        LLSD& lock = row["columns"][0];
        lock["column"] = "lock";
        if (KSLightingPresets::isBuiltIn(name))
        {
            lock["type"] = "icon";
            lock["value"] = "Lock";
        }
        else
        {
            lock["value"] = "";
        }
        LLSD& label = row["columns"][1];
        label["column"] = "name";
        label["value"] = name == active ? name + " " + getString("active_suffix") : name;
        label["font"]["name"] = "SansSerif";
        label["font"]["style"] = name == active ? "BOLD" : "NORMAL";
        LLSD& scope = row["columns"][2];
        scope["column"] = "scope";
        scope["value"] = KSLightingPresets::scopeText(name);
        mList->addElement(row);
    }
    if (!mList->selectByValue(LLSD(selected)))
    {
        mList->selectFirstItem();
    }
    refreshButtons();
}

void KSFloaterLightingPresets::refreshButtons()
{
    const std::string name = selectedName();
    const bool selected = !name.empty();
    const bool editable = selected && !KSLightingPresets::isBuiltIn(name);
    const bool active = selected && name == KSLightingPresets::active();
    mApplyButton->setLabel(active ? std::string("Turn off") : std::string("Apply"));
    mApplyButton->setEnabled(selected);
    getChild<LLUICtrl>("ks_preset_update")->setEnabled(editable);
    getChild<LLUICtrl>("ks_preset_rename")->setEnabled(editable);
    getChild<LLUICtrl>("ks_preset_duplicate")->setEnabled(selected);
    getChild<LLUICtrl>("ks_preset_delete")->setEnabled(editable);
    getChild<LLUICtrl>("ks_preset_contents")->setEnabled(selected);
}

void KSFloaterLightingPresets::onApply()
{
    const std::string name = selectedName();
    if (name.empty())
    {
        return;
    }
    if (name == KSLightingPresets::active())
    {
        KSLightingPresets::turnOff();
    }
    else
    {
        KSLightingPresets::apply(name);
    }
}

void KSFloaterLightingPresets::onUpdate()
{
    const std::string name = selectedName();
    if (!name.empty())
    {
        KSLightingPresets::update(name);
    }
}

void KSFloaterLightingPresets::onDuplicate()
{
    const std::string copy = KSLightingPresets::duplicate(selectedName());
    if (!copy.empty())
    {
        mList->selectByValue(LLSD(copy));
        refreshButtons();
    }
}

void KSFloaterLightingPresets::onDelete()
{
    const std::string name = selectedName();
    if (name.empty() || KSLightingPresets::isBuiltIn(name))
    {
        return;
    }
    std::string message = "Delete the lighting preset \"" + name + "\"? This cannot be undone.";
    if (name == KSLightingPresets::active())
    {
        message += " It is active: the values from before it was applied are restored first.";
    }
    confirm(message, [name]() { KSLightingPresets::remove(name); });
}

void KSFloaterLightingPresets::openEditor(const std::string& mode)
{
    LLSD key;
    key["mode"] = mode;
    key["name"] = selectedName();
    LLFloaterReg::showInstance(EDITOR, key, true);
}

// ---------------------------------------------------------------------------
// Dialog

namespace
{
    // A new preset holds the values in use, so it becomes the active one: applying it writes
    // nothing and records the current values (or the active preset's restore point) for Turn off.
    bool saveAndApply(const std::string& name, const std::vector<std::string>& groups)
    {
        if (!KSLightingPresets::saveCurrent(name, groups))
        {
            return false;
        }
        std::string saved = name;
        LLStringUtil::trim(saved);
        KSLightingPresets::apply(saved);
        if (KSFloaterLightingPresets* pane = LLFloaterReg::findTypedInstance<KSFloaterLightingPresets>("ks_lighting_presets"))
        {
            pane->selectPreset(saved);
        }
        return true;
    }
}

KSFloaterLightingPresetEdit::KSFloaterLightingPresetEdit(const LLSD& key)
    : LLFloater(key)
{
}

bool KSFloaterLightingPresetEdit::postBuild()
{
    mNameEditor = getChild<LLLineEditor>("ks_edit_name");
    mNameEditor->setKeystrokeCallback([this](LLLineEditor*, void*) { refreshOK(); }, nullptr);
    mNote = getChild<LLTextBox>("ks_edit_note");
    mOKButton = getChild<LLButton>("ks_edit_ok");
    mOKButton->setCommitCallback([this](LLUICtrl*, const LLSD&) { onOK(); });
    getChild<LLUICtrl>("ks_edit_cancel")->setCommitCallback([this](LLUICtrl*, const LLSD&) { closeFloater(); });
    for (const KSLightingPresets::Group& group : KSLightingPresets::groups())
    {
        LLCheckBoxCtrl* check = getChild<LLCheckBoxCtrl>("ks_group_" + group.mId);
        check->setCommitCallback([this](LLUICtrl*, const LLSD&) { refreshOK(); });
        mGroupChecks.push_back(check);
    }
    // The group block runs from the first check box to the note.
    mFullHeight = getRect().getHeight();
    mGroupsHeight = mGroupChecks.front()->getRect().mTop - mNote->getRect().mTop;
    return true;
}

void KSFloaterLightingPresetEdit::onOpen(const LLSD& key)
{
    mMode = key["mode"].asString();
    mName = key["name"].asString();
    const bool built_in = KSLightingPresets::isBuiltIn(mName);

    mOKButton->setVisible(true);
    getChild<LLButton>("ks_edit_cancel")->setLabel(std::string("Cancel"));
    mNameEditor->setEnabled(true);
    getChild<LLTextEditor>("ks_edit_text")->setText(LLStringUtil::null);

    if (mMode == "rename")
    {
        setTitle(getString("title_rename"));
        mNameEditor->setText(mName);
        mOKButton->setLabel(std::string("Rename"));
        layout(false, false);
    }
    else if (mMode == "contents")
    {
        setTitle(getString("title_contents"));
        mNameEditor->setText(mName);
        mNameEditor->setEnabled(false);
        if (built_in)
        {
            getChild<LLTextEditor>("ks_edit_text")->setText(KSLightingPresets::contentsText(mName));
            mOKButton->setVisible(false);
            getChild<LLButton>("ks_edit_cancel")->setLabel(std::string("Close"));
            layout(false, true);
        }
        else
        {
            const std::vector<std::string> ids = KSLightingPresets::presetGroups(mName);
            const std::vector<KSLightingPresets::Group>& groups = KSLightingPresets::groups();
            for (size_t i = 0; i < groups.size(); ++i)
            {
                mGroupChecks[i]->set(std::find(ids.begin(), ids.end(), groups[i].mId) != ids.end());
            }
            mOKButton->setLabel(std::string("Save"));
            layout(true, false);
        }
    }
    else
    {
        mMode = "save";
        setTitle(getString("title_save"));
        mNameEditor->setText(LLStringUtil::null);
        for (LLCheckBoxCtrl* check : mGroupChecks)
        {
            check->set(true);
        }
        mOKButton->setLabel(std::string("Save"));
        layout(true, false);
    }
    refreshOK();
    if (mNameEditor->getEnabled())
    {
        mNameEditor->setFocus(true);
    }
}

void KSFloaterLightingPresetEdit::layout(bool show_groups, bool show_text)
{
    for (LLCheckBoxCtrl* check : mGroupChecks)
    {
        check->setVisible(show_groups);
    }
    getChild<LLView>("ks_edit_text")->setVisible(show_text);
    // Drop the group block when unused; the note and buttons follow the bottom edge.
    const S32 height = (show_groups || show_text) ? mFullHeight : mFullHeight - mGroupsHeight;
    if (height != getRect().getHeight())
    {
        LLRect rect = getRect();
        rect.mBottom -= height - rect.getHeight();
        setShape(rect);
    }
}

std::vector<std::string> KSFloaterLightingPresetEdit::checkedGroups() const
{
    std::vector<std::string> ids;
    const std::vector<KSLightingPresets::Group>& groups = KSLightingPresets::groups();
    for (size_t i = 0; i < groups.size(); ++i)
    {
        if (mGroupChecks[i]->get())
        {
            ids.push_back(groups[i].mId);
        }
    }
    return ids;
}

void KSFloaterLightingPresetEdit::refreshOK()
{
    std::string name = mNameEditor->getText();
    LLStringUtil::trim(name);
    std::string note;
    bool ok = true;
    if (mMode == "save")
    {
        note = "Saves the current values of the checked groups.";
        if (name.empty())
        {
            ok = false;
        }
        else if (KSLightingPresets::isBuiltIn(name))
        {
            ok = false;
            note = "\"" + name + "\" is reserved for the built-in preset.";
        }
        else if (KSLightingPresets::exists(name))
        {
            note = "A preset with this name exists; saving replaces it.";
        }
        if (checkedGroups().empty())
        {
            ok = false;
        }
    }
    else if (mMode == "rename")
    {
        if (name.empty() || name == mName)
        {
            ok = false;
        }
        else if (KSLightingPresets::isBuiltIn(name) || KSLightingPresets::exists(name))
        {
            ok = false;
            note = "\"" + name + "\" is taken.";
        }
    }
    else if (KSLightingPresets::isBuiltIn(mName))
    {
        note = "Built in: its values are set by KStorm. Duplicate it to make an editable copy.";
    }
    else
    {
        note = "Unchecking a group drops its values; checking one captures its current values.";
        const S32 others = KSLightingPresets::ungroupedCount(mName);
        if (others > 0)
        {
            note += llformat(" It also holds %d values outside these groups.", others);
        }
        ok = !checkedGroups().empty();
    }
    mNote->setText(note);
    mOKButton->setEnabled(ok);
}

void KSFloaterLightingPresetEdit::save(const std::string& name, const std::vector<std::string>& groups)
{
    if (saveAndApply(name, groups))
    {
        closeFloater();
    }
    else
    {
        mNote->setText(std::string("The preset could not be saved."));
    }
}

void KSFloaterLightingPresetEdit::onOK()
{
    std::string name = mNameEditor->getText();
    LLStringUtil::trim(name);
    if (mMode == "save")
    {
        const std::vector<std::string> groups = checkedGroups();
        if (KSLightingPresets::exists(name))
        {
            LLHandle<KSFloaterLightingPresetEdit> handle = getDerivedHandle<KSFloaterLightingPresetEdit>();
            confirm("Replace the lighting preset \"" + name + "\"?", [handle, name, groups]()
                {
                    if (KSFloaterLightingPresetEdit* self = handle.get())
                    {
                        self->save(name, groups);
                    }
                    else
                    {
                        saveAndApply(name, groups);
                    }
                });
            return;
        }
        save(name, groups);
    }
    else if (mMode == "rename")
    {
        if (KSLightingPresets::rename(mName, name))
        {
            closeFloater();
        }
        else
        {
            mNote->setText(std::string("The preset could not be renamed."));
        }
    }
    else if (mMode == "contents" && !KSLightingPresets::isBuiltIn(mName))
    {
        if (KSLightingPresets::setGroups(mName, checkedGroups()))
        {
            closeFloater();
        }
        else
        {
            mNote->setText(std::string("The preset could not be saved."));
        }
    }
}
