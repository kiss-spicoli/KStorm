/**
 * @file ksdarklight.cpp
 * @author Kissmy Spicoli
 * @brief Dark My Lights spots: a multiplicative pass that darkens a cone.
 */

#include "llviewerprecompiledheaders.h"

#include "ksdarklight.h"

#include "aslightrigrenderer.h"
#include "llgl.h"
#include "llglslshader.h"
#include "llrender.h"
#include "llshadermgr.h"
#include "llviewercontrol.h"
#include "llvovolume.h"
#include "pipeline.h"

extern bool gCubeSnapshot;

namespace
{
    LLGLSLShader sDarkSpotProgram;

    // Same scales as the stock spot paths in LLPipeline::renderDeferredLighting().
    constexpr F32 DEFERRED_RADIUS_SCALE = 1.5f;
    constexpr F32 DEFERRED_LIGHT_FALLOFF = 0.5f;
}

void KSDarkLight::registerShader(std::vector<LLGLSLShader*>& shaders)
{
    shaders.push_back(&sDarkSpotProgram);
}

bool KSDarkLight::createShader(S32 shader_level)
{
    // Features and permutations as gDeferredMultiSpotLightProgram, including
    // add_common_permutations() (static in llviewershadermgr.cpp).
    static LLCachedControl<bool> emissive(gSavedSettings, "RenderEnableEmissiveBuffer", false);

    sDarkSpotProgram.mName = "KStorm Dark Spot Light Shader";
    sDarkSpotProgram.mFeatures.hasSrgb = true;
    sDarkSpotProgram.mFeatures.isDeferred = true;
    sDarkSpotProgram.mFeatures.hasFullGBuffer = true;
    sDarkSpotProgram.mFeatures.hasShadows = true;
    sDarkSpotProgram.clearPermutations();
    sDarkSpotProgram.addPermutation("MULTI_SPOTLIGHT", "1");
    if (emissive)
    {
        sDarkSpotProgram.addPermutation("HAS_EMISSIVE", "1");
    }
    sDarkSpotProgram.mShaderFiles.clear();
    sDarkSpotProgram.mShaderFiles.emplace_back("deferred/multiPointLightV.glsl", GL_VERTEX_SHADER);
    sDarkSpotProgram.mShaderFiles.emplace_back("deferred/ksDarkSpotLightF.glsl", GL_FRAGMENT_SHADER);
    sDarkSpotProgram.mShaderLevel = shader_level;
    return sDarkSpotProgram.createShader();
}

void KSDarkLight::unloadShader()
{
    sDarkSpotProgram.unload();
}

void KSDarkLight::render(LLPipeline& pipeline, const LLDrawable::drawable_list_t& dark_spots, F32 light_scale)
{
    // Reflection probes see neither the dark spot nor its darkening.
    if (dark_spots.empty() || gCubeSnapshot || !sDarkSpotProgram.isComplete())
    {
        return;
    }

    LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - dark spots");
    LL_PROFILE_GPU_ZONE("dark spots");

    // Full-screen triangles like the stock fullscreen spot path; the shader
    // discards pixels outside the light's radius and frustum.
    LLGLDepthTest depth(GL_FALSE);
    gGL.setSceneBlendType(LLRender::BT_MULT);

    const glm::mat4 modelview = get_current_modelview();
    pipeline.bindDeferredShader(sDarkSpotProgram);
    sDarkSpotProgram.enableTexture(LLShaderMgr::DEFERRED_PROJECTION);
    pipeline.mScreenTriangleVB->setBuffer();

    for (const LLPointer<LLDrawable>& drawablep : dark_spots)
    {
        LLVOVolume* volume = drawablep ? drawablep->getVOVolume() : nullptr;
        if (!volume)
        {
            continue;
        }

        LLPipeline::sVisibleLightCount++;

        const glm::vec3 view_center = mul_mat4_vec3(modelview, glm::vec3(drawablep->getPositionAgent()));
        const LLVector3 center(view_center.x, view_center.y, view_center.z);

        // Projector frustum, image and focus uniforms, as for any spot.
        pipeline.setupSpotLight(sDarkSpotProgram, drawablep);

        // Colour sets the strength and tint of the darkness.
        const LLColor3 col = volume->getLightLinearColor() * (light_scale * ASLightRigRenderer::getLightBoost(volume));

        sDarkSpotProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, center.mV);
        sDarkSpotProgram.uniform1f(LLShaderMgr::LIGHT_SIZE, volume->getLightRadius() * DEFERRED_RADIUS_SCALE);
        sDarkSpotProgram.uniform3fv(LLShaderMgr::DIFFUSE_COLOR, 1, col.mV);
        sDarkSpotProgram.uniform1f(LLShaderMgr::LIGHT_FALLOFF, volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF));

        pipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    }

    sDarkSpotProgram.disableTexture(LLShaderMgr::DEFERRED_PROJECTION);
    pipeline.unbindDeferredShader(sDarkSpotProgram);

    // The local-light block runs with additive blending.
    gGL.setSceneBlendType(LLRender::BT_ADD);
}
