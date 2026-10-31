/**
 * @file ksfloaterlighting.cpp
 * @author Kissmy Spicoli
 * @brief KStorm Lighting launch list.
 */

#include "llviewerprecompiledheaders.h"

#include "ksfloaterlighting.h"

#include "ksenvironment.h"
#include "ksfloaterlightingpane.h"
#include "kslightingfields.h"
#include "kslightingpresets.h"
#include "ksmotionmode.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewercontrol.h"

#include <map>
#include <vector>

namespace
{
    constexpr S32 COMPACT_WIDTH = 44;
    constexpr S32 BUTTON_MARGIN = 8;
    constexpr S32 FIRST_OPEN_RIGHT_MARGIN = 20;

    std::string formatF32(F32 value, S32 digits)
    {
        return llformat("%.*f", digits, value);
    }

    // Docked panes and their side, kept in KSLightingDockedPanes as "name:R;name:L".
    std::map<std::string, bool> loadDocked()
    {
        std::map<std::string, bool> docked;
        std::vector<std::string> items;
        LLStringUtil::getTokens(gSavedSettings.getString("KSLightingDockedPanes"), items, ";");
        for (const std::string& item : items)
        {
            const size_t colon = item.rfind(':');
            if (colon != std::string::npos && colon > 0)
            {
                docked[item.substr(0, colon)] = item.substr(colon + 1) == "R";
            }
        }
        return docked;
    }

    void saveDocked(const std::map<std::string, bool>& docked)
    {
        std::string text;
        for (const auto& [name, on_right] : docked)
        {
            text += (text.empty() ? "" : ";") + name + (on_right ? ":R" : ":L");
        }
        if (text != gSavedSettings.getString("KSLightingDockedPanes"))
        {
            gSavedSettings.setString("KSLightingDockedPanes", text);
        }
    }

    // An image-only header button shows the same image in every state.
    void setButtonImage(LLButton* button, const std::string& image_name)
    {
        LLPointer<LLUIImage> image = LLUI::getUIImage(image_name);
        button->setImageUnselected(image);
        button->setImageSelected(image);
        button->setImageHoverUnselected(image);
        button->setImageHoverSelected(image);
        button->setImagePressed(image);
        button->setImageDisabled(image);
        button->setImageDisabledSelected(image);
    }
}

KSFloaterLighting::KSFloaterLighting(const LLSD& key)
    : LLFloater(key)
{
}

KSFloaterLighting::~KSFloaterLighting()
{
    for (auto& connection : mConnections)
    {
        connection.disconnect();
    }
    mEnvironmentConnection.disconnect();
}

bool KSFloaterLighting::postBuild()
{
    // Connects the KSSetEnvironment listener (Use KS Reference); this used to
    // happen in the retired KStorm Environment Effects panel.
    KSEnvironment::init();

    mFullWidth = getRect().getWidth();

    addEntry("quick", "ks_lighting_quick",
        []() { return std::string(gSavedSettings.getBOOL("KSSetEnvironment") ? "KS Reference" : ""); },
        { "KSSetEnvironment" });
    addEntry("presets", "ks_lighting_presets",
        []()
        {
            const std::string& active = KSLightingPresets::active();
            return active.empty() ? std::string("none active") : active;
        });
    addEntry("sunmoon", "ks_lighting_sunmoon",
        []()
        {
            LLSettingsSky::ptr_t sky = KSEnvironment::getDisplaySky();
            if (!sky)
            {
                return std::string();
            }
            return llformat("sun %.0f / %.0f, x%.2f",
                            KSLightingFields::getSkyScalar("sun_azimuth", *sky),
                            KSLightingFields::getSkyScalar("sun_elevation", *sky),
                            gSavedSettings.getF32("RenderHDRSkySunlightScale"));
        },
        { "RenderHDRSkySunlightScale" });
    addEntry("atmosphere", "ks_lighting_atmosphere",
        []()
        {
            LLSettingsSky::ptr_t sky = KSEnvironment::getDisplaySky();
            if (!sky)
            {
                return std::string();
            }
            const F32 haze = sky->getHazeDensity();
            return haze <= 0.f ? std::string("off (glow inactive)") : formatF32(haze, 2);
        });
    addEntry("clouds", "ks_lighting_clouds",
        []()
        {
            LLSettingsSky::ptr_t sky = KSEnvironment::getDisplaySky();
            return sky ? llformat("coverage %.0f%%", sky->getCloudShadow() * 100.f) : std::string();
        });
    addEntry("water", "ks_lighting_water");
    addEntry("reflections", "ks_lighting_reflections",
        []()
        {
            static const char* LEVELS[] = { "sky only", "manual only", "manual + terrain", "full" };
            return std::string(LEVELS[llclamp(gSavedSettings.getS32("RenderReflectionProbeLevel"), 0, 3)]);
        },
        { "RenderReflectionProbeLevel" });
    addEntry("env_effects", "as_environment_effects");
    mEntries.back().mProxy = "ks_lighting_proxy_env_effects";
    addEntry("all_lights", "ks_lighting_all_lights",
        []()
        {
            std::string text = llformat("%d max", gSavedSettings.getS32("RenderLocalLightCount"));
            if (gSavedSettings.getBOOL("KSLightRigOnlyMyLights"))
            {
                text += ", viewer only";
            }
            return text;
        },
        { "RenderLocalLightCount", "KSLightRigOnlyMyLights" });
    addEntry("viewer_lights", "as_my_lights");
    mEntries.back().mProxy = "ks_lighting_proxy_viewer_lights";
    addEntry("shadows", "ks_lighting_shadows",
        []()
        {
            static const char* DETAIL[] = { "off", "sun & moon", "sun, moon, projectors" };
            const S32 detail = llclamp(gSavedSettings.getS32("RenderShadowDetail"), 0, 2);
            std::string text = DETAIL[detail];
            if (detail > 0)
            {
                text += ", " + formatF32(gSavedSettings.getF32("RenderShadowResolutionScale"), 1) + "x";
            }
            return text;
        },
        { "RenderShadowDetail", "RenderShadowResolutionScale" });
    addEntry("exposure", "ks_lighting_exposure",
        []() { return formatF32(gSavedSettings.getF32("RenderExposure"), 1); },
        { "RenderExposure" });
    addEntry("glow", "ks_lighting_glow",
        []() { return std::string(gSavedSettings.getBOOL("RenderGlow") ? "on" : "off"); },
        { "RenderGlow" });
    addEntry("dof", "ks_lighting_dof",
        []()
        {
            if (!gSavedSettings.getBOOL("RenderDepthOfField"))
            {
                return std::string("off");
            }
            return llformat("on, f/%.1f, %.0f mm", gSavedSettings.getF32("CameraFNumber"),
                            gSavedSettings.getF32("CameraFocalLength"));
        },
        { "RenderDepthOfField", "CameraFNumber", "CameraFocalLength" });
    addEntry("vignette", "ks_lighting_vignette");
    addEntry("camera_effects", "as_camera_effects");
    mEntries.back().mProxy = "ks_lighting_proxy_camera_effects";
    // Motion mode is a switch, not a pane: no floater, pressed while on.
    addEntry("motion", "", []() { return std::string(KSMotionMode::isOn() ? "on" : ""); }, { "KSMotionMode" });
    mEntries.back().mButton->setCommitCallback([](LLUICtrl*, const LLSD&)
        {
            KSMotionMode::setOn(!KSMotionMode::isOn());
        });

    mModeButton = getChild<LLButton>("ks_mode");
    mModeButton->setCommitCallback([this](LLUICtrl*, const LLSD&) { setCompact(!mCompact); });
    // XUI places children below the header (LLFloater stretches the floater up by the header
    // height before postBuild()), so move the toggle into the header. It takes the slot a
    // minimize button would use, with the same size, offset and look as the header buttons
    // (LLFloater::buildButtons()): an image-only box that glows on hover.
    static LLUICachedControl<S32> close_box_size("UIFloaterCloseBoxSize", 0);
    static LLUICachedControl<S32> close_box_from_top("UICloseBoxFromTop", 0);
    LLRect mode_rect;
    mode_rect.setLeftTopAndSize(getRect().getWidth() - LLPANEL_BORDER_WIDTH - 2 * (close_box_size + 1),
                                getRect().getHeight() - close_box_from_top,
                                close_box_size, close_box_size);
    mModeButton->setRect(mode_rect);
    mModeButton->setLabel(std::string());
    mModeButton->setHoverGlowStrength(0.33f);

    // The dock button takes the same header slot while the mode toggle is hidden.
    mDockButton = getChild<LLButton>("ks_dock");
    mDockButton->setRect(mode_rect);
    mDockButton->setLabel(std::string());
    mDockButton->setHoverGlowStrength(0.33f);
    setButtonImage(mDockButton, "Icon_Restore_Foreground");
    mDockButton->setCommitCallback([this](LLUICtrl*, const LLSD&) { onDockToggle(); });

    mEnvironmentConnection = LLEnvironment::instance().setEnvironmentChanged(
        [this](LLEnvironment::EnvSelection_t, S32) { refreshStatuses(); });
    // Pane edits change the sky in place, which sends no environment signal.
    mConnections.push_back(KSEnvironment::editedSignal().connect([this]() { refreshStatuses(); }));
    mConnections.push_back(KSLightingPresets::changedSignal().connect([this]() { refreshStatuses(); }));

    refreshStatuses();
    // The launcher stays closed (icons only; the hover tooltip names the pane) and its
    // open/closed toggle is hidden in XUI. Open mode is kept for later: to bring it back,
    // show ks_mode and restore setCompact(gSavedSettings.getBOOL("KSLightingCompact")).
    setCompact(true);
    return true;
}

void KSFloaterLighting::addEntry(const std::string& key, const std::string& floater,
                                 std::function<std::string()> status_text,
                                 const std::vector<std::string>& watched)
{
    Entry entry;
    entry.mButton = getChild<LLButton>("ks_btn_" + key);
    entry.mStatus = getChild<LLTextBox>("ks_st_" + key);
    entry.mLabel = entry.mButton->getLabelUnselected();
    entry.mFloater = floater;
    entry.mStatusText = std::move(status_text);
    if (!floater.empty())
    {
        const size_t index = mEntries.size();
        entry.mButton->setCommitCallback([this, floater, index](LLUICtrl*, const LLSD&)
            {
                // Our panes open fully or close; the oversized AyaneStorm floaters open with their
                // proxy headers or close; others open, come forward or close.
                if (floater.compare(0, 12, "ks_lighting_") == 0)
                {
                    onPaneButton(mEntries[index]);
                }
                else if (!mEntries[index].mProxy.empty())
                {
                    onProxyButton(mEntries[index]);
                }
                else
                {
                    KSFloaterLightingPane::cycleFloater(floater, this);
                }
            });
    }
    for (const std::string& name : watched)
    {
        if (LLControlVariable* control = gSavedSettings.getControl(name))
        {
            mConnections.push_back(control->getSignal()->connect(
                [this](LLControlVariable*, const LLSD&, const LLSD&) { refreshStatuses(); }));
        }
    }
    mEntries.push_back(entry);
}

// static
LLFloater* KSFloaterLighting::findPaneSnap(LLFloater* pane, const LLRect& pane_rect, LLRect& slot)
{
    static LLCachedControl<S32> radius(gSavedSettings, "KSLightingPaneSnapRadius", 40);
    KSFloaterLighting* launcher = LLFloaterReg::findTypedInstance<KSFloaterLighting>("ks_lighting");
    if (radius() <= 0 || !pane || !launcher || !launcher->getVisible() || launcher->isMinimized()
        || launcher->getHost() || pane->getHost() || launcher->getParent() != pane->getParent())
    {
        return nullptr;
    }
    for (const Entry& entry : launcher->mEntries)
    {
        if (entry.mFloater != pane->getInstanceName())
        {
            continue;
        }
        // Both floaters share a parent, so their rects are in the same coordinates.
        const LLRect& launcher_rect = launcher->getRect();
        const S32 button_top = launcher_rect.mBottom + entry.mButton->getRect().mTop;
        const bool on_right = launcher->sideFor(pane, pane_rect.getCenterX() >= launcher_rect.getCenterX());
        LLRect candidate = pane_rect;
        candidate.translate(on_right ? launcher_rect.mRight - pane_rect.mLeft : launcher_rect.mLeft - pane_rect.mRight,
                            button_top - pane_rect.mTop);
        std::map<std::string, bool> docked = loadDocked();
        if (llabs(candidate.mLeft - pane_rect.mLeft) > radius() || llabs(candidate.mTop - pane_rect.mTop) > radius())
        {
            docked.erase(entry.mFloater);
            saveDocked(docked);
            return nullptr;
        }
        docked[entry.mFloater] = on_right;
        saveDocked(docked);
        slot = candidate;
        return launcher;
    }
    return nullptr;     // not a pane with a launcher button (Ambient opens from Settings...)
}

// static
bool KSFloaterLighting::isDocked(const std::string& name)
{
    return loadDocked().count(name) > 0 || !KSFloaterLightingProxy::targetOf(name).empty();
}

bool KSFloaterLighting::sideFor(LLFloater* pane, bool on_right) const
{
    const LLView* parent = getParent();
    if (!parent || !pane)
    {
        return on_right;
    }
    S32 width = pane->getRect().getWidth();
    if (KSFloaterLightingPaneBase* ks_pane = dynamic_cast<KSFloaterLightingPaneBase*>(pane))
    {
        width = llmax(width, ks_pane->getFullWidth());
    }
    const bool room_right = parent->getRect().getWidth() - getRect().mRight >= width;
    const bool room_left = getRect().mLeft >= width;
    if (on_right && !room_right && room_left)
    {
        return false;
    }
    if (!on_right && !room_left && room_right)
    {
        return true;
    }
    return on_right;
}

LLRect KSFloaterLighting::paneSlot(const Entry& entry, const LLRect& pane_rect, bool on_right) const
{
    const LLRect& launcher_rect = getRect();
    const S32 button_top = launcher_rect.mBottom + entry.mButton->getRect().mTop;
    LLRect slot = pane_rect;
    slot.translate(on_right ? launcher_rect.mRight - pane_rect.mLeft : launcher_rect.mLeft - pane_rect.mRight,
                   button_top - pane_rect.mTop);
    return slot;
}

std::map<std::string, bool> KSFloaterLighting::dockedPanes() const
{
    std::map<std::string, bool> docked = loadDocked();
    for (const Entry& entry : mEntries)
    {
        if (!entry.mProxy.empty())
        {
            docked[entry.mProxy] = false;
        }
    }
    return docked;
}

void KSFloaterLighting::onDockToggle()
{
    const std::map<std::string, bool> docked = dockedPanes();
    std::vector<const Entry*> panes;        // launcher order: top to bottom
    bool any_open = false;
    for (const Entry& entry : mEntries)
    {
        if (!entry.mFloater.empty() && docked.count(dockName(entry)))
        {
            panes.push_back(&entry);
            any_open |= LLFloaterReg::instanceVisible(dockName(entry));
        }
    }
    if (any_open)
    {
        for (const Entry* entry : panes)
        {
            if (LLFloaterReg::instanceVisible(dockName(*entry)))
            {
                LLFloaterReg::hideInstance(dockName(*entry));
            }
        }
        return;
    }
    // As each was when it closed (collapsed to the header or expanded); top to bottom, each in front.
    for (const Entry* entry : panes)
    {
        const std::string& name = dockName(*entry);
        if (LLFloater* pane = LLFloaterReg::showInstance(name, LLSD(), false))
        {
            KSFloaterLightingPaneBase* ks_pane = dynamic_cast<KSFloaterLightingPaneBase*>(pane);
            if (ks_pane)
            {
                ks_pane->setCollapsed(false);
            }
            pane->setShape(paneSlot(*entry, pane->getRect(), sideFor(pane, docked.at(name))));
            if (ks_pane)
            {
                // A proxy is only a header; a pane reopens as it was.
                ks_pane->setCollapsed(entry->mProxy.empty() ? KSFloaterLightingPaneBase::wasCollapsed(name) : true);
            }
            pane->setVisibleAndFrontmost(false);
        }
    }
}

void KSFloaterLighting::onPaneButton(const Entry& entry)
{
    KSFloaterLightingPaneBase* pane = dynamic_cast<KSFloaterLightingPaneBase*>(LLFloaterReg::findInstance(entry.mFloater));
    if (pane && pane->isShown() && !pane->isMinimized() && !pane->isCollapsed()
        && KSFloaterLightingPaneBase::topmostFloater(this) == pane)
    {
        pane->closeFloater();
        return;
    }
    pane = dynamic_cast<KSFloaterLightingPaneBase*>(LLFloaterReg::showInstance(entry.mFloater, LLSD(), false));
    if (!pane)
    {
        return;
    }
    if (pane->isMinimized())
    {
        pane->setMinimized(false);
    }
    pane->setCollapsed(false);
    const std::map<std::string, bool> docked = loadDocked();
    auto side = docked.find(entry.mFloater);
    if (side != docked.end())
    {
        pane->setShape(paneSlot(entry, pane->getRect(), sideFor(pane, side->second)));
    }
    pane->setVisibleAndFrontmost(true);
}

void KSFloaterLighting::onProxyButton(const Entry& entry)
{
    LLFloater* floater = LLFloaterReg::findInstance(entry.mFloater);
    if (floater && floater->isShown() && !floater->isMinimized()
        && KSFloaterLightingPaneBase::topmostFloater(this) == floater)
    {
        floater->closeFloater();
        if (LLFloater* proxy = LLFloaterReg::findInstance(entry.mProxy))
        {
            proxy->closeFloater();
        }
        return;
    }
    if (LLFloater* proxy = LLFloaterReg::showInstance(entry.mProxy, LLSD(), false))
    {
        proxy->setShape(paneSlot(entry, proxy->getRect(), sideFor(proxy, false)));
        proxy->setVisibleAndFrontmost(false);
    }
    if (LLFloater* opened = LLFloaterReg::showInstance(entry.mFloater, LLSD(), true))
    {
        if (opened->isMinimized())
        {
            opened->setMinimized(false);
        }
        opened->setVisibleAndFrontmost(true);
    }
}

void KSFloaterLighting::refreshDockButton()
{
    const std::map<std::string, bool> docked = dockedPanes();
    bool any_docked = false;
    bool any_open = false;
    for (const Entry& entry : mEntries)
    {
        if (!entry.mFloater.empty() && docked.count(dockName(entry)))
        {
            any_docked = true;
            any_open |= LLFloaterReg::instanceVisible(dockName(entry));
        }
    }
    mDockButton->setToolTip(std::string(!any_docked ? "No docked panes: drag a pane next to its button to dock it."
                                        : any_open  ? "Close the docked panes"
                                                    : "Open the docked panes"));
}

void KSFloaterLighting::onOpen(const LLSD& key)
{
    refreshStatuses();
    // Here, not in postBuild(): the floater's saved rect is applied after postBuild(), and only an
    // opened floater saves its rect.
    if (!gSavedSettings.getBOOL("KSLightingFirstOpenDone"))
    {
        gSavedSettings.setBOOL("KSLightingFirstOpenDone", true);
        applyFirstOpenLayout();
    }
}

void KSFloaterLighting::applyFirstOpenLayout()
{
    // Every pane docked on the left. Closed panes need no placing: each goes to its slot when it
    // opens. A side this install already chose stays: sideFor() still puts the pane on the left,
    // since the launcher has no room on its right. Proxies are always docked on the left
    // (dockedPanes()), and the AyaneStorm floaters are not touched.
    std::map<std::string, bool> docked = loadDocked();
    for (const Entry& entry : mEntries)
    {
        if (!entry.mFloater.empty() && entry.mProxy.empty())
        {
            docked.emplace(entry.mFloater, false);
        }
    }
    saveDocked(docked);
    closeDockedPanes();

    // How far below the launcher's top its panes reach. A pane's top follows its button, so the
    // tall panes on the low buttons reach below the launcher (Depth of field, by 172 px). Only
    // their height is needed, so they are built and stay closed.
    const S32 width = getRect().getWidth();
    const S32 height = getRect().getHeight();
    S32 reach = height;
    for (const Entry& entry : mEntries)
    {
        if (entry.mFloater.empty() || !entry.mProxy.empty())
        {
            continue;
        }
        if (LLFloater* pane = LLFloaterReg::getInstance(entry.mFloater))
        {
            reach = llmax(reach, getRect().mTop - paneSlot(entry, pane->getRect(), false).mBottom);
        }
    }

    // Near the right edge, halfway up, raised as far as the panes need. The launcher's header
    // comes first: a screen too short for both keeps it at the top. postBuild() already made the
    // launcher narrow.
    const LLRect snap = gFloaterView->getSnapRect();
    const S32 halfway_top = snap.mBottom + (snap.getHeight() + height) / 2;
    LLRect rect;
    rect.setLeftTopAndSize(snap.mRight - FIRST_OPEN_RIGHT_MARGIN - width,
                           llmin(llmax(halfway_top, snap.mBottom + reach), snap.mTop),
                           width, height);
    // As a user's move, so the floater saves it and keeps its place when the window resizes.
    setShape(rect, true);
}

void KSFloaterLighting::closeDockedPanes()
{
    const std::map<std::string, bool> docked = dockedPanes();
    for (const Entry& entry : mEntries)
    {
        const std::string& name = dockName(entry);
        if (!entry.mFloater.empty() && docked.count(name) && LLFloaterReg::instanceVisible(name))
        {
            LLFloaterReg::hideInstance(name);
        }
    }
}

void KSFloaterLighting::onClose(bool app_quitting)
{
    if (!app_quitting)
    {
        // Docked panes belong to the launcher: they go with it. Undocked panes stay.
        closeDockedPanes();
    }
    LLFloater::onClose(app_quitting);
}

void KSFloaterLighting::draw()
{
    // Buttons show pressed while their pane is open (design R15).
    for (const Entry& entry : mEntries)
    {
        if (!entry.mFloater.empty())
        {
            entry.mButton->setToggleState(LLFloaterReg::instanceVisible(entry.mFloater));
        }
        else
        {
            const bool on = KSMotionMode::isOn();
            entry.mButton->setToggleState(on);     // Motion mode, a switch
            // The meter reads fast while on and, mirrored, slow while off.
            if (on != mMotionIconOn)
            {
                mMotionIconOn = on;
                entry.mButton->setImageOverlay(on ? "Command_Performance_Icon" : "Command_KSMotionOff_Icon",
                    entry.mButton->getImageOverlayHAlign(), LLColor4::white % 0.75f);   // the XUI default color
            }
        }
    }
    refreshDockButton();
    keepDockedPanesInPlace();
    LLFloater::draw();
}

void KSFloaterLighting::keepDockedPanesInPlace()
{
    if (isMinimized() || getHost())
    {
        return;
    }
    const std::map<std::string, bool> docked = dockedPanes();
    // A pane being dragged follows the mouse; findPaneSnap() decides whether it stays docked.
    const LLView* captor = dynamic_cast<const LLView*>(gFocusMgr.getMouseCapture());
    for (const Entry& entry : mEntries)
    {
        auto side = docked.find(dockName(entry));
        if (entry.mFloater.empty() || side == docked.end())
        {
            continue;
        }
        LLFloater* pane = LLFloaterReg::findInstance(dockName(entry));
        if (!pane || !pane->getVisible() || pane->isMinimized() || pane->getHost()
            || pane->getParent() != getParent() || (captor && captor->hasAncestor(pane)))
        {
            continue;
        }
        const LLRect slot = paneSlot(entry, pane->getRect(), sideFor(pane, side->second));
        if (slot.mLeft != pane->getRect().mLeft || slot.mTop != pane->getRect().mTop)
        {
            pane->setShape(slot);
        }
    }
}

void KSFloaterLighting::refreshStatuses()
{
    for (const Entry& entry : mEntries)
    {
        entry.mStatus->setText(entry.mStatusText ? entry.mStatusText() : std::string());
    }
}

void KSFloaterLighting::setCompact(bool compact)
{
    mCompact = compact;
    gSavedSettings.setBOOL("KSLightingCompact", compact);

    const S32 width = compact ? COMPACT_WIDTH : mFullWidth;
    for (const Entry& entry : mEntries)
    {
        entry.mButton->setLabel(compact ? std::string() : entry.mLabel);
        entry.mStatus->setVisible(!compact);
        LLRect rect = entry.mButton->getRect();
        rect.mRight = rect.mLeft + width - BUTTON_MARGIN;
        entry.mButton->setRect(rect);
    }
    setButtonImage(mModeButton, compact ? "Arrow_Small_Right" : "Arrow_Small_Left");
    mModeButton->setToolTip(std::string(compact ? "Show labels and status" : "Icons only"));
    setTitle(compact ? std::string() : getString("title_full"));
    // Keep the right edge in place: folding shrinks toward the right, unfolding grows to the left.
    const S32 right = getRect().mRight;
    reshape(width, getRect().getHeight());
    translate(right - getRect().mRight, 0);
}
