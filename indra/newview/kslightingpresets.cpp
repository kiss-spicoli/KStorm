/**
 * @file kslightingpresets.cpp
 * @author Kissmy Spicoli
 * @brief KStorm Lighting Presets: storage, apply with restore point, KS Reference.
 */

#include "llviewerprecompiledheaders.h"

#include "kslightingpresets.h"

#include "ksenvironment.h"
#include "llcallbacklist.h"
#include "lldiriterator.h"
#include "llenvironment.h"
#include "llfile.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "lluictrlfactory.h"
#include "lluri.h"
#include "llviewercontrol.h"
#include "llxmlnode.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

const std::string KSLightingPresets::KS_REFERENCE = "KS Reference";

namespace
{
    const std::string PRESETS_DIR = "ks_lighting_presets";
    constexpr S32 FILE_VERSION = 1;

    // ---------------------------------------------------------------------
    // KS Reference values (moved from ksenvironment.cpp, unchanged)

    // Viewer settings set by KS Reference besides the reference list below.
    // KSRenderLocalLightSubjectSort comes from feat-local-light-subject-sort.
    const std::map<std::string, LLSD> KS_SETTINGS =
    {
        { "RenderFarClip",                 LLSD(16.0) },
        { "RenderAvatarMaxNonImpostors",   LLSD(4) },
        { "AvatarNameTagMode",             LLSD(0) },
        { "KSRenderLocalLightSubjectSort", LLSD(true) },
        // Auto-exposure is pinned at 1 by the KS Sky HDR Scale anyway; off makes that visible.
        { "RenderDynamicExposureEnabled",  LLSD(false) },
    };

    // Reference settings (design section 12.1, approved as A14): held at their
    // LL/FS/AS default while KS Reference is on, except the values listed in
    // KS_REFERENCE_VALUES. Most are reachable only through Debug Settings.
    const char* const KS_REFERENCE_DEFAULTS[] =
    {
        // (a) Debug Settings only
        "RenderHDREnabled", "RenderDefaultProbeUpdatePeriod", "RenderDesaturateIrradiance",
        "RenderReflectionProbeMaxLocalLightAmbiance", "RenderReflectionsEnabled",
        "RenderHeroProbeConservativeUpdateMultiplier",
        "RenderScreenSpaceReflectionIterations", "RenderScreenSpaceReflectionRayStep",
        "RenderScreenSpaceReflectionDistanceBias", "RenderScreenSpaceReflectionDepthRejectBias",
        "RenderScreenSpaceReflectionAdaptiveStepMultiplier", "RenderScreenSpaceReflectionGlossySamples",
        "RenderShadowSplits", "RenderShadowOffset", "RenderShadowOffsetError", "RenderShadowBiasError",
        "RenderShadowErrorCutoff", "RenderShadowNoise", "RenderSpotShadowBias",
        "RenderGTAOBackend", "RenderDeferredNoise", "RenderEdgeDepthCutoff", "RenderEdgeNormCutoff",
        "RenderEnableEmissiveBuffer",
        "RenderGlowLumWeights", "RenderGlowMinLuminance", "RenderGlowMaxExtractAlpha",
        "RenderGlowWarmthAmount", "RenderGlowWarmthWeights", "RenderGlowNoise",
        "RenderDeferredDisplayGamma",
        "RenderHDRIExposure", "RenderHDRIRotation", "RenderHDRISplitScreen", "RenderHDRIIrradianceOnly",
        "RenderSkyAutoAdjustLegacy", "RenderSkyAutoAdjustProbeAmbiance", "RenderSkyAutoAdjustAmbientScale",
        "RenderSkyAutoAdjustBlueDensityScale", "RenderSkyAutoAdjustBlueHorizonScale",
        "RenderSkyAutoAdjustSunColorScale",
        "RenderVolumetricLightingSampleCountOverride", "ASLightRigEyeMaskRadius", "KSHairOcclusionHeadOffset",
        // (b) pre-EEP, non-HDR and non-PBR
        "RenderDisableVintageMode", "WindLightUseAtmosShaders", "RenderSkySunlightScale",
        "RenderSunDynamicRange", "WaterGLFogDepthScale", "WaterGLFogDepthFloor", "RenderFogRatio",
        "RenderSpecularResX", "RenderSpecularResY", "RenderSpotLightsInNondeferred",
    };
    // Policy values (R1, R2): differ from the default, or must stay at it.
    const std::map<std::string, LLSD> KS_REFERENCE_VALUES =
    {
        { "RenderHDREnabled",          LLSD(true) },
        { "RenderSkyAutoAdjustLegacy", LLSD(true) },   // LL default is FALSE
        { "RenderDisableVintageMode",  LLSD(true) },
        { "WindLightUseAtmosShaders",  LLSD(true) },
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

    // ---------------------------------------------------------------------
    // Groups and the values they hold

    const std::vector<KSLightingPresets::Group> GROUPS =
    {
        { "sunmoon", "Sun/Moon" }, { "atmosphere", "Atmosphere" }, { "clouds", "Clouds" },
        { "water", "Water" }, { "reflections", "Reflections" }, { "all_lights", "All lights" },
        { "shadows", "Shadows" }, { "ambient", "Ambient" }, { "exposure", "Exposure" },
        { "glow", "Glow" }, { "dof", "Depth of field" }, { "vignette", "Vignette" },
        { "ayanestorm", "AyaneStorm floaters" },
    };

    // Viewer settings of a group come from the controls its pane shows, so the
    // two cannot drift apart. The AyaneStorm group reads her floaters' panels.
    const std::map<std::string, std::vector<std::string>> GROUP_FILES =
    {
        { "sunmoon",     { "floater_ks_lighting_sunmoon.xml" } },
        { "atmosphere",  { "floater_ks_lighting_atmosphere.xml" } },
        { "clouds",      { "floater_ks_lighting_clouds.xml" } },
        { "water",       { "floater_ks_lighting_water.xml" } },
        { "reflections", { "floater_ks_lighting_reflections.xml" } },
        { "all_lights",  { "floater_ks_lighting_all_lights.xml" } },
        { "shadows",     { "floater_ks_lighting_shadows.xml" } },
        { "ambient",     { "floater_ks_lighting_ambient.xml" } },
        { "exposure",    { "floater_ks_lighting_exposure.xml" } },
        { "glow",        { "floater_ks_lighting_glow.xml" } },
        { "dof",         { "floater_ks_lighting_dof.xml" } },
        { "vignette",    { "floater_ks_lighting_vignette.xml" } },
        { "ayanestorm",  { "panel_as_volumetric_lighting.xml", "panel_as_camera_effects.xml",
                           "panel_as_color_grading.xml", "panel_as_sun_settings.xml",
                           "panel_as_horizon_settings.xml", "panel_as_moon_settings.xml",
                           "panel_as_stars_settings.xml", "panel_as_aurora_settings.xml",
                           "panel_as_weather.xml", "floater_as_depth_of_field.xml" } },
    };
    // Settings AyaneStorm's color grading and weather panels set in code (per band, per effect).
    const char* const AYANESTORM_PREFIXES[] = { "ASColorGrade", "ASWeather" };
    // Never part of a preset: the switch of KS Reference itself.
    const char* const EXCLUDED_SETTINGS[] = { "KSSetEnvironment" };

    template <typename T>
    struct EnvField
    {
        std::string mGroup;
        std::function<LLSD(T&)> get;
        std::function<void(T&, const LLSD&)> set;
    };
    using SkyField = EnvField<LLSettingsSky>;
    using WaterField = EnvField<LLSettingsWater>;

    LLSD realValue(F32 value) { return LLSD((F64)value); }

    // Raw sky values (not the display values the panes show), so a preset round-trips exactly.
    const std::map<std::string, SkyField>& skyFields()
    {
        static const std::map<std::string, SkyField> fields =
        {
            { "sun_rotation", { "sunmoon", [](LLSettingsSky& s) { return s.getSunRotation().getValue(); },
                                [](LLSettingsSky& s, const LLSD& v) { LLQuaternion q; q.setValue(v); s.setSunRotation(q); } } },
            { "moon_rotation", { "sunmoon", [](LLSettingsSky& s) { return s.getMoonRotation().getValue(); },
                                 [](LLSettingsSky& s, const LLSD& v) { LLQuaternion q; q.setValue(v); s.setMoonRotation(q); } } },
            { "sunlight_color", { "sunmoon", [](LLSettingsSky& s) { return s.getSunlightColor().getValue(); },
                                  [](LLSettingsSky& s, const LLSD& v) { s.setSunlightColor(LLColor3(v)); } } },
            { "sun_scale", { "sunmoon", [](LLSettingsSky& s) { return realValue(s.getSunScale()); },
                             [](LLSettingsSky& s, const LLSD& v) { s.setSunScale((F32)v.asReal()); } } },
            { "moon_scale", { "sunmoon", [](LLSettingsSky& s) { return realValue(s.getMoonScale()); },
                              [](LLSettingsSky& s, const LLSD& v) { s.setMoonScale((F32)v.asReal()); } } },
            { "moon_brightness", { "sunmoon", [](LLSettingsSky& s) { return realValue(s.getMoonBrightness()); },
                                   [](LLSettingsSky& s, const LLSD& v) { s.setMoonBrightness((F32)v.asReal()); } } },
            { "star_brightness", { "sunmoon", [](LLSettingsSky& s) { return realValue(s.getStarBrightness()); },
                                   [](LLSettingsSky& s, const LLSD& v) { s.setStarBrightness((F32)v.asReal()); } } },

            { "max_y", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getMaxY()); },
                         [](LLSettingsSky& s, const LLSD& v) { s.setMaxY((F32)v.asReal()); } } },
            { "density_multiplier", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getDensityMultiplier()); },
                                      [](LLSettingsSky& s, const LLSD& v) { s.setDensityMultiplier((F32)v.asReal()); } } },
            { "distance_multiplier", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getDistanceMultiplier()); },
                                       [](LLSettingsSky& s, const LLSD& v) { s.setDistanceMultiplier((F32)v.asReal()); } } },
            { "moisture_level", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getSkyMoistureLevel()); },
                                  [](LLSettingsSky& s, const LLSD& v) { s.setSkyMoistureLevel((F32)v.asReal()); } } },
            { "droplet_radius", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getSkyDropletRadius()); },
                                  [](LLSettingsSky& s, const LLSD& v) { s.setSkyDropletRadius((F32)v.asReal()); } } },
            { "ice_level", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getSkyIceLevel()); },
                             [](LLSettingsSky& s, const LLSD& v) { s.setSkyIceLevel((F32)v.asReal()); } } },
            { "haze_density", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getHazeDensity()); },
                                [](LLSettingsSky& s, const LLSD& v) { s.setHazeDensity((F32)v.asReal()); } } },
            { "haze_horizon", { "atmosphere", [](LLSettingsSky& s) { return realValue(s.getHazeHorizon()); },
                                [](LLSettingsSky& s, const LLSD& v) { s.setHazeHorizon((F32)v.asReal()); } } },
            { "glow", { "atmosphere", [](LLSettingsSky& s) { return s.getGlow().getValue(); },
                        [](LLSettingsSky& s, const LLSD& v) { s.setGlow(LLColor3(v)); } } },
            { "blue_horizon", { "atmosphere", [](LLSettingsSky& s) { return s.getBlueHorizon().getValue(); },
                                [](LLSettingsSky& s, const LLSD& v) { s.setBlueHorizon(LLColor3(v)); } } },
            { "blue_density", { "atmosphere", [](LLSettingsSky& s) { return s.getBlueDensity().getValue(); },
                                [](LLSettingsSky& s, const LLSD& v) { s.setBlueDensity(LLColor3(v)); } } },

            { "cloud_color", { "clouds", [](LLSettingsSky& s) { return s.getCloudColor().getValue(); },
                               [](LLSettingsSky& s, const LLSD& v) { s.setCloudColor(LLColor3(v)); } } },
            { "cloud_shadow", { "clouds", [](LLSettingsSky& s) { return realValue(s.getCloudShadow()); },
                                [](LLSettingsSky& s, const LLSD& v) { s.setCloudShadow((F32)v.asReal()); } } },
            { "cloud_scale", { "clouds", [](LLSettingsSky& s) { return realValue(s.getCloudScale()); },
                               [](LLSettingsSky& s, const LLSD& v) { s.setCloudScale((F32)v.asReal()); } } },
            { "cloud_variance", { "clouds", [](LLSettingsSky& s) { return realValue(s.getCloudVariance()); },
                                  [](LLSettingsSky& s, const LLSD& v) { s.setCloudVariance((F32)v.asReal()); } } },
            { "cloud_pos_density1", { "clouds", [](LLSettingsSky& s) { return s.getCloudPosDensity1().getValue(); },
                                      [](LLSettingsSky& s, const LLSD& v) { s.setCloudPosDensity1(LLColor3(v)); } } },
            { "cloud_pos_density2", { "clouds", [](LLSettingsSky& s) { return s.getCloudPosDensity2().getValue(); },
                                      [](LLSettingsSky& s, const LLSD& v) { s.setCloudPosDensity2(LLColor3(v)); } } },

            { "ambient", { "ambient", [](LLSettingsSky& s) { return s.getAmbientColor().getValue(); },
                           [](LLSettingsSky& s, const LLSD& v) { s.setAmbientColor(LLColor3(v)); } } },
            // Read as the renderer uses it (a legacy sky's auto-adjusted value); setting it
            // makes a legacy copy an EEP sky, as the panes do.
            { "reflection_probe_ambiance", { "ambient",
                [](LLSettingsSky& s)
                {
                    static LLCachedControl<bool> auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
                    return realValue(s.getReflectionProbeAmbiance(auto_adjust()));
                },
                [](LLSettingsSky& s, const LLSD& v) { s.setReflectionProbeAmbiance((F32)v.asReal()); } } },

            { "gamma", { "exposure", [](LLSettingsSky& s) { return realValue(s.getGamma()); },
                         [](LLSettingsSky& s, const LLSD& v) { s.setGamma((F32)v.asReal()); } } },
        };
        return fields;
    }

    const std::map<std::string, WaterField>& waterFields()
    {
        static const std::map<std::string, WaterField> fields =
        {
            { "fog_color", { "water", [](LLSettingsWater& w) { return w.getWaterFogColor().getValue(); },
                             [](LLSettingsWater& w, const LLSD& v) { w.setWaterFogColor(LLColor3(v)); } } },
            { "fog_density", { "water", [](LLSettingsWater& w) { return realValue(w.getWaterFogDensity()); },
                               [](LLSettingsWater& w, const LLSD& v) { w.setWaterFogDensity((F32)v.asReal()); } } },
            { "fog_mod", { "water", [](LLSettingsWater& w) { return realValue(w.getFogMod()); },
                           [](LLSettingsWater& w, const LLSD& v) { w.setFogMod((F32)v.asReal()); } } },
            { "fresnel_scale", { "water", [](LLSettingsWater& w) { return realValue(w.getFresnelScale()); },
                                 [](LLSettingsWater& w, const LLSD& v) { w.setFresnelScale((F32)v.asReal()); } } },
            { "fresnel_offset", { "water", [](LLSettingsWater& w) { return realValue(w.getFresnelOffset()); },
                                  [](LLSettingsWater& w, const LLSD& v) { w.setFresnelOffset((F32)v.asReal()); } } },
            { "normal_scale", { "water", [](LLSettingsWater& w) { return w.getNormalScale().getValue(); },
                                [](LLSettingsWater& w, const LLSD& v) { LLVector3 n; n.setValue(v); w.setNormalScale(n); } } },
            { "scale_above", { "water", [](LLSettingsWater& w) { return realValue(w.getScaleAbove()); },
                               [](LLSettingsWater& w, const LLSD& v) { w.setScaleAbove((F32)v.asReal()); } } },
            { "scale_below", { "water", [](LLSettingsWater& w) { return realValue(w.getScaleBelow()); },
                               [](LLSettingsWater& w, const LLSD& v) { w.setScaleBelow((F32)v.asReal()); } } },
            { "blur_multiplier", { "water", [](LLSettingsWater& w) { return realValue(w.getBlurMultiplier()); },
                                   [](LLSettingsWater& w, const LLSD& v) { w.setBlurMultiplier((F32)v.asReal()); } } },
        };
        return fields;
    }

    void collectControls(LLXMLNodePtr node, std::set<std::string>& names)
    {
        std::string value;
        if (node->getAttributeString("control_name", value) && !value.empty())
        {
            names.insert(value);
        }
        // Vector components handled by KSFloaterLightingPane: "ksv_<Setting>_<x|y|z>".
        if (node->getAttributeString("name", value) && value.size() > 6 && value.compare(0, 4, "ksv_") == 0
            && value[value.size() - 2] == '_')
        {
            names.insert(value.substr(4, value.size() - 6));
        }
        for (LLXMLNodePtr child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
        {
            collectControls(child, names);
        }
    }

    struct PrefixCollector : public LLControlGroup::ApplyFunctor
    {
        std::set<std::string>& mNames;
        explicit PrefixCollector(std::set<std::string>& names) : mNames(names) {}
        void apply(const std::string& name, LLControlVariable*) override
        {
            for (const char* prefix : AYANESTORM_PREFIXES)
            {
                if (name.compare(0, strlen(prefix), prefix) == 0)
                {
                    mNames.insert(name);
                }
            }
        }
    };

    // Viewer settings of a group, read once from its XUI files.
    const std::vector<std::string>& groupSettings(const std::string& group_id)
    {
        static std::map<std::string, std::vector<std::string>> cache;
        auto found = cache.find(group_id);
        if (found != cache.end())
        {
            return found->second;
        }
        std::set<std::string> names;
        auto files = GROUP_FILES.find(group_id);
        if (files != GROUP_FILES.end())
        {
            for (const std::string& file : files->second)
            {
                LLXMLNodePtr root;
                if (LLUICtrlFactory::getLayeredXMLNode(file, root) && root.notNull())
                {
                    collectControls(root, names);
                }
                else
                {
                    LL_WARNS("KStorm") << "Lighting presets: cannot read " << file << LL_ENDL;
                }
            }
        }
        if (group_id == "ayanestorm")
        {
            PrefixCollector collector(names);
            gSavedSettings.applyToAll(&collector);
        }
        std::vector<std::string>& result = cache[group_id];
        for (const std::string& name : names)
        {
            const bool excluded = std::find(std::begin(EXCLUDED_SETTINGS), std::end(EXCLUDED_SETTINGS), name)
                                  != std::end(EXCLUDED_SETTINGS);
            if (!excluded && gSavedSettings.controlExists(name))
            {
                result.push_back(name);
            }
        }
        return result;
    }

    // ---------------------------------------------------------------------
    // Presets

    struct Preset
    {
        std::string mName;
        std::vector<std::string> mGroups;
        LLSD mSettings = LLSD::emptyMap();      // setting name -> value
        LLSD mSky = LLSD::emptyMap();           // sky field -> value
        LLSD mWater = LLSD::emptyMap();         // water field -> value
        LLSD mSkyFull;                          // the whole sky and water, kept for a later
        LLSD mWaterFull;                        // "Save sky to My Environments" (section 12.7)

        bool hasEnvironment() const { return mSky.size() > 0 || mWater.size() > 0; }
    };

    struct RestorePoint
    {
        LLSD mSettings = LLSD::emptyMap();
        LLSD mSky = LLSD::emptyMap();
        LLSD mWater = LLSD::emptyMap();
        bool mHadLocal = false;
    };

    bool sConnected = false;
    std::string sActive;
    Preset sActivePreset;
    RestorePoint sRestore;
    // Set while presets change settings or the environment themselves, so their own
    // changes do not turn the active preset off (A4) or schedule a reapply (D1, D2).
    bool sBusy = false;
    // Set to turn KS Reference off without restoring the environment (A4).
    bool sKeepEnvironment = false;
    bool sReapplyPending = false;

    const Preset& ksReference()
    {
        static Preset preset;
        if (!preset.mName.empty())
        {
            return preset;
        }
        preset.mName = KSLightingPresets::KS_REFERENCE;
        for (const auto& [name, value] : KS_SETTINGS)
        {
            preset.mSettings[name] = value;
        }
        for (const char* name : KS_REFERENCE_DEFAULTS)
        {
            auto policy = KS_REFERENCE_VALUES.find(name);
            if (policy != KS_REFERENCE_VALUES.end())
            {
                preset.mSettings[name] = policy->second;
            }
            else if (LLControlVariable* control = gSavedSettings.getControl(name))
            {
                preset.mSettings[name] = control->getDefault();
            }
        }
        preset.mSky["gamma"] = realValue(KS_HDR_SCALE);
        preset.mSky["reflection_probe_ambiance"] = realValue(KS_PROBE_AMBIANCE);
        preset.mSky["haze_horizon"] = realValue(KS_HAZE_HORIZON);
        preset.mSky["haze_density"] = realValue(KS_HAZE_DENSITY);
        preset.mSky["sunlight_color"] = (KS_SUN_COLOR * SLIDER_SCALE_SUN_AMBIENT).getValue();
        preset.mSky["ambient"] = (KS_AMBIENT_COLOR * SLIDER_SCALE_SUN_AMBIENT).getValue();
        // Personal Lighting glow mapping: size 0-1.99 -> 40-0.2, focus scaled.
        preset.mSky["glow"] = LLColor3((2.0f - KS_GLOW_SIZE) * SLIDER_SCALE_GLOW_R, 0.f,
                                       KS_GLOW_FOCUS * SLIDER_SCALE_GLOW_B).getValue();
        preset.mSky["sun_rotation"] = KSEnvironment::rotationFromDegrees(KS_SUN_AZIMUTH, KS_SUN_ELEVATION).getValue();
        preset.mSky["moon_rotation"] = KSEnvironment::rotationFromDegrees(KS_MOON_AZIMUTH, KS_MOON_ELEVATION).getValue();
        return preset;
    }

    std::string presetsDir()
    {
        return gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, PRESETS_DIR);
    }

    std::string presetPath(const std::string& name)
    {
        return presetsDir() + gDirUtilp->getDirDelimiter() + LLURI::escape(name) + ".xml";
    }

    LLSD toLLSD(const Preset& preset)
    {
        LLSD data;
        data["version"] = FILE_VERSION;
        data["name"] = preset.mName;
        data["groups"] = LLSD::emptyArray();
        for (const std::string& group : preset.mGroups)
        {
            data["groups"].append(group);
        }
        data["settings"] = preset.mSettings;
        data["sky"] = preset.mSky;
        data["water"] = preset.mWater;
        if (preset.mSkyFull.isDefined())
        {
            data["sky_full"] = preset.mSkyFull;
        }
        if (preset.mWaterFull.isDefined())
        {
            data["water_full"] = preset.mWaterFull;
        }
        return data;
    }

    bool loadPreset(const std::string& name, Preset& preset)
    {
        llifstream file(presetPath(name).c_str());
        if (!file.is_open())
        {
            return false;
        }
        LLSD data;
        if (LLSDSerialize::fromXML(data, file) == LLSDParser::PARSE_FAILURE || !data.isMap())
        {
            LL_WARNS("KStorm") << "Lighting presets: cannot parse " << presetPath(name) << LL_ENDL;
            return false;
        }
        preset = Preset();
        preset.mName = name;
        if (data["groups"].isArray())
        {
            for (const LLSD& group : llsd::inArray(data["groups"]))
            {
                preset.mGroups.push_back(group.asString());
            }
        }
        if (data["settings"].isMap()) preset.mSettings = data["settings"];
        if (data["sky"].isMap()) preset.mSky = data["sky"];
        if (data["water"].isMap()) preset.mWater = data["water"];
        preset.mSkyFull = data["sky_full"];
        preset.mWaterFull = data["water_full"];
        return true;
    }

    bool savePreset(const Preset& preset)
    {
        LLFile::mkdir(presetsDir());
        llofstream file(presetPath(preset.mName).c_str());
        if (!file.is_open())
        {
            LL_WARNS("KStorm") << "Lighting presets: cannot write " << presetPath(preset.mName) << LL_ENDL;
            return false;
        }
        LLSDSerialize::toPrettyXML(toLLSD(preset), file);
        return true;
    }

    bool findPreset(const std::string& name, Preset& preset)
    {
        if (name == KSLightingPresets::KS_REFERENCE)
        {
            preset = ksReference();
            return true;
        }
        return loadPreset(name, preset);
    }

    // Values within this distance count as unchanged when matching the local sky (A4).
    bool approxEqual(const LLSD& a, const LLSD& b)
    {
        constexpr F64 TOLERANCE = 0.001;
        if (a.isArray() && b.isArray())
        {
            if (a.size() != b.size())
            {
                return false;
            }
            for (S32 i = 0; i < a.size(); ++i)
            {
                if (!approxEqual(a[i], b[i]))
                {
                    return false;
                }
            }
            return true;
        }
        if (a.isReal() || b.isReal())
        {
            return fabs(a.asReal() - b.asReal()) < TOLERANCE;
        }
        return llsd_equals(a, b);
    }

    // ---------------------------------------------------------------------
    // Apply and turn off

    void applyEnvironment(const LLSD& sky_values, const LLSD& water_values, LLSD* sky_prior, LLSD* water_prior)
    {
        if (sky_values.size() > 0)
        {
            if (LLSettingsSky::ptr_t sky = KSEnvironment::getEditableSky())
            {
                for (const auto& [key, value] : llsd::inMap(sky_values))
                {
                    auto field = skyFields().find(key);
                    if (field == skyFields().end())
                    {
                        continue;
                    }
                    if (sky_prior)
                    {
                        (*sky_prior)[key] = field->second.get(*sky);
                    }
                    field->second.set(*sky, value);
                }
                KSEnvironment::commitSky(sky);
            }
        }
        if (water_values.size() > 0)
        {
            if (LLSettingsWater::ptr_t water = KSEnvironment::getEditableWater())
            {
                for (const auto& [key, value] : llsd::inMap(water_values))
                {
                    auto field = waterFields().find(key);
                    if (field == waterFields().end())
                    {
                        continue;
                    }
                    if (water_prior)
                    {
                        (*water_prior)[key] = field->second.get(*water);
                    }
                    field->second.set(*water, value);
                }
                water->update();
            }
        }
        KSEnvironment::notifyEdited();
    }

    void applyInternal(const Preset& preset)
    {
        sBusy = true;
        sRestore = RestorePoint();
        for (const auto& [name, value] : llsd::inMap(preset.mSettings))
        {
            LLControlVariable* control = gSavedSettings.getControl(name);
            if (!control)
            {
                LL_INFOS("KStorm") << "Lighting presets: no setting " << name << ", skipped" << LL_ENDL;
                continue;
            }
            sRestore.mSettings[name] = control->getValue();
            // Only values that differ: several settings rebuild GPU buffers on any write (section 12.5).
            if (!llsd_equals(control->getValue(), value))
            {
                control->setValue(value);
            }
        }
        if (preset.hasEnvironment())
        {
            sRestore.mHadLocal = LLEnvironment::instance().hasEnvironment(LLEnvironment::ENV_LOCAL);
            applyEnvironment(preset.mSky, preset.mWater, &sRestore.mSky, &sRestore.mWater);
        }
        sActive = preset.mName;
        sActivePreset = preset;
        sBusy = false;
        KSLightingPresets::changedSignal()();
    }

    void restoreSettings()
    {
        for (const auto& [name, value] : llsd::inMap(sRestore.mSettings))
        {
            LLControlVariable* control = gSavedSettings.getControl(name);
            if (control && !llsd_equals(control->getValue(), value))
            {
                control->setValue(value);
            }
        }
    }

    void turnOffInternal(bool restore_environment)
    {
        if (sActive.empty())
        {
            return;
        }
        sBusy = true;
        restoreSettings();
        if (restore_environment && sActivePreset.hasEnvironment())
        {
            LLEnvironment& environment = LLEnvironment::instance();
            if (!sRestore.mHadLocal)
            {
                // The preset created the local environment; drop it as Personal Lighting's Reset does.
                environment.clearEnvironment(LLEnvironment::ENV_LOCAL);
                environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
            }
            else
            {
                applyEnvironment(sRestore.mSky, sRestore.mWater, nullptr, nullptr);
            }
        }
        sActive.clear();
        sActivePreset = Preset();
        sRestore = RestorePoint();
        sBusy = false;
        KSLightingPresets::changedSignal()();
    }

    // KSSetEnvironment is KS Reference's switch.
    void onKSSetEnvironmentChanged(const LLSD& new_value)
    {
        if (new_value.asBoolean())
        {
            if (sActive == KSLightingPresets::KS_REFERENCE)
            {
                return;
            }
            turnOffInternal(true);
            applyInternal(ksReference());
        }
        else if (sActive == KSLightingPresets::KS_REFERENCE)
        {
            turnOffInternal(!sKeepEnvironment);
        }
    }

    // True while the local sky and water still hold the active preset's values
    // (Personal Lighting's cloud texture picker replaces the sky with such a copy).
    bool localEnvironmentMatches()
    {
        LLEnvironment& environment = LLEnvironment::instance();
        if (!environment.hasEnvironment(LLEnvironment::ENV_LOCAL) || environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
        {
            return false;
        }
        if (sActivePreset.mSky.size() > 0)
        {
            LLSettingsSky::ptr_t sky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
            if (!sky)
            {
                return false;
            }
            for (const auto& [key, value] : llsd::inMap(sActivePreset.mSky))
            {
                auto field = skyFields().find(key);
                if (field != skyFields().end() && !approxEqual(field->second.get(*sky), value))
                {
                    return false;
                }
            }
        }
        if (sActivePreset.mWater.size() > 0)
        {
            LLSettingsWater::ptr_t water = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL);
            if (!water)
            {
                return false;
            }
            for (const auto& [key, value] : llsd::inMap(sActivePreset.mWater))
            {
                auto field = waterFields().find(key);
                if (field != waterFields().end() && !approxEqual(field->second.get(*water), value))
                {
                    return false;
                }
            }
        }
        return true;
    }

    // A4: Use Shared (the local environment is cleared) and applying an environment
    // set without a version (My Environments, inventory, World menu presets) turn off
    // a preset that holds sky or water values. Edits by KStorm, Personal Lighting and
    // the celestial panels carry their own versions and leave it on (A3).
    void onEnvironmentChanged(LLEnvironment::EnvSelection_t env, S32 version)
    {
        if (sActive.empty() || sBusy || env != LLEnvironment::ENV_LOCAL || !sActivePreset.hasEnvironment())
        {
            return;
        }
        if (version == LLEnvironment::VERSION_CLEANUP
            || (version == LLEnvironment::NO_VERSION && !localEnvironmentMatches()))
        {
            KSLightingPresets::turnOff(false);
        }
    }

    // D1, D2: a quality change or a graphics preset load rewrites settings; write the
    // active preset's again on the next idle. The values the load brought become the
    // ones restored when the preset is turned off.
    void reapplySettings()
    {
        sReapplyPending = false;
        if (sActive.empty())
        {
            return;
        }
        sBusy = true;
        for (const auto& [name, value] : llsd::inMap(sActivePreset.mSettings))
        {
            LLControlVariable* control = gSavedSettings.getControl(name);
            if (control && !llsd_equals(control->getValue(), value))
            {
                sRestore.mSettings[name] = control->getValue();
                control->setValue(value);
            }
        }
        sBusy = false;
    }

    void scheduleReapply()
    {
        if (sActive.empty() || sBusy || sReapplyPending)
        {
            return;
        }
        sReapplyPending = true;
        doOnIdleOneTime(&reapplySettings);
    }

    // Current values of the given groups, as a preset.
    Preset captureCurrent(const std::string& name, const std::vector<std::string>& group_ids)
    {
        Preset preset;
        preset.mName = name;
        preset.mGroups = group_ids;
        for (const std::string& group : group_ids)
        {
            for (const std::string& setting : groupSettings(group))
            {
                if (LLControlVariable* control = gSavedSettings.getControl(setting))
                {
                    preset.mSettings[setting] = control->getValue();
                }
            }
        }
        LLSettingsSky::ptr_t sky = KSEnvironment::getDisplaySky();
        LLSettingsWater::ptr_t water = KSEnvironment::getDisplayWater();
        for (const auto& [key, field] : skyFields())
        {
            if (sky && std::find(group_ids.begin(), group_ids.end(), field.mGroup) != group_ids.end())
            {
                preset.mSky[key] = field.get(*sky);
            }
        }
        for (const auto& [key, field] : waterFields())
        {
            if (water && std::find(group_ids.begin(), group_ids.end(), field.mGroup) != group_ids.end())
            {
                preset.mWater[key] = field.get(*water);
            }
        }
        if (sky && preset.mSky.size() > 0)
        {
            preset.mSkyFull = sky->getSettings();
        }
        if (water && preset.mWater.size() > 0)
        {
            preset.mWaterFull = water->getSettings();
        }
        return preset;
    }
}

const std::vector<KSLightingPresets::Group>& KSLightingPresets::groups()
{
    return GROUPS;
}

void KSLightingPresets::init()
{
    if (sConnected)
    {
        return;
    }
    LLControlVariablePtr ks = gSavedSettings.getControl("KSSetEnvironment");
    if (!ks)
    {
        return;
    }
    sConnected = true;
    ks->getSignal()->connect([](LLControlVariable*, const LLSD& new_value, const LLSD&)
        {
            onKSSetEnvironmentChanged(new_value);
        });
    LLEnvironment::instance().setEnvironmentChanged(&onEnvironmentChanged);
    if (LLControlVariable* quality = gSavedSettings.getControl("RenderQualityPerformance"))
    {
        quality->getSignal()->connect([](LLControlVariable*, const LLSD&, const LLSD&) { scheduleReapply(); });
    }
    // Set to the preset's name after a graphics preset loads; any other change clears it
    // (LLPresetsManager::handleGraphicPresetControlChanged), including our own writes.
    if (LLControlVariable* graphics = gSavedSettings.getControl("PresetGraphicActive"))
    {
        graphics->getSignal()->connect([](LLControlVariable*, const LLSD& new_value, const LLSD&)
            {
                if (!new_value.asString().empty())
                {
                    scheduleReapply();
                }
            });
    }
    // Catch up if it was turned on (Debug Settings) before the first KStorm Lighting floater was built.
    onKSSetEnvironmentChanged(ks->getValue());
}

std::vector<std::string> KSLightingPresets::names()
{
    std::vector<std::string> saved;
    LLFile::mkdir(presetsDir());    // the iterator warns about a missing folder
    LLDirIterator iter(presetsDir(), "*.xml");
    std::string file;
    while (iter.next(file))
    {
        const std::string name = LLURI::unescape(gDirUtilp->getBaseFileName(file, true));
        if (!name.empty() && name != KS_REFERENCE)
        {
            saved.push_back(name);
        }
    }
    std::sort(saved.begin(), saved.end(), [](const std::string& a, const std::string& b)
        {
            return LLStringUtil::compareDict(a, b) < 0;
        });
    saved.insert(saved.begin(), KS_REFERENCE);
    return saved;
}

bool KSLightingPresets::exists(const std::string& name)
{
    return name == KS_REFERENCE || LLFile::isfile(presetPath(name));
}

bool KSLightingPresets::isBuiltIn(const std::string& name)
{
    return name == KS_REFERENCE;
}

std::vector<std::string> KSLightingPresets::presetGroups(const std::string& name)
{
    Preset preset;
    return findPreset(name, preset) ? preset.mGroups : std::vector<std::string>();
}

const std::string& KSLightingPresets::active()
{
    return sActive;
}

void KSLightingPresets::apply(const std::string& name)
{
    init();
    if (name == sActive)
    {
        return;
    }
    turnOff();
    if (name == KS_REFERENCE)
    {
        // Through its switch, so Quick's check box follows; the listener applies it.
        if (gSavedSettings.getBOOL("KSSetEnvironment"))
        {
            onKSSetEnvironmentChanged(LLSD(true));
        }
        else
        {
            gSavedSettings.setBOOL("KSSetEnvironment", true);
        }
        return;
    }
    Preset preset;
    if (!loadPreset(name, preset))
    {
        LL_WARNS("KStorm") << "Lighting presets: no preset " << name << LL_ENDL;
        return;
    }
    applyInternal(preset);
}

void KSLightingPresets::turnOff(bool restore_environment)
{
    if (sActive.empty())
    {
        return;
    }
    if (sActive == KS_REFERENCE)
    {
        // Through its switch, so Quick's check box follows.
        sKeepEnvironment = !restore_environment;
        gSavedSettings.setBOOL("KSSetEnvironment", false);
        sKeepEnvironment = false;
    }
    // Also when the switch was already off.
    turnOffInternal(restore_environment);
}

bool KSLightingPresets::saveCurrent(const std::string& name, const std::vector<std::string>& group_ids)
{
    std::string trimmed = name;
    LLStringUtil::trim(trimmed);
    if (trimmed.empty() || trimmed == KS_REFERENCE)
    {
        return false;
    }
    if (!savePreset(captureCurrent(trimmed, group_ids)))
    {
        return false;
    }
    changedSignal()();
    return true;
}

bool KSLightingPresets::remove(const std::string& name)
{
    if (name == KS_REFERENCE || !LLFile::isfile(presetPath(name)))
    {
        return false;
    }
    if (name == sActive)
    {
        turnOff();
    }
    LLFile::remove(presetPath(name));
    changedSignal()();
    return true;
}

bool KSLightingPresets::update(const std::string& name)
{
    Preset preset;
    if (isBuiltIn(name) || !loadPreset(name, preset))
    {
        return false;
    }
    for (const auto& [setting, value] : llsd::inMap(preset.mSettings))
    {
        if (LLControlVariable* control = gSavedSettings.getControl(setting))
        {
            preset.mSettings[setting] = control->getValue();
        }
    }
    LLSettingsSky::ptr_t sky = KSEnvironment::getDisplaySky();
    LLSettingsWater::ptr_t water = KSEnvironment::getDisplayWater();
    if (sky && preset.mSky.size() > 0)
    {
        for (const auto& [key, value] : llsd::inMap(preset.mSky))
        {
            auto field = skyFields().find(key);
            if (field != skyFields().end())
            {
                preset.mSky[key] = field->second.get(*sky);
            }
        }
        preset.mSkyFull = sky->getSettings();
    }
    if (water && preset.mWater.size() > 0)
    {
        for (const auto& [key, value] : llsd::inMap(preset.mWater))
        {
            auto field = waterFields().find(key);
            if (field != waterFields().end())
            {
                preset.mWater[key] = field->second.get(*water);
            }
        }
        preset.mWaterFull = water->getSettings();
    }
    if (!savePreset(preset))
    {
        return false;
    }
    if (name == sActive)
    {
        sActivePreset = preset;     // the restore point stays the one from before it was applied
    }
    changedSignal()();
    return true;
}

bool KSLightingPresets::rename(const std::string& name, const std::string& new_name)
{
    std::string trimmed = new_name;
    LLStringUtil::trim(trimmed);
    Preset preset;
    if (isBuiltIn(name) || trimmed.empty() || isBuiltIn(trimmed) || exists(trimmed) || !loadPreset(name, preset))
    {
        return false;
    }
    preset.mName = trimmed;
    if (!savePreset(preset))
    {
        return false;
    }
    LLFile::remove(presetPath(name));
    if (name == sActive)
    {
        sActive = trimmed;
        sActivePreset.mName = trimmed;
    }
    changedSignal()();
    return true;
}

std::string KSLightingPresets::duplicate(const std::string& name)
{
    Preset preset;
    if (!findPreset(name, preset))
    {
        return std::string();
    }
    std::string copy_name = name + " copy";
    for (S32 i = 2; exists(copy_name); ++i)
    {
        copy_name = llformat("%s copy %d", name.c_str(), i);
    }
    preset.mName = copy_name;
    if (!savePreset(preset))
    {
        return std::string();
    }
    changedSignal()();
    return copy_name;
}

bool KSLightingPresets::setGroups(const std::string& name, const std::vector<std::string>& group_ids)
{
    Preset preset;
    if (isBuiltIn(name) || !loadPreset(name, preset))
    {
        return false;
    }
    auto contains = [](const std::vector<std::string>& list, const std::string& id)
    {
        return std::find(list.begin(), list.end(), id) != list.end();
    };
    // Settings still covered by a kept group stay (some controls sit on two panes).
    std::set<std::string> kept;
    for (const std::string& group : group_ids)
    {
        for (const std::string& setting : groupSettings(group))
        {
            kept.insert(setting);
        }
    }
    std::vector<std::string> added;
    for (const std::string& group : group_ids)
    {
        if (!contains(preset.mGroups, group))
        {
            added.push_back(group);
        }
    }
    for (const std::string& group : preset.mGroups)
    {
        if (contains(group_ids, group))
        {
            continue;
        }
        for (const std::string& setting : groupSettings(group))
        {
            if (!kept.count(setting))
            {
                preset.mSettings.erase(setting);
            }
        }
        for (const auto& [key, field] : skyFields())
        {
            if (field.mGroup == group)
            {
                preset.mSky.erase(key);
            }
        }
        for (const auto& [key, field] : waterFields())
        {
            if (field.mGroup == group)
            {
                preset.mWater.erase(key);
            }
        }
    }
    const Preset captured = captureCurrent(name, added);
    for (const auto& [key, value] : llsd::inMap(captured.mSettings))
    {
        preset.mSettings[key] = value;
    }
    for (const auto& [key, value] : llsd::inMap(captured.mSky))
    {
        preset.mSky[key] = value;
    }
    for (const auto& [key, value] : llsd::inMap(captured.mWater))
    {
        preset.mWater[key] = value;
    }
    if (preset.mSky.size() == 0)
    {
        preset.mSkyFull.clear();
    }
    else if (captured.mSkyFull.isDefined())
    {
        preset.mSkyFull = captured.mSkyFull;
    }
    if (preset.mWater.size() == 0)
    {
        preset.mWaterFull.clear();
    }
    else if (captured.mWaterFull.isDefined())
    {
        preset.mWaterFull = captured.mWaterFull;
    }
    preset.mGroups = group_ids;
    if (!savePreset(preset))
    {
        return false;
    }
    if (name == sActive)
    {
        // Newly captured values equal the current ones, so their restore value is the current one.
        for (const auto& [key, value] : llsd::inMap(captured.mSettings))
        {
            if (!sRestore.mSettings.has(key))
            {
                sRestore.mSettings[key] = value;
            }
        }
        for (const auto& [key, value] : llsd::inMap(captured.mSky))
        {
            if (!sRestore.mSky.has(key))
            {
                sRestore.mSky[key] = value;
            }
        }
        for (const auto& [key, value] : llsd::inMap(captured.mWater))
        {
            if (!sRestore.mWater.has(key))
            {
                sRestore.mWater[key] = value;
            }
        }
        sActivePreset = preset;
    }
    changedSignal()();
    return true;
}

S32 KSLightingPresets::ungroupedCount(const std::string& name)
{
    Preset preset;
    if (!findPreset(name, preset))
    {
        return 0;
    }
    std::set<std::string> covered;
    for (const std::string& group : preset.mGroups)
    {
        for (const std::string& setting : groupSettings(group))
        {
            covered.insert(setting);
        }
    }
    auto in_groups = [&preset](const std::string& group)
    {
        return std::find(preset.mGroups.begin(), preset.mGroups.end(), group) != preset.mGroups.end();
    };
    S32 count = 0;
    for (const auto& [key, value] : llsd::inMap(preset.mSettings))
    {
        count += covered.count(key) ? 0 : 1;
    }
    for (const auto& [key, value] : llsd::inMap(preset.mSky))
    {
        auto field = skyFields().find(key);
        count += (field != skyFields().end() && in_groups(field->second.mGroup)) ? 0 : 1;
    }
    for (const auto& [key, value] : llsd::inMap(preset.mWater))
    {
        auto field = waterFields().find(key);
        count += (field != waterFields().end() && in_groups(field->second.mGroup)) ? 0 : 1;
    }
    return count;
}

std::string KSLightingPresets::contentsText(const std::string& name)
{
    Preset preset;
    if (!findPreset(name, preset))
    {
        return std::string();
    }
    auto format = [](const LLSD& value)
    {
        if (value.isArray())
        {
            std::string text;
            for (const LLSD& item : llsd::inArray(value))
            {
                text += (text.empty() ? "" : ", ") + (item.isReal() ? llformat("%.3f", item.asReal()) : item.asString());
            }
            return text;
        }
        if (value.isReal())
        {
            return llformat("%g", value.asReal());
        }
        if (value.isBoolean())
        {
            return std::string(value.asBoolean() ? "TRUE" : "FALSE");
        }
        return value.asString();
    };
    std::string text;
    if (preset.mSky.size() > 0)
    {
        text += "Sky\n";
        for (const auto& [key, value] : llsd::inMap(preset.mSky))
        {
            text += "  " + key + ": " + format(value) + "\n";
        }
    }
    if (preset.mWater.size() > 0)
    {
        text += "Water\n";
        for (const auto& [key, value] : llsd::inMap(preset.mWater))
        {
            text += "  " + key + ": " + format(value) + "\n";
        }
    }
    if (preset.mSettings.size() > 0)
    {
        text += "Settings\n";
        for (const auto& [key, value] : llsd::inMap(preset.mSettings))
        {
            text += "  " + key + ": " + format(value) + "\n";
        }
    }
    return text;
}

std::string KSLightingPresets::scopeText(const std::string& name)
{
    if (isBuiltIn(name))
    {
        return "built in";
    }
    const std::vector<std::string> ids = presetGroups(name);
    if (ids.empty())
    {
        return "custom";
    }
    if (ids.size() == GROUPS.size())
    {
        return "full";
    }
    std::string text;
    for (const Group& group : GROUPS)
    {
        if (std::find(ids.begin(), ids.end(), group.mId) != ids.end())
        {
            text += (text.empty() ? "" : ", ") + group.mLabel;
        }
    }
    return text;
}

void KSLightingPresets::onUseShared()
{
    if (!sActive.empty() && sActivePreset.hasEnvironment())
    {
        turnOff(false);
    }
}

void KSLightingPresets::restoreForShutdown()
{
    if (sActive.empty())
    {
        return;
    }
    restoreSettings();
    sActive.clear();
    sActivePreset = Preset();
    sRestore = RestorePoint();
}

boost::signals2::signal<void()>& KSLightingPresets::changedSignal()
{
    static boost::signals2::signal<void()> signal;
    return signal;
}
