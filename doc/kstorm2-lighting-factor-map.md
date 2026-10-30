# KStorm2 lighting factor map (HDR mode C only)

Author: Kissmy Spicoli. Date: 2026-10-24. Branch `kstorm` at `c5194292095` (after the ks-rename, slider-fix and docs merges; KStorm files, classes and settings use `ks*`/`KS*`, ownership tags `<KS:KS>`).

Derived from the original lighting map (`doc/kstorm-lighting-factor-map.md`, retired 2026-10-24; in git history at `bc9e1ec3fc3` and earlier; the parts still needed are Appendices B–D), reduced to the lighting KStorm will use: EEP/PBR with HDR. EEP skies run in mode C; legacy skies use LL's default auto-adjust (mode B, the same HDR shading with LL's conversion scales), decided 2026-10-24. Content that applied only to classic, auto-adjusted legacy or hybrid skies has been removed; for that material see Appendices B–D.

This document changes no code. The one related code change, the Advanced Lighting atmosphere sliders (3.2), was made separately on `feat-al-atmosphere-sliders` and merged. Paths are relative to `indra/newview/` unless they start with `indra/`.

Purpose: list every factor that changes scene lighting, group it (micro = code site, macro = environment vs local, sub-grouped by lighting model), cross-reference it with the panels that expose it, and list the factors with no UI. The last sections group the factors into functional centroids, which is the basis for any later UI redesign.

Search terms: lighting map, lighting factors, HDR, mode C, probe ambiance, HDR scale, scene gamma, exposure, tonemap, sunlight scale, ambient scale, local lights, My Lights, Phototools, Personal Lighting, Advanced Lighting, KS lighting, unsurfaced settings.

---

## 1. The spine: the lighting model

Derived in `LLSettingsVOSky::applySpecial` (`llsettingsvo.cpp:831-882`), `LLPipeline::tonemap` (`pipeline.cpp:8132-8173`) and `generateExposure` (`pipeline.cpp:8060-8099`):

| Mode | Condition | Shading | Post |
|---|---|---|---|
| **B. Auto-adjusted legacy (LL default conversion)** | legacy sky (no key), `RenderSkyAutoAdjustLegacy` on | Same HDR shading as C (classic off). Ambient × `RenderSkyAutoAdjustAmbientScale`; sky/background HDR scale = `RenderSkyAutoAdjustHDRScale` instead of the sky's gamma; Blue Horizon, Blue Density and sun colour × their AutoAdjust scales; probe ambiance = `RenderSkyAutoAdjustProbeAmbiance` (`llsettingsvo.cpp:861-873`). Shipped defaults: all scales 1.0, probe ambiance 0.01. Local lights stay ×0.9 because the sky is still legacy-origin (`pointLightF.glsl:184`). | Post on. Auto-exposure range still from the sky's own gamma (`pipeline.cpp:8084-8093`). Applied per frame as uniform scaling: no cloning, no asset change, day cycles included. |
| **C. EEP / PBR HDR** | sky has key, ambiance > 0 | Ambient is **reduced to luminance**, so the ambient colour's hue is discarded (`atmosphericsFuncs.glsl:157`). Probe irradiance blends with ambient by `min(ambiance, 1)`; at the 0.01 default, fill ≈ sky ambient (8.3). Background/WL sky × `sky_hdr_scale = sqrt(gamma)*2` (`llsettingsvo.cpp:855`). | Post on. Dynamic-exposure range = [1/hdr_scale, hdr_scale] when hdr_scale > 1 (`pipeline.cpp:8084-8093`). At Sky HDR Scale ≤ 0.25 (hdr_scale ≤ 1) the range collapses to 1–1, so **auto-exposure is effectively off** even with dynamic exposure enabled. Set KS environment uses gamma 0.1. |

"Sky has key" means the sky asset contains `reflection_probe_ambiance` (`canAutoAdjust()` is FALSE, `indra/llinventory/llsettingssky.cpp:1175`). Mode C also needs HDR on (`RenderHDREnabled`, set through `RenderDisableVintageMode`, `llviewercontrol.cpp:355`). Section 8 records how KStorm guarantees HDR shading for every sky: HDR forced on, legacy skies auto-adjusted (mode B), probe ambiance never below 0.01.

Exceptions inside mode C that still skip post-processing: snapshot no-post (`gSnapshotNoPost`), and `RenderDisablePostProcessing` while the Build floater is open (`pipeline.cpp:8132`).

**Consequence for the UI:** Exposure, Tonemap type, Tonemap mix and dynamic exposure always work. Ambient colour hue does nothing on surfaces (it still tints sky, clouds and haze, section 8.2). Scene Gamma is the HDR scale: sky/background brightness ×`sqrt(gamma)*2` plus the auto-exposure range.

---

## 2. Frame order (where each factor enters)

1. CPU sky update: `LLSettingsVOSky::updateSettings/applyToUniforms/applySpecial`. Sky uniforms, `ASCelestialTwilight` fade of sun/moon colour.
2. Shadow maps: `generateSunShadow` (4 cascades), spot shadows (2 slots), `pipeline.cpp:11460`.
3. G-buffer: material response written (legacy Blinn-Phong in sRGB, PBR in linear, emissive and glow).
4. Light map: sun shadow + SSAO or GTAO (`asambientocclusion.cpp`). Hair occlusion is injected for self alpha hair.
5. Soften/sun pass: `class3/deferred/softenLightF.glsl`. Sun, ambient, probe IBL, atmospheric haze.
6. Local lights: `renderDeferredLighting` (`pipeline.cpp:10065-10400`). Point, spot and fullscreen batches, then dark spots (`ksdarklight.cpp`, class `KSDarkLight`, shader `ksDarkSpotLightF.glsl`).
7. Forward/alpha: `setupHWLights` (`pipeline.cpp:6570`). Sun plus at most 6 local lights in slots 2-7.
8. Volumetric (god rays) `asvolumetriclighting.cpp`, horizon scattering, sky effects.
9. Post: SSR copy, luminance → exposure (`pipeline.cpp:7953-8111`), HDR bloom (`asdiffuseglow`), tonemap (exposure, type, mix), colour grading, CAS, gamma, glow composite.

---

## 3. ENVIRONMENT factors

Key to columns. **Behaviour** describes the factor in mode C. **UI**: where the factor is surfaced. Abbreviations: PL = Personal Lighting (`floater_adjust_environment.xml`), AL = Advanced Lighting (`floater_advanced_lighting.xml`), ED = Fixed sky editor (`panel_settings_sky_*`, or the `panel_fs_settings_sky_*` compact variant), DC = Day Cycle editor, PT = Phototools (`floater_phototools.xml`), APT = Advanced Phototools, PG = Preferences > Graphics (`panel_preferences_graphics1.xml`), PGA = Advanced graphics prefs floater, KS = KStorm Environment Effects (`panel_ks_lighting.xml`), AEE = AyaneStorm Environment Effects tabs, PA = Prefs > AyaneStorm.

### 3.1 Sky asset fields: direct light (EEP data)

| Factor (sky field) | Micro code site | Behaviour | UI |
|---|---|---|---|
| `sunlight_color` | `applySpecial` → `sunlight_color` and `moonlight_color`; `atmosphericsFuncs.glsl:61` | **Also used as the moonlight colour** (`llsettingssky.cpp:1686` `getMoonlightColor()` returns sunlight). Editor scale ÷3 (`llpaneleditsky.cpp:113`). | ED, PL, AL |
| `ambient` | `getTotalAmbient` → `ambient_color`; `atmosphericsFuncs.glsl:110-152` | Luminance only on surfaces; hue still tints sky, clouds and haze (8.2). Clouds add ambient (`+cloud_shadow*0.5`). Editor scale ÷3. | ED, PL, AL |
| `sun_rotation` / `moon_rotation` | light direction (`lightnorm`), shadows, twilight | Light source switches from sun to moon when the sun fades below its disc edge (`ascelestialtwilight.cpp`). | ED, PL, AL, AEE Sun/Moon az/el, KS (fixed values) |
| `sun_scale` / `moon_scale` | `ASCelestialTwilight::influence` (disc-edge elevation) | **Hidden coupling**: disc size changes when sun and moon light fade out at the horizon. | ED, PL |
| `moon_brightness` | `moonF.glsl:270`; `ascelestialtwilight.cpp:80` (glow factor ×0.25); lens flare | Moon disc and night haze glow only. **Does not scale moonlight on surfaces.** | ED (sun/moon tab) only. Not in PL/AL. |
| `gamma` (widget `scene_gamma`, on screen "Brightness:"; KS "HDR Scale") | `SKY_HDR_SCALE` (`llsettingsvo.cpp:855`); exposure range (`pipeline.cpp:8084-8093`) | HDR scale: sky/background ×`sqrt(gamma)*2`, auto-exposure range [1/s, s]; at gamma ≤ 0.25 auto-exposure is pinned at 1. | ED, PL, AL ("scene_gamma"), KS ("HDR Scale", writes the ENV_LOCAL sky) |
| `reflection_probe_ambiance` | `REFLECTION_PROBE_AMBIANCE`; `llreflectionmapmanager.cpp:1221` (`max(sky, probe)`) | Must stay above 0 for mode C (minimum 0.01, section 8). Minimum ambiance for every probe; weight of probe irradiance vs ambient (8.3). Forced to 1.0 when probes are off (`llsettingsvo.cpp:858`). | ED, PL, AL. KS sets 0.01 via Set KS environment. |
| `cloud_shadow` (coverage) | `atmosphericsFuncs.glsl:113,119` | Removes sun, adds ambient. | ED clouds, PL |
| `glow` (size→x, focus→z) | `atmosphericsFuncs.glsl` haze glow | Sun-side haze brightness. Editor maps size `(2-v)*20` and focus `v*-5`. | ED, PL, AL |
| `star_brightness` | `lldrawpoolwlsky.cpp:250`, `asmilkyway.cpp:210` | Visual only. | ED, PL |

### 3.2 Sky asset fields: atmosphere and haze (WindLight haze model, used by EEP)

| Factor | Micro code site | Behaviour | UI |
|---|---|---|---|
| `blue_horizon`, `blue_density` | `calcAtmosphericVars`; desaturated in irradiance capture (`llsettingsvo.cpp:699`, `RenderDesaturateIrradiance`) | Sky colour, sun attenuation, haze additive. Editor ÷2. | ED, PL, AL |
| `haze_horizon`, `haze_density` | same | Haze, sun attenuation (`haze_density*0.25`). | ED, PL, AL |
| `density_multiplier` | same | Haze thickness and sun extinction. Stored = display × 0.001. | ED. AL: hidden (commented out of `floater_advanced_lighting.xml`) until `feat-al-atmosphere-sliders`, which shows and wires it (merged into `kstorm` at `6719de8c790`, not yet built). |
| `distance_multiplier` | same | Haze distance. | ED. AL: hidden (commented out of `floater_advanced_lighting.xml`) until `feat-al-atmosphere-sliders`, which shows and wires it (merged into `kstorm` at `6719de8c790`, not yet built). |
| `max_y` (Max Altitude) | same; `calculateLightSettings` | Sun extinction altitude. | ED. AL: hidden (commented out of `floater_advanced_lighting.xml`) until `feat-al-atmosphere-sliders`, which shows and wires it (merged into `kstorm` at `6719de8c790`, not yet built). |
| `cloud_*` (colour, scale, variance, density/detail, scroll, map) | `cloudsV/F.glsl`; `cloud_shadow` above | Mostly visual. Coverage feeds lighting. | ED clouds, PL (colour, coverage, scale, map) |
| `moisture_level`, `droplet_radius`, `ice_level`, rainbow/halo IDs | `skyF.glsl` | Rainbow and halo only. | ED atmos tab. Not in PL/AL. |
| dome offset/radius | `llvowlsky.cpp`, `ashorizonscattering.cpp:69` | Geometry constants. | none (asset-only) |

### 3.3 Viewer global lighting scales (settings; affect every sky)

| Setting | Default (xml) | Micro site | Notes | UI |
|---|---|---|---|---|
| `RenderHDRSkySunlightScale` | 1.0 | `llsettingsvo.cpp:838` → `sky_sunlight_scale`; `atmosphericsFuncs.glsl:163` | sun scale in HDR | AL (0–50), KS (0–20) |
| `RenderSkyAmbientScale` | 1.0 | `atmosphericsFuncs.glsl:164` | | AL (**−10**–40), KS (0–20) |
| `RenderExposure` | 1.0 | `pipeline.cpp:8161-8163`, **clamped 0.5–4** | | PG, PGA, AL (0.2–4), KS (0.5–4), PT (**0–10**) |
| `RenderTonemapType` (0 = Khronos Neutral, 1 = ACES Hill) | 1 | `tonemapUtilF.glsl:131` | | PG, PGA, PT, APT |
| `RenderTonemapMix` | 0.7 | `psky->setTonemapMix` (`llsettingsvo.cpp:835`) | | PG, PGA, PT, APT |
| `RenderDynamicExposureEnabled` / `SpeedError` / `SpeedTarget` | runtime-declared only (not in settings.xml; `LLCachedControl` declares them, "Declared In Code") | `pipeline.cpp:8055-8058` | | **none** (Debug Settings after first use) |
| `RenderDynamicExposureCoefficient` | 0.5, Persist 0 | `pipeline.cpp:8056` | | none |
| `RenderDiffuseLuminanceScale` | 0.5, Persist 0 | luminance pass `pipeline.cpp:7965` | exposure metering | none |
| `RenderUseExposureSkySettings` | 0 | uses the sky HDR min/max/offset, which are **hard-coded** 0.5/2.0/1.0 (`llsettingssky.cpp:1185`) and not in the asset | | none |
| `RenderHDREnabled` | 1 | buffers, probes, sunlight-scale branch | forced on (section 8) | indirect (Vintage mode) |

### 3.4 Indirect light: reflection probes, SSR, mirrors

| Factor | Micro site | UI |
|---|---|---|
| `RenderReflectionProbeLevel` (none / manual / manual+terrain / full) | `llreflectionmapmanager.cpp`; also gates local lights in probe capture (`pipeline.cpp:10071`) | PG, PGA, PT, APT |
| `RenderReflectionProbeDetail` (static/dynamic/realtime) | same | PG, PGA, PT, APT |
| `RenderReflectionProbeCount`, `RenderReflectionProbeResolution`, `RenderReflectionProbeDrawDistance` | same; `llviewerdisplay.cpp` | PG/PGA (count); PT (res, draw distance) |
| `RenderDefaultProbeUpdatePeriod` (2 s) | `llreflectionmapmanager.cpp:488` | none |
| `RenderReflectionProbeMaxLocalLightAmbiance` (4, Persist 0) | dims local lights in probe capture when probe ambiance exceeds it (`llreflectionmapmanager.cpp:802`) | none |
| `RenderDesaturateIrradiance` | irradiance pass greys blue sky colours (`llsettingsvo.cpp:695`) | none |
| `RenderScreenSpaceReflections` + 6 tuning settings | `pipeline.cpp:1380-1386` | toggle: PG, PGA, PT, APT, AL. Tuning: none |
| `RenderMirrors`, hero probe resolution / update rate | `llheroprobemanager.cpp` | PG, PGA, PT, APT. `RenderHeroProbeDistance`: none |
| HDRI sky (`RenderHDRIExposure`, `Rotation`, `SplitScreen`, `IrradianceOnly`) | `lldrawpoolwlsky.cpp` | Develop menu > "HDRI Preview" loads it. The 4 tuning settings have no UI. **Not in the KStorm UI** (decided 2026-10-25): a developer tool for checking materials against an HDRI, not a lighting control; stays on the Develop menu. |

### 3.5 Shadows and ambient occlusion

| Factor | UI |
|---|---|
| `RenderShadowDetail` (none / sun+moon / +projectors) | PG, PGA, PT, APT, KS, perf prefs (2), menu, PA; enable gate in volumetric panel (10 XUI files) |
| `RenderShadowResolutionScale`, `RenderShadowBias`, `RenderShadowBlurSize` | PT, APT (res also PG) |
| `RenderShadowBlurDistFactor`, `RenderShadowFOVCutoff`, `RenderSpotShadowOffset` | PT only |
| `RenderShadowGaussian`, `RenderShadowSplitExponent`, `RenderSSAOEffect` (vectors) | PT, through code-driven sliders (`quickprefs.cpp:222-224`) |
| `RenderShadowSplits`, `ShadowOffset`, `OffsetError`, `BiasError`, `ErrorCutoff`, `Noise`, `RenderSpotShadowBias`, `RenderDeferredSpot/TreeShadow*` | **none** |
| `RenderDeferredSSAO`, `RenderAOTechnique` (legacy/GTAO), `RenderAODebugWhite` | PG, PGA, PT, APT, AL, KS, PA, menu |
| SSAO: `Scale`, `MaxScale`, `Factor`; `IrradianceScale/Max` (irradiance side, `softenLightF adjustIrradiance`) | PT/APT; irradiance pair PT only |
| GTAO: `Quality`, `Radius`, `FinalValuePower`, `DenoisePasses`, `BentNormals`, `BentNormalInfluence` | PT/APT (bent normals PT only). `RenderGTAOBackend`: none |
| Cloud shadow (sky field) | ED, PL |
| `RenderShaderLightingMaxLevel` (shader class limit) | PG, PT |

### 3.6 Water

Water fog colour, density, underwater modifier, fresnel scale/offset, normal map/scale, wave directions, refraction above/below, blur: `panel_settings_water.xml` (ED water), PL (normal map only). Lighting coupling: `waterFogKS = 1/max(light.z, 0.3)` (`llsettingsvo.cpp:1152`). AS water horizon fog: AEE Horizon. `RenderTransparentWater`: PG/PGA/PT.

### 3.7 Sky-dome effects (AyaneStorm, viewer-local, not saved in EEP)

| Module | Effect on scene light? | UI |
|---|---|---|
| Procedural sun (`asproceduralsun.cpp`) | visual disc/halo only | AEE Sun (+ its own az/el sliders) |
| Moon rendering (`asmoonrendering.cpp`) | visual. Horizon tint and phase also feed volumetric. | AEE Moon |
| Stars / Milky Way | visual | AEE Stars |
| Horizon scattering + water horizon fog | visual haze overlay | AEE Horizon |
| Aurora | visual | AEE Auroras |
| Volumetric lighting (`asvolumetriclighting.cpp`) | adds in-scatter (sun/moon and optionally local lights) | AEE Volumetric, PA (toggle). `SampleCountOverride`: none |
| Celestial twilight (`ascelestialtwilight.cpp`) | **yes**: fades sun/moon light below the disc edge, picks the source, glow factor | `ASRenderPartialMoonBelowHorizon` in PA. Fade angles (−6°/−8°) hard-coded. |

### 3.8 Environment source selection (who owns the sky)

Region, parcel, estate, day cycle, local (ENV_LOCAL), RLV `@setenv`, KS Set KS environment (version −7), AS celestial panels (−5), Personal Lighting (−2), Quick Prefs presets (NO_VERSION −3). UI: World > Environment menu (Use Shared, My Environments, Personal Lighting, Advanced Lighting), Phototools WL/Water/DC preset combos (`quickprefs.cpp`), region panel. Any local edit creates or freezes an ENV_LOCAL sky. Legacy WindLight XML presets are imported as EEP objects without a probe-ambiance key (`buildFromLegacyPreset`, `llsettingsvo.cpp:974+`), so like legacy region skies they are auto-adjusted (mode B, section 8, P2).

---

## 4. LOCAL factors

### 4.1 Prim light parameters (`LLLightParams`, `LLLightImageParams`)

| Parameter | Hidden transform (micro) | UI |
|---|---|---|
| Colour, Intensity | `getLightLinearColor() = linear(color)*intensity` (`llvovolume.cpp:3473`) | Build > Features |
| Radius | **×1.5** for volume/size (`pipeline.cpp:10159`) | Build > Features |
| Falloff | **×0.5** (`DEFERRED_LIGHT_FALLOFF`, `pipeline.cpp:314`); `calcLegacyDistanceAttenuation` ×2 (`deferredUtil.glsl:95-104`) | Build > Features |
| Projector texture, FOV, Focus, Ambiance | `setupSpotLight` (`pipeline.cpp:10666`). **Near clip comes from the prim's scale and FOV**, so resizing the prim moves the projector origin. | Build > Features |
| Intensity balance constants | deferred point/spot ×3.25 (`multiPointLightF.glsl:142`, `pointLightF.glsl:133`, `spotLightF.glsl:176,183`); forward PBR ×3.0 (`deferredUtil.glsl:550`); converging on 3.25 (8.4) | none (constants) |

Model split: legacy G-buffer surfaces use Blinn-Phong with the `lightFunc` LUT (`RenderSpecularResX/Y`, `RenderSpecularExponent`, no UI). PBR surfaces use `pbrPunctual`.

### 4.2 Light budget, selection and order

| Factor | Behaviour | UI |
|---|---|---|
| `RenderLocalLightCount` (256) | deferred cap; rejected lights still use slots unless subject sort is on | PG, PGA (0–8192), PT (0–8192, spinner 0–10000), APT, AL, My Lights (0–512) |
| `RenderAttachedLights` | worn lights on/off | PG, PT, APT, AL, menu |
| `RenderFarClip` | light candidate range in deferred mode | many (draw distance) |
| Avatar too complex / muted / jellydolled | drops their lights | indirect (complexity settings) |
| `KSRenderLocalLightSubjectSort` | rank by distance from the Alt-click subject | **none**; only set by Set KS environment |
| Fade-in/out (`LIGHT_FADE_TIME`), moving bonus 0.25×radius | hard-coded | none |
| Forward/alpha path | 6 lights (slots 2-7), camera order, ignores subject sort, skips dark lights | none |

### 4.3 My Lights (`aslightrig*`, `floater_as_my_lights.xml`)

Per-light rig data (`aslightrig.h:111-141`): placement joint, distance, height, azimuth, offsets, intensity, radius, falloff (labelled "Softness"), colour, boost (1–4×, `getLightBoost`), spot flag, aim yaw/pitch, spot FOV, focus, ambiance, texture, dark. Globals: master enable, render backend (`LLVOVolume` shares the light budget and subject sort; `Shader` has its own cap and sort), only-my-lights (`KSLightRigOnlyMyLights`; filters every other local light in `calcNearbyLights`), no-eye-specular, show other avatars, beacons, presets. All surfaced in the floater. Not surfaced: `ASLightRigEyeMaskRadius`.

### 4.4 Dark spots and hair occlusion

- Dark My Lights spots: multiplicative darkening pass after the additive lights (`ksdarklight.cpp`, class `KSDarkLight`, shader `ksDarkSpotLightF.glsl`). Per-light "Dark" checkbox. Skipped on alpha.
- Hair occlusion (`kshairocclusion.cpp`, `ksHairOcclusionF.glsl`, `alphaF.glsl:417`): body-proxy shadow and darkening for self alpha hair. 14 settings; 13 are in `floater_ks_hair_occlusion.xml`, `KSHairOcclusionHeadOffset` is not surfaced.

### 4.5 Manual reflection probes (Build > Features)

Probe toggle, volume type (sphere/box), dynamic, update type, mirror, **ambiance** (combined as `max(sky ambiance, probe ambiance)` per probe), near clip. Global caps: section 3.4.

### 4.6 Surface response (materials)

| Model | Factors | UI |
|---|---|---|
| Legacy (Blinn-Phong) | diffuse, normal (bumpy), specular map/colour, glossiness, environment intensity, alpha mode/emissive mask, fullbright, glow, legacy shiny/bump presets | Build > Texture (`panel_tools_texture.xml` / `panel_fs_tools_texture.xml`) |
| PBR (GLTF) | base colour/alpha, metallic, roughness, ORM occlusion, emissive colour/map, normal, alpha mode/cutoff, double-sided | `panel_gltf_material.xml`, material editors. No emissive strength. |
| Shared | glow (`RenderGlow*`), emissive buffer (`RenderEnableEmissiveBuffer`, on with HDR) | glow: PG, PT, APT |

---

## 5. Panels → factors (surface inventory)

| Panel | Opened from | Lighting factors | Notes |
|---|---|---|---|
| Personal Lighting `env_adjust_snapshot` | World > Environment | ambient, blue H/D, sun colour, cloud colour/map/coverage/scale, haze H/D, probe ambiance, scene gamma, sun/moon rotation, az/el, sun scale, glow, stars, water normal map | Creates ENV_LOCAL sky |
| Advanced Lighting `env_adjust_snapshot_advanced` | World > Environment, PT, APT, Quick Prefs, nav bar | probe ambiance, scene gamma, HDR sunlight scale, ambient scale, haze H/D, exposure, glow, density multiplier, distance multiplier, max altitude (wired on `feat-al-atmosphere-sliders`, merged `6719de8c790`), SSAO, SSR, DoF, local light count, attached lights, sun/moon trackballs, colours | The nearest thing to a unified lighting panel today |
| Fixed sky editor / Day Cycle | My Environments, inventory | every sky field. | Asset editing |
| KStorm Environment Effects `ks_environment_effects` | KStorm menu | Set KS environment, shadows, HDR sunlight scale, ambient scale, HDR scale (= gamma), exposure, AO block, DoF button | Duplicates 4 AL sliders with different ranges |
| AyaneStorm Environment Effects | World > Environment, toolbar | volumetric, camera effects, colour grading, sun, horizon, moon, stars, auroras, weather | Viewer-local visuals |
| Phototools | toolbar / menu | Env: water, SSR, mirrors, probes. Shdw: shadows, light count, SSAO/GTAO. VFX: vintage mode, tonemap, glow, DoF. Gen: exposure (0–10), far clip, CAS | Largest surface. WL preset combos. |
| Advanced Phototools | menu | subset of PT, with a button to AL | |
| Prefs > Graphics / Advanced floater | Preferences | quality presets, SSAO, SSR, mirrors, shadows, probes, exposure, tonemap, vintage, glow, light count | |
| Prefs > AyaneStorm | Preferences | volumetric toggle, partial moon, AO technique | |
| My Lights | menu, toolbar | section 4.3 + light count | |
| Hair Occlusion | KStorm menu | section 4.4 | |
| Build > Features / Texture, material editors | Build | sections 4.1, 4.5, 4.6 | |
| Quick Prefs | toolbar | user-configurable, can hold any setting. Defaults contain no lighting controls. | |
| Debug Settings | Advanced menu | everything in settings.xml | The only surface for section 6 |

Redundancy (number of XUI files binding the control): `RenderShadowDetail` 10, `RenderDeferredSSAO` 8, `RenderLocalLightCount` 6, `RenderExposure` 5, `RenderTonemapType/Mix` 4, `RenderScreenSpaceReflections` 5. Ranges disagree between panels (exposure 0–10 / 0.2–4 / 0.5–4 against a code clamp of 0.5–4; ambient scale −10–40 / 0–20; light count 0–512 / 0–8192 / 0–10000).

---

## 6. Unsurfaced factors (no XUI and no panel code)

High impact (they change what the user sees with no visible cause):

1. Whether the current sky is legacy (auto-adjusted, mode B) or EEP (mode C) is never shown.
2. `KSRenderLocalLightSubjectSort` (only through Set KS environment).
3. Dynamic exposure: `RenderDynamicExposureEnabled/Coefficient/SpeedError/SpeedTarget`, `RenderDiffuseLuminanceScale`, `RenderUseExposureSkySettings`.
4. Hard-coded transforms: radius ×1.5, falloff ×0.5, ×3.25/×3.0, twilight fade −6°/−8°, sky HDR min/max/offset.
5. `RenderReflectionProbeMaxLocalLightAmbiance`, `RenderDefaultProbeUpdatePeriod`, `RenderDesaturateIrradiance`, `RenderHeroProbeDistance`.

Tuning (expert, low impact): shadow internals (`RenderShadowSplits`, `Offset`, `OffsetError`, `BiasError`, `ErrorCutoff`, `Noise`, `RenderSpotShadowBias`, `RenderDeferredSpot/TreeShadowBias/Offset`), SSR internals (6), `RenderGlowLumWeights/MinLuminance/MaxExtractAlpha/Warmth*/Noise`, `RenderGTAOBackend`, `RenderVolumetricLightingSampleCountOverride`, `RenderHDRI*` (4; not in the KStorm UI, decided 2026-10-25), `RenderSpecularExponent/ResX/ResY`, `RenderEdgeDepth/NormCutoff`, `KSHairOcclusionHeadOffset`, `ASLightRigEyeMaskRadius`, `RenderHDREnabled` (indirect only).

The full machine-generated list (settings read by render code with no UI binding) can be regenerated with the scripts in Appendix A.

---

## 7. Functional centroids and centroid groups

A centroid is a set of factors that a user perceives as one control goal. Today each centroid is spread over the panels listed.

| Group | Centroid | Factors | Panels today |
|---|---|---|---|
| **G1 Light character** | C1 Sky origin | legacy (auto-adjusted) vs EEP (P2), probe ambiance ≥ 0.01, HDR forced on | none (proposed P5 status readout) |
| **G2 Direct light** | C2 Key light (sun/moon) | sun colour (also moonlight), rotation, scale→twilight, `RenderHDRSkySunlightScale`, cloud coverage, haze extinction (blue density, haze density, density multiplier, max_y) | ED, PL, AL, KS, AEE |
| | C3 Shadows | shadow detail/res/bias/blur/splits, spot shadows | PG, PT, APT, KS |
| **G3 Fill light** | C4 Ambient / fill | ambient colour, `RenderSkyAmbientScale`, probe ambiance, probe irradiance, cloud ambient boost, `ambientLighting()` | ED, PL, AL, KS |
| | C5 Contact shadow (AO) | SSAO/GTAO and their irradiance pair, hair occlusion | PG, PT, APT, KS, PA, hair floater |
| | C6 Reflections | probes level/detail/count/res, SSR, mirrors, local probe ambiance | PG, PT, APT, AL, Build |
| **G4 Local light** | C7 Practical lights | prim light params, falloff/radius transforms, spot projector | Build |
| | C8 Portrait rig | My Lights, boost, dark spots, eye mask | My Lights |
| | C9 Light budget and priority | `RenderLocalLightCount`, attached lights, subject sort, only-my-lights, forward 6-light cap | PG, PT, APT, AL, My Lights, KS (hidden) |
| **G5 Atmosphere** | C10 Haze and aerial perspective | blue H/D, haze H/D, distance multiplier, glow, horizon scattering, volumetric | ED, PL, AL, AEE |
| | C11 Sky dome visuals | sun/moon discs, stars, aurora, clouds | ED, AEE |
| **G6 Camera response** | C12 Exposure and tone | Scene Gamma (HDR scale), `RenderExposure`, dynamic exposure, tonemap type/mix, colour grading | PG, PGA, PT, APT, AL, KS, AEE |
| | C13 Bloom / glow | `RenderGlow*`, AS diffuse glow, lens flare | PG, PT, APT, AEE |

Usability observations derived from the map:

1. **One centroid, many panels.** C12 is split across 7 panels with conflicting ranges. C9 across 6. C3 across 4.
2. **One panel, mixed centroids.** Advanced Lighting mixes C2, C4, C10, C12, C5, C6, C9, plus DoF. KS repeats four of its sliders with different ranges.
3. **Hidden couplings cross centroids.** Sun/moon scale (C11 visual) changes twilight light (C2). Sun colour is also moonlight. Moon brightness only touches C11. Prim scale moves the projector origin (C7). Ambient hue (C4) only reaches the sky, clouds and haze (C10, C11).
4. **Local light scale constants differ by path** (×3.25 deferred, ×3.0 forward PBR), so the same light looks different on alpha vs opaque surfaces.

---

## 8. HDR-only policy (decided 2026-10-24)

Decisions:

| # | Decision | Effect on the map |
|---|---|---|
| P1 | HDR is always on. No control. | `RenderDisableVintageMode` fixed TRUE (so `RenderHDREnabled` and `RenderEnableEmissiveBuffer` are on). Needs GL > 4.05 (`pipeline.cpp:9290`). Hardware the feature table drops to vintage (`llfeaturemanager.cpp:778`) is unsupported. |
| P2 | Legacy skies use LL's default auto-adjust (revised 2026-10-24; replaces local conversion). | `RenderSkyAutoAdjustLegacy` forced on with its shipped scale defaults (1.0) and probe ambiance 0.01; nothing chosen by KStorm. Chosen for performance: a few uniform multiplies per frame, no cloning or per-keyframe conversion, day cycles handled. EEP skies and Use Shared Environment are unaffected. Editing a legacy sky's Probe Fill Strength turns that copy into an EEP sky (mode C), as today. Graphic presets also save `RenderSkyAutoAdjustLegacy`, so loading an old preset must not turn it off. |
| P3 | Probe ambiance on conversion = **0.01** (revised from 1.0; the user's usual value). Slider minimum 0.01, step 0.01. | Ambiance never reaches 0, so every sky stays in mode C. At 0.01, surface fill is ~99% sky ambient (neutral, hue-less) and probe irradiance (bounce, indoor darkening, colour bleed) is effectively off, except inside manual probes whose own ambiance is higher. Reflections, exposure and tonemapping are unaffected. See 8.3. |
| L1 | Ambient is shown as a brightness slider, not a colour picker. | See 8.2: the slider must scale the stored colour, not replace it. |
| L2 | Probe ambiance cannot go below 0.01. | Same as P3. Also applies to KS (it already uses 0.01) and to asset editors. |

### 8.1 Controls in scope

**HDR-specific:** `RenderExposure`; `RenderTonemapType`, `RenderTonemapMix`; dynamic exposure (`RenderDynamicExposureEnabled`, `Coefficient`, `SpeedError`, `SpeedTarget`), `RenderDiffuseLuminanceScale`, `RenderUseExposureSkySettings`; `RenderHDRSkySunlightScale`; sky `gamma` relabelled **Sky HDR Scale** (new UI); probe ambiance (0.01+); probes, SSR, mirrors, hero probes; `RenderGlowHDR`.

Controls that matter only for the removed models are out of scope; the list is in Appendix C.

**Model-independent:** sun/moon position, sun colour, sun/moon scale, haze/blue/density/distance/max altitude, clouds, glow size/focus, rainbow/halo, water, shadows, AO, hair occlusion, prim lights, light budget, My Lights, dark spots, volumetric, AS sky visuals, bloom, colour grading, CAS.

**Cannot be tagged cleanly (redesign needed):** ambient (8.2), probe ambiance (8.3), legacy Blinn-Phong materials (content stays and renders through the legacy path in HDR), local light constants (8.4), atmosphere values of legacy skies tuned for HDR (retuning method: Appendix D).

### 8.2 Ambient as a brightness slider: hue must be preserved

In mode C the ambient colour is reduced to luminance only for **surface fill** (`atmosphericsFuncs.glsl:157`). Its hue is still used in:

- the sky dome: `skyV.glsl:142-155` (`blue_horizon * blue_weight * (sunlight + ambient_color)` and the haze term);
- clouds: `cloudsV.glsl:158-175`;
- haze/fog on geometry: the `additive` term in `calcAtmosphericVars` (`tmpAmbient`).

Radiance probes capture the sky dome, so reflections carry it too. Irradiance captures zero the ambient (`llsettingsvo.cpp:848`).

Therefore the brightness slider must **scale the asset's existing ambient colour** (keep its hue, change its magnitude). Replacing it with grey would recolour the sky, clouds and haze of every converted sky.

Second interaction: probe irradiance is mixed with ambient at weight `min(ambiance, 1)` (`reflectionProbeF.glsl:571`). At the P3 default of 0.01, ambient brightness controls ~99% of surface fill. At 1.0 or above, wherever automatic probes cover the scene (everywhere at full probe level), **the ambient brightness does nothing on surfaces**; it still sets sky, cloud and haze brightness. Between the two they crossfade. The slider tooltip should say this.

### 8.3 Probe ambiance

What 0.01 keeps and loses (per `reflectionProbeF.glsl:569-571`, `llreflectionmapmanager.cpp:1221`): irradiance is `tex * a` mixed with ambient by `min(a, 1)`, so at a = 0.01 surface fill = 0.99 × sky ambient + 0.0001 × probe irradiance. **Lost:** probe-based indirect light, meaning interiors no longer darker than outdoors, no colour bleed from walls or ground, no sky-coloured shadows; fill is uniform neutral grey shaped only by the normal-vs-sun term, AO and the ambient sliders. **Kept:** reflections (radiance scale is independent of ambiance), exposure, tonemapping, HDR Scale, all direct light. **Gained:** the ambient brightness slider fully controls surface fill (the 8.2 conflict disappears). Local lights are not dimmed in probe captures. Manual probes default to ambiance 0 (`REFLECTION_PROBE_DEFAULT_AMBIANCE`, `llprimitive.cpp:95`) and follow the sky's 0.01; only probes a builder set higher still add indirect light inside their volume. Probe captures still run, because reflections use them.

Range 0.01 to the editor maximum (10), step 0.01. When probes are disabled or at low quality the value is forced to 1.0 (`llsettingsvo.cpp:858`), so the slider should be disabled in that state. Values above 1 brighten probe irradiance linearly. `RenderReflectionProbeMaxLocalLightAmbiance` (4) then dims local lights during probe capture (`llreflectionmapmanager.cpp:802`).

### 8.4 Local light multipliers: why they differ, and convergence

| Path | Factor | Site |
|---|---|---|
| PBR sun | ×3.0 | `deferredUtil.glsl:615` |
| PBR local, deferred (point, spot, projector ambiance) | ×3.25 | `pointLightF.glsl:133`, `multiPointLightF.glsl:142`, `spotLightF.glsl:176,183` |
| PBR local, forward (alpha) | ×3.0 | `deferredUtil.glsl:550` |
| Legacy (Blinn-Phong) local, deferred | none (only the ×2 inside `calcLegacyDistanceAttenuation`) | `multiPointLightF.glsl:178` |

Why: the PBR diffuse term divides by π (`deferredUtil.glsl:422`, Lambert BRDF), and legacy materials do not. A factor of ~π puts PBR diffuse back on the legacy scale, so 3.0 ≈ π is a rounded Lambert compensation. PBR also loses `f0 = 0.04` of diffuse to the dielectric base (`diffuseColor = base*(1-0.04)`), and π/0.96 ≈ 3.27, which matches 3.25. So 3.25 most likely compensates for that loss as well, and was tuned on the deferred path only. History could not be traced: this checkout is a blobless partial clone and `git log -S` started downloading objects, so the search was stopped.

Effect: the same light is ~8% brighter on opaque PBR than on alpha PBR (hair next to skin). The PBR sun:local balance also differs from the legacy-material balance by the same ~8%.

Decision (2026-10-24): **local-light paths converge on 3.25**; the sun stays at 3.0.

"Forward" means surfaces that can't go through the G-buffer, mainly alpha-blended ones (hair, glass, semi-transparent clothing). They are lit while being drawn, with the sun plus at most 6 local lights uploaded by `setupHWLights`. There a spotlight is sent as a plain GL spot: direction only, 90° cutoff, `cos²` falloff (`pipeline.cpp`, `setSpotCutoff(90)`, `setSpotExponent(2)`). Projector FOV, focus, texture and the near-clip frustum are ignored; ambiance is passed separately. On opaque surfaces the same light is a textured projector frustum.

Recommendation:
1. Define one constant in `deferredUtil.glsl` and use it in all three PBR sites.
2. Converge the **local** paths on 3.25. Opaque content (most of what's on screen) stays unchanged; alpha PBR lights rise ~8% and match it.
3. Leave the sun at 3.0, or move it to 3.25 and compensate once with the default `RenderHDRSkySunlightScale`. The sun is user-scaled anyway; the local factor is not.
4. The multiplier is not the only forward/deferred difference. Forward PBR spot lights are cones with a spot² falloff and no projector texture/FOV frustum (`deferredUtil.glsl:546`). The forward legacy path uses a different attenuation (`alphaF.glsl:182-229`, the SL-14895 workaround). Forward is capped at 6 lights. Converging the constant reduces the alpha/opaque mismatch but does not remove it.

### 8.5 Blue Horizon and Blue Density

Both are RGB colours. They model the clear-air (Rayleigh-like, "blue sky") part of the atmosphere; Haze Horizon/Density model the white (Mie-like) part.

- **Blue Density = how much clear air there is, per colour channel.** It is used three ways (`atmosphericsFuncs.glsl:66-83`, `skyV.glsl:108-112`):
  1. **Sun colour extinction**: `light_atten = (blue_density + haze_density*0.25) * density_multiplier * max_y`, applied as `exp(-light_atten / sun_height)`. Channels with higher density are removed more from the sunlight as the sun gets low, which gives the sunset tint and dimming on surfaces.
  2. **Distance fog transparency**: `exp(-(blue_density + haze_density) * distance * density_multiplier * distance_multiplier)`. How fast distant objects fade.
  3. **Blue/haze mix**: `blue_weight = blue_density / (blue_density + haze_density)`. Which share of the scattered light takes Blue Horizon's colour versus Haze Horizon's.
- **Blue Horizon = the colour and brightness of light scattered by that clear air.** It doesn't affect extinction. It sets the sky dome colour and the tint of distance haze: `additive = blue_horizon*blue_weight*(sun + ambient) + haze_horizon*haze_weight*(sun*glow + ambient)`.
- Editors show both at half value (`SLIDER_SCALE_BLUE_HORIZON_DENSITY = 2`).
- In HDR the haze term is computed in sRGB, then doubled, linearised and multiplied by HDR Scale (`atmosphericsF.glsl:38-39`).
- During irradiance capture both are desaturated (`RenderDesaturateIrradiance`), so they do not tint probe fill light, only its brightness.
- Extinction on geometry uses only the red channel of the attenuation (`atmosphericsF.glsl:37`, `light *= atten.r`), so fading on objects is grey even when Blue Density is coloured.

Adopted UI labels (decided 2026-10-24; asset field names unchanged): Blue Density → **Air Density (per colour)**; Blue Horizon → **Air Glow Colour**; Haze Density → **Haze Density** (keep); Haze Horizon → **Haze Glow Brightness**; Glow Size → **Haze Glow Strength**; Glow Focus → **Haze Glow Focus** ("Haze" prefix separates these from material Glow); Density Multiplier → **Atmosphere Thickness**; Distance Multiplier → **Fog Distance Scale**. Tooltips: Air Density sets sunset reddening, fog fade rate and the air/haze mix; Air Glow Colour sets sky and distant-fog colour only.

### 8.6 When probes are captured

- **SSR** is screen-space and uses no probes. **Mirrors** use hero probes (`llheroprobemanager.cpp`), captured only while `RenderMirrors` is on. Turning both off does not stop reflection-probe captures.
- **`RenderReflectionProbeLevel` = None:** only the default probe is captured, at most once per `RenderDefaultProbeUpdatePeriod` (2 s) (`llreflectionmapmanager.cpp:488-500`). At ambiance 0.01, fill light is unchanged (~sky ambient). Reflections come from that one capture. This is the cheapest setting that keeps the 0.01 look.
- **Level above None:** the default probe at least every 2 s plus the oldest probe each frame, and with realtime detail the nearest dynamic probe every frame (`llreflectionmapmanager.cpp:425-460`).
- **`RenderReflectionsEnabled` = FALSE** (master; feature-table gated; no UI toggle, Prefs only reads it, `llfloaterpreference.cpp:2234-2244`): no captures at all, and the no-probe shader is used (`class2/deferred/reflectionProbeF.glsl`). But the sky ambiance uniform is then forced to 1.0 (`llsettingsvo.cpp:858`), and the stub computes `mix(ambient, 0.25, ambiance)`. So **fill becomes a fixed 0.25 grey** and the ambient sliders stop working. Reflections show the sky cubemap only. Passing the sky's real ambiance in that branch instead of 1.0 would keep the 0.01 look with zero captures (one-line code change; not made).

---

## 9. Completeness audit (Preferences, Graphics and every other panel)

Method: every `control_name` (and code-wired widget) in all XUI files whose setting is read by render code was checked against this document. Findings:

**Lighting controls that were missing from sections 3–5 (now accounted for):**

| Control | Panel(s) | Function | Section |
|---|---|---|---|
| `RenderQualityPerformance` (quality slider) | PG, perf prefs | **Bulk writer**: applies feature-table values to shadows, probes, SSAO, SSR, mirrors, light count, exposure, tonemap, vintage, glow res, transparent water (`featuretable.txt`) | 10 (P5) |
| Graphic presets (save/load) | PG | **Bulk writer**: restores 116 settings, including every lighting setting above (`app_settings/graphic_preset_controls.xml`) | 10 (P5) |
| `RenderGlowHDR` | PT | glow map in HDR | 4.6 / none (Debug Settings; persists) |
| `RenderAutoMaskAlphaDeferred` / `NonDeferred` | PG, menu | alpha-masked surfaces go through the deferred (lit, shadowed) path instead of forward | P4 |
| `RenderNormalMapScale` | PT | strength of normals generated from legacy bump maps (`lldrawpoolbump.cpp:838`) | P4 surface |
| `WLSkyDetail` | PG, PGA, PT | sky dome tessellation | P1 |
| `FSEnvironmentManualTransitionTime` | PT (code) | blend time when switching sky/water/day presets | P1 |
| PT "Shadow Clarity" / "Shadow Soften" / "AO Soften" / "Effect" / "GTAO Effect" | PT, APT (code-wired) | `RenderShadowSplitExponent.y`, `RenderShadowGaussian.x/.y`, `RenderSSAOEffect.x` (GTAO Effect writes the same `.x`) | P3 |
| `RenderLightRadius`, "Show bounding boxes for all lights" | PT, APT, menu | light debug visualisation | P4 |
| `RenderReflectionProbeVolumes`, `RenderReflectionProbeShowTransparent` | menu | probe debug | P3 |
| `RenderEdgeDepthCutoff`, `RenderEdgeNormCutoff`, `RenderDeferredNoise` | none | edge-aware shadow/AO blur and shadow noise | P3 |
| `RenderDeferredAtmospheric` | none | atmospheric pass on geometry | P1 |
| `RenderDeferredDisplayGamma` | none | gamma used by alpha pool | P5 |
| `RenderReflectionProbeDynamicAllocation`, `RenderHeroProbeConservativeUpdateMultiplier` | none | probe memory and update policy | P3 |

**Checked and out of scope (not lighting; they stay where they are):** LOD factors, avatar complexity/impostors, particles, anisotropic/FSAA/VSync/VRAM, texture resolution, terrain scale, DoF optics (`Camera*`, `ASDepthOfField*`), joystick/flycam, auto-tune FPS, name tags, `UseOcclusion`, wireframe/debug displays. `RenderFarClip` and auto-tune far clip are listed only because they set the light candidate range.

Result: with this section, every lighting control in Preferences > Graphics, the advanced graphics floater, Phototools, Advanced Phototools, Advanced/Personal Lighting, the env editors, KS, AEE, PA, My Lights, Hair Occlusion, Build and the menus is accounted for. Section 10 assigns each one a home.

---

## 10. Proposed consolidated panels

Five panels hold every lighting control (surfaced and currently unsurfaced). All existing panels stay. The new ones are a superset organised by centroid (section 7). Asset editors (Fixed sky, Day Cycle, Water) and per-object editing (Build, material editors) remain the authoring tools; the new panels edit the live local copy and link out to those editors. All labels below are the decided new-UI labels (12.1, 12.8); existing panels keep theirs.

**The per-control specification (label, binding, range, step, tooltip, dependency, code site) is in `doc/kstorm2-lighting-UI-design.md`.** This section is the summary it was built from.

### P1 Environment (G2 key light, G3 fill, G5 atmosphere)
- **Source:** Use Shared Environment, My Environments, sky/water/day presets (prev/next), reset to region default, Set KS environment (transition time stays 0 and is not exposed), "Edit as asset…" buttons. Shows the sky origin (legacy auto-adjusted / EEP, P2) and "Local override active" with Reset.
- **Key light:** sun azimuth/elevation, moon azimuth/elevation (no trackball), **Sun & Moon Light Colour**, Sunlight Gain (`RenderHDRSkySunlightScale`), sun scale, moon scale (tooltip: larger discs fade out later at the horizon), **Moon Disk Brightness**, partial moon below horizon, star brightness.
- **Fill:** **Sky Ambient Gain** (hue-preserving, 8.2), Surface Ambient Gain (`RenderSkyAmbientScale`, negatives allowed), **Probe Fill Strength** (0.01–10, step 0.01; 11.1 tooltip; disabled when probes are off, 8.3).
- **Haze** (one section labelled "Haze", no special highlighting; Haze Density first): Haze Density; Haze Glow Brightness (Haze Horizon); Haze Glow Strength (Glow Size); Haze Glow Focus (Glow Focus); Air Glow Colour (Blue Horizon); Air Density (Blue Density) (both 0–3 per channel, the code range). Behaviour required by 12.7: while Haze Density is 0, Haze Glow Brightness, Haze Glow Strength and Haze Glow Focus show as inactive with the note "No effect while Haze Density is 0"; the two Air controls carry the haze-0 notes.
- **Atmosphere — other:** Atmosphere Thickness (density multiplier), Fog Distance Scale (distance multiplier), **Haze Ceiling (m)** (max altitude; renamed **Atmosphere Height (m)**, decided 2026-10-25), moisture/droplet/ice, `RenderDeferredAtmospheric`.
- **Clouds:** coverage, scale, variance, colour, density X/Y/D, detail X/Y/D, scroll, map.
- **Water:** fog colour, density, underwater modifier, fresnel scale/offset, normal map/scale, waves, refraction above/below, blur, transparent water.
- **Sky visuals (tabs linking the AEE panels):** procedural sun, moon rendering, stars/Milky Way, horizon scattering and water horizon fog, auroras, weather; Sky Detail (`WLSkyDetail`).

### P2 Camera & Tone (G6)
- **Sky HDR Scale** (sky gamma; tooltip notes auto-exposure is pinned at ≤ 0.25), Exposure (0.5–4, the code clamp; tooltip "Overall scene exposure."), dynamic exposure (enable, coefficient, speed error, speed target, luminance scale, use-sky-HDR-range), tonemap type and mix.
- Bloom/glow: `RenderGlow`, `RenderGlowHDR`, resolution, iterations, strength, width, plus internals (lum weights, min luminance, max extract alpha, warmth amount/weights, noise); AS diffuse glow; lens flare.
- Lens: vignette (AS and FS), chromatic aberration, camera-effects master; colour grading (tab); CAS sharpness; Depth of Field button (DoF settings stay in their floater).

### P3 Shadows, Occlusion & Reflections (C3, C5, C6)
- **Shadows:** detail, resolution scale, bias, blur size, blur distance factor, FOV cutoff, spot offset, soften (Gaussian x/y), clarity (split exponent), internals (splits, offset, offset error, bias error, error cutoff, noise, spot bias, deferred spot/tree bias and offset, edge depth/normal cutoffs, deferred noise).
- **Occlusion:** AO toggle, technique, white-material debug, SSAO scale/max scale/factor/effect, irradiance scale/max, GTAO quality/radius/power/denoise/bent normals/influence/effect/backend (one Effect control for the active technique); Hair Occlusion (all 14 settings).
- **Reflections:** probe level (first option labelled **Sky only**), detail, count, resolution, draw distance, update period, max local light ambiance, desaturate irradiance, dynamic allocation, probe-volume debug; SSR toggle and its 6 tuning values; mirrors, hero probe resolution/update rate/distance/conservative multiplier.
- **Volumetric:** all 11 volumetric settings.
- `RenderShaderLightingMaxLevel`.

### P4 Local Lights (G4)
- **Budget:** **Max nearby lights (global)**, attached lights, subject sort (now visible), only My Lights, show other avatars, far clip note.
- **My Lights:** the rig editor (tab or embedded), backend, boost, dark, no eye highlights, eye mask radius, presets. Softness shapes the curve (12.4). My Lights tooltips: radius reaches about 1.5 × the value; spot FOV/focus/texture are not applied to alpha surfaces.
- **Selected object:** mirror of Build > Features for the selection, with Build's own labels and behaviour (local lighting outside My Lights is unchanged): light on, colour, intensity, radius, falloff, projector texture/FOV/focus/ambiance, reflection probe type/dynamic/mirror/ambiance/near clip.
- **Surface response:** `RenderAutoMaskAlphaDeferred/NonDeferred`, `RenderNormalMapScale`, link to the material editors.
- **Debug:** light beacons, show light radius, light bounding boxes.

### P5 Lighting Status & Expert
- **Status readout:** sky origin (legacy auto-adjusted / EEP), effective probe ambiance, HDR on, post active, probe level, local override active (with Reset), and "auto-exposure pinned" when Sky HDR Scale ≤ 0.25 with dynamic exposure on. This surfaces C1.
- **Presets & quality** (the bulk writers): quality slider (`RenderQualityPerformance`) and graphic presets, with a warning that they overwrite P1–P4 values.
- **Expert:** HDRI preview and its 4 settings (not in the KStorm UI, decided 2026-10-25), specular LUT (`RenderSpecularResX/Y`, `RenderSpecularExponent`), `RenderDeferredDisplayGamma`.

Coverage check against existing panels: Personal Lighting → P1. Advanced Lighting → P1 + P2 + P3 + P4 (and DoF button in P2). KS → P1 (Set KS env, scales, Sky HDR Scale) + P2 (exposure) + P3 (shadows, AO). AEE → P1 sky tabs + P2 lens/grading + P3 volumetric. Phototools/Advanced Phototools lighting parts → P1 (presets, water) + P2 (tone, glow, CAS) + P3 (shadows, AO, probes, SSR, mirrors) + P4 (lights, debug). Prefs Graphics lighting parts → P2 + P3 + P4 + P5. PA → P1 (partial moon) + P3 (AO, volumetric). My Lights, Hair Occlusion → P4, P3. Build light/probe → P4 (mirror). Debug-only lighting settings → P1–P5 as listed.

---

## 11. Misleading controls and recommended alignment

Live controls whose label, tooltip, range or placement misrepresents what they do in mode C. This section is the rationale; **where it differs from section 12, section 12 wins**. The status of every row is in 12.8. Labels were read from the en XUI files. "Label" changes are XUI/tooltip edits only. "Code" means a behaviour change.

### 11.1 Wrong name or wrong description

| Control (setting / field) | Shown as | What it actually does | Recommendation |
|---|---|---|---|
| sky `gamma` | "Brightness:" (ED atmos, PL, AL); "HDR Scale" (KS) | Sky/background and haze ×`sqrt(gamma)*2`, plus the auto-exposure range. Does not brighten lit surfaces. | Label **Sky HDR Scale** everywhere. Tooltip: "Brightness of sky and haze relative to the scene; also sets how far auto-exposure can adjust." |
| sky `moon_brightness` | "Brightness:" (ED sun/moon tab) | Moon disc and night haze glow only; not moonlight on surfaces. | Label **Moon Disc Brightness**. Note: moonlight intensity has no control (only the shared sun colour and twilight fade). |
| sky `sunlight_color` | "Color:" (ED moon/sun), "Sun Color" (PL, AL) | Also the moonlight colour. | Label **Sun & Moon Light Colour**. |
| sky `ambient` | "Ambient Color" / "Ambient" swatch | Surface fill uses luminance only; hue only tints sky, clouds and haze. | Decided (8, L1): brightness slider that scales the stored colour. Tooltip states that hue comes from the sky. |
| sky `reflection_probe_ambiance` | "Reflection Probe Ambiance (HDR)" (ED), "Reflection Probe Ambiance" (PL, AL); tooltips "At zero HDR scale becomes Brightness", "At zero, HDR is turned off…", "Above 1 is entirely HDR" | At 0.01–1 it crossfades sky ambient (low) to probe irradiance (high); at ≥ 1 probe light fully replaces ambient. HDR is on at any value > 0. | Label **Probe Fill Strength**. Tooltip: "Low: fill from sky ambient (uniform). High: fill from probes (bounce light, darker interiors). Reflections are unaffected." Remove the "zero" and "entirely HDR" wording (zero is no longer allowed). |
| Build light "Falloff"; My Lights "Softness" (same parameter) | "Falloff" / "Softness — Softness of the light's edge falloff" | On opaque surfaces and PBR alpha, `calcLegacyDistanceAttenuation` = `2·((1−d)/(1+f))²` with `f = falloff × 0.5` (`deferredUtil.glsl:95-104`, `pipeline.cpp:314`). The curve shape is always `(1−d)²`; falloff only **divides brightness** (×1 at 0, ×0.25 at 2). Only the legacy alpha path (`alphaF.glsl:182-229`) changes shape. | Label: one name in both places, e.g. **Falloff (dims light)**, tooltip "Higher = dimmer; does not change the edge shape." Code option (larger, changes the look of existing builds): make falloff shape the curve consistently on all paths. |
| Build light "Radius"; My Lights "Radius (m) — Light falloff distance" | metres | Deferred lights reach **1.5 ×** the radius (`pipeline.cpp:10159`). | Tooltip: "Light reaches about 1.5 × this distance." Or show the effective reach next to the value. |
| Build "Ambiance" (light) and "Ambiance" (reflection probe), same Features tab | "Ambiance" ×2 | Projector ambient fill vs probe fill strength (combined `max(sky, probe)`). | Labels **Projector Ambient** and **Probe Fill Strength**. |
| Projector FOV / Focus / texture (Build, My Lights spot) | — | Ignored on alpha surfaces (forward path: plain 90° cone, `cos²`). Near clip comes from the prim size. | Tooltips: "Not applied to alpha surfaces (hair, glass)." and "Resizing the prim moves the projector's origin." |
| `RenderExposure` (AL) | tooltip "If this control has no effect, enable 'Render HDR and Emissive'… and ensure Haze is > 0." | Needs probe ambiance > 0, not haze. In mode C it always works. | Rewrite tooltip: "Overall scene exposure." |
| `RenderReflectionProbeLevel` | "None" | Still captures the default (sky) probe every 2 s (`llreflectionmapmanager.cpp:488-500`). | Label **Sky only**. |
| `RenderLocalLightCount` | "Max. nearby lights", My Lights "Max lights … rendered" | Off-screen and rejected lights use slots unless subject sort is on (`pipeline.cpp:10128-10198`). | Tooltip: "Counts nearby candidates, not only visible lights." Code option: always count drawn lights only (the subject-sort counting rule). |
| My Lights "Max lights" | in the per-rig floater | Global setting shared with Preferences. | Label **Max nearby lights (global)**. |
| sky `sun_scale` / `moon_scale` | "Image and scale" / "Scale" | Also sets when sun/moon light fades at the horizon (`ascelestialtwilight.cpp`). | Tooltip: "Larger discs fade out later at the horizon." |
| sky `max_y` | "Maximum Altitude:" | Height that limits haze integration and sun extinction. | Label **Haze Ceiling (m)**; renamed **Atmosphere Height (m)** (decided 2026-10-25): it bounds air and haze alike, not haze only. |
| sky `glow` | "Glow Size", "Glow Focus" | Editor maps size `(2−v)·20` and focus `v·−5`; focus is negative-going. | Tooltips with direction ("higher = tighter halo"). |
| Blue Density, Blue Horizon, Haze Horizon, Density/Distance Multiplier | current names | — | Adopted renames (8.5). |

### 11.2 Wrong or inconsistent ranges

| Control | Ranges now | Code limit | Recommendation |
|---|---|---|---|
| `RenderExposure` | PT 0–10, AL 0.2–4, PG/PGA/KS 0.5–4 | clamped 0.5–4 (`pipeline.cpp:8163`) | 0.5–4 everywhere. |
| `RenderSkyAmbientScale` | AL −10–40, KS 0–20 | none | 0–20 everywhere (negative fill is not meaningful). |
| `RenderHDRSkySunlightScale` | AL 0–50, KS 0–20 | none | One range (0–20, matching KS). |
| `RenderLocalLightCount` | My Lights 0–512, PG/PGA/PT 0–8192, PT spinner 0–10000 | none | One range (for example 0–1024). |
| `RenderShadowResolutionScale` | PG 1–4, APT 1–4 / spinner 0.1–5, PT 0–5 / spinner 0–7 | none | One range (PG's 1–4). |
| sky colour swatches (sun, ambient ×3; blue ×2) | swatch shows value ÷ 3 or ÷ 2 (`llpaneleditsky.cpp:113-114`) | — | Separate colour (normalised) and intensity controls, so a white swatch is not secretly 3.0. |

### 11.3 Duplicate or conflicting controls

| Issue | Recommendation |
|---|---|
| PT "Effect" and "GTAO Effect" both write `RenderSSAOEffect.x` (`quickprefs.cpp`). | One control, shown for the active AO technique. |
| "HDR and Emissive" toggle (PG, PGA, PT) | Remove under P1 (HDR forced on). |
| Same setting at different ranges across 4–10 panels (section 5) | Fix ranges here (11.2); consolidate per section 10. |
| Personal Lighting, Advanced Lighting and KS HDR Scale silently create or overwrite the ENV_LOCAL sky. | Show "Local override active" with a Reset button wherever a control writes the local sky (P5 status readout). |

---

## 12. Decisions on section 11 and follow-up findings (2026-10-24)

The slider wiring from section 3.2 is committed (`afa5c90931e`) and merged into `kstorm` (`6719de8c790`); not yet built.

### 12.1 Decisions

| Item | Decision |
|---|---|
| Label scope | New labels apply **only to new KStorm UI** (the panels in section 10). Existing panels keep their labels. |
| sky `gamma` | **Sky HDR Scale** (new UI). |
| sky `moon_brightness` | **Moon Disk Brightness** (new UI). |
| Light falloff | Make falloff **shape the curve**, for **My Lights only** (12.4). All other local lighting (prim lights, Build controls, other paths) stays unchanged. |
| sky `reflection_probe_ambiance` | **Probe Fill Strength** with the 11.1 tooltip (new UI). |
| Build "Ambiance" labels | Unchanged. |
| Light count off-screen | Report only (12.3). |
| UI ranges | Revised: in KStorm2 (the new UI) correct only Exposure and Blue Horizon/Blue Density to their code ranges; leave the other 12.5 mismatches as they are. Existing panels unchanged. |
| `RenderSkyAmbientScale` | Negative values allowed in the UI, since the code allows them (12.5). |
| Duplicates | Remove duplicates that control a single value within one pane (12.6). Slider + number-box pairs are allowed, including pairs whose two ranges differ. |
| Haze group | Wherever these controls are surfaced, they appear together and show their haze-0 dependency (12.7). On screen the section is simply "Haze", with no special highlighting. |
| 11.1 scope | Applies to the new UI (still in design) and to My Lights only. |
| 11.2 ranges | Surface Ambient Gain allows negatives; light count and Sunlight Gain keep their existing ranges. |
| Unresolved 11.x items | Adopted as recommended (12.8). |
| Legacy skies | LL's default auto-adjust (mode B), forced on; replaces local conversion (8, P2). |
| Trackball | Not in the new UI. |
| New-UI ranges | Sunlight Gain 0–20 (the KS range); Max nearby lights 0–512. |
| NaN guards | Added on branch `fix-haze-zero-nan-guards` (uncommitted): `cloudsV.glsl`, `asHorizonCloudTintV.glsl`, `lllegacyatmospherics.cpp`. |
| Open items | All resolved except O5 (deferred); see `doc/kstorm2-lighting-UI-design.md` §7-8. Includes: Sky Ambient Gain 0–3 / 0.01; My Lights curve set as recommended; dynamic-exposure ranges; Sky HDR Scale edits `RenderSkyAutoAdjustHDRScale` on legacy skies; forced settings reapplied; local lights keep LL's ×0.9 under legacy skies. |

### 12.2 Haze Density at 0 vs above 0

The formulas are continuous at 0, but several effects ride only on the haze term, so 0 is a different regime and the change near 0 is steep:

- **Sun glow disappears.** The sun-side glow (`haze_glow`, shaped by Glow Size/Focus) multiplies only the haze term: `additive = blue_horizon·blue_weight·(sun + ambient) + haze_horizon·haze_weight·(sun·haze_glow + ambient)` (`atmosphericsFuncs.glsl:85-118`, `skyV.glsl:142-143`). At haze 0, `haze_weight = 0`, so **Glow Size, Glow Focus and Haze Horizon have no effect** on the sky dome, clouds or fog.
- **Fog colour becomes pure Blue Horizon** (`blue_weight = 1`).
- **Fog strength becomes a colour-channel artefact.** The fog blend on geometry uses only the red channel of the transmittance: `alpha = atten.r` (`hazeF.glsl:89`), with `atten = exp(-(blue_density + haze_density)·dist·density_multiplier·distance_multiplier)`. Above 0, the grey haze term usually dominates. At 0, fog amount depends only on **Blue Density's red component**, which is small by default (0.24), so distance fog nearly vanishes.
- **Sunset reddening weakens.** Sun extinction loses its `haze_density·0.25` term (`atmosphericsFuncs.glsl:66`).
- **Steep near 0.** `haze_weight = h/(b+h)`: with Blue Density ≈ 0.24, haze 0.05 already gives about 17% weight, so the first few hundredths change the look the most.
- **Possible NaN.** `cloudsV.glsl:125-127` and `asHorizonCloudTintV.glsl:141-143` divide by `abs(blue_density) + abs(haze_density)` **without** the `1e-6` floor that `skyV.glsl:111` and `atmosphericsFuncs.glsl:70` use. With haze 0 and any Blue Density channel at 0, clouds and the AS horizon cloud tint divide 0 by 0; `lllegacyatmospherics.cpp:319` (`componentDiv` by total density) has the same gap on the CPU side. The soften pass zeroes NaN (`clampHDRRange`), but these passes write their own colours. Not runtime-confirmed.
- No code path ties Exposure to haze. The Advanced Lighting Exposure tooltip ("ensure Haze is > 0") has no basis in the code, unless the NaN case above was what was seen.

### 12.3 Can the light count skip off-screen lights? (report only)

Yes, and the mechanism already exists. With subject sort on, `count_drawn_only` increments the counter only after the off-screen, zero-radius and dark checks (`pipeline.cpp:10118, 10128-10198`). Making that unconditional is a one-line change (`count_drawn_only = true`, or a new setting). Effect: up to `RenderLocalLightCount` *visible* lights are drawn even when many lights sit behind the camera. GPU cost stays bounded by the cap, and the extra CPU is a few skipped checks per light. Light order (camera or subject distance) is unchanged.

Not covered by that change:
- the forward/alpha path (6 lights by camera order, `setupHWLights`);
- My Lights' `Shader` backend, whose own index counts off-screen lights (`doc/ks-local-light-sort.md`).

### 12.4 My Lights-only falloff shaping (design notes, no change made)

**Superseded mechanism:** the negative-value marker below is replaced by a positive out-of-range falloff value (≥ 10), which also reaches PBR alpha surfaces; see `doc/kstorm2-lighting-UI-design.md` §8.3.


- Today falloff is a brightness divisor: `2·((1−d)/(1+f))²`, `f = falloff × 0.5`.
- Both My Lights backends draw through the stock light programs: `ASLightRigRenderer::render` binds `gDeferredLightProgram` / `gDeferredMultiLightProgram` (`aslightrigrenderer.cpp:158-232`), and the `LLVOVolume` backend goes through `renderDeferredLighting`. So a My Lights-only curve needs a **per-light marker** that stock lights never carry.
- **Marker:** `pipeline.cpp` already identifies rig lights per light (`ASLightRigRenderer::getLightBoost(volume)` at the colour calc). Sending a rig light's falloff as a **negative** value is unambiguous, because prim falloff is clamped to 0–2 (`LIGHT_MIN_FALLOFF`, `llprimitive.cpp:85`).
- **Shaders:** in `calcLegacyDistanceAttenuation`, `falloff < 0` selects a shaped curve. That function serves the deferred point/multi/spot paths and forward PBR, so one change covers them. The forward legacy path (`alphaF.glsl:182-229`) and `appendForwardLights` (`aslightrigrenderer.cpp:239-278`) need the same marker.
- **Proposed curve:** keep the centre brightness and make falloff set the exponent, e.g. `2·(1−d)^p` with `p` rising with falloff, so falloff changes the edge without dimming.
- **Knock-on:** the My Lights intensity tooltip ("decrease Softness for a brighter effect") becomes wrong; Boost remains the brightness lever. The "Softness" label direction must match the new curve (higher = softer edge).
- **Marker limits (review finding):** a negative falloff is safe only inside `calcLegacyDistanceAttenuation`. In `setupHWLights` falloff feeds `x = 3·(1 + falloff·2)` and quadratic attenuation `falloff + 1` (`pipeline.cpp:6762, 6778`); in `alphaF.glsl:182-229` it feeds `12·fa − 9`. Both break before any decode, so the forward path needs a different carrier (an unused channel) or an early branch.
- **Paths to cover:** `renderDeferredLighting` point, multi, spot and multi-spot; `setupHWLights` for the `LLVOVolume` backend; `ASLightRigRenderer::render` and `appendForwardLights` for the `Shader` backend; `ksdarklight.cpp:108` (passes falloff for dark spots).
- **Direction:** Softness up → exponent down (softer, longer edge).
- **Touches:** `pipeline.cpp` (tagged, 5 light paths), `aslightrigrenderer.cpp`, `ksdarklight.cpp`, `deferredUtil.glsl`, `alphaF.glsl`, `floater_as_my_lights.xml` tooltips.

### 12.5 UI ranges vs code ranges

Code ranges: the sky validation table (`indra/llinventory/llsettingssky.cpp`, `validationList` and `legacyHazeValidationList`) and clamps in the render code. Only mismatches are listed. Checked: every sky field in the editors, the settings duplicated across panels, and My Lights. Not checked one by one: Phototools tuning sliders for settings with no code clamp.

| Control | UI now | Code range | Align to |
|---|---|---|---|
| sky `density_multiplier` (stored) | ED/AL display 0.0001–2 (stored = display × 0.001, so 1e-7–0.002); ED compact 0–2 | 1e-7–2 | stored 1e-7–2 (display 0.0001–2000; a log scale is advisable) |
| sky `distance_multiplier` | ED/AL 0.05–1000; ED compact 0–1000 | 0.0001–1000 | 0.0001–1000 |
| sky `blue_horizon`, `blue_density` | swatch ÷2, so max 2 per channel | 0–3 per channel | **Decided for KStorm2:** 0–3 per channel (swatch ÷3, or colour + intensity) |
| sky `cloud_scale` | 0.01–3 | 0.001–3 | 0.001–3 |
| sky `reflection_probe_ambiance` | 0–10 | 0–10; KStorm minimum 0.01 | 0.01–10 |
| `RenderExposure` | PT 0–10 (spinner 0–10), AL 0.2–4, PG/PGA/KS 0.5–4 | clamped 0.5–4 (`pipeline.cpp:8163`) | **Decided for KStorm2:** 0.5–4 |
| `RenderSkyAmbientScale` | AL −10–40, KS 0–20 | unbounded; negative darkens lit areas, per-pixel floor 0 (`clampHDRRange`) | one range including negatives (AL's −10–40) |
| `RenderHDRSkySunlightScale` | AL 0–50, KS 0–20 | unbounded (≥ 0 meaningful) | one range (AL's 0–50) |
| `RenderLocalLightCount` | My Lights 0–512, PG/PGA/PT 0–8192, PT spinner 0–10000 | ≤ 0 turns lights off; no maximum | one range (0–8192 or 0–10000) |
| `RenderShadowResolutionScale` | PG 1–4, APT 1–4 (spinner 0.1–5), PT 0–5 (spinner 0–7) | floor 0 (`pipeline.cpp:1173`), no maximum | minimum 0 everywhere, one maximum |
| My Lights radius | 0.1–20 | 0–20 (`LIGHT_MAX_RADIUS`) | 0–20 |

Rows not marked "Decided for KStorm2" are left unchanged by decision.

Already aligned: gamma 0–20, haze 0–5, max altitude 0–10000, glow, sun/ambient swatches (÷3 to 0–3), moon brightness 0–1, star 0–500, sun/moon scale 0.25–20, cloud coverage/variance/density, moisture/droplet/ice, tonemap mix 0–1, My Lights intensity 0–1, falloff 0–2, boost 1–4.

### 12.6 Duplicates controlling one value within one pane

- **Not duplicates:** Phototools' legacy-SSAO and GTAO blocks repeat irradiance scale/max and "Effect", but only one block is visible at a time (`quickprefs.cpp:1133-1141`, by `RenderAOTechnique`).
- **Slider + number box pairs.** About 50 settings in Phototools and Advanced Phototools have a `slider_bar` plus a `spinner` bound to the same setting (for example `SB_Exposure` / `S_Exposure`, `SB_Local_Lights` / `S_Local_Lights`, `SB_Shadow_Res` / `S_Shadow_Res`). Their ranges often differ within the pair: Phototools exposure is 0–10 on both, shadow resolution 0–5 vs 0–7, light count 0–8192 vs 0–10000.
- **Trackball + azimuth/elevation sliders:** Personal Lighting, Advanced Lighting and the sky editor show both for the same sun/moon rotation.

Decided: slider + number box pairs are allowed (one logical control with typed entry), including pairs whose ranges differ. They are not removed.

### 12.7 Haze group: association requirement (decided 2026-10-24)

Rule: any UI that surfaces one of these controls surfaces the group together and shows the haze-0 dependency. Basis: 12.2.

| Control (new-UI label) | Relationship to Haze Density | Required UI association |
|---|---|---|
| **Haze Density** | Master. At 0, effects that exist only through the haze term switch off. | First in the group. Tooltip: "At 0, Haze Glow Strength, Haze Glow Focus and Haze Glow Brightness stop working, fog takes Air Glow Colour only, and distance fog depends on Air Density's red channel." |
| **Haze Glow Brightness** (Haze Horizon) | Multiplied by `haze_weight`; **no effect at 0**. | Adjacent; inactive with note while Haze Density is 0 (value kept, still editable). |
| **Haze Glow Strength** (Glow Size) | Scales `haze_glow` brightness (`glow.x^z`, not its spread; `atmosphericsFuncs.glsl:97-102`), which multiplies only the haze term; **no effect at 0**. Relabelled Haze Glow Strength (decided 2026-10-24) because it scales brightness, not spread. | Adjacent; inactive with note at 0. |
| **Haze Glow Focus** (Glow Focus) | Sets the glow's spread (`glow.z`, higher = tighter); multiplies only the haze term; **no effect at 0**. | Adjacent; inactive with note at 0. |
| **Air Glow Colour** (Blue Horizon) | At 0 it becomes the **only** fog and sky-scatter colour (`blue_weight = 1`). | In the group. Note at 0: "Sole fog colour while Haze Density is 0." |
| **Air Density** (Blue Density) | Sets the haze/air mix. At 0, distance-fog strength comes only from its **red** channel (`alpha = atten.r`), and it alone sets sun extinction (sunset tint). | In the group. Note at 0: "Fog strength follows the red component while Haze Density is 0." |

Related but not dependent on haze 0 (keep nearby, outside the group): Atmosphere Thickness and Fog Distance Scale, which scale fog and extinction for any haze value.

Existing panels: in the sky editor, Glow Size/Focus are on the Sun & Moon tab and the haze/air controls on the Atmosphere tab, which splits the group. Personal Lighting spreads the group over three columns (blue colours in `lp_1`, haze in `lp_2`, glow in `lp_3`). Advanced Lighting keeps haze and glow together (`lp_2`) but puts the blue colours in the sun/moon/atmosphere column. Neither shows the dependency. Labels on existing panels stay unchanged (12.1); applying the grouping there is an XUI change not yet requested.

Engineering note for any UI that sets Haze Density to 0: guard the NaN case in 12.2 (Air Density channel 0 with haze 0) by clamping the denominators in `cloudsV.glsl` and `asHorizonCloudTintV.glsl` as `skyV.glsl` already does.


### 12.8 Status of every section 11 item

Decided = adopted (scope: new UI, plus My Lights where marked). Declined = left as is. Open = needs a decision or a design detail; the open items are restated in `doc/kstorm2-lighting-UI-design.md` §7.

| Section 11 item | Status | Outcome |
|---|---|---|
| sky `gamma` label | Decided | **Sky HDR Scale**; tooltip adds that auto-exposure is pinned at ≤ 0.25. |
| sky `moon_brightness` label | Decided | **Moon Disk Brightness**. |
| sky `sunlight_color` label | Decided (as recommended) | **Sun & Moon Light Colour**. |
| sky `ambient` | Decided; definition Open | Sky Ambient Gain slider scaling the stored colour (L1). Unit, range, black-colour fallback: Open (O1). |
| sky `reflection_probe_ambiance` | Decided | **Probe Fill Strength**, 0.01–10, step 0.01, 11.1 tooltip, disabled when probes are off. |
| Falloff / Softness | Decided | Shapes the curve for **My Lights only** (12.4); Build Falloff unchanged. Marker design: Open (O3). |
| Radius ×1.5 tooltip | Decided (as recommended) | My Lights only; Build unchanged. |
| Build "Ambiance" ×2 | Declined | Unchanged. |
| Projector tooltips (alpha, prim size) | Decided (as recommended) | My Lights spot only; Build unchanged. |
| `RenderExposure` tooltip | Decided (as recommended) | "Overall scene exposure." (new UI). |
| Probe level "None" | Decided (as recommended) | **Sky only** (new UI). |
| Light count tooltip | Decided (as recommended) | "Counts nearby candidates, not only visible lights." Code option (count drawn lights only) reported only (12.3). |
| My Lights "Max lights" label | Decided (as recommended) | **Max nearby lights (global)** (My Lights and new UI). |
| Sun/moon scale tooltip | Decided (as recommended) | "Larger discs fade out later at the horizon." |
| sky `max_y` label | Decided (as recommended) | **Haze Ceiling (m)**. Superseded 2026-10-25: **Atmosphere Height (m)**. |
| Glow tooltips | Decided | Haze Glow Strength / Haze Glow Focus with direction tooltips (12.7). |
| Blue/Haze/Multiplier renames | Decided | 8.5. |
| 11.2 Exposure range | Decided | 0.5–4 (new UI). |
| 11.2 Surface Ambient Gain range | Decided | Negatives allowed; new UI uses −10–40. |
| 11.2 Sunlight Gain range | Decided | Existing ranges kept; new UI uses the existing 0–50 (Advanced Lighting). |
| 11.2 Light count range | Decided | Existing ranges kept; new UI uses the existing slider 0–8192 with number box 0–10000. |
| 11.2 Shadow resolution range | Declined | Left as is (12.5). |
| 11.2 Colour swatches ÷3/÷2 | Decided (as recommended) | Colour + intensity controls (new UI); blue colours 0–3. |
| 11.3 two "Effect" controls | Decided | One Effect control for the active AO technique (new UI). |
| 11.3 "HDR and Emissive" toggle | Decided | Not in the new UI (P1, HDR forced). Existing panels unchanged. |
| 11.3 local override indicator | Decided (as recommended) | "Local override active" + Reset in P1 and P5. |

---

## Appendix A: Method and verification

- Uniform producers: `LLSettingsVOSky::applyToUniforms/applySpecial`, `LLShaderMgr` reserved uniforms (`indra/llrender/llshadermgr.cpp:1273-1617`). Consumers were traced in the shaders cited above.
- Settings cross-reference: a script parsed `app_settings/settings.xml` and `settings_per_account.xml` (2913 + per-account), found C++ references by quoted name, XUI references (`control_name`, `enabled_control`, `parameter`, ...) in `skins/default/xui/en`, and panel-code references (`llfloater*`, `llpanel*`, `quickprefs`, `as*floater/panel`, `kspanel`, `llviewermenu`). Code-only "UI" hits were checked by hand: `RenderSkyAutoAdjustLegacy` is only read by the editors; `RenderShadowGaussian`, `RenderShadowSplitExponent` and `RenderSSAOEffect` are real Phototools controls.
- Scripts (local only, not in the repo, kept in a temporary folder): `xref.py`, `uidump.py`, `tabdump.py`. Outputs: `settings_xref2.tsv`, `render_settings.tsv`, `undefined_controls.txt` in the same folder. The folder is temporary; rerun the scripts to regenerate.
- Double-checked claims: ambient luminance in mode C, moonlight = sunlight colour, the probe-ambiance flag flip used by conversion.
- Not done: runtime testing. Everything here is from source reading only.

---

## Appendix B: The original viewer as built (all four modes)

Kept from the retired `doc/kstorm-lighting-factor-map.md` (removed 2026-10-24; full text in git history at `bc9e1ec3fc3` and earlier). This is how the viewer behaves **today**, before section 8 and the UI design are implemented; useful when debugging current behaviour.

Four conditions select the mode. None has its own control.

| Input | Where | Meaning |
|---|---|---|
| `canAutoAdjust()` | `indra/llinventory/llsettingssky.cpp:1175` | TRUE when the sky asset has **no** `reflection_probe_ambiance` key (a legacy sky). The value is then 0. |
| `RenderSkyAutoAdjustLegacy` | settings.xml, default **0** in this tree | Turns on LL's legacy→HDR auto-adjust. No UI; the env editors only read it (`llpaneleditsky.cpp:211`, `llfloaterenvironmentadjust.cpp:181`). KStorm forces it on (8, P2; UI design R2, R12). |
| sky `reflection_probe_ambiance` | sky asset / editors | `0` vs `> 0` switches post-processing. Moving the slider calls `setReflectionProbeAmbiance()`, which sets `mCanAutoAdjust = false` (`llsettingssky.cpp:1517`): any commit, even back to 0, makes that sky copy an EEP sky. Discarding the edited sky restores the original; saving writes the key into the asset (`llsettingssky.cpp:1271`). |
| `RenderDisableVintageMode` | Prefs / Phototools / KS | Its handler sets `RenderHDREnabled` and `RenderEnableEmissiveBuffer` (`llviewercontrol.cpp:355`). KStorm forces it on (R1, R12). |

Modes B and C are described in section 1. The other two:

| Mode | Condition | Shading | Post |
|---|---|---|---|
| **A. Classic (pre-EEP / WindLight look)** | legacy sky, auto-adjust off | `classic_mode = 1`: sun ×1.35 (`softenLightF.glsl:226`), final ×1.1 (`:363`), sRGB-space mix 0.9/0.7 (`:299`). Probe irradiance is bypassed, so ambient = sky ambient only (`reflectionProbeF.glsl:747,864`). Ambient keeps its hue. Local lights ×0.9. | **No post**: no exposure, tonemap or dynamic exposure (`tonemapUtilF.glsl:145`). Scene Gamma is applied as the legacy gamma curve (`postDeferredGammaCorrect.glsl:41`). |
| **D. Hybrid (EEP with ambiance 0)** | sky has key, ambiance == 0 | Non-classic shading (ambient as luminance), `sky_hdr_scale = 1`. | **No post** and legacy gamma, as in mode A. Undocumented and invisible to the user. |

Consequences in the current viewer: Exposure, Tonemap type, Tonemap mix and dynamic exposure do nothing in modes A and D. Scene Gamma is the legacy gamma curve in A and D, the HDR scale in C, and only the auto-exposure range in B. No panel shows the active mode. KStorm reaches neither A (auto-adjust forced on) nor D (probe ambiance minimum 0.01).

## Appendix C: Controls that matter only for modes A and D

Hidden or removed under the KStorm policy (section 8), because they only act in the classic or non-HDR paths:

- Vintage mode toggle ("HDR and Emissive"); forced on instead.
- `RenderSkySunlightScale` (sun scale when HDR is off).
- The legacy gamma meaning of Scene Gamma.
- `RenderSunDynamicRange` (legacy forward lighting only).
- `SkyAmbientScale`, `SkyNightColorShift`, `WaterGLFogDepthScale`, `WaterGLFogDepthFloor` (legacy fog colour, `lllegacyatmospherics.cpp`).
- `WindLightUseAtmosShaders` (deprecated, forced TRUE).
- Classic shader constants: ×1.35, ×1.1, the 0.9/0.7 mix.
- `RenderReflectionProbeAmbiance` (saved in graphic presets, never read by code).

No longer in this list since the auto-adjust decision: `RenderSkyAutoAdjustLegacy` and its six scale settings, and the local-light ×0.9 under legacy skies. They now act in mode B (section 1; UI design O11, R2, R12, R13).

## Appendix D: Retuning legacy sky values

Under the auto-adjust decision (8, P2) legacy skies are not converted; LL's auto-adjust scales are applied live each frame. This method applies when a legacy sky's look is tuned for HDR by hand, for example after editing its Probe Fill Strength turns that copy into an EEP sky, or when saving a tuned EEP version.

Why values shift: classic composed sun and ambient in sRGB with the 1.35 / 1.1 / 0.9 / 0.7 constants and showed the result without tonemapping. The HDR path linearises the sun, reduces ambient to luminance for surfaces, adds haze as `linear(2·additive)·HDRScale`, then applies exposure, ACES/Neutral tonemapping and mix 0.7. Typical result: distant haze and the sky look brighter and more saturated relative to lit surfaces, and low-sun scenes gain contrast.

Method:
1. **Freeze the camera response first.** Exposure 1.0, dynamic exposure at default, tonemap type and mix fixed, Sky HDR Scale 1.0. Tune the atmosphere only after that.
2. **Start from LL's mapping.** The auto-adjust settings are LL's legacy→HDR transfer (ambient, Air Glow Colour, Air Density and sun colour × their scales; HDR scale = `RenderSkyAutoAdjustHDRScale`). Shipped settings.xml defaults are 1.0 for all; the code fallbacks are 0.75 (ambient) and 2.0 (HDR scale).
3. **Make a reference set while the classic path still exists** (until R12 forces auto-adjust on): capture the same camera views per sky (noon, golden hour, night; near and far) in classic and HDR, and compare mean luminance and per-region colour with a local script.
4. **Adjust in dependency order:**
   1. Air Density and Atmosphere Thickness (sun colour and surface tint at low sun);
   2. Haze Density (white haze amount);
   3. Fog Distance Scale (fog distance);
   4. Air Glow Colour / Haze Glow Brightness (fog and sky colour and brightness);
   5. Haze Glow Strength and Focus (sun halo);
   6. Sky HDR Scale last: it scales sky and haze only, not surfaces, so it is the best "haze vs scene" balance control.
5. **Keep ambient hue** (8.2); set Sky Ambient Gain only where Probe Fill Strength is below 1.
6. Save the tuned result as an EEP sky so the work is not repeated per region.
