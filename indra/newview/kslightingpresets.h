/**
 * @file kslightingpresets.h
 * @author Kissmy Spicoli
 * @brief KStorm Look Inventory: lighting presets (doc/kstorm2-lighting-UI-design.md, section 12.6).
 *
 * A lighting preset holds viewer settings and values of the local sky and
 * water, by name. Applying one records a restore point (the current values of
 * everything it will change) and writes only the values that differ; turning
 * it off writes the recorded values back. One preset is active at a time.
 *
 * KS Reference is the built-in, read-only preset; the KSSetEnvironment setting
 * (Quick > Use KS Reference) is its on/off switch. Presets you save live as
 * files under the user settings folder (ks_lighting_presets).
 *
 * While a preset is active:
 *  - Use Shared and applying another environment (My Environments, inventory,
 *    World menu presets) turn it off when it holds sky or water values, without
 *    restoring them: the new environment replaces them (design A4);
 *  - editing one of its values leaves it active; turning it off restores the
 *    value from before it was applied (A3);
 *  - a graphics preset load or a quality change writes its settings again (D1, D2).
 *
 * The active preset is never saved: restoreForShutdown() puts its settings
 * back before the viewer saves settings at exit.
 */
#ifndef KS_LIGHTINGPRESETS_H
#define KS_LIGHTINGPRESETS_H

#include "llsd.h"

#include <boost/signals2.hpp>
#include <string>
#include <vector>

namespace KSLightingPresets
{
    // Name of the built-in preset (reserved: no saved preset can use it).
    extern const std::string KS_REFERENCE;

    // One group of values a saved preset can include (design section 12.6, "Partial").
    struct Group
    {
        std::string mId;        // file and child-name key, e.g. "sunmoon"
        std::string mLabel;     // shown in dialogs and the list, e.g. "Sun/Moon"
    };
    const std::vector<Group>& groups();

    // Connects the KSSetEnvironment, environment, quality and graphics preset
    // listeners. Safe to call repeatedly.
    void init();

    // Names of every preset: KS Reference first, then the saved ones, sorted.
    std::vector<std::string> names();
    bool exists(const std::string& name);
    bool isBuiltIn(const std::string& name);

    // Groups a preset includes ("" for KS Reference, which names none).
    std::vector<std::string> presetGroups(const std::string& name);

    // Active preset, or "" when none is active.
    const std::string& active();

    // Applies a preset, turning the active one off first.
    void apply(const std::string& name);

    // Turns the active preset off. restore_environment false keeps the current
    // sky and water (used when another environment replaced them, A4).
    void turnOff(bool restore_environment = true);

    // Makes a preset just saved from the current values the active one without turning the
    // active preset off or writing any value (no rebuilds, nothing changes on screen). The
    // active preset's restore point carries over. False for KS Reference or a missing preset.
    bool activateSaved(const std::string& name);

    // Saves the current values of the given groups as a preset (replacing one of
    // the same name; an active preset of that name stays active with the new
    // values). False if the name is empty, reserved or cannot be written.
    bool saveCurrent(const std::string& name, const std::vector<std::string>& group_ids);

    // Deletes a saved preset, turning it off first when it is active.
    bool remove(const std::string& name);

    // Captures the current values of everything a saved preset holds. An active
    // preset stays active with its new values; its restore point is kept.
    bool update(const std::string& name);

    // Renames a saved preset. False if new_name is empty, reserved or taken.
    bool rename(const std::string& name, const std::string& new_name);

    // Saves a copy under a free name ("<name> copy", "<name> copy 2", ...) and
    // returns that name, or "" on failure. A copy of KS Reference is editable.
    std::string duplicate(const std::string& name);

    // Changes the groups a saved preset includes: an unchecked group's values are
    // dropped, a newly checked group's current values are captured. Values that
    // belong to no group (from a copy of KS Reference) are kept.
    bool setGroups(const std::string& name, const std::vector<std::string>& group_ids);

    // Number of values in a preset that belong to none of its groups.
    S32 ungroupedCount(const std::string& name);

    // What a preset holds, one value per line (for KS Reference's Contents).
    std::string contentsText(const std::string& name);

    // Short description for the list: "built in", "full", "custom" or group labels.
    std::string scopeText(const std::string& name);

    // Use Shared Environment: turns off an active preset that holds sky or water
    // values (A4) without restoring them.
    void onUseShared();

    // Puts the active preset's viewer settings back before settings are saved at exit.
    void restoreForShutdown();

    // Fired when the active preset or the list of presets changes.
    boost::signals2::signal<void()>& changedSignal();
}

#endif // KS_LIGHTINGPRESETS_H
