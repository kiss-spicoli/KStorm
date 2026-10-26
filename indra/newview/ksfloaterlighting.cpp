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
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewercontrol.h"

namespace
{
    constexpr S32 COMPACT_WIDTH = 44;
    constexpr S32 BUTTON_MARGIN = 8;

    std::string formatF32(F32 value, S32 digits)
    {
        return llformat("%.*f", digits, value);
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
        entry.mButton->setCommitCallback([this, floater](LLUICtrl*, const LLSD&)
            {
                KSFloaterLightingPane::cycleFloater(floater, this);
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
        const bool on_right = pane_rect.getCenterX() >= launcher_rect.getCenterX();
        LLRect candidate = pane_rect;
        candidate.translate(on_right ? launcher_rect.mRight - pane_rect.mLeft : launcher_rect.mLeft - pane_rect.mRight,
                            button_top - pane_rect.mTop);
        if (llabs(candidate.mLeft - pane_rect.mLeft) > radius() || llabs(candidate.mTop - pane_rect.mTop) > radius())
        {
            return nullptr;
        }
        slot = candidate;
        return launcher;
    }
    return nullptr;     // not a pane with a launcher button (Ambient opens from Settings...)
}

void KSFloaterLighting::onOpen(const LLSD& key)
{
    refreshStatuses();
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
    }
    LLFloater::draw();
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
    LLPointer<LLUIImage> arrow = LLUI::getUIImage(compact ? "Arrow_Small_Right" : "Arrow_Small_Left");
    mModeButton->setImageUnselected(arrow);
    mModeButton->setImageSelected(arrow);
    mModeButton->setImageHoverUnselected(arrow);
    mModeButton->setImageHoverSelected(arrow);
    mModeButton->setImagePressed(arrow);
    mModeButton->setImageDisabled(arrow);
    mModeButton->setImageDisabledSelected(arrow);
    mModeButton->setToolTip(std::string(compact ? "Show labels and status" : "Icons only"));
    setTitle(compact ? std::string() : getString("title_full"));
    // Keep the right edge in place: folding shrinks toward the right, unfolding grows to the left.
    const S32 right = getRect().mRight;
    reshape(width, getRect().getHeight());
    translate(right - getRect().mRight, 0);
}
