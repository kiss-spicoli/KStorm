/**
 * @file kspanellighting.h
 * @author Kissmy Spicoli
 * @brief KS lighting panel of the KStorm > Environment Effects floater.
 *
 * Gathers lighting controls from Preferences and Personal Lighting in one
 * panel, plus the "Set KS environment" toggle (KSEnvironment).
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
    void refreshHdrScale();
    void refreshShadowState();
    void onHdrScaleChanged();

    LLUICtrl* mHdrScale = nullptr;
    LLComboBox* mShadowDetail = nullptr;
    LLEnvironment::connection_t mEnvironmentConnection;
};

#endif // KS_PANELLIGHTING_H
