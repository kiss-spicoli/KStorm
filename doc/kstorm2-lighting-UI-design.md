# KStorm2 lighting UI design

Author: Kissmy Spicoli. Date: 2026-10-24. Status: **in design**; nothing here is implemented.

Input: `doc/kstorm2-lighting-factor-map.md` section 10 (panel summary), with the decisions in its sections 8 and 12 (12.1, 12.7, 12.8) folded in. That document holds the rationale and code analysis. This one is the specification: per control, the final label, binding, range, step, tooltip, dependency behaviour and code site.

Search terms: KStorm2 UI, lighting panels, P1 Environment, P2 Tone, P3 Shadows Occlusion Reflections, P4 Local Lights, Camera tab, P5 More, labels, ranges, tooltips, haze group.

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
| R7 | The Haze controls are always surfaced together in one section labelled **Haze** (no special highlighting) and show their haze-0 dependency (§2.4). | map §12.7 |
| R8 | Range rule. Decided ranges are used as written. Undecided ranges: sky fields use the sky editor's range; settings use the existing Phototools slider + number pair where one exists, otherwise the Preferences range. | map §12.5, §12.8 |
| R9 | Any control that writes the sky writes the ENV_LOCAL sky and turns on the "Local override active" indicator (with Reset) in P1 and P5. | map §12.8 |
| R10 | Stored = display × factor where the sky editor scales a field (density multiplier ×0.001, glow size `(2−v)·20`, glow focus `v·−5`). The new UI shows the same display values as the sky editor. | `llpaneleditsky.cpp:113-117` |
| R11 | `FSEnvironmentManualTransitionTime` stays at its default 0 (instant switch) and is not exposed. | decided 2026-10-24 |
| R12 | Forced settings are reapplied: `RenderDisableVintageMode` = TRUE, `RenderSkyAutoAdjustLegacy` = TRUE at startup and after any graphics-preset load or quality-slider change. | O12, decided 2026-10-24 |
| R13 | Local lights keep LL's ×0.9 under legacy (auto-adjusted) skies. | O13, decided 2026-10-24 |
| R14 | Sections are flat accordion tabs (`accordion` / `accordion_tab`), never nested. There are no separate detail sections: expert controls sit at the end of their parent section, after its common controls ("Expert" or "expert sub-block" in the tables below means that position). Former detail sets with no natural parent are ordinary sections (Surface and debug; HDRI and expert). Open/closed state lasts while the floater exists and is not saved across restarts for now. | decided 2026-10-24 |
| R15 | Existing floaters are never embedded. They are opened by toggle buttons (`Floater.Toggle <name>`, shown pressed while the floater is open): buttons inside the tab they belong to; there is no separate floater list (§6.1). Built-in controls for the same settings are still allowed where a section needs them (Volumetric, selected My Light). | decided 2026-10-24 |

Range notation: `min–max / step`. A pair is written `slider min–max / step; number min–max / step`.

---

## 2. P1 Environment

### 2.1 Source

| Control | Binding / action | Notes |
|---|---|---|
| Use Shared Environment | `LLEnvironment` clear local | Turns off "Local override active". |
| My Environments… | toggle button `my_environments` (R15) | The sky, water and day-cycle asset editors are not reachable from this panel (decided 2026-10-24). |
| Sky / Water / Day presets, prev/next | as Phototools preset combos (`quickprefs.cpp`) | |
| Reset to region default | as Phototools `ResetToRegionDefault` | |
| Set KS environment | `KSSetEnvironment` (check box) | Note: sets Sky HDR Scale 0.1, which pins auto-exposure (P2). |
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
| **Ambient Brightness** | sky `ambient` (scales the stored colour; hue kept) | 0–3 / 0.01 (Rec.709 luminance of the stored colour; grey fallback when black, §8.1) | "Brightness of sky fill light. Its colour comes from the sky; surfaces use brightness only. At Probe Fill Strength ≥ 1 this no longer lights surfaces." | Writes ENV_LOCAL (R9). | `atmosphericsFuncs.glsl:110-157` |
| Ambient Scale | `RenderSkyAmbientScale` | −10–40 / 0.1 | "Scales all sky fill. Negative values darken lit areas." | Negatives allowed. | `atmosphericsFuncs.glsl:164` |
| **Probe Fill Strength** | sky `reflection_probe_ambiance` | 0.01–10 / 0.01 | "Low: fill from sky ambient (uniform). High: fill from probes (bounce light, darker interiors). Reflections are unaffected." | Disabled when probes are off (value forced to 1.0 then, `llsettingsvo.cpp:858`). Writes ENV_LOCAL. | `reflectionProbeF.glsl:569-571`, `llreflectionmapmanager.cpp:1221` |

### 2.4 Haze (section label "Haze", no special highlighting; one block, in this order)

| Label | Binding | Range / step | Tooltip | Haze-0 behaviour | Code site |
|---|---|---|---|---|---|
| **Haze Density** | sky `haze_density` | 0–5 / 0.01 | "At 0, Haze Glow Strength, Haze Glow Focus and Haze Glow Brightness stop working, fog takes Air Glow Colour only, and distance fog depends on Air Density's red channel." | Master. | `atmosphericsFuncs.glsl:66-118` |
| **Haze Glow Brightness** | sky `haze_horizon` | 0–5 / 0.01 | "Brightness of white haze and its sun glow." | Inactive with "No effect while Haze Density is 0" (value kept, editable). | same |
| **Haze Glow Strength** | sky `glow.x` = `(2−v)·20` | 0–1.99 / 0.01 | "Strength of the haze glow around the sun; higher = brighter. Does not change its spread." | Inactive at 0, as above. | same |
| **Haze Glow Focus** | sky `glow.z` = `v·−5` | −2–2 / 0.01 | "Tightness of the haze glow; higher = tighter, more concentrated halo." | Inactive at 0, as above. | same |
| **Air Glow Colour** | sky `blue_horizon` | 0–3 per channel (colour + intensity) | "Sky and distant-fog colour from clear air." | Note at 0: "Sole fog colour while Haze Density is 0." | `skyV.glsl:142-155` |
| **Air Density** | sky `blue_density` | 0–3 per channel (colour + intensity) | "Clear-air amount per colour: sunset reddening, fog fade rate and the air/haze mix." | Note at 0: "Fog strength follows the red component while Haze Density is 0." | `atmosphericsFuncs.glsl:66-83`, `hazeF.glsl:89` |

Engineering prerequisite (O6): the 0/0 guards in `cloudsV.glsl`, `asHorizonCloudTintV.glsl` and `lllegacyatmospherics.cpp` are committed on `fix-haze-zero-nan-guards` (`2c8a7e9d438`) and must be merged before this group can set Haze Density to 0.

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

An Environment Effects… toggle button (`as_environment_effects`, R15), the wrapper holding the Sun, Moon, Stars, Horizon, Auroras and Weather tabs. Sky Detail (`WLSkyDetail`): 4–512 / 1; number 4–512 / 4 (Phototools pair).

---

## 3. P2 Tone

| Label | Binding | Range / step | Tooltip | Behaviour | Code site |
|---|---|---|---|---|---|
| **Sky HDR Scale** | sky `gamma` | 0–20 / 0.01 | "Brightness of sky and haze relative to the scene; also sets how far auto-exposure can adjust. At 0.25 or below, auto-exposure is fixed at 1." | Writes ENV_LOCAL. On a legacy (auto-adjusted) sky the slider edits `RenderSkyAutoAdjustHDRScale` instead (that sky's brightness source); on an EEP sky it edits the sky's gamma. Same range. | `llsettingsvo.cpp:855`, `pipeline.cpp:8084-8093` |
| Exposure | `RenderExposure` | 0.5–4 / 0.1 | "Overall scene exposure." | | `pipeline.cpp:8161-8163` |
| Dynamic exposure | `RenderDynamicExposureEnabled` | check box | "Adapts exposure to scene brightness." | Shows "pinned" note when Sky HDR Scale ≤ 0.25. | `pipeline.cpp:8055` |
| Bright-scene level; Settle accuracy; Settle time (s); Object weight | `RenderDynamicExposureCoefficient`; `…SpeedError`; `…SpeedTarget`; `RenderDiffuseLuminanceScale` | 0.05–2 / 0.01; 0.01–0.5 / 0.01; 0.1–10 / 0.1; 0–2 / 0.05 | meanings in §8.2 | Expert sub-block. | `pipeline.cpp:7965, 8056-8058`; `exposureF.glsl`, `luminanceF.glsl` |
| Use fixed range | `RenderUseExposureSkySettings` | check box | "Uses a fixed 0.5–3 range instead of Sky HDR Scale." (HDR min/max/offset are hard-coded) | Expert. | `pipeline.cpp:8067-8081` |
| Tone mapping | `RenderTonemapType` | Khronos Neutral / ACES | | | `tonemapUtilF.glsl:131` |
| Tone mapping mix | `RenderTonemapMix` | 0–1 / 0.05; number 0–1 / 0.001 | | | `llsettingsvo.cpp:835` |
| Glow | `RenderGlow` | check box | "Material glow (not haze glow)." | `RenderGlowHDR` is not in the panels and the code does not force it: the user sets it once in Debug Settings and it persists (`Persist` 1; 16-bit float glow buffers, `pipeline.cpp:1565`; decided 2026-10-25). | `pipeline.cpp:1331-1341` |
| Glow resolution, iterations, strength, width | `RenderGlowResolutionPow`, `…Iterations`, `…Strength`, `…Width` | 8–10 / 1; 0–200 / 1 (number 0–500); 0–0.5 / 0.001 (number 0–10); 0–50 / 0.001 (number 0–5000) | | | same |
| Glow internals | `RenderGlowLumWeights`, `…MinLuminance`, `…MaxExtractAlpha`, `…WarmthAmount`, `…WarmthWeights`, `…Noise` | Open O5 (no existing UI) | | Expert. | same |

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
| Hair occlusion | Hair Occlusion… toggle button (`ks_hair_occlusion`, R15); its 13 controls stay in that floater; `KSHairOcclusionHeadOffset` expert (Open O5) |

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

Volumetric: the 10 controls of `panel_as_volumetric_lighting.xml`, built in, with their ranges (albedo 0–1 / 0.01, density 0–0.25 / 0.001, sun and moon gradient 0–0.9 / 0.01, quality combo, debug 0–11 / 1, local lights check box, local intensity 0–6 / 0.01, local max count 0–64 / 1), plus `RenderVolumetricLightingSampleCountOverride` (expert, Open O5). Shader lighting level: `RenderShaderLightingMaxLevel` (existing combo).

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
| Curve (new) | Combo: Stock / Shaped / Smooth plateau / Physical; default Shaped (§8.3). |
| Softness 0–2 / 0.05 | Meaning follows the curve; default Softness 1. Shaped: Softness up → exponent down (softer edge). Applies on opaque, PBR alpha and legacy-material alpha surfaces (§8.3). |
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
| Light beacons, light radius, light bounding boxes | `ASRenderLightBeacon`, `RenderLightRadius`, info display "lights" | check boxes |

---

## 5A. Camera (tab, decided 2026-10-24)

Camera-specific controls, moved out of Tone. Ranges follow R8 (Phototools slider ranges; its ±128 number boxes are not copied).

### 5A.1 Depth of field

| Label | Binding | Range / step | Notes |
|---|---|---|---|
| Depth of field | `RenderDepthOfField` | check box | Header status: "on, f/9.0, 50 mm" / "off". |
| DoF mode | `ASDepthOfFieldMode` | Standard / Firestorm; Advanced / AyaneStorm; Aperture-sampled (stills); Live / AyaneStorm | |
| F-number | `CameraFNumber` | 0.01–22 / 0.01 | |
| Focal length (mm) | `CameraFocalLength` | 1–300 / 1 | |
| Field of view (deg) | `CameraFieldOfView` | 0.1–180 / 0.1 | |
| Zoom | `CameraAngle` | 0.08–3.06 / 0.002 | |
| Focus follows pointer | `FSFocusPointFollowsPointer` | check box | |
| Focus locked (Alt-Shift-X) | `FSFocusPointLocked` | check box | |
| Show focus point | `FSFocusPointRender` | check box | |
| Focus transition (s) | `CameraFocusTransitionTime` | 0–5 / 0.01 | |
| Max blur (CoC) | `CameraMaxCoF` | 0–150 / 0.01 | |
| DoF resolution | `CameraDoFResScale` | 0–1 / 0.01 | |
| DoF while editing | `RenderDepthOfFieldInEditMode` | check box | |
| Depth of Field… | toggle button `as_depth_of_field` (R15) | | Full AyaneStorm DoF floater: aperture, autofocus, highlights. |

### 5A.2 Lens

| Label | Binding | Range / step | Tooltip | Behaviour | Code site |
|---|---|---|---|---|---|
| Camera Effects… | toggle button `as_camera_effects` (AyaneStorm menu item: diffuse glow, lens flare, vignette, chromatic aberration) | | | R15. Color Grading stays reachable through Environment Effects (Quick, Sky). | `as*.cpp` |
| FS vignette | `FSRenderVignette` x/y/z | 0–1 / 0.05; 0–40 / 0.1 (number 0–32); 0–8 / 0.1 | | | `quickprefs.cpp` |
| Sharpening | `RenderCASSharpness` | 0–1 / 0.001; number 0–1 / 0.1 | | | `pipeline.cpp:9307` |

---

## 6. P5 More (status, presets, floaters)

### 6.1 Floater buttons (R15)

No separate list (decided 2026-10-24). Each toggle button sits in its tab:

| Floater | Button placement |
|---|---|
| Environment Effects (`as_environment_effects`; wrapper with tabs for Volumetric, Camera Effects, Color Grading, Sun, Horizon, Moon, Stars, Auroras, Weather) | Quick; also Sky › Sky visuals |
| My Environments (`my_environments`) | Quick; also Sky › Source |
| My Lights (`as_my_lights`) | Lights › My Lights |
| Hair Occlusion (`ks_hair_occlusion`) | Shadow › Occlusion |
| Depth of Field (`as_depth_of_field`) | Camera › Depth of field |
| Camera Effects (`as_camera_effects`) | Camera › Lens |

Not reachable from this panel (decided 2026-10-24):
- Covered by the KStorm tabs: Volumetric Lighting (`as_volumetric_lighting`), KStorm Env. Effects (`ks_environment_effects`), Personal Lighting (`env_adjust_snapshot`; its sun/moon beacon check boxes are dropped), Advanced Lighting (`env_adjust_snapshot_advanced`; its DoF controls are in Camera and our DoF floater, O14).
- Reached through Environment Effects: Moon, Sun, Color Grading, Aurora, Star, Horizon and Weather settings.
- Out of scope: Phototools (`phototools`), Advanced Phototools (`phototools_advanced`), Graphics preferences (`preferences`), Sky, Water and Day Cycle editors, Build (`build`), Material editor (`live_material_editor`). Their lighting is in the tabs, DoF in Camera, name tags and avatar toggle in More › Display; the rest is performance and camera control.

HDRI preview is an action, not a floater, and stays a plain button.

### 6.2 Display

| Label | Binding | Range | Tooltip |
|---|---|---|---|
| Show avatars | render type `character` via `Advanced.ToggleRenderType` / `Advanced.CheckRenderType` (as Advanced Phototools "Toggle Avatars") | check box | "Shows or hides all avatars." |
| Show avatar names | `AvatarNameTagMode` | check box (as Advanced Phototools) | "Name tags over avatars." |

Not included (decided 2026-10-24): "Enable Reflection Probe Selection" (`SelectReflectionProbes`, Phototools › Env).

### 6.3 Status, presets and expert

| Item | Content |
|---|---|
| Sky origin | Legacy (auto-adjusted, mode B) / EEP (mode C) |
| Effective Probe Fill Strength | value in use (shows 1.0 forced when probes are off) |
| HDR / post | "HDR on"; "Post-processing off" during snapshot no-post or Build with `RenderDisablePostProcessing` |
| Probe level | current coverage |
| Auto-exposure | "Pinned at 1" when Sky HDR Scale ≤ 0.25 and dynamic exposure is on |
| Local override active | indicator + Reset (R9) |
| Presets & quality | Quality slider (`RenderQualityPerformance`, existing Preferences slider) and graphics presets (save/load). One-line warning shown with them: "Changing these overwrites the lighting settings in this floater." Forced settings are reapplied afterwards (R12). |
| Expert | HDRI preview + `RenderHDRIExposure`, `…Rotation`, `…SplitScreen`, `…IrradianceOnly` (Open O5); `RenderSpecularResX/Y`, `RenderSpecularExponent`; `RenderDeferredDisplayGamma` |

---

## 7. Open items

| # | Item | Status |
|---|---|---|
| O1 | Ambient Brightness definition | **Resolved:** Rec.709 luminance of the stored colour, 0–3 / 0.01, grey fallback when black (§8.1). |
| O2 | Legacy-sky conversion | **Resolved:** LL's default auto-adjust (R2). |
| O3 | My Lights falloff | **Resolved as recommended:** curve combo Stock / Shaped / Smooth plateau / Physical, default Shaped at Softness 1; positive out-of-range falloff value decoded in `calcLegacyDistanceAttenuation`; legacy-material alpha included via `alphaF.glsl` (§8.3). |
| O4 | Dynamic exposure | **Resolved:** labels and ranges as proposed in §8.2 (now in §3). |
| O5 | Expert settings with no existing UI | **Deferred**, unchanged: glow internals, shadow internals, SSR tuning, probe internals, GTAO backend, hero distance, HDRI, eye mask radius, head offset, volumetric sample override. |
| O6 | NaN guard for Haze Density 0 | **Done:** committed `2c8a7e9d438` on `fix-haze-zero-nan-guards`; not merged, not built (the shader validator named in AGENTS.md is not in the repo). |
| O7 | "As-is" ranges | **Resolved:** Sunlight Scale 0–20 / 0.01 (KS); Max nearby lights 0–512. |
| O8 | Transition time | **Resolved:** kept at 0 (instant), not exposed (R11). Effect: seconds to blend when a sky, water or day cycle is picked manually (inventory apply, World > Environment presets, preset combos); not used for region/parcel changes or day-cycle progression (`llenvironment.cpp:1391-1395`). |
| O9 | Trackball | **Resolved:** removed from the new UI. |
| O10 | Glow Size label | **Resolved:** Haze Glow Strength. |
| O11 | Sky HDR Scale on legacy skies | **Resolved:** on a legacy (auto-adjusted) sky the slider edits `RenderSkyAutoAdjustHDRScale` (§3). |
| O12 | Keeping forced settings on | **Resolved:** reapply at startup and after preset loads and quality changes (R12). |
| O13 | Local lights ×0.9 under legacy skies | **Resolved:** keep LL's behaviour (R13). |
| O14 | Depth of field controls | **Resolved:** our Depth of Field floater (`as_depth_of_field`) gains the 11 DoF and camera controls it lacks, from Advanced Lighting, Phototools, Advanced Phototools and Graphics prefs: `CameraFNumber`, `CameraFocalLength`, `CameraFieldOfView`, `CameraFocusTransitionTime`, `CameraMaxCoF`, `CameraDoFResScale`, `FSFocusPointFollowsPointer`, `FSFocusPointLocked`, `FSFocusPointRender`, `RenderDepthOfFieldInEditMode`, `CameraAngle` (zoom). It already has `RenderDepthOfField` and `ASDepthOfFieldMode`. Implementation is a change to `floater_as_depth_of_field.xml`. The Camera tab (§5A.1) also carries them built in (R15 allows built-in controls). |

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

**Specific cases (checked against `pipeline.cpp:8064-8099`):**

| Case | Auto-exposure? | Why |
|---|---|---|
| Sky HDR Scale 0.1, Only My Lights on | **No.** Exposure stays at exactly `RenderExposure`. | `s = sqrt(0.1) × 2 ≈ 0.63`, not above 1, so the range stays 1–1. Which lights are drawn plays no part in the range; it only changes what is metered, and metering cannot move a 1–1 range. |
| Set KS environment on | **No**, for the same reason. | It writes Sky HDR Scale 0.1 into the local sky (`ksenvironment.cpp`). |
| Either case with `RenderUseExposureSkySettings` on | **Yes**, range 0.5–3. | That branch uses the sky's hard-coded HDR min/max/offset (0.5 / 2.0 / 1.0, `llsettingssky.cpp:1185`) and ignores gamma. |

So for portrait work at Sky HDR Scale 0.1, exposure is fully manual unless Use fixed range is turned on.

### 8.3 My Lights falloff (O3)

**What "negative falloff" meant:** not a user value. It was a proposed internal signal: My Lights would send its falloff to the shader as a negative number, so the shared light shader could tell My Lights apart from prim lights (whose falloff is clamped to 0–2) and apply a different curve. Nobody would see or set a negative value, and it is fragile on the alpha path (map §12.4).

**Alpha surfaces can get the curve (checked 2026-10-24), which changes the recommended mechanism.**

- Forward (alpha) lights share one set of per-slot uniforms for all light types (`llrender.cpp:966-991`): `light_position`, `light_direction`, `light_attenuation` (linear, quadratic, point flag, ambiance), `light_diffuse`, and `light_deferred_attenuation` (size, falloff). A uniform set only by My Lights cannot tell slots apart, so a **per-light value is required** on the alpha path.
- **PBR alpha** (`pbralphaF.glsl:289`) reads falloff from `light_deferred_attenuation.y` and runs it through `calcLegacyDistanceAttenuation`, the same function the deferred point, multi-point, spot and dark-spot shaders use. My Lights fills that slot itself (`appendForwardLights`, `state->setFalloff(...)`, `aslightrigrenderer.cpp:278`); prim lights get it from `setupHWLights` (`pipeline.cpp:6768-6773`).
- **Legacy-material alpha** (`alphaF.glsl:182-229`) uses the linear/quadratic attenuation instead and does not read `light_deferred_attenuation`; it would need that uniform added to get the curve.
- `light_position.w` is not free: hair occlusion tests `lp.w > 0` (`ksHairOcclusionF.glsl:135,157`), and the position is transformed as a point.

Recommended mechanism: **an out-of-range positive falloff value written only for My Lights lights**, decoded once in `calcLegacyDistanceAttenuation`. Prim-light falloff reaching the shaders is at most 1.0 (0–2 × 0.5), so values from 10 upward are unambiguous (for example `10 + curve × 10 + Softness`). Because it is positive and lives only in the falloff slot, the linear/quadratic attenuation that `setupHWLights` and `alphaF.glsl` compute from the raw falloff stays stock, so the earlier negative-value hazards do not apply. Coverage:

| Path | Gets the curve by |
|---|---|
| Deferred point / multi-point / spot / dark spot | the encoded value in their falloff uniform (`Shader` backend: `aslightrigrenderer.cpp:191,205`; `LLVOVolume` backend: `renderDeferredLighting`, which already identifies rig lights through `getLightBoost`) |
| PBR alpha | the encoded value in `light_deferred_attenuation.y` (`appendForwardLights`; `setupHWLights` for the `LLVOVolume` backend) |
| Legacy-material alpha | adding `light_deferred_attenuation` to `alphaF.glsl` and the same decode |

Side finding: with the `Shader` backend, `appendForwardLights` sends every My Lights light to alpha surfaces as a plain point light (point flag 1, cutoff 180°, `aslightrigrenderer.cpp:279-281`), spots included. Alpha surfaces never see a My Lights spot cone.

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

---

## 9. UI approaches

The design so far uses four structural approaches: the KStorm Lighting floater with tabs, accordion sections inside tabs, toggle buttons that open other floaters, and controls bound to properties (settings or sky fields). Read-only property displays (status footer, section status texts) and debug views are also in use.

Candidate approaches not yet adopted (listed 2026-10-24):

| # | Approach | Notes |
|---|---|---|
| A1 | Lighting presets ("looks") | Save and load a named set of KStorm lighting values (local sky fields plus viewer settings). EEP assets save only the sky; graphics presets mix lighting with performance. Natural home: Presets & quality. |
| A2 | A/B compare | Hold two lighting states and flip between them with one button or key; for judging a change in a shot. |
| A3 | Reset to default | Per control or per section (AyaneStorm panels already have per-control reset buttons); compact form: right-click Reset or a reset icon on the section header. |
| A4 | Undo | Short in-session history of recent changes. |
| A5 | Search filter | Type to show only matching controls; the accordion widget has a no-matches message parameter (`no_matched_tabs_text`), Preferences has a search box. |
| A6 | User-chosen Quick list | Users add controls to Quick themselves. Quick Prefs does this for settings only; sky fields would need new code. |
| A7 | Toolbar button and KStorm menu item | For the lighting floater itself (`commands.xml`, `menu_viewer.xml`). |
| A8 | Keyboard shortcuts | Toggle the floater, flip A/B, cycle presets, show/hide avatars. |
| A9 | Chat commands | The tree has a command line (`chatbar_as_cmdline.cpp`), e.g. `/ks exposure 1.2`. |
| A10 | Status-bar indicator | Small icon for local override, pinned auto-exposure or post-processing off, visible without the floater. |
| A11 | Notifications | One-off toast when the mode changes behind the user's back (legacy sky turned EEP, preset re-enabled HDR). |
| A12 | RLV and scripts | `@setenv` lets scripts drive the environment; show "locked by RLV" and grey out affected controls (the viewer already disables environment floaters under RLV). |

Highest value for portrait and photo work: A1–A2, A3 (per section) and A5.

Layout variant under evaluation (2026-10-24): accordions replaced by buttons that open each section as its own floater (mock-up `v3-floaters.html`, kept local and outside the repository).

