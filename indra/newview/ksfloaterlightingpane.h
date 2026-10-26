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
 *
 * KSFloaterLightingPaneBase holds what every KStorm Lighting pane shares (this class and
 * the Lighting Presets pane): docking beside the launcher, and the header click that
 * collapses the pane to its header bar or expands it again.
 */
#ifndef KS_FLOATERLIGHTINGPANE_H
#define KS_FLOATERLIGHTINGPANE_H

#include "llfloater.h"

#include <vector>

class LLCheckBoxCtrl;
class LLColorSwatchCtrl;
class LLTextBox;
class LLUICtrl;

class KSFloaterLightingPaneBase : public LLFloater
{
public:
    explicit KSFloaterLightingPaneBase(const LLSD& key);

    // Collapsed, only the header bar shows, at half the width; the top edge, and the side edge
    // facing the launcher, stay in place either way.
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return mCollapsed; }

    // Topmost visible floater other than ignore (the floater holding a clicked button,
    // which the click itself brought to the front).
    static LLFloater* topmostFloater(const LLFloater* ignore);

    // A click on the header bar (not a drag, not a header button) collapses or expands the
    // pane when it is in front; a collapsed pane behind others expands and comes forward on
    // the same click, an expanded one only comes forward.
    bool handleMouseDown(S32 x, S32 y, MASK mask) override;
    void draw() override;
    // Remembers whether the pane was collapsed, then expands before closing, so the saved
    // size is the full one.
    void onClose(bool app_quitting) override;

    // Whether a pane was collapsed when it last closed (KSLightingPaneStates); a pane never
    // closed counts as collapsed. The launcher's dock button reopens docked panes this way.
    static bool wasCollapsed(const std::string& name);

    // Docks beside the launcher when dragged near this pane's slot (KSFloaterLighting::findPaneSnap).
    LLView* findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type,
                         S32 threshold, S32 padding = 0) override;

protected:
    // Changes the pane's full height (Quick's note row), also while collapsed.
    void changeExpandedHeight(S32 delta);
    // What a header click does: collapse or expand.
    virtual void onHeaderClicked() { setCollapsed(!mCollapsed); }

private:
    bool mCollapsed = false;
    S32 mExpandedHeight = 0;
    S32 mExpandedWidth = 0;
    bool mHeaderClickPending = false;
    LLRect mHeaderClickRect;
};

// A header-only stand-in, docked left of the launcher, for an AyaneStorm floater too large to
// dock (Environment Effects, Viewer Lights, Camera Effects). A click on its header opens that
// floater in front, or closes it. One XUI file, one registered instance per floater.
class KSFloaterLightingProxy : public KSFloaterLightingPaneBase
{
public:
    explicit KSFloaterLightingProxy(const LLSD& key);

    bool postBuild() override;
    void onOpen(const LLSD& key) override;

    // The AyaneStorm floater a proxy stands for ("" if the name is not a proxy).
    static std::string targetOf(const std::string& proxy_name);

protected:
    void onHeaderClicked() override;
};

class KSFloaterLightingPane : public KSFloaterLightingPaneBase
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
