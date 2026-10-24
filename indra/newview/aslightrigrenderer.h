/**
 * @file aslightrigrenderer.h
 * @author chanayane@firestorm
 * @brief Direct deferred-light backend for My Lights, with no viewer objects.
 */

#ifndef AS_LIGHTRIGRENDERER_H
#define AS_LIGHTRIGRENDERER_H

#include <map>
#include <set>
#include <vector>

#include "lluuid.h"
#include "v3color.h"
#include "v3math.h"

class LLPipeline;
class LLVOVolume;

class ASLightRigRenderer
{
public:
    struct Light
    {
        LLVector3 position_agent;
        LLColor3 color_srgb;
        F32 intensity;
        F32 radius;
        F32 falloff;
    };

    static bool usesShaderBackend();

    // Replaces the small CPU-side snapshot consumed by the render pass.
    static void setLights(const std::vector<Light>& lights);

    // Adds the snapshot through the viewer's stock deferred point-light
    // shaders. The caller already has the deferred light target bound.
    static void render(LLPipeline& pipeline, F32 light_scale);

    // Reserves hardware-light slots for the same synthetic lights so forward
    // transparency receives the contribution LLVOVolume lights normally get
    // through LLPipeline::mNearbyLights. Returns the next free slot.
    static S32 appendForwardLights(LLPipeline& pipeline, S32 first_slot, F32 light_scale);

    // "Only My Lights" (ASLightRigOnlyMyLights): when on, every local light
    // except the My Lights objects is kept out of LLPipeline::mNearbyLights,
    // which feeds the deferred, forward (alpha), projector-shadow,
    // reflection-probe and volumetric local-light paths. Worn lights are
    // excluded too. Shader-backend My Lights never enter mNearbyLights and
    // are unaffected.
    static void setLightRigIds(const std::set<LLUUID>& ids);
    // True when the light may enter mNearbyLights.
    static bool allowsLight(const LLVOVolume* light);

    // Per-light brightness multiplier (Boost, 1-4) for LLVOVolume My Lights.
    // LLLightParams clamps colour and intensity to 0-1, so the boost is
    // applied where the renderer reads a light's linear colour instead
    // (tagged call sites in pipeline.cpp, and asvolumetriclighting.cpp).
    // The floater refreshes the map every idle tick with the lights whose
    // boost is not 1. The shader backend applies the boost to its own
    // snapshot's intensity.
    static void setLightBoosts(const std::map<LLUUID, F32>& boosts);
    // Boost for this light object; 1 for anything that is not a boosted My Light.
    static F32 getLightBoost(const LLVOVolume* light);

    // Dark spots (ASLightRig::mDark): My Lights spot objects drawn by
    // ASDarkLight's multiplicative pass instead of the additive light passes,
    // and kept out of forward lights, volumetrics and projector shadows.
    // The floater refreshes the set every idle tick.
    static void setDarkLights(const std::set<LLUUID>& ids);
    // True for a dark My Lights spot; false at once while there are none.
    static bool isDarkLight(const LLVOVolume* light);
};

#endif // AS_LIGHTRIGRENDERER_H
