/**
 * @file ksfloaterlightingpane.cpp
 * @author Kissmy Spicoli
 * @brief One pane of the KStorm Lighting floater.
 */

#include "llviewerprecompiledheaders.h"

#include "ksfloaterlightingpane.h"

#include "ksfloaterlighting.h"
#include "kslightingfields.h"
#include "ksenvironment.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llenvironment.h"
#include "llcombobox.h"
#include "lldraghandle.h"
#include "llfeaturemanager.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "lllocalcliprect.h"
#include "llselectmgr.h"
#include "lltextbox.h"
#include "lluictrl.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llxmlnode.h"
#include "pipeline.h"

#include <map>

namespace
{
    const std::string VECTOR_PREFIX = "ksv_";
    const std::string SSAO_PREFIX = "ssao_";
    const std::string GTAO_PREFIX = "gtao_";
    const std::string SKY_PREFIX = "sky_";
    const std::string SKY_COLOR_PREFIX = "skyc_";
    const std::string WATER_PREFIX = "water_";
    const std::string WATER_COLOR_PREFIX = "waterc_";
    const std::string INTENSITY_SUFFIX = "_i";

    // Sky HDR Scale at or below this pins the auto-exposure range to 1 (LLPipeline::generateExposure()).
    constexpr F32 PINNED_HDR_SCALE = 0.25f;

    // Height of one check box or text row in the generated pane layout.
    constexpr S32 ROW_HEIGHT = 22;

    // AvatarNameTagMode to restore when "Show avatar names" is checked again (0 is off).
    S32 sLastNameTagMode = 1;

    bool startsWith(const std::string& s, const std::string& prefix)
    {
        return s.compare(0, prefix.size(), prefix) == 0;
    }
}

// static
void KSFloaterLightingPane::registerUICallbacks()
{
    LLUICtrl::CommitCallbackRegistry::defaultRegistrar().add("KS.CycleFloater",
        [](LLUICtrl* ctrl, const LLSD& param)
        {
            cycleFloater(param.asString(), ctrl ? ctrl->getParentByType<LLFloater>() : nullptr);
        });
}

// static
void KSFloaterLightingPane::cycleFloater(const std::string& name, const LLFloater* from_floater)
{
    LLFloater* target = LLFloaterReg::getInstance(name);
    if (!target)
    {
        return;
    }
    if (target->isMinimized() || !target->isShown())
    {
        LLFloaterReg::toggleInstanceOrBringToFront(name); // opens or restores it
        return;
    }
    if (KSFloaterLightingPaneBase::topmostFloater(from_floater) == target)
    {
        target->closeFloater();
    }
    else
    {
        target->setVisibleAndFrontmost(true);
    }
}

// ---------------------------------------------------------------------------
// KSFloaterLightingPaneBase

KSFloaterLightingPaneBase::KSFloaterLightingPaneBase(const LLSD& key)
    : LLFloater(key)
{
}

// static
LLFloater* KSFloaterLightingPaneBase::topmostFloater(const LLFloater* ignore)
{
    if (!gFloaterView)
    {
        return nullptr;
    }
    for (LLView* view : *gFloaterView->getChildList())
    {
        if (view == ignore || !view->getVisible() || view->isDead())
        {
            continue;
        }
        if (LLFloater* floater = dynamic_cast<LLFloater*>(view))
        {
            return floater;
        }
    }
    return nullptr;
}

void KSFloaterLightingPaneBase::setCollapsed(bool collapsed)
{
    if (collapsed == mCollapsed)
    {
        return;
    }
    mCollapsed = collapsed;
    applyPaneSize();
}

void KSFloaterLightingPaneBase::changeExpandedHeight(S32 delta)
{
    if (mFullWidth <= 0)
    {
        // Before initPaneSize() (still in postBuild): the XUI size is not final yet.
        LLRect rect = getRect();
        rect.mBottom -= delta;
        setShape(rect);
        return;
    }
    mFullHeight += delta;
    applyPaneSize();
}

void KSFloaterLightingPaneBase::initPaneSize()
{
    mFullWidth = getRect().getWidth();
    mFullHeight = getRect().getHeight();
}

void KSFloaterLightingPaneBase::applyPaneSize()
{
    if (mFullWidth <= 0 || isMinimized() || getHost())
    {
        return;
    }
    // Collapsed panes are header bars at half the width, so an expanded pane sticks out beyond
    // them. The top edge and the side edge facing the launcher stay put, so a docked pane
    // stays against it.
    const S32 width = mCollapsed ? mFullWidth / 2 : mFullWidth;
    const S32 height = mCollapsed ? getHeaderHeight() : mFullHeight;
    LLRect rect = getRect();
    if (rect.getWidth() == width && rect.getHeight() == height)
    {
        return;
    }
    LLFloater* launcher = LLFloaterReg::findInstance("ks_lighting");
    const bool keep_right = launcher && launcher->getParent() == getParent()
                            && rect.getCenterX() < launcher->getRect().getCenterX();
    if (keep_right)
    {
        rect.mLeft = rect.mRight - width;
    }
    else
    {
        rect.mRight = rect.mLeft + width;
    }
    rect.mBottom = rect.mTop - height;
    setShape(rect);
}

bool KSFloaterLightingPaneBase::handleMouseDown(S32 x, S32 y, MASK mask)
{
    mHeaderClickPending = false;
    const LLView* handle = getDragHandle();
    // A collapsed pane expands on the click that brings it forward; an expanded pane behind
    // others only comes forward.
    if (handle && handle->getVisible() && handle->getRect().pointInRect(x, y)
        && (mCollapsed || topmostFloater(LLFloaterReg::findInstance("ks_lighting")) == this))
    {
        // The header buttons (close, minimize) sit on the header bar.
        bool on_button = false;
        for (LLView* child : *getChildList())
        {
            if (child != handle && child->getVisible() && dynamic_cast<LLButton*>(child)
                && child->getRect().pointInRect(x, y))
            {
                on_button = true;
                break;
            }
        }
        mHeaderClickPending = !on_button;
        mHeaderClickRect = getRect();
    }
    return LLFloater::handleMouseDown(x, y, mask);
}

void KSFloaterLightingPaneBase::draw()
{
    // The size always follows the collapsed state (a stale size from anywhere is corrected).
    applyPaneSize();
    // The drag handle holds the mouse until the button is released; a release without
    // the pane having moved is a click.
    if (mHeaderClickPending && !gFocusMgr.getMouseCapture())
    {
        mHeaderClickPending = false;
        if (getRect().mLeft == mHeaderClickRect.mLeft && getRect().mTop == mHeaderClickRect.mTop)
        {
            onHeaderClicked();
        }
    }
    if (mCollapsed)
    {
        // The controls keep their places below the header; clip them away.
        LLLocalClipRect clip(getLocalRect());
        LLFloater::draw();
    }
    else
    {
        LLFloater::draw();
    }
}

void KSFloaterLightingPaneBase::setMinimized(bool minimize)
{
    if (minimize && !isMinimized() && KSFloaterLighting::isDocked(getInstanceName()))
    {
        setCollapsed(!mCollapsed);
        return;
    }
    LLFloater::setMinimized(minimize);
}

void KSFloaterLightingPaneBase::onClose(bool app_quitting)
{
    // KSLightingPaneStates holds "name:C" (collapsed) or "name:E" (expanded), separated by ";".
    const std::string& name = getInstanceName();
    std::vector<std::string> items;
    LLStringUtil::getTokens(gSavedSettings.getString("KSLightingPaneStates"), items, ";");
    std::string states;
    for (const std::string& item : items)
    {
        if (item.compare(0, name.size() + 1, name + ":") != 0)
        {
            states += (states.empty() ? "" : ";") + item;
        }
    }
    states += (states.empty() ? "" : ";") + name + (mCollapsed ? ":C" : ":E");
    gSavedSettings.setString("KSLightingPaneStates", states);

    setCollapsed(false);
    LLFloater::onClose(app_quitting);
}

// static
bool KSFloaterLightingPaneBase::wasCollapsed(const std::string& name)
{
    std::vector<std::string> items;
    LLStringUtil::getTokens(gSavedSettings.getString("KSLightingPaneStates"), items, ";");
    for (const std::string& item : items)
    {
        if (item.compare(0, name.size() + 1, name + ":") == 0)
        {
            return item.substr(name.size() + 1) != "E";
        }
    }
    return true;
}

LLView* KSFloaterLightingPaneBase::findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type,
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

// ---------------------------------------------------------------------------
// KSFloaterLightingProxy

namespace
{
    struct ProxyTarget
    {
        const char* mFloater;   // the AyaneStorm floater
        const char* mTitle;     // as its launcher button
    };
    const std::map<std::string, ProxyTarget> PROXIES =
    {
        { "ks_lighting_proxy_env_effects",    { "as_environment_effects", "Environment Effects" } },
        { "ks_lighting_proxy_viewer_lights",  { "as_my_lights",           "Viewer Lights" } },
        { "ks_lighting_proxy_camera_effects", { "as_camera_effects",      "Camera Effects" } },
    };
}

KSFloaterLightingProxy::KSFloaterLightingProxy(const LLSD& key)
    : KSFloaterLightingPaneBase(key)
{
}

bool KSFloaterLightingProxy::postBuild()
{
    // It stays in its slot left of the launcher: no dragging, so every header click is a click.
    setCanDrag(false);
    initPaneSize();
    return true;
}

void KSFloaterLightingProxy::onOpen(const LLSD& key)
{
    // The instance name is known only after building.
    auto proxy = PROXIES.find(getInstanceName());
    if (proxy != PROXIES.end())
    {
        setTitle(proxy->second.mTitle);
    }
    // Header only (its XUI has no content), and collapsed so every header click is ours.
    setCollapsed(true);
}

// static
std::string KSFloaterLightingProxy::targetOf(const std::string& proxy_name)
{
    auto proxy = PROXIES.find(proxy_name);
    return proxy == PROXIES.end() ? std::string() : std::string(proxy->second.mFloater);
}

void KSFloaterLightingProxy::onHeaderClicked()
{
    const std::string target = targetOf(getInstanceName());
    if (target.empty())
    {
        return;
    }
    // Closed: open it in front; open behind other floaters: bring it forward; open in front:
    // close it. The proxy itself, which the click brought forward, does not count.
    KSFloaterLightingPane::cycleFloater(target, this);
}

// ---------------------------------------------------------------------------
// KSFloaterLightingPane

KSFloaterLightingPane::KSFloaterLightingPane(const LLSD& key)
    : KSFloaterLightingPaneBase(key)
{
    // Before the XML binds: declares the code-only exposure settings the panes bind to, and
    // connects Use KS Reference even when a pane opens before the launch list.
    KSEnvironment::init();
}

KSFloaterLightingPane::~KSFloaterLightingPane()
{
    for (auto& connection : mConnections)
    {
        connection.disconnect();
    }
}

bool KSFloaterLightingPane::postBuild()
{
    connectVectorSliders();

    mLightBoxes = findChild<LLCheckBoxCtrl>("ks_light_boxes");
    if (mLightBoxes)
    {
        mLightBoxes->setCommitCallback([](LLUICtrl*, const LLSD&)
            {
                LLPipeline::toggleRenderDebug(LLPipeline::RENDER_DEBUG_LIGHTS);
            });
    }

    // control_name stores the setting; the selection manager keeps its own copy (llviewermenu.cpp).
    if (LLUICtrl* radius = findChild<LLUICtrl>("ks_light_radius"))
    {
        radius->setCommitCallback([](LLUICtrl* ctrl, const LLSD&)
            {
                LLSelectMgr::sRenderLightRadius = ctrl->getValue().asBoolean();
            });
    }

    connectEnvironment();
    connectQuickControls();
    fillDoFModes();

    if (findChild<LLUICtrl>("ks_ao_tech"))
    {
        if (LLControlVariable* control = gSavedSettings.getControl("RenderAOTechnique"))
        {
            mConnections.push_back(control->getSignal()->connect(
                [this](LLControlVariable*, const LLSD&, const LLSD&) { refreshAOTechnique(); }));
        }
    }
    // The full (expanded) size: the XUI size after the header stretch and Quick's note row.
    initPaneSize();
    return true;
}


void KSFloaterLightingPane::onOpen(const LLSD& key)
{
    refreshVectorSliders();
    refreshShadowState();
    refreshAOTechnique();
    refreshEnvironment();
    refreshKSState();
    refreshNameTags();
}

void KSFloaterLightingPane::draw()
{
    // The lights info display can also change from the Develop menu.
    if (mLightBoxes)
    {
        const bool on = gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_LIGHTS);
        if (mLightBoxes->getValue().asBoolean() != on)
        {
            mLightBoxes->setValue(on);
        }
    }
    // The render type can also change from the Advanced menu, Phototools or a shortcut.
    if (mShowAvatars)
    {
        const bool on = LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR);
        if (mShowAvatars->getValue().asBoolean() != on)
        {
            mShowAvatars->setValue(on);
        }
    }
    KSFloaterLightingPaneBase::draw();
}

void KSFloaterLightingPane::connectVectorSliders()
{
    for (LLView* view : *getChildList())
    {
        const std::string& name = view->getName();
        LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(view);
        // "ksv_<Setting>_<x|y|z>"
        if (!ctrl || !startsWith(name, VECTOR_PREFIX) || name.size() < VECTOR_PREFIX.size() + 3)
        {
            continue;
        }
        const char axis = name.back();
        if (name[name.size() - 2] != '_' || axis < 'x' || axis > 'z')
        {
            continue;
        }
        VectorSlider slider;
        slider.mCtrl = ctrl;
        slider.mSetting = name.substr(VECTOR_PREFIX.size(), name.size() - VECTOR_PREFIX.size() - 2);
        slider.mComponent = axis - 'x';
        LLControlVariable* control = gSavedSettings.getControl(slider.mSetting);
        if (!control)
        {
            LL_WARNS("KStorm") << "KS lighting pane: no setting " << slider.mSetting << LL_ENDL;
            ctrl->setEnabled(false);
            continue;
        }

        const std::string setting = slider.mSetting;
        const S32 component = slider.mComponent;
        ctrl->setCommitCallback([setting, component](LLUICtrl* c, const LLSD&)
            {
                LLVector3 value = gSavedSettings.getVector3(setting);
                value.mV[component] = (F32)c->getValue().asReal();
                gSavedSettings.setVector3(setting, value);
            });
        mConnections.push_back(control->getSignal()->connect(
            [this](LLControlVariable*, const LLSD&, const LLSD&) { refreshVectorSliders(); }));
        mVectorSliders.push_back(slider);
    }
    refreshVectorSliders();
}

void KSFloaterLightingPane::refreshVectorSliders()
{
    for (const VectorSlider& slider : mVectorSliders)
    {
        slider.mCtrl->setValue(gSavedSettings.getVector3(slider.mSetting).mV[slider.mComponent]);
    }
}

void KSFloaterLightingPane::refreshShadowState()
{
    LLComboBox* shadows = findChild<LLComboBox>("ks_shadow_detail");
    if (!shadows)
    {
        return;
    }
    // Same rules as LLFloaterPreference::refreshEnabledState().
    LLFeatureManager* features = LLFeatureManager::getInstance();
    const bool shadows_available = features->isFeatureAvailable("RenderShadowDetail");
    shadows->setEnabled(features->isFeatureAvailable("RenderDeferredSSAO") && shadows_available);
    if (!shadows_available)
    {
        shadows->setValue(0);
    }
}

void KSFloaterLightingPane::refreshAOTechnique()
{
    if (!findChild<LLUICtrl>("ks_ao_tech"))
    {
        return;
    }
    const bool gtao = gSavedSettings.getS32("RenderAOTechnique") != 0;
    for (LLView* view : *getChildList())
    {
        const std::string& name = view->getName();
        if (startsWith(name, SSAO_PREFIX))
        {
            view->setEnabled(!gtao);
        }
        else if (startsWith(name, GTAO_PREFIX))
        {
            view->setEnabled(gtao);
        }
    }
}

void KSFloaterLightingPane::connectEnvironment()
{
    for (LLView* view : *getChildList())
    {
        const std::string& name = view->getName();
        LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(view);
        const bool intensity = name.size() > INTENSITY_SUFFIX.size()
            && name.compare(name.size() - INTENSITY_SUFFIX.size(), INTENSITY_SUFFIX.size(), INTENSITY_SUFFIX) == 0;
        if (!ctrl || intensity)
        {
            continue; // intensity spinners are picked up with their swatch
        }
        EnvBinding binding;
        binding.mCtrl = ctrl;
        bool known = false;
        if (startsWith(name, SKY_COLOR_PREFIX))
        {
            binding.mKey = name.substr(SKY_COLOR_PREFIX.size());
            binding.mColor = true;
            known = KSLightingFields::hasSkyColor(binding.mKey);
        }
        else if (startsWith(name, WATER_COLOR_PREFIX))
        {
            binding.mKey = name.substr(WATER_COLOR_PREFIX.size());
            binding.mColor = binding.mWater = true;
            known = KSLightingFields::hasWaterColor(binding.mKey);
        }
        else if (startsWith(name, SKY_PREFIX))
        {
            binding.mKey = name.substr(SKY_PREFIX.size());
            known = KSLightingFields::hasSkyScalar(binding.mKey);
        }
        else if (startsWith(name, WATER_PREFIX))
        {
            binding.mKey = name.substr(WATER_PREFIX.size());
            binding.mWater = true;
            known = KSLightingFields::hasWaterScalar(binding.mKey);
        }
        if (!known)
        {
            continue;
        }
        if (binding.mColor)
        {
            binding.mIntensity = findChild<LLUICtrl>(name + INTENSITY_SUFFIX);
        }
        mEnvBindings.push_back(binding);
    }

    // Commit callbacks hold an index: the vector does not change after this point.
    for (size_t i = 0; i < mEnvBindings.size(); ++i)
    {
        auto commit = [this, i](LLUICtrl*, const LLSD&) { onEnvCommit(mEnvBindings[i]); };
        mEnvBindings[i].mCtrl->setCommitCallback(commit);
        if (mEnvBindings[i].mIntensity)
        {
            mEnvBindings[i].mIntensity->setCommitCallback(commit);
        }
    }

    mOrigin = findChild<LLTextBox>("ks_ro_origin");
    mProbeInUse = findChild<LLTextBox>("ks_ro_probe");
    mPinnedNote = findChildView("ks_note_pinned");
    if (!mPinnedNote)
    {
        mPinnedNote = findChildView("ks_note_pinned_row");
        mPinnedRow = (mPinnedNote != nullptr);
        setPinnedVisible(false);
    }

    if (mEnvBindings.empty() && !mOrigin && !mProbeInUse && !mPinnedNote)
    {
        return;
    }
    mConnections.push_back(LLEnvironment::instance().setEnvironmentChanged(
        [this](LLEnvironment::EnvSelection_t, S32) { refreshEnvironment(); }));
    mConnections.push_back(KSEnvironment::editedSignal().connect([this]() { refreshEnvironment(); }));
    for (const char* setting : { "RenderSkyAutoAdjustLegacy", "RenderSkyAutoAdjustHDRScale",
                                 "RenderUseExposureSkySettings", "RenderDynamicExposureEnabled" })
    {
        if (LLControlVariable* control = gSavedSettings.getControl(setting))
        {
            mConnections.push_back(control->getSignal()->connect(
                [this](LLControlVariable*, const LLSD&, const LLSD&) { refreshEnvironment(); }));
        }
    }
    refreshEnvironment();
}

void KSFloaterLightingPane::refreshEnvironment()
{
    if (mRefreshing)
    {
        return;
    }
    mRefreshing = true;

    const LLSettingsSky::ptr_t sky = KSEnvironment::getDisplaySky();
    const LLSettingsWater::ptr_t water = KSEnvironment::getDisplayWater();
    const bool legacy = KSEnvironment::isAutoAdjustedLegacy(sky);

    for (const EnvBinding& binding : mEnvBindings)
    {
        const bool available = binding.mWater ? water != nullptr : sky != nullptr;
        binding.mCtrl->setEnabled(available);
        if (binding.mIntensity)
        {
            binding.mIntensity->setEnabled(available);
        }
        if (!available)
        {
            continue;
        }
        if (binding.mColor)
        {
            const LLColor3 color = binding.mWater ? KSLightingFields::getWaterColor(binding.mKey, *water)
                                                  : KSLightingFields::getSkyColor(binding.mKey, *sky);
            if (LLColorSwatchCtrl* swatch = dynamic_cast<LLColorSwatchCtrl*>(binding.mCtrl))
            {
                if (binding.mIntensity)
                {
                    // The swatch shows the hue at full brightness; the spinner holds the brightness.
                    const F32 intensity = llmax(color.mV[0], llmax(color.mV[1], color.mV[2]));
                    swatch->set(intensity > 0.f ? LLColor4(color * (1.f / intensity), 1.f) : LLColor4::white);
                    binding.mIntensity->setValue(intensity);
                }
                else
                {
                    swatch->set(LLColor4(color, 1.f));
                }
            }
        }
        else if (!binding.mWater && binding.mKey == "gamma" && legacy)
        {
            // A legacy sky's HDR scale comes from the auto-adjust setting, not the sky.
            binding.mCtrl->setValue(gSavedSettings.getF32("RenderSkyAutoAdjustHDRScale"));
        }
        else
        {
            binding.mCtrl->setValue(binding.mWater ? KSLightingFields::getWaterScalar(binding.mKey, *water)
                                                   : KSLightingFields::getSkyScalar(binding.mKey, *sky));
        }
    }

    if (mOrigin)
    {
        mOrigin->setText(!sky ? std::string("-") : legacy ? std::string("Legacy (auto-adjusted)") : std::string("EEP"));
    }
    if (mProbeInUse && sky)
    {
        mProbeInUse->setText(llformat("%.2f", KSLightingFields::getSkyScalar("probe_fill", *sky)));
    }
    if (mPinnedNote)
    {
        const bool pinned = sky && gSavedSettings.getBOOL("RenderDynamicExposureEnabled")
                            && !gSavedSettings.getBOOL("RenderUseExposureSkySettings")
                            && sky->getGamma() <= PINNED_HDR_SCALE;
        setPinnedVisible(pinned);
    }
    mRefreshing = false;
}

void KSFloaterLightingPane::onEnvCommit(const EnvBinding& binding)
{
    // Read the control first: creating the local environment fires refreshEnvironment(),
    // which would reset the control to the old value.
    LLColor3 color;
    F32 value = 0.f;
    if (binding.mColor)
    {
        LLColorSwatchCtrl* swatch = dynamic_cast<LLColorSwatchCtrl*>(binding.mCtrl);
        if (!swatch)
        {
            return;
        }
        color = LLColor3(swatch->get());
        if (binding.mIntensity)
        {
            color = color * (F32)binding.mIntensity->getValue().asReal();
        }
    }
    else
    {
        value = (F32)binding.mCtrl->getValue().asReal();
    }

    if (binding.mWater)
    {
        LLSettingsWater::ptr_t water = KSEnvironment::getEditableWater();
        if (!water)
        {
            return;
        }
        if (binding.mColor)
        {
            KSLightingFields::setWaterColor(binding.mKey, *water, color);
        }
        else
        {
            KSLightingFields::setWaterScalar(binding.mKey, *water, value);
        }
        water->update();
    }
    else if (binding.mKey == "gamma" && KSEnvironment::isAutoAdjustedLegacy(KSEnvironment::getDisplaySky()))
    {
        gSavedSettings.setF32("RenderSkyAutoAdjustHDRScale", value);
    }
    else
    {
        // Edited in place like Personal Lighting's sliders; commitSky() would reset
        // reflection probes on every drag step.
        LLSettingsSky::ptr_t sky = KSEnvironment::getEditableSky();
        if (!sky)
        {
            return;
        }
        if (binding.mColor)
        {
            KSLightingFields::setSkyColor(binding.mKey, *sky, color);
        }
        else
        {
            KSLightingFields::setSkyScalar(binding.mKey, *sky, value);
        }
        sky->update();
    }
    KSEnvironment::notifyEdited();
}

void KSFloaterLightingPane::connectQuickControls()
{
    if (LLUICtrl* shared = findChild<LLUICtrl>("ks_use_shared"))
    {
        shared->setCommitCallback([](LLUICtrl*, const LLSD&) { KSEnvironment::useShared(); });
    }

    if (findChild<LLUICtrl>("ks_dynamic_exposure"))
    {
        if (LLControlVariable* control = gSavedSettings.getControl("KSSetEnvironment"))
        {
            mConnections.push_back(control->getSignal()->connect(
                [this](LLControlVariable*, const LLSD&, const LLSD&) { refreshKSState(); }));
        }
        refreshKSState();
    }

    mShowAvatars = findChild<LLCheckBoxCtrl>("ks_show_avatars");
    if (mShowAvatars)
    {
        mShowAvatars->setCommitCallback([](LLUICtrl* ctrl, const LLSD&)
            {
                // Same as Advanced > Rendering Types > Avatars ("character").
                if (ctrl->getValue().asBoolean() != LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR))
                {
                    LLPipeline::toggleRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR);
                }
            });
    }

    mNameTags = findChild<LLCheckBoxCtrl>("ks_name_tags");
    if (mNameTags)
    {
        // AvatarNameTagMode is 0 (off) or a display mode; a plain control_name binding would
        // turn any mode into 1 when checked again.
        mNameTags->setCommitCallback([](LLUICtrl* ctrl, const LLSD&)
            {
                gSavedSettings.setS32("AvatarNameTagMode", ctrl->getValue().asBoolean() ? sLastNameTagMode : 0);
            });
        if (LLControlVariable* control = gSavedSettings.getControl("AvatarNameTagMode"))
        {
            mConnections.push_back(control->getSignal()->connect(
                [this](LLControlVariable*, const LLSD&, const LLSD&) { refreshNameTags(); }));
        }
        refreshNameTags();
    }
}

void KSFloaterLightingPane::refreshKSState()
{
    // Use KS Reference turns Dynamic exposure off and restores it when turned off; keep it from
    // being turned back on in between (design section 10, Quick).
    if (LLUICtrl* dynamic_exposure = findChild<LLUICtrl>("ks_dynamic_exposure"))
    {
        dynamic_exposure->setEnabled(!gSavedSettings.getBOOL("KSSetEnvironment"));
    }
}

void KSFloaterLightingPane::refreshNameTags()
{
    if (!mNameTags)
    {
        return;
    }
    const S32 mode = gSavedSettings.getS32("AvatarNameTagMode");
    if (mode != 0)
    {
        sLastNameTagMode = mode;
    }
    mNameTags->setValue(mode != 0);
}

void KSFloaterLightingPane::fillDoFModes()
{
    LLComboBox* combo = findChild<LLComboBox>("ks_dof_mode");
    if (!combo)
    {
        return;
    }
    // Read AyaneStorm's list from her floater's XML, so ours follows any change to it.
    LLXMLNodePtr root;
    if (!LLUICtrlFactory::getLayeredXMLNode("floater_as_depth_of_field.xml", root) || root.isNull())
    {
        LL_WARNS("KStorm") << "KS lighting pane: cannot read floater_as_depth_of_field.xml" << LL_ENDL;
        combo->setEnabled(false);
        return;
    }
    for (LLXMLNodePtr node = root->getFirstChild(); node.notNull(); node = node->getNextSibling())
    {
        std::string name;
        if (!node->hasName("combo_box") || !node->getAttributeString("name", name) || name != "dof_mode")
        {
            continue;
        }
        for (LLXMLNodePtr item = node->getFirstChild(); item.notNull(); item = item->getNextSibling())
        {
            std::string label;
            std::string value;
            if (item->hasName("combo_box.item") && item->getAttributeString("label", label)
                && item->getAttributeString("value", value))
            {
                combo->add(label, LLSD(atoi(value.c_str())));
            }
        }
        break;
    }
    if (combo->getItemCount() == 0)
    {
        LL_WARNS("KStorm") << "KS lighting pane: no dof_mode items in floater_as_depth_of_field.xml" << LL_ENDL;
        combo->setEnabled(false);
        return;
    }
    // The items arrive after control_name bound the combo; select the current mode now.
    combo->setValue(gSavedSettings.getS32("ASDepthOfFieldMode"));
}

void KSFloaterLightingPane::setPinnedVisible(bool visible)
{
    if (!mPinnedNote)
    {
        return;
    }
    mPinnedNote->setVisible(visible);
    if (!mPinnedRow || visible == mPinnedRowShown)
    {
        return;
    }
    // Grow or shrink by the note's row, keeping the top edge (and every row) in place.
    mPinnedRowShown = visible;
    changeExpandedHeight(visible ? ROW_HEIGHT : -ROW_HEIGHT);
}
