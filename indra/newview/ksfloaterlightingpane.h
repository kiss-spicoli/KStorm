/**
 * @file ksfloaterlightingpane.h
 * @author Kissmy Spicoli
 * @brief One pane of the KStorm Lighting floater (doc/kstorm2-lighting-UI-design.md, section 10).
 *
 * Every pane uses this class; its XML decides which features apply. Plain
 * viewer settings bind with control_name and need no code. This class only
 * handles what control_name cannot, found by child name:
 *  - "ksv_<Setting>_<x|y|z>": a slider for one component of a Vector3 setting;
 *  - "ks_light_boxes": the "lights" info display (pipeline debug mask);
 *  - "ks_light_radius": also updates LLSelectMgr::sRenderLightRadius;
 *  - "ks_shadow_detail": feature-manager availability, as in Preferences;
 *  - "ssao_*" / "gtao_*": enabled for the matching RenderAOTechnique;
 *  - "sky_<key>" / "water_<key>": a sky or water value (KSLightingFields), edited
 *    on the local environment through KSEnvironment;
 *  - "skyc_<key>" / "waterc_<key>": a color swatch, with an optional "<name>_i"
 *    intensity spinner (stored color = swatch color x intensity);
 *  - "ks_ro_origin", "ks_ro_probe": readouts; "ks_note_pinned": shown while
 *    auto-exposure is fixed at 1; "ks_note_pinned_row": the same note on its own
 *    last row, which the pane drops (and shrinks) while the note is hidden;
 *  - "ks_use_shared": Use Shared Environment (KSEnvironment::useShared());
 *  - "ks_dynamic_exposure": disabled while Use KS Reference is on;
 *  - "ks_show_avatars": the "character" render type (Advanced.ToggleRenderType);
 *  - "ks_name_tags": AvatarNameTagMode as on/off, keeping the last "on" mode;
 *  - "ks_dof_mode": items copied from AyaneStorm's dof_mode combo, so the
 *    list follows hers.
 *
 * Buttons that open floaters use "KS.CycleFloater" (see cycleFloater()).
 */
#ifndef KS_FLOATERLIGHTINGPANE_H
#define KS_FLOATERLIGHTINGPANE_H

#include "llfloater.h"

#include <vector>

class LLCheckBoxCtrl;
class LLColorSwatchCtrl;
class LLTextBox;
class LLUICtrl;

class KSFloaterLightingPane : public LLFloater
{
public:
    explicit KSFloaterLightingPane(const LLSD& key);

    // Registers "KS.CycleFloater"; called before floaters are built (llviewerfloaterreg.cpp).
    static void registerUICallbacks();

    // Three states per click: open a closed (or minimized) floater; bring an open one to the
    // front; close it when it is already in front. "In front" ignores from_floater, the floater
    // holding the clicked button, which the click itself brought to the front.
    static void cycleFloater(const std::string& name, const LLFloater* from_floater);
    ~KSFloaterLightingPane() override;

    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void draw() override;

private:
    struct VectorSlider
    {
        LLUICtrl* mCtrl = nullptr;
        std::string mSetting;
        S32 mComponent = 0;
    };

    void connectVectorSliders();
    void refreshVectorSliders();
    void refreshShadowState();
    void refreshAOTechnique();
    void connectQuickControls();
    void refreshKSState();
    void refreshNameTags();
    void fillDoFModes();
    void setPinnedVisible(bool visible);

    struct EnvBinding
    {
        LLUICtrl* mCtrl = nullptr;                  // slider, or the swatch for colors
        LLUICtrl* mIntensity = nullptr;             // optional, colors only
        std::string mKey;
        bool mWater = false;
        bool mColor = false;
    };

    void connectEnvironment();
    void refreshEnvironment();
    void onEnvCommit(const EnvBinding& binding);

    std::vector<VectorSlider> mVectorSliders;
    std::vector<boost::signals2::connection> mConnections;
    LLCheckBoxCtrl* mLightBoxes = nullptr;
    std::vector<EnvBinding> mEnvBindings;
    LLTextBox* mOrigin = nullptr;
    LLTextBox* mProbeInUse = nullptr;
    LLView* mPinnedNote = nullptr;
    bool mPinnedRow = false;        // the note has its own last row
    bool mPinnedRowShown = true;    // the floater's height includes that row
    LLCheckBoxCtrl* mShowAvatars = nullptr;
    LLCheckBoxCtrl* mNameTags = nullptr;
    bool mRefreshing = false;
};

#endif // KS_FLOATERLIGHTINGPANE_H
