/**
 * @file ksmotionmode.cpp
 * @author Kissmy Spicoli
 * @brief Motion mode: fast rendering while moving, with save and restore.
 */

#include "llviewerprecompiledheaders.h"

#include "ksmotionmode.h"

#include "llapp.h"
#include "llcallbacklist.h"
#include "llsdutil.h"
#include "llviewercontrol.h"

#include <map>
#include <string>

namespace
{
    // What Motion mode turns off (design section 11.7). Antialiasing, HDR and the emissive
    // buffer stay as they are: HDR and emissive changes rebuild every buffer and shader and
    // change the look itself, not only its quality.
    struct MotionSetting
    {
        const char* mName;
        LLSD mValue;
        // Applied only when the current value is higher (a lower one is already cheaper).
        bool mOnlyLower;
    };

    const MotionSetting MOTION_SETTINGS[] =
    {
        { "RenderShadowDetail",           LLSD(0),     false },
        { "RenderVolumetricLighting",     LLSD(false), false },   // god rays (needs shadows anyway)
        { "RenderDeferredSSAO",           LLSD(false), false },
        { "RenderScreenSpaceReflections", LLSD(false), false },
        // Static only: probes stop updating but keep lighting the scene. Disabling them
        // (-1, or RenderReflectionsEnabled) would remove all sky ambiance.
        { "RenderReflectionProbeDetail",  LLSD(0),     true },
        { "RenderMirrors",                LLSD(false), false },
        { "RenderGlow",                   LLSD(false), false },
        { "RenderDepthOfField",           LLSD(false), false },
        { "ASDepthOfFieldMode",           LLSD(0),     false },   // Standard renderer
        { "RenderMaxPartCount",           LLSD(0),     false },   // particles
        { "RenderLocalLightCount",        LLSD(0),     false },   // also stops My Lights
        // AyaneStorm Camera Effects (master switch: lens flare, chromatic aberration,
        // diffuse glow, vignette).
        { "ASCameraEffectsEnabled",       LLSD(false), false },
        // AyaneStorm Environment Effects (its tabs; Volumetric lighting is above).
        { "ASColorGradingEnabled",        LLSD(false), false },
        { "ASProceduralSunEnabled",       LLSD(false), false },
        { "ASHorizonScatteringEnabled",   LLSD(false), false },
        { "ASMoonHaloEnabled",            LLSD(false), false },
        { "ASStarsEnabled",               LLSD(false), false },
        { "ASMilkyWayEnabled",            LLSD(false), false },
        { "ASAuroraEnabled",              LLSD(false), false },
        { "ASWeatherSnowEnabled",         LLSD(false), false },
    };

    bool sConnected = false;
    bool sOn = false;
    bool sBusy = false;
    // For each setting Motion mode wrote: the value it set, and the value to restore.
    std::map<std::string, LLSD> sApplied;
    std::map<std::string, LLSD> sSaved;

    // Owned: set by Motion mode and still at that value. A value changed since then
    // (by the user or anything else) is kept when Motion mode turns off.
    bool owns(LLControlVariable& control)
    {
        if (!sOn)
        {
            return false;
        }
        auto applied = sApplied.find(control.getName());
        return applied != sApplied.end() && llsd_equals(control.getValue(), applied->second);
    }

    void turnOn()
    {
        sApplied.clear();
        sSaved.clear();
        // One buffer and shader rebuild for the whole set, not one per setting.
        ks_begin_render_settings_batch();
        for (const MotionSetting& setting : MOTION_SETTINGS)
        {
            LLControlVariable* control = gSavedSettings.getControl(setting.mName);
            if (!control)
            {
                LL_INFOS("KStorm") << "Motion mode: no setting " << setting.mName << ", skipped" << LL_ENDL;
                continue;
            }
            const LLSD current = control->getValue();
            if (setting.mOnlyLower && current.asInteger() <= setting.mValue.asInteger())
            {
                continue;
            }
            sSaved[setting.mName] = current;
            sApplied[setting.mName] = setting.mValue;
            // Only values that differ: several settings rebuild GPU buffers on any write.
            if (!llsd_equals(current, setting.mValue))
            {
                control->setValue(setting.mValue);
            }
        }
        sOn = true;
        ks_end_render_settings_batch();
    }

    void turnOff()
    {
        ks_begin_render_settings_batch();
        for (const auto& [name, value] : sSaved)
        {
            LLControlVariable* control = gSavedSettings.getControl(name);
            if (control && owns(*control) && !llsd_equals(control->getValue(), value))
            {
                control->setValue(value);
            }
        }
        sOn = false;
        sApplied.clear();
        sSaved.clear();
        ks_end_render_settings_batch();
    }

    void onSwitchChanged(const LLSD& new_value)
    {
        if (sBusy || new_value.asBoolean() == sOn)
        {
            return;
        }
        sBusy = true;
        if (new_value.asBoolean())
        {
            turnOn();
        }
        else
        {
            turnOff();
        }
        sBusy = false;
    }
}

void KSMotionMode::init()
{
    if (sConnected)
    {
        return;
    }
    LLControlVariablePtr control = gSavedSettings.getControl("KSMotionMode");
    if (!control)
    {
        return;
    }
    sConnected = true;
    control->getSignal()->connect([](LLControlVariable*, const LLSD& new_value, const LLSD&)
        {
            onSwitchChanged(new_value);
        });
    onSwitchChanged(control->getValue());
}

bool KSMotionMode::isOn()
{
    return sOn;
}

void KSMotionMode::setOn(bool on)
{
    init();
    // Through the switch, so Quick's check box and the launcher button follow.
    gSavedSettings.setBOOL("KSMotionMode", on);
}

LLSD KSMotionMode::getValue(LLControlVariable& control)
{
    if (owns(control))
    {
        return sSaved[control.getName()];
    }
    return control.getValue();
}

void KSMotionMode::setValue(LLControlVariable& control, const LLSD& value)
{
    if (owns(control))
    {
        sSaved[control.getName()] = value;
    }
    else if (!llsd_equals(control.getValue(), value))
    {
        control.setValue(value);
    }
}

void KSMotionMode::onTeleportStarted()
{
    static LLCachedControl<bool> on_teleport(gSavedSettings, "KSMotionModeOnTeleport", true);
    if (!on_teleport || sOn)
    {
        return;
    }
    doOnIdleOneTime([]()
        {
            if (!LLApp::isExiting())
            {
                KSMotionMode::setOn(true);
            }
        });
}

void KSMotionMode::restoreForShutdown()
{
    if (sOn)
    {
        sBusy = true;
        turnOff();
        gSavedSettings.setBOOL("KSMotionMode", false);
        sBusy = false;
    }
}
