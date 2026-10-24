/**
 * @file ksDarkSpotLightF.glsl
 * @author Kissmy Spicoli
 * @brief Dark My Lights spot: per-pixel multiplier for a BT_MULT pass.
 *
 * Covers exactly what spotLightF.glsl would light (radius, projector
 * frustum, image, focus, distance falloff, N.L) and outputs
 * 1 - darkness instead of adding light. darkness is the light's colour
 * (Intensity x Boost) times those terms, per channel, so a white dark spot
 * darkens neutrally and a coloured one removes that colour. Ambiance is
 * ignored. Alpha is 1 so destination alpha is unchanged.
 */

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D lightMap;

uniform vec3 proj_origin; // origin of projection, for N.L
uniform int proj_shadow_idx;
uniform float shadow_fade;

// Light params (MULTI_SPOTLIGHT: view-space centre as a uniform)
uniform vec3 center;
uniform float falloff;

in vec4 vary_fragcoord;

float calcLegacyDistanceAttenuation(float distance, float falloff);
bool clipProjectedLightVars(vec3 center, vec3 pos, out float dist, out float l_dist, out vec3 lv, out vec4 proj_tc);
vec4 getNorm(vec2 screenpos);
vec3 getProjectedLightDiffuseColor(float light_distance, vec2 projected_uv);
vec2 getScreenCoord(vec4 clip);
vec4 getPosition(vec2 pos_screen);

void main()
{
    vec2 tc  = getScreenCoord(vary_fragcoord);
    vec3 pos = getPosition(tc).xyz;

    vec3 lv;
    vec4 proj_tc;
    float dist, l_dist;
    if (clipProjectedLightVars(center, pos, dist, l_dist, lv, proj_tc))
    {
        discard;
    }

    // Outside the projector frustum's sides the light does not reach.
    if (proj_tc.x <= 0.0 || proj_tc.x >= 1.0 || proj_tc.y <= 0.0 || proj_tc.y >= 1.0)
    {
        discard;
    }

    float dist_atten = calcLegacyDistanceAttenuation(dist, falloff);
    if (dist_atten <= 0.0)
    {
        discard;
    }

    // Surfaces facing away from the light are not darkened.
    vec3 n = getNorm(tc).xyz;
    float nl = dot(n, normalize(proj_origin - pos));
    if (nl <= 0.0)
    {
        discard;
    }

    // Dark spots are kept out of the projector shadow maps; this only
    // matters while a previous shadow assignment fades out.
    float shadow = 1.0;
    if (proj_shadow_idx >= 0)
    {
        vec4 shd = texture(lightMap, tc);
        shadow = (proj_shadow_idx == 0) ? shd.b : shd.a;
        shadow += shadow_fade;
        shadow = clamp(shadow, 0.0, 1.0);
    }

    // Light colour times projector image (white darkens, black does not).
    vec3 dlit = getProjectedLightDiffuseColor(l_dist, proj_tc.xy);

    vec3 darkness = clamp(dlit * dist_atten * nl * shadow, vec3(0.0), vec3(1.0));
    frag_color = vec4(vec3(1.0) - darkness, 1.0);
}
