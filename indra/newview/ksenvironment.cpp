/**
 * @file ksenvironment.cpp
 * @author Kissmy Spicoli
 * @brief Sky and water editing helpers for the KStorm Lighting panes and presets.
 */

#include "llviewerprecompiledheaders.h"

#include "ksenvironment.h"

#include "kslightingpresets.h"
#include "ksmotionmode.h"
#include "llenvironment.h"
#include "llviewercontrol.h"
#include "pipeline.h"

#include <memory>

namespace
{
    bool sConnected = false;

    // Our own copy of the ENV_LOCAL water (getEditableWater()). Weak: the environment owns it,
    // and a static owner would outlive the texture system at exit.
    std::weak_ptr<LLSettingsWater> sEditableWater;

    // Same conversion as ASPanelCelestialPosition::onPositionChanged().
    LLQuaternion rotationFromDegrees(F32 azimuth_deg, F32 elevation_deg)
    {
        F32 azimuth = azimuth_deg * DEG_TO_RAD;
        F32 elevation = elevation_deg * DEG_TO_RAD;
        if (is_approx_zero(elevation))
        {
            elevation = F_APPROXIMATELY_ZERO;
        }

        LLQuaternion rotation;
        rotation.setAngleAxis(-elevation, 0.f, 1.f, 0.f);
        LLQuaternion azimuth_rotation;
        azimuth_rotation.setAngleAxis(F_TWO_PI - azimuth, 0.f, 0.f, 1.f);
        rotation *= azimuth_rotation;
        return rotation;
    }

    // Settings LLPipeline declares only in code (LLCachedControl in generateExposure()). Declared
    // here as LLControlCache would (same defaults, not persisted), so presets and the panes can use
    // them before the renderer first reads them.
    void declareCodeSettings()
    {
        if (!gSavedSettings.controlExists("RenderDynamicExposureEnabled"))
        {
            gSavedSettings.declareBOOL("RenderDynamicExposureEnabled", true, "Declared In Code", LLControlVariable::PERSIST_NO);
        }
        if (!gSavedSettings.controlExists("RenderDynamicExposureSpeedError"))
        {
            gSavedSettings.declareF32("RenderDynamicExposureSpeedError", 0.1f, "Declared In Code", LLControlVariable::PERSIST_NO);
        }
        if (!gSavedSettings.controlExists("RenderDynamicExposureSpeedTarget"))
        {
            gSavedSettings.declareF32("RenderDynamicExposureSpeedTarget", 2.f, "Declared In Code", LLControlVariable::PERSIST_NO);
        }
    }
}

void KSEnvironment::init()
{
    if (sConnected)
    {
        return;
    }
    sConnected = true;
    declareCodeSettings();
    // Use KS Reference applies the Reference look, the built-in lighting preset.
    KSLightingPresets::init();
    KSMotionMode::init();
}

LLSettingsSky::ptr_t KSEnvironment::getEditableSky()
{
    // Same source as Personal Lighting and ASPanelCelestialPosition::getEditableSky().
    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t sky;

    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        sky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
        // A local environment set with water only shares the parcel's sky object; never edit that.
        const bool shared = sky && sky == environment.getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL, true);
        if ((environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL) || shared) && sky)
        {
            // Freeze the local day at its current frame, as Personal Lighting does.
            sky = sky->buildClone();
            environment.setEnvironment(LLEnvironment::ENV_LOCAL,
                LLEnvironment::fixedEnvironment_t(sky, LLSettingsWater::ptr_t()), KS_ENVIRONMENT_UPDATE);
        }
    }
    else
    {
        const LLSettingsSky::ptr_t parcel_sky =
            environment.getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL, true);
        if (!parcel_sky)
        {
            return nullptr;
        }
        sky = parcel_sky->buildClone();
        environment.setEnvironment(LLEnvironment::ENV_LOCAL,
            LLEnvironment::fixedEnvironment_t(sky, LLSettingsWater::ptr_t()), KS_ENVIRONMENT_UPDATE);
    }

    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
    return sky;
}

void KSEnvironment::commitSky(const LLSettingsSky::ptr_t& sky)
{
    if (!sky)
    {
        return;
    }
    sky->update();
    // Re-set the edited sky so ENV_LOCAL listeners refresh their controls.
    LLEnvironment& environment = LLEnvironment::instance();
    environment.setEnvironment(LLEnvironment::ENV_LOCAL,
        LLEnvironment::fixedEnvironment_t(sky, LLSettingsWater::ptr_t()), KS_ENVIRONMENT_UPDATE);
    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
    // Environment listeners may pause reflection updates (SL-20456).
    gPipeline.mReflectionMapManager.resume();
}

void KSEnvironment::useShared()
{
    // The active preset off first: restoring its sky afterwards would recreate a local environment (A4).
    KSLightingPresets::onUseShared();
    // Same as World > Environment > Use Shared Environment (llviewermenu.cpp, "region").
    gPipeline.mReflectionMapManager.reset();
    LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_LOCAL);
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
}

void KSEnvironment::restoreForShutdown()
{
    // Motion mode first, so the presets restore over the real values.
    KSMotionMode::restoreForShutdown();
    KSLightingPresets::restoreForShutdown();
}

LLSettingsWater::ptr_t KSEnvironment::getEditableWater()
{
    LLEnvironment& environment = LLEnvironment::instance();
    const bool has_local = environment.hasEnvironment(LLEnvironment::ENV_LOCAL);
    const bool local_day = has_local && environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL);
    if (has_local && !local_day)
    {
        const LLSettingsWater::ptr_t ours = sEditableWater.lock();
        if (ours && environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL) == ours)
        {
            return ours;
        }
    }

    const LLSettingsWater::ptr_t current = environment.getCurrentWater();
    if (!current)
    {
        return nullptr;
    }
    const LLSettingsWater::ptr_t water = current->buildClone();
    sEditableWater = water;

    // Without a local fixed sky, setEnvironment() would share the current sky object;
    // give the local environment its own copy (frozen at the current frame).
    LLSettingsSky::ptr_t sky;
    if (!has_local || local_day)
    {
        if (const LLSettingsSky::ptr_t current_sky = environment.getCurrentSky())
        {
            sky = current_sky->buildClone();
        }
    }
    environment.setEnvironment(LLEnvironment::ENV_LOCAL,
        LLEnvironment::fixedEnvironment_t(sky, water), KS_ENVIRONMENT_UPDATE);
    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
    return water;
}

LLSettingsSky::ptr_t KSEnvironment::getDisplaySky()
{
    LLEnvironment& environment = LLEnvironment::instance();
    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        if (LLSettingsSky::ptr_t sky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL))
        {
            return sky;
        }
    }
    return environment.getCurrentSky();
}

LLSettingsWater::ptr_t KSEnvironment::getDisplayWater()
{
    LLEnvironment& environment = LLEnvironment::instance();
    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        if (LLSettingsWater::ptr_t water = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL))
        {
            return water;
        }
    }
    return environment.getCurrentWater();
}

bool KSEnvironment::isAutoAdjustedLegacy(const LLSettingsSky::ptr_t& sky)
{
    static LLCachedControl<bool> auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    return sky && sky->canAutoAdjust() && auto_adjust();
}

LLQuaternion KSEnvironment::rotationFromDegrees(F32 azimuth_deg, F32 elevation_deg)
{
    return ::rotationFromDegrees(azimuth_deg, elevation_deg);
}

boost::signals2::signal<void()>& KSEnvironment::editedSignal()
{
    static boost::signals2::signal<void()> signal;
    return signal;
}

void KSEnvironment::notifyEdited()
{
    editedSignal()();
}
