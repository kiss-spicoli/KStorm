/**
 * @file ksenvironment.h
 * @author Kissmy Spicoli
 * @brief "Set KS environment" toggle for the KS tab of Environment Effects.
 *
 * Turning KSSetEnvironment on caches the current values of a fixed set of
 * viewer settings and Personal Lighting (ENV_LOCAL sky) properties, then
 * applies the KS values. Turning it off writes the cached values back. If
 * no local environment existed when it was turned on, the local
 * environment is cleared instead of restoring the sky values.
 *
 * KSSetEnvironment does not persist. restoreForShutdown() is called before
 * the viewer saves settings at exit so the KS values are not saved.
 */
#ifndef KS_ENVIRONMENT_H
#define KS_ENVIRONMENT_H

#include "llsettingssky.h"

namespace KSEnvironment
{
    // Environment version passed to LLEnvironment::setEnvironment() so our
    // own updates can be told apart from other ENV_LOCAL changes.
    // Personal Lighting uses -2, NO_VERSION is -3, celestial panels use -5.
    constexpr S32 KS_ENVIRONMENT_UPDATE = -7;

    // Connects the KSSetEnvironment listener. Safe to call repeatedly.
    void init();

    // Returns the ENV_LOCAL fixed sky, creating it from the parcel sky as
    // Personal Lighting does when none exists. May return null.
    LLSettingsSky::ptr_t getEditableSky();

    // Notifies ENV_LOCAL listeners (Personal Lighting, KS and celestial
    // panels) after a sky was edited in place.
    void commitSky(const LLSettingsSky::ptr_t& sky);

    // Restores cached viewer settings without touching the environment.
    void restoreForShutdown();
}

#endif // KS_ENVIRONMENT_H
