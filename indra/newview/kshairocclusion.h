/**
 * @file kshairocclusion.h
 * @author Kissmy Spicoli
 * @brief Body-proxy hair occlusion for the self avatar's forward alpha.
 *
 * The self avatar's head (sphere), neck and shoulders (capsules) are built
 * from joint positions once per frame. The forward alpha shaders
 * (alphaF, materialF blend mode, pbralphaF, via ksHairOcclusionF.glsl) use
 * them to occlude the sun/moon and local lights and to darken fragments next
 * to the body, but only while drawing the self avatar's attachments.
 * Settings: KSHairOcclusion*. Design: doc/hair-occlusion.md.
 */
#ifndef KS_HAIROCCLUSION_H
#define KS_HAIROCCLUSION_H

class LLDrawInfo;
class LLGLSLShader;
class LLSpatialGroup;

namespace KSHairOcclusion
{
    // Uploads the view-space shapes and parameters to a bound forward alpha
    // program, or a shape count of 0 when off or in a pass that must not be
    // affected (HUD, impostor, reflection, cube snapshot). Called from
    // prepare_alpha_shader().
    void applyShader(LLGLSLShader& shader);

    // Tells the bound program whether the next alpha draw belongs to the
    // self avatar (rigged to it, or attached to it). Called per draw from
    // LLDrawPoolAlpha::renderAlpha().
    void applyDraw(LLGLSLShader* shader, const LLDrawInfo& params, LLSpatialGroup* group);

    // Wireframe shapes (KSHairOcclusionShowProxies), drawn with the My Lights
    // beacons under the UI debug-feature gate, so never in snapshots.
    void renderProxies();
}

#endif // KS_HAIROCCLUSION_H
