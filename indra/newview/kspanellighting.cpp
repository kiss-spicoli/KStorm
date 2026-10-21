/**
 * @file kspanellighting.cpp
 * @author Kissmy Spicoli
 * @brief KS tab of the Environment Effects floater.
 */

#include "llviewerprecompiledheaders.h"

#include "kspanellighting.h"

#include "ksenvironment.h"
#include "llcombobox.h"
#include "llfeaturemanager.h"
#include "lltabcontainer.h"
#include "lluictrl.h"
#include "llviewercontrol.h"

static LLPanelInjector<KSPanelLighting> t_ks_lighting_panel("ks_lighting_panel");

KSPanelLighting::~KSPanelLighting()
{
    mEnvironmentConnection.disconnect();
}

bool KSPanelLighting::postBuild()
{
    KSEnvironment::init();

    mHdrScale = getChild<LLUICtrl>("ks_hdr_scale");
    mHdrScale->setCommitCallback([this](LLUICtrl*, const LLSD&) { onHdrScaleChanged(); });
    mShadowDetail = getChild<LLComboBox>("ks_shadow_detail");

    // HDR Scale is a property of the Personal Lighting sky, not a setting.
    mEnvironmentConnection = LLEnvironment::instance().setEnvironmentChanged(
        [this](LLEnvironment::EnvSelection_t env, S32)
        {
            if (env == LLEnvironment::ENV_LOCAL)
            {
                refreshHdrScale();
            }
        });

    // The panel is already in the tab container when postBuild() runs
    // (LLPanel::initPanelXML adds it to its parent first).
    if (LLControlVariablePtr control = gSavedSettings.getControl("KSLightingControls"))
    {
        mShowConnection = control->getSignal()->connect(
            [this](LLControlVariable*, const LLSD&, const LLSD&) { updateTabVisibility(); });
    }
    updateTabVisibility();

    refreshHdrScale();
    refreshShadowState();
    return true;
}

void KSPanelLighting::onVisibilityChange(bool new_visibility)
{
    LLPanel::onVisibilityChange(new_visibility);
    if (new_visibility)
    {
        refreshHdrScale();
        refreshShadowState();
    }
}

void KSPanelLighting::updateTabVisibility()
{
    LLTabContainer* tabs = dynamic_cast<LLTabContainer*>(getParent());
    if (!tabs)
    {
        return;
    }

    const bool show = gSavedSettings.getBOOL("KSLightingControls");
    if (tabs->getTabVisibility(this) == show)
    {
        return;
    }

    // setTabVisibility() selects the first visible tab; keep the user's tab.
    LLPanel* current = tabs->getCurrentPanel();
    tabs->setTabVisibility(this, show);
    if (current && current != this)
    {
        tabs->selectTabPanel(current);
    }
}

void KSPanelLighting::refreshHdrScale()
{
    if (!mHdrScale)
    {
        return;
    }

    // Show the Personal Lighting sky when present, otherwise the current
    // sky; a local sky is only created once the slider is moved.
    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t sky = environment.hasEnvironment(LLEnvironment::ENV_LOCAL)
        ? environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL)
        : environment.getCurrentSky();
    mHdrScale->setEnabled(sky != nullptr);
    if (sky)
    {
        mHdrScale->setValue(sky->getGamma());
    }
}

void KSPanelLighting::refreshShadowState()
{
    if (!mShadowDetail)
    {
        return;
    }

    // Same rules as LLFloaterPreference::refreshEnabledState().
    LLFeatureManager* features = LLFeatureManager::getInstance();
    const bool shadows_available = features->isFeatureAvailable("RenderShadowDetail");
    mShadowDetail->setEnabled(features->isFeatureAvailable("RenderDeferredSSAO") && shadows_available);
    if (!shadows_available)
    {
        mShadowDetail->setValue(0);
    }
}

void KSPanelLighting::onHdrScaleChanged()
{
    // Read first: creating the local sky fires refreshHdrScale(), which
    // would reset the slider to the old value.
    const F32 gamma = (F32)mHdrScale->getValue().asReal();
    LLSettingsSky::ptr_t sky = KSEnvironment::getEditableSky();
    if (!sky)
    {
        return;
    }
    // Edit in place like Personal Lighting's slider; commitSky() would reset
    // reflection probes on every drag step.
    sky->setGamma(gamma);
    sky->update();
}
