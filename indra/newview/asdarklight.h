/**
 * @file asdarklight.h
 * @author Kissmy Spicoli
 * @brief Dark My Lights spots: a multiplicative pass that darkens a cone.
 *
 * A dark spot (ASLightRig::mDark, LLVOVolume backend) is skipped by the
 * additive deferred light passes and drawn here afterwards with BT_MULT
 * (dst = src * dst). The shader outputs 1 - darkness per channel, where
 * darkness follows the same cone, projector image, focus, distance falloff
 * and N.L as the light would, scaled by its colour (Intensity x Boost).
 * Darkening is proportional: highlights dim, black stays black, nothing goes
 * below zero. A white dark spot is neutral; a coloured one removes that
 * colour.
 *
 * Opaque deferred surfaces only. Dark spots are kept out of forward (alpha)
 * lights, the volumetric collector, reflection-probe captures and projector
 * shadow maps.
 */
#ifndef AS_DARKLIGHT_H
#define AS_DARKLIGHT_H

#include <vector>

#include "lldrawable.h"

class LLGLSLShader;
class LLPipeline;

namespace ASDarkLight
{
    void registerShader(std::vector<LLGLSLShader*>& shaders);
    bool createShader(S32 shader_level);
    void unloadShader();

    // Draws the dark spots collected by LLPipeline::renderDeferredLighting().
    // Call after every additive local light, with the light target bound.
    void render(LLPipeline& pipeline, const LLDrawable::drawable_list_t& dark_spots, F32 light_scale);
}

#endif // AS_DARKLIGHT_H
