/**
 * @file kslightingfields.cpp
 * @author Kissmy Spicoli
 * @brief Sky and water values shown by the KStorm Lighting panes, by key.
 */

#include "llviewerprecompiledheaders.h"

#include "kslightingfields.h"

#include "ksenvironment.h"
#include "llviewercontrol.h"
#include "llvirtualtrackball.h"

#include <functional>
#include <map>

namespace
{
    template <typename T, typename V>
    struct Field
    {
        std::function<V(T&)> get;
        std::function<void(T&, V)> set;
    };

    using SkyScalar = Field<LLSettingsSky, F32>;
    using SkyColor = Field<LLSettingsSky, LLColor3>;
    using WaterScalar = Field<LLSettingsWater, F32>;
    using WaterColor = Field<LLSettingsWater, LLColor3>;

    // Personal Lighting / sky editor display scales (llpaneleditsky.cpp).
    constexpr F32 GLOW_SCALE_R = 20.f;
    constexpr F32 GLOW_SCALE_B = -5.f;
    constexpr F32 DENSITY_MULTIPLIER_SCALE = 0.001f;
    constexpr F32 MIN_PROBE_FILL = 0.01f;          // design decision: never below 0.01

    F32 luminance(const LLColor3& c)
    {
        // Rec.709 weights, as the shaders use (design section 8.1).
        return 0.2126f * c.mV[0] + 0.7152f * c.mV[1] + 0.0722f * c.mV[2];
    }

    void getAngles(const LLQuaternion& rotation, F32& azimuth, F32& elevation)
    {
        LLVirtualTrackball::getAzimuthAndElevationDeg(rotation, azimuth, elevation);
    }

    SkyScalar rotationField(bool sun, bool azimuth_part)
    {
        return {
            [sun, azimuth_part](LLSettingsSky& sky)
            {
                F32 az, el;
                getAngles(sun ? sky.getSunRotation() : sky.getMoonRotation(), az, el);
                return azimuth_part ? az : el;
            },
            [sun, azimuth_part](LLSettingsSky& sky, F32 v)
            {
                F32 az, el;
                getAngles(sun ? sky.getSunRotation() : sky.getMoonRotation(), az, el);
                (azimuth_part ? az : el) = v;
                const LLQuaternion rotation = KSEnvironment::rotationFromDegrees(az, el);
                if (sun)
                {
                    sky.setSunRotation(rotation);
                }
                else
                {
                    sky.setMoonRotation(rotation);
                }
            } };
    }

    SkyScalar cloudDensityField(bool first, S32 axis)
    {
        return {
            [first, axis](LLSettingsSky& sky)
            {
                return (first ? sky.getCloudPosDensity1() : sky.getCloudPosDensity2()).mV[axis];
            },
            [first, axis](LLSettingsSky& sky, F32 v)
            {
                LLColor3 value = first ? sky.getCloudPosDensity1() : sky.getCloudPosDensity2();
                value.mV[axis] = v;
                if (first)
                {
                    sky.setCloudPosDensity1(value);
                }
                else
                {
                    sky.setCloudPosDensity2(value);
                }
            } };
    }

    const std::map<std::string, SkyScalar>& skyScalars()
    {
        static const std::map<std::string, SkyScalar> fields = {
            { "sun_azimuth", rotationField(true, true) },
            { "sun_elevation", rotationField(true, false) },
            { "moon_azimuth", rotationField(false, true) },
            { "moon_elevation", rotationField(false, false) },
            { "haze_density", { [](LLSettingsSky& s) { return s.getHazeDensity(); }, [](LLSettingsSky& s, F32 v) { s.setHazeDensity(v); } } },
            { "haze_horizon", { [](LLSettingsSky& s) { return s.getHazeHorizon(); }, [](LLSettingsSky& s, F32 v) { s.setHazeHorizon(v); } } },
            // Haze Glow Strength: glow.x = (2 - v) * 20; Haze Glow Focus: glow.z = v * -5.
            { "glow_strength", { [](LLSettingsSky& s) { return 2.f - s.getGlow().mV[0] / GLOW_SCALE_R; },
                                 [](LLSettingsSky& s, F32 v) { LLColor3 g = s.getGlow(); g.mV[0] = (2.f - v) * GLOW_SCALE_R; s.setGlow(g); } } },
            { "glow_focus", { [](LLSettingsSky& s) { return s.getGlow().mV[2] / GLOW_SCALE_B; },
                              [](LLSettingsSky& s, F32 v) { LLColor3 g = s.getGlow(); g.mV[2] = v * GLOW_SCALE_B; s.setGlow(g); } } },
            { "density_multiplier", { [](LLSettingsSky& s) { return s.getDensityMultiplier() / DENSITY_MULTIPLIER_SCALE; },
                                      [](LLSettingsSky& s, F32 v) { s.setDensityMultiplier(v * DENSITY_MULTIPLIER_SCALE); } } },
            { "distance_multiplier", { [](LLSettingsSky& s) { return s.getDistanceMultiplier(); }, [](LLSettingsSky& s, F32 v) { s.setDistanceMultiplier(v); } } },
            { "max_y", { [](LLSettingsSky& s) { return s.getMaxY(); }, [](LLSettingsSky& s, F32 v) { s.setMaxY(v); } } },
            { "moisture_level", { [](LLSettingsSky& s) { return s.getSkyMoistureLevel(); }, [](LLSettingsSky& s, F32 v) { s.setSkyMoistureLevel(v); } } },
            { "droplet_radius", { [](LLSettingsSky& s) { return s.getSkyDropletRadius(); }, [](LLSettingsSky& s, F32 v) { s.setSkyDropletRadius(v); } } },
            { "ice_level", { [](LLSettingsSky& s) { return s.getSkyIceLevel(); }, [](LLSettingsSky& s, F32 v) { s.setSkyIceLevel(v); } } },
            { "gamma", { [](LLSettingsSky& s) { return s.getGamma(); }, [](LLSettingsSky& s, F32 v) { s.setGamma(v); } } },
            { "probe_fill", { [](LLSettingsSky& s)
                              {
                                  static LLCachedControl<bool> auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
                                  return s.getReflectionProbeAmbiance(auto_adjust());
                              },
                              // Setting it clears the legacy auto-adjust flag: the local copy becomes an EEP sky.
                              [](LLSettingsSky& s, F32 v) { s.setReflectionProbeAmbiance(llmax(MIN_PROBE_FILL, v)); } } },
            { "sun_scale", { [](LLSettingsSky& s) { return s.getSunScale(); }, [](LLSettingsSky& s, F32 v) { s.setSunScale(v); } } },
            { "moon_scale", { [](LLSettingsSky& s) { return s.getMoonScale(); }, [](LLSettingsSky& s, F32 v) { s.setMoonScale(v); } } },
            { "moon_brightness", { [](LLSettingsSky& s) { return s.getMoonBrightness(); }, [](LLSettingsSky& s, F32 v) { s.setMoonBrightness(v); } } },
            { "star_brightness", { [](LLSettingsSky& s) { return s.getStarBrightness(); }, [](LLSettingsSky& s, F32 v) { s.setStarBrightness(v); } } },
            // Ambient Brightness: luminance of the stored ambient color, hue kept (gray when black).
            { "ambient_brightness", { [](LLSettingsSky& s) { return luminance(s.getAmbientColor()); },
                                      [](LLSettingsSky& s, F32 v)
                                      {
                                          LLColor3 c = s.getAmbientColor();
                                          F32 lum = luminance(c);
                                          if (lum < 1e-6f)
                                          {
                                              c = LLColor3(1.f, 1.f, 1.f);
                                              lum = 1.f;
                                          }
                                          s.setAmbientColor(c * (llmax(0.f, v) / lum));
                                      } } },
            { "cloud_shadow", { [](LLSettingsSky& s) { return s.getCloudShadow(); }, [](LLSettingsSky& s, F32 v) { s.setCloudShadow(v); } } },
            { "cloud_scale", { [](LLSettingsSky& s) { return s.getCloudScale(); }, [](LLSettingsSky& s, F32 v) { s.setCloudScale(v); } } },
            { "cloud_variance", { [](LLSettingsSky& s) { return s.getCloudVariance(); }, [](LLSettingsSky& s, F32 v) { s.setCloudVariance(v); } } },
            { "cloud_density1_x", cloudDensityField(true, 0) },
            { "cloud_density1_y", cloudDensityField(true, 1) },
            { "cloud_density1_z", cloudDensityField(true, 2) },
            { "cloud_density2_x", cloudDensityField(false, 0) },
            { "cloud_density2_y", cloudDensityField(false, 1) },
            { "cloud_density2_z", cloudDensityField(false, 2) },
        };
        return fields;
    }

    const std::map<std::string, SkyColor>& skyColors()
    {
        static const std::map<std::string, SkyColor> fields = {
            { "sunlight_color", { [](LLSettingsSky& s) { return s.getSunlightColor(); }, [](LLSettingsSky& s, LLColor3 v) { s.setSunlightColor(v); } } },
            { "blue_horizon", { [](LLSettingsSky& s) { return s.getBlueHorizon(); }, [](LLSettingsSky& s, LLColor3 v) { s.setBlueHorizon(v); } } },
            { "blue_density", { [](LLSettingsSky& s) { return s.getBlueDensity(); }, [](LLSettingsSky& s, LLColor3 v) { s.setBlueDensity(v); } } },
            { "cloud_color", { [](LLSettingsSky& s) { return s.getCloudColor(); }, [](LLSettingsSky& s, LLColor3 v) { s.setCloudColor(v); } } },
        };
        return fields;
    }

    WaterScalar normalScaleField(S32 axis)
    {
        return {
            [axis](LLSettingsWater& w) { return w.getNormalScale().mV[axis]; },
            [axis](LLSettingsWater& w, F32 v) { LLVector3 n = w.getNormalScale(); n.mV[axis] = v; w.setNormalScale(n); } };
    }

    const std::map<std::string, WaterScalar>& waterScalars()
    {
        static const std::map<std::string, WaterScalar> fields = {
            { "fog_density", { [](LLSettingsWater& w) { return w.getWaterFogDensity(); }, [](LLSettingsWater& w, F32 v) { w.setWaterFogDensity(v); } } },
            { "fog_mod", { [](LLSettingsWater& w) { return w.getFogMod(); }, [](LLSettingsWater& w, F32 v) { w.setFogMod(v); } } },
            { "fresnel_scale", { [](LLSettingsWater& w) { return w.getFresnelScale(); }, [](LLSettingsWater& w, F32 v) { w.setFresnelScale(v); } } },
            { "fresnel_offset", { [](LLSettingsWater& w) { return w.getFresnelOffset(); }, [](LLSettingsWater& w, F32 v) { w.setFresnelOffset(v); } } },
            { "normal_scale_x", normalScaleField(0) },
            { "normal_scale_y", normalScaleField(1) },
            { "normal_scale_z", normalScaleField(2) },
            { "scale_above", { [](LLSettingsWater& w) { return w.getScaleAbove(); }, [](LLSettingsWater& w, F32 v) { w.setScaleAbove(v); } } },
            { "scale_below", { [](LLSettingsWater& w) { return w.getScaleBelow(); }, [](LLSettingsWater& w, F32 v) { w.setScaleBelow(v); } } },
            { "blur_multiplier", { [](LLSettingsWater& w) { return w.getBlurMultiplier(); }, [](LLSettingsWater& w, F32 v) { w.setBlurMultiplier(v); } } },
        };
        return fields;
    }

    const std::map<std::string, WaterColor>& waterColors()
    {
        static const std::map<std::string, WaterColor> fields = {
            { "fog_color", { [](LLSettingsWater& w) { return w.getWaterFogColor(); }, [](LLSettingsWater& w, LLColor3 v) { w.setWaterFogColor(v); } } },
        };
        return fields;
    }

    template <typename Map>
    const typename Map::mapped_type* findField(const Map& map, const std::string& key)
    {
        auto it = map.find(key);
        return it == map.end() ? nullptr : &it->second;
    }
}

bool KSLightingFields::hasSkyScalar(const std::string& key) { return findField(skyScalars(), key) != nullptr; }
bool KSLightingFields::hasSkyColor(const std::string& key) { return findField(skyColors(), key) != nullptr; }
bool KSLightingFields::hasWaterScalar(const std::string& key) { return findField(waterScalars(), key) != nullptr; }
bool KSLightingFields::hasWaterColor(const std::string& key) { return findField(waterColors(), key) != nullptr; }

F32 KSLightingFields::getSkyScalar(const std::string& key, LLSettingsSky& sky)
{
    const SkyScalar* f = findField(skyScalars(), key);
    return f ? f->get(sky) : 0.f;
}

void KSLightingFields::setSkyScalar(const std::string& key, LLSettingsSky& sky, F32 value)
{
    if (const SkyScalar* f = findField(skyScalars(), key))
    {
        f->set(sky, value);
    }
}

LLColor3 KSLightingFields::getSkyColor(const std::string& key, LLSettingsSky& sky)
{
    const SkyColor* f = findField(skyColors(), key);
    return f ? f->get(sky) : LLColor3::black;
}

void KSLightingFields::setSkyColor(const std::string& key, LLSettingsSky& sky, const LLColor3& value)
{
    if (const SkyColor* f = findField(skyColors(), key))
    {
        f->set(sky, value);
    }
}

F32 KSLightingFields::getWaterScalar(const std::string& key, LLSettingsWater& water)
{
    const WaterScalar* f = findField(waterScalars(), key);
    return f ? f->get(water) : 0.f;
}

void KSLightingFields::setWaterScalar(const std::string& key, LLSettingsWater& water, F32 value)
{
    if (const WaterScalar* f = findField(waterScalars(), key))
    {
        f->set(water, value);
    }
}

LLColor3 KSLightingFields::getWaterColor(const std::string& key, LLSettingsWater& water)
{
    const WaterColor* f = findField(waterColors(), key);
    return f ? f->get(water) : LLColor3::black;
}

void KSLightingFields::setWaterColor(const std::string& key, LLSettingsWater& water, const LLColor3& value)
{
    if (const WaterColor* f = findField(waterColors(), key))
    {
        f->set(water, value);
    }
}
