/**
 * @file ksenvironment.h
 * @author Kissmy Spicoli
 * @brief Sky and water editing helpers used by the KStorm Lighting panes and
 *        lighting presets, plus the entry points the viewer calls (init,
 *        Use Shared, shutdown).
 *
 * Use KS Reference is the built-in lighting preset; see kslightingpresets.h.
 */
#ifndef KS_ENVIRONMENT_H
#define KS_ENVIRONMENT_H

#include "llsettingssky.h"
#include "llsettingswater.h"

#include <boost/signals2.hpp>

namespace KSEnvironment
{
    // Environment version passed to LLEnvironment::setEnvironment() so our
    // own updates can be told apart from other ENV_LOCAL changes.
    // Personal Lighting uses -2, NO_VERSION is -3, celestial panels use -5.
    constexpr S32 KS_ENVIRONMENT_UPDATE = -7;

    // Declares the code-only exposure settings and connects the lighting presets
    // (Use KS Reference). Safe to call repeatedly.
    void init();

    // Use Shared Environment: turns off an active preset that holds sky or water values
    // without restoring them (A4), then clears the local environment as
    // World > Environment > Use Shared Environment does.
    void useShared();

    // Returns the ENV_LOCAL fixed sky, creating it from the parcel sky as
    // Personal Lighting does when none exists. May return null.
    LLSettingsSky::ptr_t getEditableSky();

    // Notifies ENV_LOCAL listeners (Personal Lighting, KS and celestial
    // panels) after a sky was edited in place.
    void commitSky(const LLSettingsSky::ptr_t& sky);

    // Puts the active lighting preset's viewer settings back before settings are
    // saved at exit, without touching the environment.
    void restoreForShutdown();

    // Returns the ENV_LOCAL fixed water as our own copy, creating the local
    // environment from the current sky and water when needed. May return null.
    LLSettingsWater::ptr_t getEditableWater();

    // What the lighting panes show: the ENV_LOCAL sky or water when present,
    // otherwise the current one. May return null.
    LLSettingsSky::ptr_t getDisplaySky();
    LLSettingsWater::ptr_t getDisplayWater();

    // True for a legacy (pre-EEP) sky shown through LL's auto-adjust conversion.
    bool isAutoAdjustedLegacy(const LLSettingsSky::ptr_t& sky);

    // Sun/moon rotation from azimuth and elevation in degrees
    // (same conversion as ASPanelCelestialPosition).
    LLQuaternion rotationFromDegrees(F32 azimuth_deg, F32 elevation_deg);

    // Fired after a sky or water was edited in place, which sends no
    // environment-changed signal, so every pane can refresh.
    boost::signals2::signal<void()>& editedSignal();
    void notifyEdited();
}

#endif // KS_ENVIRONMENT_H
