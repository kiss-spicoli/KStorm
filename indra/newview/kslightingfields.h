/**
 * @file kslightingfields.h
 * @author Kissmy Spicoli
 * @brief Sky and water values shown by the KStorm Lighting panes, by key.
 *
 * Each key maps one pane control to an LLSettingsSky or LLSettingsWater value,
 * including the display transforms the design uses (degrees for sun and moon
 * position, Atmosphere Thickness x 1000, the haze glow mappings, Ambient
 * Brightness as luminance with the hue kept). See
 * doc/kstorm2-lighting-UI-design.md, section 10.
 */
#ifndef KS_LIGHTINGFIELDS_H
#define KS_LIGHTINGFIELDS_H

#include "llsettingssky.h"
#include "llsettingswater.h"

namespace KSLightingFields
{
    // Scalar sky values. Unknown keys return false.
    bool hasSkyScalar(const std::string& key);
    F32 getSkyScalar(const std::string& key, LLSettingsSky& sky);
    void setSkyScalar(const std::string& key, LLSettingsSky& sky, F32 value);

    // Sky colors (stored linear values, not normalized).
    bool hasSkyColor(const std::string& key);
    LLColor3 getSkyColor(const std::string& key, LLSettingsSky& sky);
    void setSkyColor(const std::string& key, LLSettingsSky& sky, const LLColor3& value);

    // Scalar water values.
    bool hasWaterScalar(const std::string& key);
    F32 getWaterScalar(const std::string& key, LLSettingsWater& water);
    void setWaterScalar(const std::string& key, LLSettingsWater& water, F32 value);

    // Water colors.
    bool hasWaterColor(const std::string& key);
    LLColor3 getWaterColor(const std::string& key, LLSettingsWater& water);
    void setWaterColor(const std::string& key, LLSettingsWater& water, const LLColor3& value);
}

#endif // KS_LIGHTINGFIELDS_H
