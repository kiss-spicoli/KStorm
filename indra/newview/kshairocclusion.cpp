/**
 * @file kshairocclusion.cpp
 * @author Kissmy Spicoli
 * @brief See kshairocclusion.h.
 */

#include "llviewerprecompiledheaders.h"

#include "kshairocclusion.h"

#include "llappviewer.h"
#include "llglslshader.h"
#include "llglstates.h"
#include "llrender.h"
#include "llspatialpartition.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "llvoavatarself.h"
#include "pipeline.h"

#include <vector>

extern bool gCubeSnapshot;

namespace
{
    // Shader uniforms, see ksHairOcclusionF.glsl.
    const LLStaticHashedString sCount("ks_hocc_count");
    const LLStaticHashedString sSelf("ks_hocc_self");
    const LLStaticHashedString sAxisA("ks_hocc_a");
    const LLStaticHashedString sAxisB("ks_hocc_b");
    const LLStaticHashedString sParams("ks_hocc_params");
    const LLStaticHashedString sParams2("ks_hocc_params2");

    const S32 MAX_SHAPES = 4;

    // Default-skeleton distances used to scale the radii with the shape
    // sliders: mHead to mSkull (eye height) and shoulder to shoulder.
    const F32 DEFAULT_HEAD_HEIGHT = 0.079f;
    const F32 DEFAULT_SHOULDER_WIDTH = 0.328f;

    // Capsule with axis a -> b in agent space; a sphere has a == b.
    struct Shape
    {
        LLVector3 a;
        LLVector3 b;
        F32 radius;
    };

    std::vector<Shape> sShapes;
    U32 sShapesFrame = U32_MAX;

    bool isEnabled()
    {
        static LLCachedControl<bool> enabled(gSavedSettings, "KSHairOcclusionEnabled", false);
        return enabled;
    }

    void addShape(const LLVector3& a, const LLVector3& b, F32 radius)
    {
        if (radius > 0.f)
        {
            sShapes.push_back({ a, b, radius });
        }
    }

    // Rebuilds the shapes from the self avatar's joints once per frame.
    const std::vector<Shape>& currentShapes()
    {
        if (sShapesFrame == gFrameCount)
        {
            return sShapes;
        }
        sShapesFrame = gFrameCount;
        sShapes.clear();

        if (!isEnabled() || !isAgentAvatarValid())
        {
            return sShapes;
        }

        // All of these are base-skeleton joints, present on legacy and Bento avatars.
        LLJoint* neck = gAgentAvatarp->getJoint("mNeck");
        LLJoint* head = gAgentAvatarp->getJoint("mHead");
        LLJoint* skull = gAgentAvatarp->getJoint("mSkull");
        LLJoint* shoulder_left = gAgentAvatarp->getJoint("mShoulderLeft");
        LLJoint* shoulder_right = gAgentAvatarp->getJoint("mShoulderRight");
        if (!neck || !head || !skull || !shoulder_left || !shoulder_right)
        {
            return sShapes;
        }

        static LLCachedControl<F32> head_radius(gSavedSettings, "KSHairOcclusionHeadRadius", 0.085f);
        static LLCachedControl<LLVector3> head_offset(gSavedSettings, "KSHairOcclusionHeadOffset", LLVector3(0.01f, 0.f, 1.f));
        static LLCachedControl<F32> neck_radius(gSavedSettings, "KSHairOcclusionNeckRadius", 0.05f);
        static LLCachedControl<F32> shoulder_radius(gSavedSettings, "KSHairOcclusionShoulderRadius", 0.055f);

        const LLVector3 neck_pos = neck->getWorldPosition();
        const LLVector3 head_pos = head->getWorldPosition();
        const LLVector3 skull_pos = skull->getWorldPosition();
        const LLVector3 left_pos = shoulder_left->getWorldPosition();
        const LLVector3 right_pos = shoulder_right->getWorldPosition();

        // mSkull sits at eye height, so this measures head size, not the crown.
        const F32 head_scale = llclamp(dist_vec(skull_pos, head_pos) / DEFAULT_HEAD_HEIGHT, 0.25f, 4.f);
        const F32 shoulder_scale = llclamp(dist_vec(left_pos, right_pos) / DEFAULT_SHOULDER_WIDTH, 0.25f, 4.f);

        // Head sphere: a fraction of the way from mHead to mSkull, then moved
        // forward/sideways in the head joint's frame.
        const LLVector3 offset = head_offset;
        const LLQuaternion head_rot = head->getWorldRotation();
        const LLVector3 forward = LLVector3::x_axis * head_rot;
        const LLVector3 side = LLVector3::y_axis * head_rot;
        const LLVector3 head_centre = head_pos + (skull_pos - head_pos) * offset.mV[VZ] +
                                      (forward * offset.mV[VX] + side * offset.mV[VY]) * head_scale;

        addShape(head_centre, head_centre, head_radius * head_scale);
        addShape(neck_pos, head_pos, neck_radius * head_scale);
        addShape(neck_pos, left_pos, shoulder_radius * shoulder_scale);
        addShape(neck_pos, right_pos, shoulder_radius * shoulder_scale);
        return sShapes;
    }

    // Passes that must stay unchanged: HUDs, impostors, probes, water reflection.
    bool passAllowed()
    {
        return !gCubeSnapshot && !LLPipeline::sRenderingHUDs && !LLPipeline::sImpostorRender &&
               !LLPipeline::sReflectionRender && !LLPipeline::sShadowRender;
    }

    // Line circle of the given radius around centre, in the plane of u and v.
    void drawCircle(const LLVector3& centre, const LLVector3& u, const LLVector3& v, F32 radius)
    {
        const S32 SEGMENTS = 24;
        LLVector3 prev = centre + u * radius;
        for (S32 i = 1; i <= SEGMENTS; ++i)
        {
            const F32 angle = F_TWO_PI * (F32)i / (F32)SEGMENTS;
            const LLVector3 next = centre + (u * cosf(angle) + v * sinf(angle)) * radius;
            gGL.vertex3fv(prev.mV);
            gGL.vertex3fv(next.mV);
            prev = next;
        }
    }
}

void KSHairOcclusion::applyShader(LLGLSLShader& shader)
{
    static const std::vector<Shape> none;
    const std::vector<Shape>& shapes = passAllowed() ? currentShapes() : none;
    const S32 count = llmin((S32)shapes.size(), MAX_SHAPES);
    shader.uniform1i(sCount, count);
    if (count == 0)
    {
        return;
    }

    // View space, matching light_position[] (LLLightState::setPosition()) and
    // vary_position. The camera modelview is current while alpha is prepared.
    const glm::mat4 modelview = get_current_modelview();
    LLVector4 axis_a[MAX_SHAPES];
    LLVector3 axis_b[MAX_SHAPES];
    for (S32 i = 0; i < count; ++i)
    {
        const glm::vec3 a = mul_mat4_vec3(modelview, glm::vec3(shapes[i].a));
        const glm::vec3 b = mul_mat4_vec3(modelview, glm::vec3(shapes[i].b));
        axis_a[i].set(a.x, a.y, a.z, shapes[i].radius);
        axis_b[i].set(b.x, b.y, b.z);
    }
    shader.uniform4fv(sAxisA, count, axis_a[0].mV);
    shader.uniform3fv(sAxisB, count, axis_b[0].mV);

    static LLCachedControl<F32> light_strength(gSavedSettings, "KSHairOcclusionLightStrength", 0.85f);
    static LLCachedControl<F32> softness(gSavedSettings, "KSHairOcclusionSoftness", 0.15f);
    static LLCachedControl<F32> ao_strength(gSavedSettings, "KSHairOcclusionAOStrength", 0.6f);
    static LLCachedControl<F32> open_angle(gSavedSettings, "KSHairOcclusionOpenAngle", 0.5f);
    static LLCachedControl<F32> ao_fade(gSavedSettings, "KSHairOcclusionAOFade", 0.04f);
    static LLCachedControl<F32> ao_inside(gSavedSettings, "KSHairOcclusionAOInside", 0.01f);
    static LLCachedControl<F32> ao_normal(gSavedSettings, "KSHairOcclusionAONormal", 0.f);
    static LLCachedControl<S32> debug_view(gSavedSettings, "KSHairOcclusionDebug", 0);
    shader.uniform4f(sParams, light_strength, llmax((F32)softness, 0.001f), ao_strength, open_angle);
    shader.uniform4f(sParams2, llmax((F32)ao_fade, 0.001f), ao_inside, ao_normal, (F32)debug_view);
}

void KSHairOcclusion::applyDraw(LLGLSLShader* shader, const LLDrawInfo& params, LLSpatialGroup* group)
{
    if (!shader || !isEnabled())
    {
        return; // the shape count is 0, so the flag is ignored
    }

    bool self = false;
    if (params.mAvatar.notNull())
    {
        self = params.mAvatar->isSelf(); // rigged to the self avatar
    }
    else if (group)
    {
        // Non-rigged attachment: its spatial bridge's root object.
        LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();
        LLViewerObject* object = bridge && bridge->mDrawable ? bridge->mDrawable->getVObj().get() : nullptr;
        LLVOAvatar* avatar = object && object->isAttachment() ? object->getAvatar() : nullptr;
        self = avatar && avatar->isSelf();
    }
    shader->uniform1i(sSelf, self ? 1 : 0);
}

void KSHairOcclusion::renderProxies()
{
    static LLCachedControl<bool> show(gSavedSettings, "KSHairOcclusionShowProxies", false);
    if (!show)
    {
        return;
    }

    const std::vector<Shape>& shapes = currentShapes();
    if (shapes.empty())
    {
        return;
    }

    LLGLSUIDefault gls_ui;
    LLGLDepthTest gls_depth(GL_TRUE);
    gUIProgram.bind();
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.setLineWidth(1.f);
    gGL.begin(LLRender::LINES);
    gGL.color4f(0.2f, 1.f, 0.6f, 1.f);
    for (const Shape& shape : shapes)
    {
        // Frame around the axis; a sphere uses the world axes.
        LLVector3 axis = shape.b - shape.a;
        const F32 length = axis.normalize();
        if (length < 0.001f)
        {
            axis = LLVector3::z_axis;
        }
        LLVector3 u = (fabsf(axis.mV[VZ]) < 0.9f ? LLVector3::z_axis : LLVector3::x_axis) % axis;
        u.normalize();
        const LLVector3 v = axis % u;

        // Rings and great circles at each end, and four side lines.
        for (const LLVector3& end : { shape.a, shape.b })
        {
            drawCircle(end, u, v, shape.radius);
            drawCircle(end, u, axis, shape.radius);
            drawCircle(end, v, axis, shape.radius);
        }
        if (length >= 0.001f)
        {
            for (const LLVector3& dir : { u, -u, v, -v })
            {
                gGL.vertex3fv((shape.a + dir * shape.radius).mV);
                gGL.vertex3fv((shape.b + dir * shape.radius).mV);
            }
        }
    }
    gGL.end();
}
