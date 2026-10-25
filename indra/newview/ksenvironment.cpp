/**
 * @file ksenvironment.cpp
 * @author Kissmy Spicoli
 * @brief "Set KS environment" (Use KS Reference) and the KStorm Lighting sky/water helpers.
 */

#include "llviewerprecompiledheaders.h"

#include "ksenvironment.h"

#include "llenvironment.h"
#include "llviewercontrol.h"
#include "pipeline.h"

#include <map>
#include <memory>

namespace
{
    // Viewer settings set by the KS environment. KSRenderLocalLightSubjectSort
    // comes from feat-local-light-subject-sort; missing controls are skipped.
    const std::map<std::string, LLSD> KS_SETTINGS =
    {
        { "RenderFarClip",                 LLSD(16.0) },
        { "RenderAvatarMaxNonImpostors",   LLSD(4) },
        { "AvatarNameTagMode",             LLSD(0) },
        { "KSRenderLocalLightSubjectSort", LLSD(true) },
        // Auto-exposure is pinned at 1 by the KS Sky HDR Scale anyway; off makes that visible.
        { "RenderDynamicExposureEnabled",  LLSD(false) },
    };

    // Personal Lighting values. Colors are the RGB equivalents of the color
    // picker HSL values (H/360, S/100, L/100), before Personal Lighting's
    // swatch scale (SLIDER_SCALE_SUN_AMBIENT).
    constexpr F32 KS_PROBE_AMBIANCE = 0.01f;
    const LLColor3 KS_SUN_COLOR(0.16f, 0.213333f, 0.24f);    // HSL 200, 20, 20 = RGB 41, 54, 61
    const LLColor3 KS_AMBIENT_COLOR(0.88f, 0.82f, 0.52f);    // HSL 50, 60, 70 = RGB 224, 209, 133
    constexpr F32 KS_HAZE_HORIZON = 0.15f;
    constexpr F32 KS_HAZE_DENSITY = 0.10f;
    constexpr F32 KS_SUN_ELEVATION = 6.0f;
    constexpr F32 KS_SUN_AZIMUTH = 0.0f;
    constexpr F32 KS_MOON_ELEVATION = -90.0f;
    constexpr F32 KS_MOON_AZIMUTH = 180.0f;
    constexpr F32 KS_GLOW_FOCUS = 0.10f;
    constexpr F32 KS_GLOW_SIZE = 1.90f;
    constexpr F32 KS_HDR_SCALE = 0.1f;

    // Personal Lighting UI scales (llfloaterenvironmentadjust.cpp).
    constexpr F32 SLIDER_SCALE_SUN_AMBIENT = 3.0f;
    constexpr F32 SLIDER_SCALE_GLOW_R = 20.0f;
    constexpr F32 SLIDER_SCALE_GLOW_B = -5.0f;

    struct SkyCache
    {
        F32 mGamma = 1.f;
        F32 mProbeAmbiance = 0.f;
        F32 mHazeHorizon = 0.f;
        F32 mHazeDensity = 0.f;
        LLColor3 mSunColor;
        LLColor3 mAmbientColor;
        LLColor3 mGlow;
        LLQuaternion mSunRotation;
        LLQuaternion mMoonRotation;
    };

    bool sConnected = false;
    bool sActive = false;
    // Set while KS changes the environment itself, so its own changes do not turn it off (A4).
    bool sBusy = false;
    // Set to turn KS off without restoring the sky: a new environment replaced it (A4).
    bool sSkipSkyRestore = false;
    bool sHadLocalEnvironment = false;
    bool sSkyCached = false;
    SkyCache sSkyCache;
    std::map<std::string, LLSD> sSettingsCache;

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

    void applySettings()
    {
        sSettingsCache.clear();
        for (const auto& [name, value] : KS_SETTINGS)
        {
            LLControlVariablePtr control = gSavedSettings.getControl(name);
            if (!control)
            {
                LL_INFOS("KStorm") << "KS environment: no setting " << name << ", skipped" << LL_ENDL;
                continue;
            }
            sSettingsCache[name] = control->getValue();
            control->setValue(value);
        }
    }

    void restoreSettings()
    {
        for (const auto& [name, value] : sSettingsCache)
        {
            if (LLControlVariablePtr control = gSavedSettings.getControl(name))
            {
                control->setValue(value);
            }
        }
        sSettingsCache.clear();
    }

    void applySky()
    {
        LLEnvironment& environment = LLEnvironment::instance();
        sHadLocalEnvironment = environment.hasEnvironment(LLEnvironment::ENV_LOCAL);

        LLSettingsSky::ptr_t sky = KSEnvironment::getEditableSky();
        sSkyCached = (sky != nullptr);
        if (!sky)
        {
            LL_WARNS("KStorm") << "KS environment: no sky to edit" << LL_ENDL;
            return;
        }

        // Cache the probe ambiance as Personal Lighting displays it.
        static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
        sSkyCache.mGamma = sky->getGamma();
        sSkyCache.mProbeAmbiance = sky->getReflectionProbeAmbiance(should_auto_adjust);
        sSkyCache.mHazeHorizon = sky->getHazeHorizon();
        sSkyCache.mHazeDensity = sky->getHazeDensity();
        sSkyCache.mSunColor = sky->getSunlightColor();
        sSkyCache.mAmbientColor = sky->getAmbientColor();
        sSkyCache.mGlow = sky->getGlow();
        sSkyCache.mSunRotation = sky->getSunRotation();
        sSkyCache.mMoonRotation = sky->getMoonRotation();

        sky->setGamma(KS_HDR_SCALE);
        sky->setReflectionProbeAmbiance(KS_PROBE_AMBIANCE);
        sky->setHazeHorizon(KS_HAZE_HORIZON);
        sky->setHazeDensity(KS_HAZE_DENSITY);
        sky->setSunlightColor(KS_SUN_COLOR * SLIDER_SCALE_SUN_AMBIENT);
        sky->setAmbientColor(KS_AMBIENT_COLOR * SLIDER_SCALE_SUN_AMBIENT);
        // Personal Lighting glow mapping: size 0-1.99 -> 40-0.2, focus scaled.
        sky->setGlow(LLColor3((2.0f - KS_GLOW_SIZE) * SLIDER_SCALE_GLOW_R, 0.f, KS_GLOW_FOCUS * SLIDER_SCALE_GLOW_B));
        sky->setSunRotation(rotationFromDegrees(KS_SUN_AZIMUTH, KS_SUN_ELEVATION));
        sky->setMoonRotation(rotationFromDegrees(KS_MOON_AZIMUTH, KS_MOON_ELEVATION));
        KSEnvironment::commitSky(sky);
    }

    void restoreSky()
    {
        if (!sSkyCached)
        {
            return;
        }
        sSkyCached = false;

        LLEnvironment& environment = LLEnvironment::instance();
        if (!sHadLocalEnvironment)
        {
            // KS created the local environment; drop it as Personal Lighting's Reset does.
            environment.clearEnvironment(LLEnvironment::ENV_LOCAL);
            environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
            return;
        }

        LLSettingsSky::ptr_t sky = KSEnvironment::getEditableSky();
        if (!sky)
        {
            return;
        }
        sky->setGamma(sSkyCache.mGamma);
        sky->setReflectionProbeAmbiance(sSkyCache.mProbeAmbiance);
        sky->setHazeHorizon(sSkyCache.mHazeHorizon);
        sky->setHazeDensity(sSkyCache.mHazeDensity);
        sky->setSunlightColor(sSkyCache.mSunColor);
        sky->setAmbientColor(sSkyCache.mAmbientColor);
        sky->setGlow(sSkyCache.mGlow);
        sky->setSunRotation(sSkyCache.mSunRotation);
        sky->setMoonRotation(sSkyCache.mMoonRotation);
        KSEnvironment::commitSky(sky);
    }

    void onSetEnvironmentChanged(const LLSD& new_value)
    {
        const bool enable = new_value.asBoolean();
        if (enable == sActive)
        {
            return;
        }
        sActive = enable;

        sBusy = true;
        if (enable)
        {
            applySettings();
            applySky();
        }
        else
        {
            restoreSettings();
            if (sSkipSkyRestore)
            {
                sSkyCached = false;
            }
            else
            {
                restoreSky();
            }
        }
        sBusy = false;
    }

    // Turns KS off, restoring its viewer settings but not the sky (A4).
    void turnOffKeepEnvironment()
    {
        if (!sActive)
        {
            return;
        }
        sSkipSkyRestore = true;
        gSavedSettings.setBOOL("KSSetEnvironment", false);
        sSkipSkyRestore = false;
    }

    // True while the local sky still holds the values KS set (edits through Personal Lighting
    // or a cloud texture change replace the sky object with a copy that keeps them).
    bool localSkyHasKSValues()
    {
        LLEnvironment& environment = LLEnvironment::instance();
        if (!environment.hasEnvironment(LLEnvironment::ENV_LOCAL) || environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
        {
            return false;
        }
        const LLSettingsSky::ptr_t sky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
        constexpr F32 TOLERANCE = 0.001f;
        return sky && fabsf(sky->getGamma() - KS_HDR_SCALE) < TOLERANCE
                   && fabsf(sky->getHazeDensity() - KS_HAZE_DENSITY) < TOLERANCE
                   && fabsf(sky->getReflectionProbeAmbiance(false) - KS_PROBE_AMBIANCE) < TOLERANCE;
    }

    // A4: Use Shared (the local environment is cleared) and applying an environment from
    // My Environments, the inventory or the World menu presets (set without a version) turn
    // KS off. Edits by KS itself, Personal Lighting and the celestial panels carry their own
    // versions and leave it on (A3).
    void onEnvironmentChanged(LLEnvironment::EnvSelection_t env, S32 version)
    {
        if (!sActive || sBusy || env != LLEnvironment::ENV_LOCAL)
        {
            return;
        }
        if (version == LLEnvironment::VERSION_CLEANUP
            || (version == LLEnvironment::NO_VERSION && !localSkyHasKSValues()))
        {
            turnOffKeepEnvironment();
        }
    }

    // Settings LLPipeline declares only in code (LLCachedControl in generateExposure()). Declared
    // here as LLControlCache would (same defaults, not persisted), so KS and the panes can use them
    // before the renderer first reads them.
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
    declareCodeSettings();
    if (LLControlVariablePtr control = gSavedSettings.getControl("KSSetEnvironment"))
    {
        control->getSignal()->connect([](LLControlVariable*, const LLSD& new_value, const LLSD&)
            {
                onSetEnvironmentChanged(new_value);
            });
        LLEnvironment::instance().setEnvironmentChanged(&onEnvironmentChanged);
        sConnected = true;
        // Catch up if it was turned on (Debug Settings) before the tab was built.
        onSetEnvironmentChanged(control->getValue());
    }
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
    // KS off first: restoring its sky afterwards would recreate a local environment (A4).
    turnOffKeepEnvironment();
    // Same as World > Environment > Use Shared Environment (llviewermenu.cpp, "region").
    gPipeline.mReflectionMapManager.reset();
    LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_LOCAL);
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
}

void KSEnvironment::restoreForShutdown()
{
    if (!sActive)
    {
        return;
    }
    sActive = false;
    restoreSettings();
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
