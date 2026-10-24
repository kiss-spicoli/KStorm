# KStorm2 lighting UI design

Author: Kissmy Spicoli. Date: 2026-10-24. Status: **in design**; nothing here is implemented.

Input: `doc/kstorm2-lighting-factor-map.md` section 10 (panel summary), with the decisions in its sections 8 and 12 (12.1, 12.7, 12.8) folded in. That document holds the rationale and code analysis. This one is the specification: per control, the final label, binding, range, step, tooltip, dependency behaviour and code site.

Search terms: KStorm2 UI, lighting panels, P1 Environment, P2 Camera & Tone, P3 Shadows Occlusion Reflections, P4 Local Lights, P5 Lighting Status, labels, ranges, tooltips, haze group.

---

## 1. Global rules

| # | Rule | Source |
|---|---|---|
| R1 | Lighting is EEP/PBR with HDR: mode C for EEP skies, mode B (LL auto-adjust) for legacy skies. HDR is forced on; there is no HDR or Vintage toggle. | map §1, §8 P1 |
| R2 | Legacy skies use LL's default auto-adjust: `RenderSkyAutoAdjustLegacy` forced on (no UI), scale settings at their shipped defaults, auto-adjust probe ambiance 0.01. No cloning or conversion; day cycles work unchanged. EEP and Shared Environment stay selectable. | map §8 P2 |
| R3 | Probe Fill Strength never goes below 0.01, in any panel. | map §8 L2 |
| R4 | New labels apply only to these new panels. Existing panels keep their labels, except My Lights, which also takes the 11.1 changes marked "My Lights". | map §12.1 |
| R5 | Local lighting outside My Lights (prim lights, Build controls) keeps its current behaviour and labels. | map §12.1 |
| R6 | Slider + number-box pairs are allowed, including pairs whose two ranges differ. No other duplicate control for one value in one pane; no sun/moon trackball. | map §12.6 |
| R7 | The Haze group is always surfaced together and shows its haze-0 dependency (section 3). | map §12.7 |
| R8 | Range rule. Decided ranges are used as written. Undecided ranges: sky fields use the sky editor's range; settings use the existing Phototools slider + number pair where one exists, otherwise the Preferences range. | map §12.5, §12.8 |
| R9 | Any control that writes the sky writes the ENV_LOCAL sky and turns on the "Local override active" indicator (with Reset) in P1 and P5. | map §12.8 |
| R10 | Stored = display × factor where the sky editor scales a field (density multiplier ×0.001, glow size `(2−v)·20`, glow focus `v·−5`). The new UI shows the same display values as the sky editor. | `llpaneleditsky.cpp:113-117` |

Range notation: `min–max / step`. A pair is written `slider min–max / step; number min–max / step`.

---

## 2. P1 Environment

### 2.1 Source

| Control | Binding / action | Notes |
|---|---|---|
| Use Shared Environment | `LLEnvironment` clear local | Turns off "Local override active". |
| My Environments… | opens `my_environments` | |
| Sky / Water / Day presets, prev/next | as Phototools preset combos (`quickprefs.cpp`) | |
| Reset to region default | as Phototools `ResetToRegionDefault` | |
| Transition time | `FSEnvironmentManualTransitionTime` | Seconds to blend when a sky, water or day cycle is picked manually (inventory apply, World > Environment presets, the preset combos). 0 = instant (default). Not used for region/parcel changes or day-cycle progression (`llenvironment.cpp:1391-1395`). Range: deferred (O8). |
| Set KS environment | `KSSetEnvironment` (check box) | Note: sets Sky HDR Scale 0.1, which pins auto-exposure (P2). |
| Edit as asset… | opens the Fixed sky / Day Cycle / Water editors | |
| Sky origin | read-only: "Legacy sky (auto-adjusted)" / "EEP sky" | R2. |
| Local override active + Reset | indicator; Reset = Personal Lighting reset | R9. |

### 2.2 Key light

| Label | Binding | Range / step | Tooltip | Behaviour | Code site |
|---|---|---|---|---|---|
| Sun azimuth, Sun elevation | sky `sun_rotation` | 0–359.99 / 0.01; −90–90 / 0.01 | | No trackball (R6). | `llpaneleditsky.cpp` |
| Moon azimuth, Moon elevation | sky `moon_rotation` | same | | same | same |
| **Sun & Moon Light Colour** | sky `sunlight_color` | colour + intensity, 0–3 per channel | "Light colour of both the sun and the moon." | Colour/intensity split replaces the ÷3 swatch. | `llsettingsvo.cpp:797-807` |
| Sunlight Scale | `RenderHDRSkySunlightScale` | 0–20 / 0.01 | "Brightness of direct sun and moon light." | | `llsettingsvo.cpp:838` |
| Sun scale | sky `sun_scale` | 0.25–20 / 0.01 | "Larger discs fade out later at the horizon." | Also moves the twilight fade. | `ascelestialtwilight.cpp` |
| Moon scale | sky `moon_scale` | 0.25–20 / 0.01 | same | same | same |
| **Moon Disk Brightness** | sky `moon_brightness` | 0–1 / 0.01 | "Brightness of the moon disc and night glow. Does not change moonlight on surfaces." | | `moonF.glsl:270` |
| Partial moon below horizon | `ASRenderPartialMoonBelowHorizon` | check box | | | `ascelestialtwilight.cpp` |
| Star brightness | sky `star_brightness` | 0–500 / 0.01 | | | `lldrawpoolwlsky.cpp:250` |

### 2.3 Fill

| Label | Binding | Range / step | Tooltip | Behaviour | Code site |
|---|---|---|---|---|---|
| **Ambient Brightness** | sky `ambient` (scales the stored colour; hue kept) | Open O1 (proposed 0–3, the code max per channel) | "Brightness of sky fill light. Its colour comes from the sky; surfaces use brightness only. At Probe Fill Strength ≥ 1 this no longer lights surfaces." | Writes ENV_LOCAL (R9). | `atmosphericsFuncs.glsl:110-157` |
| Ambient Scale | `RenderSkyAmbientScale` | −10–40 / 0.1 | "Scales all sky fill. Negative values darken lit areas." | Negatives allowed. | `atmosphericsFuncs.glsl:164` |
| **Probe Fill Strength** | sky `reflection_probe_ambiance` | 0.01–10 / 0.01 | "Low: fill from sky ambient (uniform). High: fill from probes (bounce light, darker interiors). Reflections are unaffected." | Disabled when probes are off (value forced to 1.0 then, `llsettingsvo.cpp:858`). Writes ENV_LOCAL. | `reflectionProbeF.glsl:569-571`, `llreflectionmapmanager.cpp:1221` |

### 2.4 Atmosphere: Haze group (one block, in this order)

| Label | Binding | Range / step | Tooltip | Haze-0 behaviour | Code site |
|---|---|---|---|---|---|
| **Haze Density** | sky `haze_density` | 0–5 / 0.01 | "At 0, Haze Glow Strength, Haze Glow Focus and Haze Glow Brightness stop working, fog takes Air Glow Colour only, and distance fog depends on Air Density's red channel." | Master. | `atmosphericsFuncs.glsl:66-118` |
| **Haze Glow Brightness** | sky `haze_horizon` | 0–5 / 0.01 | "Brightness of white haze and its sun glow." | Inactive with "No effect while Haze Density is 0" (value kept, editable). | same |
| **Haze Glow Strength** | sky `glow.x` = `(2−v)·20` | 0–1.99 / 0.01 | "Strength of the haze glow around the sun; higher = brighter. Does not change its spread." | Inactive at 0, as above. | same |
| **Haze Glow Focus** | sky `glow.z` = `v·−5` | −2–2 / 0.01 | "Tightness of the haze glow; higher = tighter, more concentrated halo." | Inactive at 0, as above. | same |
| **Air Glow Colour** | sky `blue_horizon` | 0–3 per channel (colour + intensity) | "Sky and distant-fog colour from clear air." | Note at 0: "Sole fog colour while Haze Density is 0." | `skyV.glsl:142-155` |
| **Air Density** | sky `blue_density` | 0–3 per channel (colour + intensity) | "Clear-air amount per colour: sunset reddening, fog fade rate and the air/haze mix." | Note at 0: "Fog strength follows the red component while Haze Density is 0." | `atmosphericsFuncs.glsl:66-83`, `hazeF.glsl:89` |

Engineering prerequisite: before this group can set Haze Density to 0 safely, clamp the denominators in `cloudsV.glsl:125` and `asHorizonCloudTintV.glsl:141` like `skyV.glsl:111`, and guard `lllegacyatmospherics.cpp:319` (O6).

### 2.5 Atmosphere: other

| Label | Binding | Range / step | Tooltip | Code site |
|---|---|---|---|---|
| **Atmosphere Thickness** | sky `density_multiplier` (stored = display × 0.001) | 0.0001–2 / 0.0001 | "Scales haze thickness and sun extinction." | `atmosphericsFuncs.glsl:66,79` |
| **Fog Distance Scale** | sky `distance_multiplier` | 0.05–1000 / 0.01 | "Scales how fast distance fog builds." | same |
| **Haze Ceiling (m)** | sky `max_y` | 0–10000 / 0.01 | "Height that limits haze and sun extinction." | `atmosphericsFuncs.glsl:55-66` |
| Moisture, Droplet radius, Ice | sky `moisture_level`, `droplet_radius`, `ice_level` | 0–1 / 0.001; 5–1000 / 0.01; 0–1 / 0.001 | "Rainbow and halo only." | `skyF.glsl` |
| Atmospheric pass | `RenderDeferredAtmospheric` | check box | "Haze on scene geometry." | `pipeline.cpp:1368` |

### 2.6 Clouds

Coverage 0–1 / 0.01 (also removes sun and adds fill); scale 0.01–3 / 0.01; variance 0–1 / 0.01; colour; density X, Y 0–1 / 0.01, D 0–3 / 0.01; detail X, Y, D 0–1 / 0.01; scroll (XY pad); map (texture). Bindings: sky `cloud_*`. Ranges as the sky editor (R8).

### 2.7 Water

Fog colour; fog density 0.001–100 / 0.01; underwater modifier 0–20 / 0.01; fresnel scale and offset (sky editor range); normal map; normal scale X, Y, Z 0–10 / 0.01; wave 1 and 2 directions; refraction above / below 0–3 / 0.01; blur 0–0.5 / 0.001; Transparent water (`RenderTransparentWater`, check box). Bindings: water settings as `panel_settings_water.xml`.

### 2.8 Sky visuals

Tabs embedding the existing AyaneStorm panels unchanged: Sun (procedural sun), Moon, Stars, Horizon, Auroras, Weather. Sky Detail (`WLSkyDetail`): 4–512 / 1; number 4–512 / 4 (Phototools pair).

---

## 3. P2 Camera & Tone

| Label | Binding | Range / step | Tooltip | Behaviour | Code site |
|---|---|---|---|---|---|
| **Sky HDR Scale** | sky `gamma` | 0–20 / 0.01 | "Brightness of sky and haze relative to the scene; also sets how far auto-exposure can adjust. At 0.25 or below, auto-exposure is fixed at 1." | Writes ENV_LOCAL. On a legacy (auto-adjusted) sky it sets only the auto-exposure range; sky brightness comes from `RenderSkyAutoAdjustHDRScale`. | `llsettingsvo.cpp:855`, `pipeline.cpp:8084-8093` |
| Exposure | `RenderExposure` | 0.5–4 / 0.1 | "Overall scene exposure." | | `pipeline.cpp:8161-8163` |
| Dynamic exposure | `RenderDynamicExposureEnabled` | check box | "Adapts exposure to scene brightness." | Shows "pinned" note when Sky HDR Scale ≤ 0.25. | `pipeline.cpp:8055` |
| Bright-scene level, Settle accuracy, Settle time, Object weight | `RenderDynamicExposureCoefficient`, `…SpeedError`, `…SpeedTarget`, `RenderDiffuseLuminanceScale` | proposed in §8.2 | see §8.2 | Expert sub-block. | `pipeline.cpp:7965, 8056-8058`; `exposureF.glsl`, `luminanceF.glsl` |
| Use sky HDR range | `RenderUseExposureSkySettings` | check box | "Uses fixed range 0.5–3 instead of Sky HDR Scale." (HDR min/max/offset are hard-coded) | Expert. | `pipeline.cpp:8067-8081` |
| Tone mapping | `RenderTonemapType` | Khronos Neutral / ACES | | | `tonemapUtilF.glsl:131` |
| Tone mapping mix | `RenderTonemapMix` | 0–1 / 0.05; number 0–1 / 0.001 | | | `llsettingsvo.cpp:835` |
| Glow, Glow in HDR | `RenderGlow`, `RenderGlowHDR` | check boxes | "Material glow (not haze glow)." | | `pipeline.cpp:1331-1341` |
| Glow resolution, iterations, strength, width | `RenderGlowResolutionPow`, `…Iterations`, `…Strength`, `…Width` | 8–10 / 1; 0–200 / 1 (number 0–500); 0–0.5 / 0.001 (number 0–10); 0–50 / 0.001 (number 0–5000) | | | same |
| Glow internals | `RenderGlowLumWeights`, `…MinLuminance`, `…MaxExtractAlpha`, `…WarmthAmount`, `…WarmthWeights`, `…Noise` | Open O5 (no existing UI) | | Expert. | same |
| Diffuse glow, Lens flare, Vignette, Chromatic aberration | AS camera-effects settings | existing `panel_as_camera_effects.xml` ranges | | Embedded tab. | `as*.cpp` |
| FS vignette | `FSRenderVignette` x/y/z | 0–1 / 0.05; 0–40 / 0.1 (number 0–32); 0–8 / 0.1 | | | `quickprefs.cpp` |
| Colour grading | AS colour grading | embedded tab | | | `ascolorgrading.cpp` |
| Sharpening | `RenderCASSharpness` | 0–1 / 0.001; number 0–1 / 0.1 | | | `pipeline.cpp:9307` |
| Depth of Field… | opens `as_depth_of_field` | button | | DoF settings stay in their floater. | |

---

## 4. P3 Shadows, Occlusion & Reflections

### 4.1 Shadows

| Label | Binding | Range / step |
|---|---|---|
| Shadows | `RenderShadowDetail` | None / Sun & Moon / Sun & Moon + Projectors |
| Resolution | `RenderShadowResolutionScale` | 0–5 / 0.5; number 0–7 / 0.5 |
| Bias | `RenderShadowBias` | −0.002–0.001 / 0.000001; number −5–5 / 0.000001 |
| Blur size | `RenderShadowBlurSize` | 0–4 / 0.01; number 0–1000 / 0.1 |
| Blur distance | `RenderShadowBlurDistFactor` | 0–1 / 0.01; number 0–2 / 0.01 |
| FOV cutoff | `RenderShadowFOVCutoff` | 0–100 / 0.1 |
| Spot offset | `RenderSpotShadowOffset` | 0–0.04 / 0.0001; number 0–100 / 0.001 |
| Soften | `RenderShadowGaussian.x` | 0.001–5 / 0.001; number −10000–10000 / 0.001 |
| AO soften | `RenderShadowGaussian.y` | 0.001–3 / 0.001 |
| Clarity | `RenderShadowSplitExponent.y` | 0–10 / 0.025; number 0–1000 / 0.001 |
| Internals (expert) | `RenderShadowSplits`, `…Offset`, `…OffsetError`, `…BiasError`, `…ErrorCutoff`, `…Noise`, `RenderSpotShadowBias`, `RenderDeferredSpot/TreeShadowBias/Offset`, `RenderEdgeDepth/NormCutoff`, `RenderDeferredNoise` | Open O5 (no existing UI) |

### 4.2 Occlusion

| Label | Binding | Range / step |
|---|---|---|
| Ambient occlusion | `RenderDeferredSSAO` | check box |
| Technique | `RenderAOTechnique` | Legacy / GTAO; shows only that technique's block |
| White-material check | `RenderAODebugWhite` | check box |
| Effect (one control) | `RenderSSAOEffect.x` | −1–1 / 0.01 |
| SSAO scale, max scale, factor | `RenderSSAOScale`, `…MaxScale`, `…Factor` | 0–10000 / 1; 0–10000 / 1; 0–250 / 1 (Phototools pairs) |
| Irradiance scale, max | `RenderSSAOIrradianceScale`, `…Max` | 0–2 / 0.01 (number 0–10); 0–1 / 0.001 (number 0–10) |
| GTAO quality, radius, power, denoise | `RenderGTAOQuality`, `…Radius`, `…FinalValuePower`, `…DenoisePasses` | combo; 0.05–5 / 0.05; 0.5–5 / 0.05; combo |
| GTAO bent normals, influence | `RenderGTAOBentNormals`, `…BentNormalInfluence` | check box; 0–1 / 0.01 |
| GTAO backend (expert) | `RenderGTAOBackend` | Open O5 |
| Hair occlusion | the 13 `KSHairOcclusion*` controls of `floater_ks_hair_occlusion.xml`, plus `KSHairOcclusionHeadOffset` (expert, Open O5) | existing ranges |

### 4.3 Reflections

| Label | Binding | Range / step |
|---|---|---|
| Reflection coverage | `RenderReflectionProbeLevel` | **Sky only** / Manual only / Manual + terrain / Full |
| Probe detail | `RenderReflectionProbeDetail` | existing combo |
| Probe count, resolution | `RenderReflectionProbeCount`, `…Resolution` | existing combos |
| Probe draw distance | `RenderReflectionProbeDrawDistance` | 0–4096 / 1; number 1–4096 / 1 |
| Update period, max local-light ambiance, desaturate irradiance, dynamic allocation (expert) | `RenderDefaultProbeUpdatePeriod`, `RenderReflectionProbeMaxLocalLightAmbiance`, `RenderDesaturateIrradiance`, `RenderReflectionProbeDynamicAllocation` | Open O5 |
| Show probe volumes | `RenderReflectionProbeVolumes` | check box (debug) |
| Screen-space reflections | `RenderScreenSpaceReflections` (+ 6 tuning settings, expert, Open O5) | check box |
| Mirrors, hero resolution, hero update rate | `RenderMirrors`, `RenderHeroProbeResolution`, `…UpdateRate` | check box; existing combos |
| Hero distance, conservative multiplier (expert) | `RenderHeroProbeDistance`, `RenderHeroProbeConservativeUpdateMultiplier` | Open O5 |

### 4.4 Volumetric and shader level

Volumetric: the 10 controls of `panel_as_volumetric_lighting.xml` with their ranges (albedo 0–1 / 0.01, density 0–0.25 / 0.001, sun and moon gradient 0–0.9 / 0.01, quality combo, debug 0–11 / 1, local lights check box, local intensity 0–6 / 0.01, local max count 0–64 / 1), plus `RenderVolumetricLightingSampleCountOverride` (expert, Open O5). Shader lighting level: `RenderShaderLightingMaxLevel` (existing combo).

---

## 5. P4 Local Lights

### 5.1 Budget

| Label | Binding | Range / step | Tooltip |
|---|---|---|---|
| **Max nearby lights (global)** | `RenderLocalLightCount` | 0–512 / 1 | "Counts nearby candidates, not only visible lights. Shared with Preferences and My Lights." |
| Worn lights | `RenderAttachedLights` | check box | |
| Subject sort | `KSRenderLocalLightSubjectSort` | check box | "Prioritise lights near the Alt-click focus." |
| Only My Lights | `KSLightRigOnlyMyLights` | check box | |
| Show other avatars | `ASLightRigShowOtherAvatars` | check box | |
| Light range note | `RenderFarClip` | read-only | "Lights beyond draw distance are not considered." |

### 5.2 My Lights (also applied to `floater_as_my_lights.xml`, R4)

Existing rig controls and ranges (distance 0.05–3, height −2–2, azimuth 0–359.9, offsets ±10, intensity 0–1 / 0.05, colour, boost 1–4 / 0.05, spot FOV 1–170 / 1, focus −20–20 / 0.5, ambiance 0–1 / 0.05, texture, dark, backend, presets), with these changes:

| Control | Change |
|---|---|
| Radius (m) 0.1–20 / 0.1 | Tooltip: "Light reaches about 1.5 × this distance." |
| Softness 0–2 / 0.05 | Shapes the edge curve instead of dimming; Softness up → exponent down (softer edge). Design: map §12.4; marker carrier Open O3. |
| Intensity | Tooltip loses "decrease Softness for a brighter effect"; Boost is the brightness control. |
| Spot FOV / focus / texture | Tooltip: "Not applied to alpha surfaces (hair, glass)." |
| Max lights | Label **Max nearby lights (global)**. |
| Eye mask radius | New control for `ASLightRigEyeMaskRadius`; range Open O5. |
| No eye highlights | Unchanged (`ASLightRigNoEyeSpecular`). |

### 5.3 Selected object (mirror of Build > Features)

Light on, colour, intensity, radius, falloff; projector texture, FOV, focus, ambiance; reflection probe type, dynamic, mirror, ambiance, near clip. **Labels, ranges and behaviour identical to Build** (R5).

### 5.4 Surface response and debug

| Label | Binding | Range |
|---|---|---|
| Alpha masks lit as opaque | `RenderAutoMaskAlphaDeferred`, `RenderAutoMaskAlphaNonDeferred` | check boxes |
| Bump normal strength | `RenderNormalMapScale` | existing Phototools combo |
| Material editors… | opens the material editors | button |
| Light beacons, light radius, light bounding boxes | `ASRenderLightBeacon`, `RenderLightRadius`, info display "lights" | check boxes |

---

## 6. P5 Lighting Status & Expert

| Item | Content |
|---|---|
| Sky origin | Legacy (auto-adjusted, mode B) / EEP (mode C) |
| Effective Probe Fill Strength | value in use (shows 1.0 forced when probes are off) |
| HDR / post | "HDR on"; "Post-processing off" during snapshot no-post or Build with `RenderDisablePostProcessing` |
| Probe level | current coverage |
| Auto-exposure | "Pinned at 1" when Sky HDR Scale ≤ 0.25 and dynamic exposure is on |
| Local override active | indicator + Reset (R9) |
| Bulk writers | Quality slider (`RenderQualityPerformance`, existing Preferences slider) and graphics presets, with "Overwrites settings in these panels." |
| Expert | HDRI preview + `RenderHDRIExposure`, `…Rotation`, `…SplitScreen`, `…IrradianceOnly` (Open O5); `RenderSpecularResX/Y`, `RenderSpecularExponent`; `RenderDeferredDisplayGamma` |

---

## 7. Open items

| # | Item | Status |
|---|---|---|
| O1 | Ambient Brightness definition | Answered (§8.1): no physical unit exists. Proposal stands; final unit and range still to confirm. |
| O2 | Legacy-sky conversion | **Resolved:** LL's default auto-adjust (R2). |
| O3 | My Lights falloff | Answered (§8.3): the negative value was only an internal signal. Recommendation and curve choices in §8.3. |
| O4 | Dynamic exposure | Answered (§8.2): how it works, debug settings to test, proposed ranges. |
| O5 | Expert settings with no existing UI | **Deferred**, unchanged: glow internals, shadow internals, SSR tuning, probe internals, GTAO backend, hero distance, HDRI, eye mask radius, head offset, volumetric sample override. |
| O6 | NaN guard for Haze Density 0 | **Done** on branch `fix-haze-zero-nan-guards` (uncommitted, not built; the shader validator named in AGENTS.md is not present in the repo). |
| O7 | "As-is" ranges | **Resolved:** Sunlight Scale 0–20 / 0.01 (KS); Max nearby lights 0–512. |
| O8 | Transition time | Effect described in §2.1; range deferred. |
| O9 | Trackball | **Resolved:** removed from the new UI. |
| O10 | Glow Size label | **Resolved:** Haze Glow Strength. |

---

## 8. Resolution notes

### 8.1 Ambient Brightness: physical units (O1)

There is no physical reference unit. The sky `ambient` field is a unitless RGB triple (validated 0–3 per channel). On its way to the screen it passes through non-photometric steps: cloud boost, `pow(x, 0.9) × 0.57`, sRGB-to-linear, reduction to Rec.709 luminance, × `RenderSkyAmbientScale` (`atmosphericsFuncs.glsl:113-164`), then exposure and tonemapping. The renderer works in arbitrary scene-referred units; the only absolute number in the chain is the 11.2 ceiling in `clampHDRRange` (`deferredUtil.glsl:83-93`), a safety clamp, not a calibration. No lux, nits or EV mapping exists.

Proposal: Ambient Brightness = Rec.709 luminance of the stored colour (the weights the shader uses), 0–3 / 0.01. It scales all three channels together and stops before any channel passes 3. If the stored colour is black, writing a value sets neutral grey of that luminance.

### 8.2 Dynamic exposure (O4)

Not previously exposed in any panel. Today only its range is affected, indirectly, through Sky HDR Scale.

How it works:
1. **Metering** (`luminanceF.glsl`): Rec.709 luminance of the central part of the frame, sampled slightly low to favour ground over sky. Scene objects are multiplied by `RenderDiffuseLuminanceScale` (0.5); sky and emissive are not. The result is mip-averaged to one value, L.
2. **Target** (`exposureF.glsl`): L is clamped to `[0, RenderDynamicExposureCoefficient]`, normalised and squared; the target exposure runs from the range maximum (dark scene) down to the range minimum (scene at or above the coefficient). The range is `[1/s, s]` with `s = sqrt(Sky HDR Scale) × 2`, and collapses to 1 when `s ≤ 1`.
3. **Smoothing**: exposure moves toward the target with rate `−ln(SpeedError) / SpeedTarget`: it covers `1 − SpeedError` of the gap in `SpeedTarget` seconds (defaults: 90% in 2 s).
4. **Final**: `RenderExposure × dynamic factor`, then tonemapping.

| Setting | Default | Meaning | Proposed label, range |
|---|---|---|---|
| `RenderDynamicExposureEnabled` | on (declared in code) | Turns adaptation on | Dynamic exposure, check box |
| `RenderDynamicExposureCoefficient` | 0.5 (settings.xml, not saved between sessions) | Scene luminance treated as fully bright; lower = darkens sooner | Bright-scene level, 0.05–2 / 0.01 |
| `RenderDynamicExposureSpeedError` | 0.1 (declared in code) | Fraction of the gap left after the settle time | Settle accuracy, 0.01–0.5 / 0.01 |
| `RenderDynamicExposureSpeedTarget` | 2 s (declared in code) | Time to get within that fraction | Settle time (s), 0.1–10 / 0.1 |
| `RenderDiffuseLuminanceScale` | 0.5 (not saved) | Weight of scene objects vs sky in metering | Object weight, 0–2 / 0.05 |
| `RenderUseExposureSkySettings` | off (not saved) | Uses the fixed 0.5–3 range instead of Sky HDR Scale | Use fixed range, check box |

Testing from Debug Settings (Advanced > Debug Settings) works today. `RenderDynamicExposureCoefficient`, `RenderDiffuseLuminanceScale` and `RenderUseExposureSkySettings` are listed from startup. `RenderDynamicExposureEnabled`, `…SpeedError` and `…SpeedTarget` appear after the first frame (declared in code, not saved between sessions). **Set Sky HDR Scale above 0.25 first** (for example 1.0, giving a 0.5–2 range); otherwise the range is 1–1 and none of these do anything. With Set KS environment on (gamma 0.1), auto-exposure is off.

### 8.3 My Lights falloff (O3)

**What "negative falloff" meant:** not a user value. It was a proposed internal signal: My Lights would send its falloff to the shader as a negative number, so the shared light shader could tell My Lights apart from prim lights (whose falloff is clamped to 0–2) and apply a different curve. Nobody would see or set a negative value, and it is fragile on the alpha path (map §12.4).

**Recommendation: no signal at all.** Apply the shaped curve only in My Lights' `Shader` backend, which binds the light programs itself (`aslightrigrenderer.cpp:158-232`) and can set a dedicated uniform. While a shaped curve is selected, My Lights renders through that backend; the `LLVOVolume` backend and all prim lights keep the stock curve. Alpha surfaces are lit through `appendForwardLights`; whether the forward shaders have a free per-light channel needs checking before alpha gets the same curve.

**Curve choices** (centre brightness unchanged in all; `d` = distance / reach, 0–1):

| Curve | Formula | Look | Softness controls |
|---|---|---|---|
| Stock | `2·(1−d)² / (1+f)²` | Current behaviour; Softness only dims | brightness (today) |
| **Shaped (recommended default)** | `2·(1−d)^p`, `p = 4 − 1.5 × Softness` | Softness 0 → `p` 4 (tight pool), 1 → 2.5 (close to stock), 2 → 1 (soft, linear) | edge spread |
| Smooth plateau | `2·(1 − smoothstep(e, 1, d))`, `e = 0.5 − 0.25 × Softness` | Even fill out to `e`, then a smooth edge; suits fill lights | plateau width |
| Physical | `2 · window(d) / (1 + (d·R/r₀)²)`, `window = (1 − d⁴)²` | Inverse-square with a smooth cut-off; bright near the source with a long tail, most like a real lamp | source size `r₀` (larger = softer) |

UI: a Curve combo (Stock / Shaped / Smooth plateau / Physical) plus the existing Softness slider, whose meaning follows the curve. Default Shaped at Softness 1.

### 8.4 Ranges confirmed (O7)

KS Sunlight scale is 0–20 / 0.01 (Advanced Lighting is 0–50 / 0.1); the new UI uses 0–20. Max nearby lights 0–512.
