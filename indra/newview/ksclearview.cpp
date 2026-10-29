/**
 * @file ksclearview.cpp
 * @author Kissmy Spicoli
 * @brief KStorm Clear View: hides all UI, name tags and HUDs, and restores them.
 */

#include "llviewerprecompiledheaders.h"

#include "ksclearview.h"

#include "llviewercontrol.h"
#include "pipeline.h"

namespace
{
    bool sOn = false;
    // What turning Clear View on hid, so turning it off shows only that again.
    bool sHidUI = false;
    bool sHidHUD = false;
}

void KSClearView::onSettingChanged(const LLSD& new_value)
{
    const bool on = new_value.asBoolean();
    if (on == sOn)
    {
        return;
    }
    sOn = on;

    // Same switches as LLViewerWindow::rawSnapshot() without UI and HUD. The UI
    // debug feature also gates name tags and hover text (LLHUDObject::renderAll()),
    // beacons, mouse and keyboard input to the UI, and makes the world view full
    // window. Toggled, not set: setRenderDebugFeatureControl(bit, false) clears
    // every feature bit.
    if (on)
    {
        sHidUI = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        if (sHidUI)
        {
            LLPipeline::toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        }
        sHidHUD = LLPipeline::sShowHUDAttachments;
        LLPipeline::sShowHUDAttachments = false;
    }
    else
    {
        if (sHidUI && !gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
        {
            LLPipeline::toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        }
        if (sHidHUD)
        {
            LLPipeline::sShowHUDAttachments = true;
        }
        sHidUI = sHidHUD = false;
    }
}
