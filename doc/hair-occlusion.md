# Hair occlusion (body-proxy capsule shadows + proximity darkening)

Status: implemented, not built or tested yet. Branch: `feat-hair-occlusion` off `kstorm`. Author: Kissmy Spicoli.

Note: AGENTS.md says author "chanayane@firestorm"; the KStorm brief says Kissmy Spicoli. This doc follows the brief.

## Decisions (user, 2026-10-23)

- Shapes: head, neck, shoulders only.
- Self avatar only, from the start. No other avatars.
- Sun/moon and local lights are all occluded by the shapes.
- Proximity darkening applies to ambient and to every light (sun/moon and locals), except where a light shines straight in from outside the crevice and is not occluded: there it is added at full strength.
- UI in the My Lights floater.
- "Show proxies" debug draw in phase 1.
- Forward alpha only; deferred light shaders untouched (see "Deferred path").

## Files

- New: `indra/newview/kshairocclusion.{h,cpp}` (`KSHairOcclusion`: shapes, uploads, per-draw self flag, proxy wireframes), `app_settings/shaders/class1/deferred/ksHairOcclusionF.glsl` (shader library).
- Tagged `<AS:KS>`: `alphaF.glsl`, `pbralphaF.glsl`, `materialF.glsl` (prototypes, AO/sun after the shadow sample, ambient, `LIGHT_LOOP`, debug view); `llviewershadermgr.cpp` (library added to the material, PBR alpha, alpha/skinned/HUD alpha and avatar alpha programs); `lldrawpoolalpha.cpp` (include, `applyShader()` in `prepare_alpha_shader()`, `applyDraw()` per draw in `renderAlpha()`); `settings.xml`; `CMakeLists.txt`.
- AS-owned, untagged with a comment: `asexactoit.cpp` (library in the three Exact OIT file lists), `asfloatermylights.cpp` (`renderProxies()` at the top of `renderAllLightBeacons()`), `floater_as_my_lights.xml` (new right-hand column, floater width 640 → 870).
- Not changed: the alpha impostor program (its `FOR_IMPOSTOR` path never calls the library) and the HUD PBR alpha program (its `IS_HUD` branch has no lighting).

## Problem

Lit alpha-blended hair glows on its inner layers (against the neck, behind the head), where real hair is darker.

Confirmed causes:
- Forward alpha shaders light local lights with distance, spot cone and N.L only (`calcPointLightOrSpotLight`, `LIGHT_LOOP`). No local-light or projector shadow reaches forward alpha. Only the sun shadow (`sampleDirectionalShadow`) does.
- SSAO/GTAO applies only to opaque deferred surfaces, so the hair/neck crevice gets full ambient.
- Alpha-masked hair goes through the deferred path and already gets GTAO and projector shadows. Plain deferred point lights cast no shadows there either.

## Approach

Approximate the self avatar's head, neck and shoulders with a few analytic shapes (1 sphere, capsules) built each frame from joint positions. In the forward alpha shaders:
1. **Light occlusion**: per light (sun/moon and locals), soft visibility of the light through the shapes (cheap analytic capsule shadow).
2. **Proximity darkening**: fragments near or inside a shape get ambient and light reduced (fake AO), except light arriving straight in from outside.

Only draws of the self avatar's attachments are affected (per-draw flag).

Same idea as the My Lights "No eye highlights" mask (`doc/my-lights.md`): geometric, view-space uniforms, no G-buffer tagging.

## Forward alpha paths (all must be covered)

Hair reaches the forward alpha pass through three fragment shaders. Covering only `alphaF.glsl` would miss most modern mesh hair.

| Path | Fragment shader | Ambient | Local light loop |
|---|---|---|---|
| Plain alpha (no material) | `class2/deferred/alphaF.glsl` | `irradiance` → `color.rgb` (:364-380) | `LIGHT_LOOP` :399-407 |
| Legacy material alpha (normal/spec maps) | `class3/deferred/materialF.glsl` | `ambenv` → `color` (:414-419) | `LIGHT_LOOP` :482-490 |
| PBR alpha blend | `class2/deferred/pbralphaF.glsl` | `irradiance`, `ao` into `pbrBaseLight()` (:249, :258-268) | `LIGHT_LOOP` :273-281 |

Shader selection per draw: `LLDrawPoolAlpha::renderAlpha()` (`lldrawpoolalpha.cpp:803-887`), through `ASOITDispatcher::alphaShader / materialAlphaShader / pbrAlphaShader`.

Program families that link these files:
- Stock (`llviewershadermgr.cpp`): `gDeferredAlphaProgram` + skinned + HUD (:1914-1983, `alphaF`), alpha impostor (:1987-, `alphaF`, `FOR_IMPOSTOR` skips lighting), `gDeferredAvatarAlphaProgram` (:2549, `alphaF`, system body alpha), `gDeferredMaterialProgram[]` (`materialF`), `gDeferredPBRAlphaProgram` (:1620, :1664, `pbralphaF`).
- AVBOIT (`asavboit.cpp:358`) and Mac OIT (`asmacoit.cpp:238`): clone `source.mShaderFiles`. They inherit an added library file automatically (the fragment-file strips at `asavboit.cpp:359-370` and `asmacoit.cpp:239-247` only apply to emissive/glow terminals).

`materialF.glsl` serves both deferred opaque (G-buffer) and forward alpha. The forward lighting is inside `#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)` (:387-549); the prototypes go in the matching block at :61. All `materialF` edits stay inside these blocks so the opaque variants are untouched.
- Exact OIT (`asexactoit.cpp:544`, `:598`, `:705`): builds its own file lists for `alphaF`, `pbralphaF`, `materialF`. Needs the library file added there too (AS-owned file, no tags).

Light slots (`LLPipeline::setupHWLights()`, `pipeline.cpp:6567-`): slot 0 = sun/moon (directional), slot 1 = avatar backlight (directional, `w = 0`), slots 2-7 = local lights (My Lights shader backend first via `ASLightRigRenderer::appendForwardLights()`, then `mNearbyLights`). The loops run 1-7. Occlusion applies to all of them: `light_position[i].w > 0` is a point/spot light, `w == 0` (slot 1) is a direction. The sun/moon is not in the loop; it uses `light_dir` (`sun_dir` or `moon_dir`, view space) and the sun shadow value (`shadow` in `alphaF`/`materialF`, `scol` in `pbralphaF`).

## Coordinate space

`LLLightState::setPosition()` (`llrender.cpp:731`) multiplies by the current modelview, and the shaders use `lp.xyz - vary_position`. So `light_position[]` and `vary_position` are **view space**. Proxies must be uploaded in the same space: `joint->getWorldPosition()` (agent space) times `get_current_modelview()` during the alpha pass, exactly as `collectEyeMasks()` does (`aslightrigrenderer.cpp:44-64`). Radii are unchanged by the (rigid) view transform. Getting this wrong is the most likely implementation bug.

## Joints and shapes

All needed joints have `support="base"` in `character/avatar_skeleton.xml`, so they exist on every avatar, legacy or Bento. No Bento/legacy split is needed.

Default skeleton offsets (parent-relative, metres): `mNeck` = chest + (−0.010, 0, 0.251); `mHead` = neck + (0, 0, 0.076); `mSkull` = head + (0, 0, 0.079); `mEyeLeft` = head + (0.098, 0.036, 0.079); `mCollarLeft` = chest + (−0.021, 0.085, 0.165); `mShoulderLeft` = collar + (0, 0.079, 0).

| # | Shape | Built from | Default radius |
|---|---|---|---|
| 1 | Head sphere | centre = `mHead` + 1.0 × (`mSkull` − `mHead`) + forward offset 0.01 along head X | 0.085 |
| 2 | Neck capsule | `mNeck` → `mHead` | 0.050 |
| 3 | Left shoulder capsule | `mNeck` → `mShoulderLeft` | 0.055 |
| 4 | Right shoulder capsule | `mNeck` → `mShoulderRight` | 0.055 |

Rules:
- `mSkull` is at eye height (same Z as `mEyeLeft`), not the crown. The head centre therefore sits at eye level; with r = 0.085 the sphere spans roughly from the jaw-joint level to the crown. A head that is tall rather than round can use a short vertical capsule instead (A = `mHead` + 0.04 up, B = `mHead` + 0.11 up, r = 0.08). Tune against the mesh head in use.
- Shapes are deliberately a little smaller than the real body so hair that rests on the scalp lies outside or on the surface. The math handles fragments inside a shape (see below), but undersized shapes give fewer artefacts.
- Scale with the shape sliders: `head_scale = |mSkull − mHead| / 0.079` (an eye-height measure, used only as a scale proxy); head radius × `head_scale`. Shoulder radius scales with `|mShoulderLeft − mShoulderRight| / 0.328`. Joint offsets from rigged meshes are already in `getWorldPosition()`.
- Head forward direction: `mHead` world rotation × X axis (`LLJoint::getWorldRotation()`).
- Mesh heads do not match system proportions, so every radius and offset is a setting.
- Shape count is fixed and small: `uniform vec4 ks_hocc_a[4]` (xyz = A, w = radius), `uniform vec3 ks_hocc_b[4]` (B; a sphere has A = B), `uniform int ks_hocc_count`, the per-draw `uniform int ks_hocc_self`, and `ks_hocc_params` (light strength, softness, AO strength, open angle) / `ks_hocc_params2` (AO fade, AO inside, AO normal weight, debug view).

## Occlusion math

Per fragment: `P` = `vary_position` (view space), `N` = normal. Shape: axis segment A→B, radius r.

Closest point on the axis to a point X: `h = clamp(dot(X − A, B − A) / dot(B − A, B − A), 0, 1)`, `Q(X) = A + h (B − A)` (sphere: `Q = A`).

### Light visibility (cheap soft capsule shadow)

Point/spot light at `Lp`: `L = Lp − P`, `dist = |L|`, `l = L / dist`. Directional light (sun/moon `light_dir`, slot 1 with `w == 0`): `l = normalize(direction)`, `dist = +infinity` (step 2's "beyond the light" test never fires).

1. Representative centre: the axis point closest to the ray. Cheap form: `C = Q(P + l · t*)`, with `t* = clamp(dot(Q(P) − P, l), 0, dist)`. One refinement step is enough for capsules this short.
2. Cone test from P toward C: `v = C − P`, `D = |v|`.
   - Shape beyond the light (`dot(v, l) > dist`): visible, `vis = 1`.
   - `sinA = min(r / D, 1)`, `cosA = sqrt(1 − sinA²)` (angular radius of the shape seen from P).
   - `cosT = dot(v / D, l)` (angle between light and shape centre).
   - `vis = 1 − smoothstep(cosA − w, cosA + w, cosT)`, `w` = softness (`KSHairOcclusionSoftness`, default 0.15). Optionally widen `w` with the light radius for big soft lights.
3. Fragment inside the shape (`D < r`): `sinA = 1`, `cosA = 0`, so the test becomes a soft terminator: fragments on the far side of the shape centre from the light are shadowed, the near side is lit. That is right for hair wrapped around the head.
4. Combine shapes with `min()` (not product, to avoid double darkening), then `vis = mix(1, vis, KSHairOcclusionLightStrength)`.
5. Apply to locals: `light += calcPointLightOrSpotLight(...) * ksHairLightFactor(P, light_position[i], light_diffuse[i])` in each of the three `LIGHT_LOOP` macros (slots 1-7). The factor is `vis` combined with the AO rule below.
6. Apply to sun/moon: `shadow = min(shadow, vis_sun)` (`scol` in `pbralphaF`), then the sun contribution × the AO rule below. `min()` with the shadow map, not a product, so a head that already shadows hair in the shadow map is not counted twice. The capsules add what the shadow map misses: its resolution is too low for hair a few cm from the neck, and with shadows off they are the only sun occlusion.

Early-outs: `ks_hocc_count == 0 || ks_hocc_self == 0` returns 1 (one uniform branch, coherent). Skip the shapes when the light's distance attenuation is already 0 (the call result is 0 anyway, so the multiply can come after a `dot(col, col) > 0` test).

### Proximity darkening (fake AO)

`d = min over shapes of (|P − Q(P)| − r)` (signed distance to the nearest shape surface; negative inside).

- `ao = smoothstep(−KSHairOcclusionAOInside, KSHairOcclusionAOFade, d)`; defaults: fade 0.04 m outside, full effect at 0.01 m inside.
- Optional normal weight (hair normals are unreliable on double-sided cards, so off by default): `ao = mix(ao, 1, KSHairOcclusionAONormal * max(0, dot(N, normalize(P − Q(P)))))`, so fragments facing away from the body darken less.
- `ao = mix(1, ao, KSHairOcclusionAOStrength)`; never below `1 − strength`.
- Ambient: `irradiance` in `alphaF`, `ambenv` in `materialF`, the `ao` argument of `pbrBaseLight()` in `pbralphaF`, all × `ao`.
- Every light (sun/moon, slot 1, locals): AO applies except where the light shines straight in. Outward direction from the nearest shape: `n_out = normalize(P − Q_nearest(P))`. Openness: `open = smoothstep(0, KSHairOcclusionOpenAngle, dot(l, n_out))` (default 0.5, about 60° from straight in). Per-light factor: `factor = vis · mix(ao, 1, open · vis)`. Light arriving from outside the crevice and not occluded (`open = 1`, `vis = 1`) is added at full strength; light that is grazing, coming from behind the shape, or in penumbra gets the AO reduction.
- PBR: `pbrBaseLight()` takes the sun term through `scol` and ambient through `irradiance`, so the sun factor goes into `scol` and the AO into `irradiance` before the call; its `ao` argument (material occlusion) is left alone.

### Debug view

`KSHairOcclusionDebug` (`ks_hocc_params2.w`): 1 = output the darkest local-light `vis` as grey, 2 = output `ao` as grey, 3 = sun/moon `vis` as grey. Non-self draws output unchanged colour, which also shows the self gating. Patterned on `as_ao_debug_white` (`alphaF.glsl:437`). Needed by the test plan.

## Shader layout

New AS-owned fragment library `class1/deferred/ksHairOcclusionF.glsl`: uniforms plus `float ksHairAO(vec3 p, vec3 n)` (also caches the nearest shape's `n_out`; call first), `float ksHairLightFactor(vec3 p, vec4 lp, vec3 col)` (loop slots; returns 1 for unused slots with zero colour), `float ksHairSunShadow(vec3 p, vec3 dir, float shadow)`, and `ksHairDebugActive()` / `ksHairDebugValue()`. Each of the three main shaders declares the prototypes (like `sampleDirectionalShadow`) and gets small `<AS:KS>`-tagged edits after the shadow sample, at the ambient line, the `LIGHT_LOOP` macro and the debug output.

Attach the library:
- `llviewershadermgr.cpp`: one tagged `mShaderFiles.push_back(...)` after `materialF` (all material programs), the PBR alpha program (its rigged variant copies the list), the alpha/skinned/HUD alpha loop, and avatar alpha. Not added to the alpha impostor and HUD PBR programs, which never call it.
- `asexactoit.cpp`: same addition at :544, :598, :705.
- AVBOIT and Mac OIT inherit it.

Alternative with fewer C++ edits: inline the ~50 lines into each of the three shaders. Rejected: three copies drift.

No shader version bump. Validate each edited shader with glslang once `.glslang/bin/glslang.exe` exists (currently absent).

## Where uniforms are set

New AS-owned module `kshairocclusion.{h,cpp}`, class `KSHairOcclusion`:
- `update()` once per frame (idle or the start of the alpha pass): read joints from `gAgentAvatarp`, build agent-space shapes, cache them. Skip when disabled, `!isAgentAvatarValid()`, or the avatar is an impostor / not drawn.
- `apply(LLGLSLShader& shader)`: transform to view space with `get_current_modelview()`, upload `ks_hocc_*`. Uploads count 0 when disabled.

Single upload point: `prepare_alpha_shader()` (`lldrawpoolalpha.cpp:109`), next to the `as_ao_debug_white` upload, one tagged call `KSHairOcclusion::apply(*shader)`. That function already recurses into `mRiggedVariant`, and every OIT backend calls it for its own programs (`asexactoit.cpp:1055`, `asavboit.cpp:760`, `asmacoit.cpp:926`). Uniforms are program state, so later `bindDeferredShaderFast()` rebinds keep them.

`gDeferredAvatarAlphaProgram` (system body alpha, drawn by `LLDrawPoolAvatar`, not via `prepare_alpha_shader()`) links the library and calls it, but receives no upload, so it sees count 0 and is unchanged. That is intended.

Force count 0 for `gCubeSnapshot`, `LLPipeline::sImpostorRender`, `LLPipeline::sRenderingHUDs`, and reflection renders, matching the debug-white guard. Cube captures must not bake avatar shadows into probes.

### Self-avatar gating (phase 1)

Per-draw `uniform int ks_hocc_self`, set in `LLDrawPoolAlpha::renderAlpha()` after the shader bind and before the draw (`lldrawpoolalpha.cpp:928` area), one tagged call `KSHairOcclusion::applyDraw(current_shader, params, group)`:
- Rigged draw: self when `params.mAvatar == gAgentAvatarp`.
- Non-rigged draw: self when the group's bridge (`group->getSpatialPartition()->asBridge()`) drawable's object is an attachment whose `getAvatar()` is self.
- Otherwise 0. Set on every draw: shaders change between draws and uniforms are per program. `LLGLSLShader::uniform1i` is value-cached (`shouldChange`), so repeated sets cost a map lookup.

The AVBOIT colour pass and Exact/Mac OIT capture all draw through `renderAlpha()`, so they get the flag. Other avatars, animesh and world alpha stay exactly as now. Only this viewer's rendering changes.

## Deferred path and sun

**Sun/moon**: in scope (forward alpha), see "Light visibility" step 6.

**Deferred local lights** (`class3/deferred/pointLightF.glsl`, `multiPointLightF.glsl`, `spotLightF.glsl`, `multiSpotLightF.glsl`): not covered (decision: forward alpha only). These shaders light everything opaque (skin, clothes, world) and alpha-masked hair from the G-buffer after the geometry is drawn. Covering them would help alpha-masked hair under point lights (projectors already shadow it). Costs:
- Stock lights are drawn in `LLPipeline::renderDeferredLighting()`; covering them needs tagged uploads there for all four programs (the eye mask only uploads in `ASLightRigRenderer::render()`, Shader backend).
- The G-buffer has no "self" or "hair" mark, so the self-only rule cannot be applied: the shapes would also shade your skin and clothes, and anything else behind your head. Neck under the chin shadowed by the head is correct; skin lying on an undersized shape's surface can show a dark terminator band.
- Deferred AO is already handled by GTAO; only the light term would be added.
- Making it self-only would need a new G-buffer flag written by self attachments, a much larger change across the deferred material shaders.

## UI (My Lights floater)

A third column on the right of `floater_as_my_lights.xml` (floater width 640 → 870), above the spot section so it stays visible when the Shader backend collapses the floater: **Hair occlusion** header, **Shade hair with head, neck, shoulders** (`KSHairOcclusionEnabled`), **Show proxies**, **Debug view** combo (Off / Local lights / Darkening / Sun/moon), sliders Shadow, Shadow softness, Darkening, Reach, Inside, Straight-in, Facing away, Head, Neck, Shoulders. Every control binds its setting with `control_name`, so no floater code is needed and changes apply live. `KSHairOcclusionHeadOffset` is a debug setting only. Works with both backends.

The floater is not resizable; if an old saved rect keeps it at 640 wide, the new column is cut off (check at runtime).

## Show proxies (debug draw)

`KSHairOcclusion::renderProxies()`, called at the top of `ASFloaterMyLights::renderAllLightBeacons()` (before its beacon-setting early return), which `llviewerdisplay.cpp:1952` calls under the UI debug-feature gate, so no new edit there and never in snapshots. Same GL setup as the beacon (`gUIProgram`, `LLRender::LINES`, agent-space vertices). Draws three circles at each capsule end plus four side lines, in green. Uses the cached agent-space shapes from `update()`, so it shows exactly what the shader gets. Setting `KSHairOcclusionShowProxies` (not persisted).

## Settings (all persisted, `settings.xml`, tagged)

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `KSHairOcclusionEnabled` | Boolean | FALSE | Master switch |
| `KSHairOcclusionLightStrength` | F32 0-1 | 0.85 | Light occlusion amount |
| `KSHairOcclusionSoftness` | F32 0.01-0.5 | 0.15 | Penumbra width (cosine units) |
| `KSHairOcclusionAOStrength` | F32 0-1 | 0.6 | Proximity darkening amount |
| `KSHairOcclusionAOFade` | F32 m | 0.04 | AO reach outside shapes |
| `KSHairOcclusionAOInside` | F32 m | 0.01 | Depth inside a shape for full AO |
| `KSHairOcclusionOpenAngle` | F32 0.05-1 | 0.5 | How directly a light must shine in to skip AO (cosine) |
| `KSHairOcclusionAONormal` | F32 0-1 | 0.0 | Normal weighting of AO |
| `KSHairOcclusionHeadRadius` | F32 m | 0.085 | Head sphere radius (before scaling) |
| `KSHairOcclusionHeadOffset` | Vector3 | 0.01, 0, 1.0 | Head centre: forward m, side m, fraction mHead→mSkull |
| `KSHairOcclusionNeckRadius` | F32 m | 0.05 | Neck capsule |
| `KSHairOcclusionShoulderRadius` | F32 m | 0.055 | Shoulder capsules |
| `KSHairOcclusionShowProxies` | Boolean | FALSE | Wireframe shapes (not persisted) |
| `KSHairOcclusionDebug` | S32 0-3 | 0 | Debug view (not persisted) |

## Performance

- Cost per self forward-alpha fragment: 4 shapes × (7 loop lights + sun) for visibility (~25 ALU each: dot, sqrt, smoothstep) plus 4 for AO. Roughly 850 ALU at worst; hair is the highest-overdraw content in a close-up, so this is the main cost. Unused light slots have zero colour; skip their shapes after the `dot(col, col) > 0` test.
- Non-self draws and the disabled case: one uniform branch (`ks_hocc_self`, `ks_hocc_count`).
- AVBOIT: only the colour pass (`avboitRasterPass == 2`) lights; occupancy passes return earlier (`alphaF.glsl:335-341`). Exact OIT lights once per captured fragment.
- CPU: ~10 joint reads and a 4-element upload per prepared program per alpha pass, plus one cached `uniform1i` per alpha draw. Negligible.
- Optional: skip shapes whose bounding sphere is farther than the light radius from the light (CPU culling per light would need per-light masks; not worth it in phase 1).

## Limitations

- Shapes are approximations; long hair down the back below the shoulders gets no AO (no back shape, by decision).
- Hair does not shadow hair (no strand self-shadowing). Only body proxies occlude.
- Self only: your head does not shade other avatars' or world alpha.
- Forward path only unless the deferred question is answered yes; alpha-masked hair under plain deferred point lights is unchanged.
- Mesh heads that differ a lot from the system head need manual radius tuning.
- Impostors, HUDs, reflection probes and cube snapshots are unaffected by design.
- Animesh and other avatars: not covered.

## Runtime test plan

Use My Lights (both backends) and ordinary world lights. Debug view on for steps 1-4.

1. Light in front of the face: hair behind the head and against the back of the neck darkens; front strands unchanged. Debug 1 shows a soft shadow behind the head.
2. Light behind the head: the inverse; face-framing strands darken.
3. Light above: hair under the jaw/at the shoulders darkens; top of the head is lit.
4. Debug 2: inner layers next to the neck and scalp are darkest; outer layers near 1. A light shining straight at the inner layers from outside (e.g. from below/behind into the neck gap) still lights them.
5. Sun/moon: debug 3 with the sun behind the head; shadows on vs off. With shadows on, no darker double shadow where the shadow map already shades.
6. Show proxies: shapes sit inside the head, neck and shoulders and follow animation; set each radius to 0 to confirm each contributes.
7. System head, small and large mesh heads, extreme head-size and shoulder sliders: shapes follow and scale; no dark band on hair resting on the scalp.
8. Self gating: another avatar's hair and world alpha next to your head are unchanged (debug views show their normal colour). Your rigged and non-rigged attachments both change.
9. Transparency modes: none, Exact OIT, AVBOIT, Mac OIT (where available). Results match.
10. Plain alpha hair, legacy-material hair (normal/spec), PBR alpha hair: all three change.
11. Classic mode (`classic_mode`), a snapshot, both My Lights backends.
12. Impostored self, HUD alpha, reflection probe capture: unchanged.
13. Toggle `KSHairOcclusionEnabled` off: output identical to before the change.
14. FPS with hair filling the screen and 6 local lights: on vs off.

## Build notes

- `.glslang/bin/glslang.exe` is absent, so the shader edits were reviewed by hand only. A compile/link failure shows in the viewer log as a shader error for "Deferred Alpha Shader", "Material Shader N" or "Deferred PBR Alpha Shader".
- No shader version bump.
