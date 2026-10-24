/**
 * @file ksHairOcclusionF.glsl
 * @author Kissmy Spicoli
 * @brief Body-proxy hair occlusion for forward alpha (see doc/ks-hair-occlusion.md).
 *
 * The self avatar's head, neck and shoulders are approximated by up to four
 * capsules (a sphere is a capsule with A == B), in view space. Self-avatar
 * draws use them to occlude the sun/moon and local lights and to darken
 * fragments next to the body (fake AO). Linked into alphaF, materialF
 * (blend mode) and pbralphaF; inactive unless ks_hocc_self and
 * ks_hocc_count are set by KSHairOcclusion.
 */

uniform int ks_hocc_count;   // shapes uploaded, 0 = off
uniform int ks_hocc_self;    // 1 while drawing the self avatar's attachments
uniform vec4 ks_hocc_a[4];   // xyz = axis start (view space), w = radius
uniform vec3 ks_hocc_b[4];   // axis end (view space)
uniform vec4 ks_hocc_params;  // x light strength, y softness, z AO strength, w open angle
uniform vec4 ks_hocc_params2; // x AO fade (m), y AO inside (m), z AO normal weight, w debug view

// Per-fragment state, written by ksHairAO() before any light factor.
float ks_hocc_ao_value = 1.0;
vec3 ks_hocc_out = vec3(0.0, 0.0, 1.0);
float ks_hocc_dbg_local = 1.0;
float ks_hocc_dbg_sun = 1.0;

bool ksHairActive()
{
    return ks_hocc_self != 0 && ks_hocc_count > 0;
}

// Closest point to x on shape i's axis segment.
vec3 ksHairAxisPoint(int i, vec3 x)
{
    vec3 a = ks_hocc_a[i].xyz;
    vec3 ab = ks_hocc_b[i] - a;
    float len2 = dot(ab, ab);
    float h = len2 > 1e-8 ? clamp(dot(x - a, ab) / len2, 0.0, 1.0) : 0.0;
    return a + ab * h;
}

// Soft visibility of a light in direction l (distance dist) past shape i.
float ksHairShapeVis(int i, vec3 p, vec3 l, float dist)
{
    float r = ks_hocc_a[i].w;
    // Axis point nearest the ray: start from the point nearest p, slide along
    // the ray to it, and take the axis point nearest that ray point.
    float t = clamp(dot(ksHairAxisPoint(i, p) - p, l), 0.0, dist);
    vec3 v = ksHairAxisPoint(i, p + l * t) - p;
    float d = length(v);
    if (d < 1e-5)
    {
        return 0.5; // on the axis: no direction to judge by
    }
    float along = dot(v, l);
    if (along > dist)
    {
        return 1.0; // shape is beyond the light
    }
    // Inside the shape sin_a is 1, which makes this a soft terminator.
    float sin_a = min(r / d, 1.0);
    float cos_a = sqrt(1.0 - sin_a * sin_a);
    float w = ks_hocc_params.y;
    return 1.0 - smoothstep(cos_a - w, cos_a + w, along / d);
}

float ksHairLightVis(vec3 p, vec3 l, float dist)
{
    float vis = 1.0;
    for (int i = 0; i < 4; ++i)
    {
        if (i >= ks_hocc_count)
        {
            break;
        }
        vis = min(vis, ksHairShapeVis(i, p, l, dist));
    }
    return mix(1.0, vis, ks_hocc_params.x);
}

// AO applies to a light unless it shines straight in (from outside the
// nearest shape) and is not occluded.
float ksHairApplyAO(float vis, vec3 l)
{
    float open = smoothstep(0.0, max(ks_hocc_params.w, 1e-3), dot(l, ks_hocc_out));
    return vis * mix(ks_hocc_ao_value, 1.0, open * vis);
}

// Proximity darkening for ambient. Call once per fragment, first.
float ksHairAO(vec3 p, vec3 n)
{
    ks_hocc_ao_value = 1.0;
    if (!ksHairActive())
    {
        return 1.0;
    }

    float best = 1e6;
    for (int i = 0; i < 4; ++i)
    {
        if (i >= ks_hocc_count)
        {
            break;
        }
        vec3 dv = p - ksHairAxisPoint(i, p);
        float len = length(dv);
        float sd = len - ks_hocc_a[i].w;
        if (sd < best)
        {
            best = sd;
            ks_hocc_out = len > 1e-5 ? dv / len : vec3(0.0, 0.0, 1.0);
        }
    }

    float ao = smoothstep(-ks_hocc_params2.y, ks_hocc_params2.x, best);
    float nlen = length(n);
    if (nlen > 1e-5)
    {
        ao = mix(ao, 1.0, ks_hocc_params2.z * max(0.0, dot(n / nlen, ks_hocc_out)));
    }
    ks_hocc_ao_value = mix(1.0, ao, ks_hocc_params.z);
    return ks_hocc_ao_value;
}

// Factor for one LIGHT_LOOP slot: lp.w > 0 is a point/spot light, 0 a direction.
float ksHairLightFactor(vec3 p, vec4 lp, vec3 col)
{
    if (!ksHairActive() || dot(col, col) <= 0.0)
    {
        return 1.0;
    }

    vec3 l;
    float dist;
    if (lp.w > 0.0)
    {
        vec3 to_light = lp.xyz - p;
        dist = length(to_light);
        if (dist < 1e-5)
        {
            return 1.0;
        }
        l = to_light / dist;
    }
    else
    {
        float len = length(lp.xyz);
        if (len < 1e-5)
        {
            return 1.0;
        }
        l = lp.xyz / len;
        dist = 1e10;
    }

    float vis = ksHairLightVis(p, l, dist);
    if (lp.w > 0.0)
    {
        ks_hocc_dbg_local = min(ks_hocc_dbg_local, vis);
    }
    return ksHairApplyAO(vis, l);
}

// Sun/moon: combine with the shadow map by min() so a head already in the
// shadow map does not darken twice, then apply AO.
float ksHairSunShadow(vec3 p, vec3 dir, float shadow)
{
    float len = length(dir);
    if (!ksHairActive() || len < 1e-5)
    {
        return shadow;
    }
    vec3 l = dir / len;
    float vis = ksHairLightVis(p, l, 1e10);
    ks_hocc_dbg_sun = vis;
    return ksHairApplyAO(min(shadow, vis), l);
}

bool ksHairDebugActive()
{
    return ksHairActive() && ks_hocc_params2.w > 0.5;
}

// 1 = local light visibility (darkest light), 2 = AO, 3 = sun/moon visibility.
vec3 ksHairDebugValue()
{
    if (ks_hocc_params2.w > 2.5)
    {
        return vec3(ks_hocc_dbg_sun);
    }
    if (ks_hocc_params2.w > 1.5)
    {
        return vec3(ks_hocc_ao_value);
    }
    return vec3(ks_hocc_dbg_local);
}
