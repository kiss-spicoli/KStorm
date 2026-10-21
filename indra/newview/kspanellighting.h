/**
 * @file kspanellighting.h
 * @author Kissmy Spicoli
 * @brief KS tab of the Environment Effects floater.
 *
 * Gathers lighting controls from Preferences and Personal Lighting in one
 * tab, plus the "Set KS environment" toggle (KSEnvironment). The tab is
 * shown only while KSLightingControls is TRUE.
 */
#ifndef KS_PANELLIGHTING_H
#define KS_PANELLIGHTING_H

#include "llenvironment.h"
#include "llpanel.h"

class LLComboBox;
class LLUICtrl;

class KSPanelLighting : public LLPanel
{
public:
    KSPanelLighting() = default;
    ~KSPanelLighting() override;

    bool postBuild() override;
    void onVisibilityChange(bool new_visibility) override;

private:
    void updateTabVisibility();
    void refreshHdrScale();
    void refreshShadowState();
    void onHdrScaleChanged();

    LLUICtrl* mHdrScale = nullptr;
    LLComboBox* mShadowDetail = nullptr;
    LLEnvironment::connection_t mEnvironmentConnection;
    boost::signals2::scoped_connection mShowConnection;
};

#endif // KS_PANELLIGHTING_H
