/**
 * @file ksenvironment.h
 * @author Kissmy Spicoli
 * @brief "Set KS environment" (Use KS Reference) and the sky/water editing helpers
 *        used by the KStorm Lighting panes.
 *
 * Turning KSSetEnvironment on caches the current values of a fixed set of
 * viewer settings and Personal Lighting (ENV_LOCAL sky) properties, then
 * applies the KS values. Turning it off writes the cached values back. If
 * no local environment existed when it was turned on, the local
 * environment is cleared instead of restoring the sky values.
 *
 * Use Shared and applying another environment (My Environments, inventory,
 * World menu presets) turn it off and restore only the viewer settings, since
 * the new environment replaces the KS sky (design A4).
 *
 * KSSetEnvironment does not persist. restoreForShutdown() is called before
 * the viewer saves settings at exit so the KS values are not saved.
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

    // Connects the KSSetEnvironment and environment listeners and declares the code-only
    // exposure settings. Safe to call repeatedly.
    void init();

    // Use Shared Environment: turns KS off without restoring its sky (A4), then clears the
    // local environment as World > Environment > Use Shared Environment does.
    void useShared();

    // Returns the ENV_LOCAL fixed sky, creating it from the parcel sky as
    // Personal Lighting does when none exists. May return null.
    LLSettingsSky::ptr_t getEditableSky();

    // Notifies ENV_LOCAL listeners (Personal Lighting, KS and celestial
    // panels) after a sky was edited in place.
    void commitSky(const LLSettingsSky::ptr_t& sky);

    // Restores cached viewer settings without touching the environment.
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
