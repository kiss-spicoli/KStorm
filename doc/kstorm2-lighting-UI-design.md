# KStorm2 lighting UI design

Author: Kissmy Spicoli. Date: 2026-10-24. Status: **in design**; nothing here is implemented.

Input: `doc/kstorm2-lighting-factor-map.md` section 10 (panel summary), with the decisions in its sections 8 and 12 (12.1, 12.7, 12.8) folded in. That document holds the rationale and code analysis. This one is the specification: per control, the final label, binding, range, step, tooltip, dependency behaviour and code site.

Search terms: KStorm2 UI, lighting panels, P1 Environment, P2 Tone, P3 Shadows Occlusion Reflections, P4 Local Lights, Camera tab, P5 More, labels, ranges, tooltips, haze group.

## Contents

- [1. Global rules](#1-global-rules)
- [2. P1 Environment](#2-p1-environment)
  - [2.1 Source](#21-source)
  - [2.2 Key light](#22-key-light)
  - [2.3 Fill](#23-fill)
  - [2.4 Haze (section label "Haze", no special highlighting; one block, in this order)](#24-haze-section-label-haze-no-special-highlighting-one-block-in-this-order)
  - [2.5 Atmosphere: other](#25-atmosphere-other)
  - [2.6 Clouds](#26-clouds)
  - [2.7 Water](#27-water)
  - [2.8 Sky visuals](#28-sky-visuals)
- [3. P2 Tone](#3-p2-tone)
- [4. P3 Shadows, Occlusion & Reflections](#4-p3-shadows-occlusion--reflections)
  - [4.1 Shadows](#41-shadows)
  - [4.2 Occlusion](#42-occlusion)
  - [4.3 Reflections](#43-reflections)
  - [4.4 Volumetric and shader level](#44-volumetric-and-shader-level)
- [5. P4 Local Lights](#5-p4-local-lights)
  - [5.1 Budget](#51-budget)
  - [5.2 My Lights (also applied to `floater_as_my_lights.xml`, R4)](#52-my-lights-also-applied-to-floater_as_my_lightsxml-r4)
  - [5.3 Selected object (mirror of Build > Features) — removed from the lighting UI (decided 2026-10-25)](#53-selected-object-mirror-of-build--features--removed-from-the-lighting-ui-decided-2026-10-25)
  - [5.4 Surface response and debug](#54-surface-response-and-debug)
- [5A. Camera (tab, decided 2026-10-24)](#5a-camera-tab-decided-2026-10-24)
  - [5A.1 Depth of field](#5a1-depth-of-field)
  - [5A.2 Lens](#5a2-lens)
- [6. P5 More (status, presets, floaters)](#6-p5-more-status-presets-floaters)
  - [6.1 Floater buttons (R15)](#61-floater-buttons-r15)
  - [6.2 Display](#62-display)
  - [6.3 Status, presets and expert](#63-status-presets-and-expert)
- [7. Open items](#7-open-items)
- [8. Resolution notes](#8-resolution-notes)
  - [8.1 Sky Ambient Gain: physical units (O1)](#81-sky-ambient-gain-physical-units-o1)
  - [8.2 Dynamic exposure (O4)](#82-dynamic-exposure-o4)
  - [8.3 My Lights falloff (O3)](#83-my-lights-falloff-o3)
  - [8.4 Ranges confirmed (O7)](#84-ranges-confirmed-o7)
  - [8.5 Viewer-only light overrides (O15)](#85-viewer-only-light-overrides-o15)
- [9. UI approaches](#9-ui-approaches)
- [10. As designed (v4, 2026-10-25)](#10-as-designed-v4-2026-10-25)
- [11. Implementation plan (draft, 2026-10-25; incorporates the Advisor review)](#11-implementation-plan-draft-2026-10-25-incorporates-the-advisor-review)
  - [11.1 Architecture](#111-architecture)
  - [11.2 Sky and water editing](#112-sky-and-water-editing)
  - [11.3 Controls that need code](#113-controls-that-need-code)
  - [11.4 Use KS Reference (`ksenvironment.cpp`)](#114-use-ks-reference-ksenvironmentcpp)
  - [11.5 Branches, in order](#115-branches-in-order)
  - [11.6 Verification per pane (user, after each build)](#116-verification-per-pane-user-after-each-build)
  - [11.7 Corrections found in review](#117-corrections-found-in-review)
  - [11.8 Test scenarios: alpha masks lit as opaque (2026-10-25)](#118-test-scenarios-alpha-masks-lit-as-opaque-2026-10-25)
- [12. Items awaiting approval (revised 2026-10-25)](#12-items-awaiting-approval-revised-2026-10-25)
  - [12.1 Candidates for Use KS Reference (not yet applied)](#121-candidates-for-use-ks-reference-not-yet-applied)
  - [12.2 Ignored: changeable elsewhere, no conflict with our controls (c)](#122-ignored-changeable-elsewhere-no-conflict-with-our-controls-c)
  - [12.3 Decisions (2026-10-25)](#123-decisions-2026-10-25)
  - [12.4 Restated items](#124-restated-items)
  - [12.5 Which §12.1 candidates can be set and restored safely](#125-which-121-candidates-can-be-set-and-restored-safely)
  - [12.6 Lighting Presets: unified design (decided 2026-10-25)](#126-lighting-presets-unified-design-decided-2026-10-25)
  - [12.7 Lighting Presets compared with My Environments](#127-lighting-presets-compared-with-my-environments)

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
| R16 | UI text uses American English (color, gray, prioritize, behavior). Setting names are unchanged. | decided 2026-10-25 |
| R17 | Color swatches sit on the label's line and stretch from the label to the value box (intensity), or to the right edge (less the usual padding) when there is none. | decided 2026-10-25 |

Range notation: `min–max / step`. A pair is written `slider min–max / step; number min–max / step`.

---

## 2. P1 Environment

### 2.1 Source

| Control | Binding / action | Notes |
|---|---|---|
| Use Shared Environment | `LLEnvironment` clear local | Turns off "Local override active". |
| My Environments… | toggle button `my_environments` (R15) | The sky, water and day-cycle asset editors are not reachable from this panel (decided 2026-10-24). |
| Sky / Water / Day presets, prev/next | as Phototools preset combos (`quickprefs.cpp`) | |
| ~~Reset to region default~~ | as Phototools `ResetToRegionDefault` | **Dropped 2026-10-25:** same result as Use Shared (both clear the local override via `LLEnvironment` ENV_LOCAL), but with a blended transition and no probe reset. Use Shared stays, wired as menu World › Environment › Use Shared Environment (`World.EnvSettings` "region": instant, resets probes). |
| Set KS environment, **labelled "Use KS Reference"** (decided 2026-10-25). **Also turns Dynamic exposure off and disables its check box** (`RenderDynamicExposureEnabled` = FALSE). Its previous state is saved and restored when Use KS Reference is turned off (decided 2026-10-25, "for now"; not yet in `ksenvironment.cpp`). Reason: with Sky HDR Scale 0.1 the auto-exposure range is 1–1, so Dynamic exposure could have no effect anyway. Exposure (`RenderExposure`) stays fully effective: it multiplies the scene in the tone-map pass whatever the auto-exposure factor is. **Surfaced values it writes** (`ksenvironment.cpp`), which every open pane must show while it is on and show restored when it is off: Sun azimuth 0, Sun elevation 6, Moon azimuth 180, Moon elevation −90, Sun & Moon Light (0.48, 0.64, 0.72), Sky Ambient Gain 2.43 (luminance of ambient (0.88, 0.82, 0.52) × 3), Haze Density 0.1, Haze Glow Brightness 0.15, Haze Glow Strength 1.9, Haze Glow Focus 0.1, Sky HDR Scale 0.1, Probe Fill Strength 0.01, Show avatar names off (`AvatarNameTagMode` 0), Subject sort on. Not surfaced: `RenderFarClip` 64 (16 until 2026-10-26), `RenderAvatarMaxNonImpostors` 4. The code disables no control; Dynamic exposure is the only one disabled, by this design. **Legacy-sky mismatch:** the code writes the sky's `gamma` and probe ambiance directly, but for an auto-adjusted legacy sky our Sky HDR Scale shows `RenderSkyAutoAdjustHDRScale` and Probe Fill shows the auto-adjust value, so neither would show what KS set. **Resolved in review:** setting probe ambiance clears the sky's auto-adjust flag, so KS turns the local copy into an EEP sky and the panes show its values after the environment-changed rebind (§11.2). | `KSSetEnvironment` (check box) | Note: sets Sky HDR Scale 0.1, which pins auto-exposure (P2). |
| Sky origin | read-only: "Legacy sky (auto-adjusted)" / "EEP sky" | R2. |
| Local override active + Reset | indicator; Reset = Personal Lighting reset | R9. |

### 2.2 Key light

| Label | Binding | Range / step | Tooltip | Behaviour | Code site |
|---|---|---|---|---|---|
| Sun azimuth, Sun elevation | sky `sun_rotation` | 0–359.99 / 0.01; −90–90 / 0.01 | | No trackball (R6). | `llpaneleditsky.cpp` |
| Moon azimuth, Moon elevation | sky `moon_rotation` | same | | same | same |
| **Sun & Moon Light Color** | sky `sunlight_color` | color + intensity, 0–3 per channel | "Light color of both the sun and the moon." | Color/intensity split replaces the ÷3 swatch. | `llsettingsvo.cpp:797-807` |
| Sunlight Gain | `RenderHDRSkySunlightScale` | 0–20 / 0.01 | "Brightness of direct sun and moon light." | | `llsettingsvo.cpp:838` |
| Sun scale | sky `sun_scale` | 0.25–20 / 0.01 | "Larger discs fade out later at the horizon." | Also moves the twilight fade. | `ascelestialtwilight.cpp` |
| Moon scale | sky `moon_scale` | 0.25–20 / 0.01 | same | same | same |
| **Moon Disk Brightness** | sky `moon_brightness` | 0–1 / 0.01 | "Brightness of the moon disc and night glow. Does not change moonlight on surfaces." | | `moonF.glsl:270` |
| Partial moon below horizon | `ASRenderPartialMoonBelowHorizon` | check box | | | `ascelestialtwilight.cpp` |
| Star brightness | sky `star_brightness` | 0–500 / 0.01 | | | `lldrawpoolwlsky.cpp:250` |

### 2.3 Fill

| Label | Binding | Range / step | Tooltip | Behaviour | Code site |
|---|---|---|---|---|---|
| **Sky Ambient Gain** | sky `ambient` (scales the stored color; hue kept) | 0–3 / 0.01 (Rec.709 luminance of the stored color; gray fallback when black, §8.1) | "Brightness of the sky's ambient color, saved with the sky (hue kept). Reaches the sky, clouds, haze and surface fill; water only through reflected sky. Surfaces use its brightness only. At Probe Fill Strength 1 or more it no longer lights surfaces." | Writes ENV_LOCAL (R9). | `atmosphericsFuncs.glsl:110-157` |
| Surface Ambient Gain | `RenderSkyAmbientScale` | −10–40 / 0.1 | "Multiplies ambient fill on surfaces only, so it shows most up close. Does not change the sky, clouds, haze or water. At Probe Fill Strength 1 or more it no longer lights surfaces. Negative values darken lit areas." | Negatives allowed. | `atmosphericsFuncs.glsl:164` |
| **Probe Fill Strength** | sky `reflection_probe_ambiance` | 0.01–10 / 0.01 | "<1: fill from sky ambient (uniform). >=1: fill from probes (bounce light, darker interiors). At a probe fill of 1 or more, probe irradiance fully replaces ambient wherever probes cover the surface. Reflections are unaffected." | Disabled when probes are off (value forced to 1.0 then, `llsettingsvo.cpp:858`). Writes ENV_LOCAL. | `reflectionProbeF.glsl:569-571`, `llreflectionmapmanager.cpp:1221` |

### 2.4 Haze (section label "Haze", no special highlighting; one block, in this order)

**Decided 2026-10-25:** no separate Haze pane. These rows sit at the bottom of the Atmosphere pane (§2.5), and Atmosphere's status shows Haze Density.

| Label | Binding | Range / step | Tooltip | Haze-0 behaviour | Code site |
|---|---|---|---|---|---|
| **Haze Density** | sky `haze_density` | 0–5 / 0.01 | "At 0, Haze Glow Strength, Haze Glow Focus and Haze Glow Brightness stop working, fog takes Air Glow Color only, and distance fog depends on Air Density's red channel." | Master. | `atmosphericsFuncs.glsl:66-118` |
| **Haze Glow Brightness** | sky `haze_horizon` | 0–5 / 0.01 | "Brightness of white haze and its sun glow." | Inactive with "No effect while Haze Density is 0" (value kept, editable). | same |
| **Haze Glow Strength** | sky `glow.x` = `(2−v)·20` | 0–1.99 / 0.01 | "Strength of the haze glow around the sun; higher = brighter. Does not change its spread." | Inactive at 0, as above. | same |
| **Haze Glow Focus** | sky `glow.z` = `v·−5` | −2–2 / 0.01 | "Tightness of the haze glow; higher = tighter, more concentrated halo." | Inactive at 0, as above. | same |
| **Air Glow Color** | sky `blue_horizon` | 0–3 per channel (color + intensity) | "Sky and distant-fog color from clear air." | Note at 0: "Sole fog color while Haze Density is 0." | `skyV.glsl:142-155` |
| **Air Density** | sky `blue_density` | 0–3 per channel (color + intensity) | "Clear-air amount per color: sunset reddening, fog fade rate and the air/haze mix." | Note at 0: "Fog strength follows the red component while Haze Density is 0." | `atmosphericsFuncs.glsl:66-83`, `hazeF.glsl:89` |

Engineering prerequisite (O6): the 0/0 guards in `cloudsV.glsl`, `asHorizonCloudTintV.glsl` and `lllegacyatmospherics.cpp` are committed on `fix-haze-zero-nan-guards` (`2c8a7e9d438`) and must be merged before this group can set Haze Density to 0.

### 2.5 Atmosphere: other

| Label | Binding | Range / step | Tooltip | Code site |
|---|---|---|---|---|
| **Atmosphere Height (m)** (was Haze Ceiling; renamed 2026-10-25) | sky `max_y` | 0–10000 / 0.01 | "Height of the atmosphere: limits fog, haze and sun dimming." It caps the view-ray height for air and haze alike, scales sun dimming (with Atmosphere Thickness) and sets cloud altitude fade; it is not a haze-only control. | `atmosphericsFuncs.glsl:55-66` |
| **Atmosphere Thickness** | sky `density_multiplier` (stored = display × 0.001) | 0.0001–2 / 0.0001 | "Scales haze thickness and sun extinction." | `atmosphericsFuncs.glsl:66,79` |
| **Fog Distance Scale** | sky `distance_multiplier` | 0.05–1000 / 0.01 | "Scales how fast distance fog builds." | same |
| Moisture, Droplet radius, Ice | sky `moisture_level`, `droplet_radius`, `ice_level` | 0–1 / 0.001; 5–1000 / 0.01; 0–1 / 0.001 | "Rainbow and halo only." | `skyF.glsl` |
| Atmospheric pass | `RenderDeferredAtmospheric` | check box | "Haze on scene geometry." | `pipeline.cpp:1368` |

### 2.6 Clouds

Cloud color is the first control in the pane (decided 2026-10-25).

Coverage 0–1 / 0.01 (also removes sun and adds fill); scale 0.01–3 / 0.01; variance 0–1 / 0.01; color; density X, Y 0–1 / 0.01, D 0–3 / 0.01; detail X, Y, D 0–1 / 0.01; scroll (XY pad); map (texture). Bindings: sky `cloud_*`. Ranges as the sky editor (R8).

### 2.7 Water

Fog color; fog density 0.001–100 / 0.01; underwater modifier 0–20 / 0.01; fresnel scale and offset (sky editor range); normal map; normal scale X, Y, Z 0–10 / 0.01; wave 1 and 2 directions; refraction above / below 0–3 / 0.01; blur 0–0.5 / 0.001; Transparent water (`RenderTransparentWater`, check box). Bindings: water settings as `panel_settings_water.xml`.

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
| Use fixed range | `RenderUseExposureSkySettings` | check box | "Uses a fixed 0.5–3 range instead of Sky HDR Scale." (HDR min/max/offset are hard-coded) | Expert. | `pipeline.cpp:8067-8081` **Corrected 2026-10-25:** relabelled "Use sky exposure range"; it uses the sky's own HDR offset ± min/max, not a fixed range (§10, pane 12). |
| Tone mapping | `RenderTonemapType` | Khronos Neutral / ACES | | | `tonemapUtilF.glsl:131` |
| Tone mapping mix | `RenderTonemapMix` | 0–1 / 0.05; number 0–1 / 0.001 | | | `llsettingsvo.cpp:835` |
| Glow | `RenderGlow` | check box | "Material glow (not haze glow)." | `RenderGlowHDR` is not in the panels and the code does not force it: the user sets it once in Debug Settings and it persists (`Persist` 1; 16-bit float glow buffers, `pipeline.cpp:1565`; decided 2026-10-25). | `pipeline.cpp:1331-1341` |
| Glow resolution, iterations, strength, width | `RenderGlowResolutionPow`, `…Iterations`, `…Strength`, `…Width` | 8–10 / 1; 0–200 / 1 (number 0–500); 0–0.5 / 0.001 (number 0–10); 0–50 / 0.001 (number 0–5000) | | | same |
| ~~Glow internals~~ | `RenderGlowLumWeights`, `…MinLuminance`, `…MaxExtractAlpha`, `…WarmthAmount`, `…WarmthWeights`, `…Noise` | ~~Open O5 (no existing UI)~~ | Not in the KStorm UI (decided 2026-10-25). | Expert. | same |

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
| ~~Internals (expert)~~ | `RenderShadowSplits`, `…Offset`, `…OffsetError`, `…BiasError`, `…ErrorCutoff`, `…Noise`, `RenderSpotShadowBias`, `RenderDeferredSpot/TreeShadowBias/Offset`, `RenderEdgeDepth/NormCutoff`, `RenderDeferredNoise` | Open O5 (no existing UI) **Not in the KStorm UI (decided 2026-10-25).** |

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

**Rename (decided 2026-10-25): "My Lights" is called "Viewer Lights" throughout the KStorm lighting UI** (section names, Only Viewer Lights, tooltips, the button to `as_my_lights`). Setting names (`ASLightRig*`, `KSLightRigOnlyMyLights`) and AyaneStorm's own floater are unchanged. Where this document says My Lights, read Viewer Lights.

**Grouping by scope (decided 2026-10-25).** Two separate questions decide where a control goes: which lights it concerns (My Lights, server lights, or both), and where its change lives. A change to an object's light parameters is stored in the object: it is sent to the server and everyone sees it, for rezzed and worn objects alike. Every other control in this panel is a viewer render setting: it changes only your view and nothing is sent. My Lights exist only in the viewer, so all their controls are view-only. Every Lights section opens with a "Scope" line, and the Server lights section labels its two kinds of control separately:

- **Viewer Lights**: a button that opens AyaneStorm's My Lights floater (§5.2, decided 2026-10-25); viewer-only lights that only you see.
- ~~**Server lights**: lights stored in objects, rezzed or worn. *Your view only:* Worn lights (`RenderAttachedLights`, from §5.1). *Edits the object:* the Selected object (§5.3), sent to the server, needs modify rights, worn objects included. Turning Worn lights off hides every attachment light, yours and other people's, in your view only; it is a local render setting, nothing is sent, and others still see them.~~ **Removed 2026-10-25:** the section was a Build tool. Worn lights moves to All lights (both); Show light radius moves there as **Show selected light radius**; the Selected object block (§5.3) leaves the lighting UI.
- **All lights (both)** (your view only): Max nearby lights, Only My Lights, Subject sort (from §5.1), and Alpha masks lit as opaque (from §5.4; Bump strength is not in the UI); these apply to server lights and My Lights alike. Listed last. Also Worn lights and Show selected light radius (`RenderLightRadius`), moved from Server lights (decided 2026-10-25).

The three §5.4 debug check boxes go to the section whose lights they show: **Light beacons** (`ASRenderLightBeacon`) to Viewer Lights, which is now AyaneStorm's floater (§5.2), since it marks only My Lights (and hair occlusion proxies) and needs the My Lights floater instance; **Show light radius** (`RenderLightRadius`) to Server lights (later moved to All lights (both) as Show selected light radius, 2026-10-25), since it draws only the selected object's light while Build is open; **Light bounding boxes** (info display "lights") to All lights (both), since it boxes every light drawable (My Lights only with the LLVOVolume backend). The "Surface and debug" section goes. The tables below are kept as written; this paragraph says where each row now sits.

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

**Decided 2026-10-25: no Viewer Lights controls of our own.** Lights › Viewer Lights opens AyaneStorm's My Lights floater (`Floater.Toggle as_my_lights`); our Viewer Lights and Viewer Lights spot sections go. Her floater has every control ours had except two, and adds about 34 (light list, add/delete, per-light name and on, placement and position, spot aim and texture, background, pose, presets). The table below now lists changes to her floater only. Tasks kept for it: the **Curve** selector (§8.3) and the **Eye mask radius** control (`ASLightRigEyeMaskRadius`, range still Open O5). Max nearby lights and Only Viewer Lights stay in All lights (both) as well as her floater; Light beacons is in her floater only ("Show light beacons").

Existing rig controls and ranges (distance 0.05–3, height −2–2, azimuth 0–359.9, offsets ±10, intensity 0–1 / 0.05, color, boost 1–4 / 0.05, spot FOV 1–170 / 1, focus −20–20 / 0.5, ambiance 0–1 / 0.05, texture, dark, backend, presets), with these changes:

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

### 5.3 Selected object (mirror of Build > Features) — removed from the lighting UI (decided 2026-10-25)

Not in the KStorm lighting UI: it edits the object, so it stays a Build tool (Build › Features). The text below is kept as the record. A viewer-only replacement is open item O15 (§8.5).

Light on, color, intensity, radius, falloff; projector texture, FOV, focus, ambiance; reflection probe type, dynamic, mirror, ambiance, near clip. **Labels, ranges and behaviour identical to Build** (R5).

### 5.4 Surface response and debug

| Label | Binding | Range |
|---|---|---|
| Alpha masks lit as opaque | `RenderAutoMaskAlphaDeferred`, `RenderAutoMaskAlphaNonDeferred` | check boxes (both to be tested). A face whose texture is effectively cut-out (`getIsAlphaMask`), with face alpha 1, no glow, not alpha-blend mode and not on a HUD, is drawn as an alpha mask (opaque deferred) instead of blended. It then gets every local light, sun shadows, occlusion and My Lights spot cones and falloff shaping; blended alpha is lit in the forward pass. `NonDeferred` covers fullbright faces (default off). |
| ~~Bump normal strength~~ | `RenderNormalMapScale` | **Not in the KStorm UI; the user sets it in Debug Settings (decided 2026-10-25).** Was: existing Phototools combo: Very fine 1024, Finer 512, Fine 256, Normal 64 (default), Rough 32, Rougher 16. Scales the normal maps the viewer generates from legacy bump maps (Brightness, Darkness and the texture-named bumps); a larger value gives a flatter, finer bump. No effect on material normal maps or PBR. |
| ~~Light beacons, light radius, light bounding boxes~~ | `ASRenderLightBeacon`, `RenderLightRadius`, info display "lights" | check boxes; **moved by scope (see §5 grouping), decided 2026-10-25** |

---

## 5A. Camera (tab, decided 2026-10-24)

Camera-specific controls, moved out of Tone. Ranges follow R8 (Phototools slider ranges; its ±128 number boxes are not copied).

### 5A.1 Depth of field

The three focus check boxes sit directly under Depth of field (decided 2026-10-25).

| Label | Binding | Range / step | Notes |
|---|---|---|---|
| Depth of field | `RenderDepthOfField` | check box | Header status: "on, f/9.0, 50 mm" / "off". |
| Focus follows pointer | `FSFocusPointFollowsPointer` | check box | |
| Focus locked (Alt-Shift-X) | `FSFocusPointLocked` | check box | |
| Show focus point | `FSFocusPointRender` | check box | |
| DoF mode | `ASDepthOfFieldMode` | ~~Standard / Firestorm; Advanced / AyaneStorm; Aperture-sampled (stills); Live / AyaneStorm~~ The items are whatever AyaneStorm's `dof_mode` combo offers (currently Standard / Firestorm = 0, Aperture-sampled (still images only) = 2, Live / AyaneStorm (new) = 3) | Decided 2026-10-25: ours follows Ayane's list, so a mode she adds, removes or renames changes ours with no edit on our side. Implementation: our XUI declares the combo with no items; `postBuild()` reads `floater_as_depth_of_field.xml` (`LLUICtrlFactory::getLayeredXMLNode`, so skin overrides apply), finds `combo_box name="dof_mode"` and adds each `combo_box.item` label and value. The values, not positions, are stored, so the gap at 1 is harmless. |
| F-number | `CameraFNumber` | 0.01–22 / 0.01 | |
| Focal length (mm) | `CameraFocalLength` | 1–300 / 1 | |
| Field of view (deg) | `CameraFieldOfView` | 0.1–180 / 0.1 | |
| Zoom | `CameraAngle` | 0.08–3.06 / 0.002 | |
| ~~Focus transition (s)~~ | `CameraFocusTransitionTime` | ~~0–5 / 0.01~~ | Not in the KStorm UI (decided 2026-10-25). |
| Max blur (CoC) | `CameraMaxCoF` | 0–150 / 0.01 | |
| DoF resolution | `CameraDoFResScale` | 0–1 / 0.01 | |
| ~~DoF while editing~~ | `RenderDepthOfFieldInEditMode` | ~~check box~~ | Not in the KStorm UI (decided 2026-10-25). |
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

HDRI preview is an action, not a floater, and stays a plain button. **Not in the KStorm UI** (decided 2026-10-25): a developer tool for checking materials against an HDRI, not a lighting control; stays on the Develop menu.

### 6.2 Display

| Label | Binding | Range | Tooltip |
|---|---|---|---|
| Show avatars | render type `character` via `Advanced.ToggleRenderType` / `Advanced.CheckRenderType` (as Advanced Phototools "Toggle Avatars") | check box | "Shows or hides all avatars." |
| Show avatar names | `AvatarNameTagMode` | check box (as Advanced Phototools) | "Name tags over avatars." |

Not included (decided 2026-10-24): "Enable Reflection Probe Selection" (`SelectReflectionProbes`, Phototools › Env).

### 6.3 Status, presets and expert

**Decided 2026-10-25 (v4 mock-up):** the More accordion goes.
- **Expert:** hidden; marked **future**. Its rows below are kept for that.
- **Display** (§6.2): its controls move to the bottom of Quick.
- **Presets & quality:** not surfaced: no Quality slider, no Save preset… or Load preset…, and no preset-overwrite warning. Preset load is future research, O16.
- **Presets (environment):** the Sky preset and Water preset pickers are not surfaced (§2.1). Quick starts with My Environments… and Environment Effects…, then the Use Shared button.
- **Quick order (decided 2026-10-25):** My Environments…, Environment Effects…; Use Shared; Dynamic exposure with Use KS Reference to its right (one row); Exposure; Shadows; Sun azimuth, Sun elevation, Sunlight Gain, Haze Density, Sky HDR Scale; Ambient occlusion with a **Settings…** button that opens the **Ambient** pane; Sky Ambient Gain, Probe Fill Strength; Show avatars, Show avatar names. The Ambient pane is the Occlusion pane (§4.2) renamed, with the Ambient controls (§2.3: Probe Fill in use, Sky Ambient Gain, Surface Ambient Gain, Probe Fill Strength and their notes) added at the top. Neither Ambient nor Occlusion is an entry in the launch list; the pane opens only from that Settings… button. Sky Ambient Gain on Quick and in Ambient is one control (same binding).
- **Status:** Sky origin moves to the top of Environment › Sun/Moon, and Probe Fill in use to the top of Environment › Ambient. HDR / post and Auto-exposure are not surfaced, as status rows or in the footer.

| Item | Content |
|---|---|
| Sky origin | Legacy (auto-adjusted, mode B) / EEP (mode C) |
| Effective Probe Fill Strength | value in use (shows 1.0 forced when probes are off) |
| HDR / post | "HDR on"; "Post-processing off" during snapshot no-post or Build with `RenderDisablePostProcessing` |
| Probe level | current coverage |
| Auto-exposure | "Pinned at 1" when Sky HDR Scale ≤ 0.25 and dynamic exposure is on |
| Local override active | indicator + Reset (R9) |
| Presets & quality | Quality slider (`RenderQualityPerformance`, existing Preferences slider) and graphics presets (save/load). One-line warning shown with them: "Changing these overwrites the lighting settings in this floater." Forced settings are reapplied afterwards (R12). |
| Expert | ~~HDRI preview + `RenderHDRIExposure`, `…Rotation`, `…SplitScreen`, `…IrradianceOnly` (Open O5)~~ not in the KStorm UI (decided 2026-10-25); `RenderSpecularResX/Y`, `RenderSpecularExponent`; `RenderDeferredDisplayGamma` |

---

## 7. Open items

| # | Item | Status |
|---|---|---|
| O1 | Sky Ambient Gain definition | **Resolved:** Rec.709 luminance of the stored color, 0–3 / 0.01, gray fallback when black (§8.1). |
| O2 | Legacy-sky conversion | **Resolved:** LL's default auto-adjust (R2). |
| O3 | My Lights falloff | **Resolved as recommended:** curve combo Stock / Shaped / Smooth plateau / Physical, default Shaped at Softness 1; positive out-of-range falloff value decoded in `calcLegacyDistanceAttenuation`; legacy-material alpha included via `alphaF.glsl` (§8.3). |
| O4 | Dynamic exposure | **Resolved:** labels and ranges as proposed in §8.2 (now in §3). |
| O5 | Expert settings with no existing UI | **Closed** (decided 2026-10-25): none are surfaced. The safe ones are held by Use KS Reference (§12.1, §12.5); glow internals and HDRI are not in the KStorm UI; Eye mask radius gets a control in AyaneStorm's My Lights (A11). |
| O6 | NaN guard for Haze Density 0 | **Done:** committed `2c8a7e9d438` on `fix-haze-zero-nan-guards`; not merged, not built (the shader validator named in AGENTS.md is not in the repo). |
| O7 | "As-is" ranges | **Resolved:** Sunlight Gain 0–20 / 0.01 (KS); Max nearby lights 0–512. |
| O8 | Transition time | **Resolved:** kept at 0 (instant), not exposed (R11). Effect: seconds to blend when a sky, water or day cycle is picked manually (inventory apply, World > Environment presets, preset combos); not used for region/parcel changes or day-cycle progression (`llenvironment.cpp:1391-1395`). |
| O9 | Trackball | **Resolved:** removed from the new UI. |
| O10 | Glow Size label | **Resolved:** Haze Glow Strength. |
| O11 | Sky HDR Scale on legacy skies | **Resolved:** on a legacy (auto-adjusted) sky the slider edits `RenderSkyAutoAdjustHDRScale` (§3). |
| O12 | Keeping forced settings on | **Resolved:** reapply at startup and after preset loads and quality changes (R12). |
| O13 | Local lights ×0.9 under legacy skies | **Resolved:** keep LL's behaviour (R13). |
| O14 | Depth of field controls | **Resolved:** our Depth of Field floater (`as_depth_of_field`) gains the 11 DoF and camera controls it lacks, from Advanced Lighting, Phototools, Advanced Phototools and Graphics prefs: `CameraFNumber`, `CameraFocalLength`, `CameraFieldOfView`, `CameraFocusTransitionTime`, `CameraMaxCoF`, `CameraDoFResScale`, `FSFocusPointFollowsPointer`, `FSFocusPointLocked`, `FSFocusPointRender`, `RenderDepthOfFieldInEditMode` (dropped 2026-10-25: not in the KStorm UI), `CameraAngle` (zoom). It already has `RenderDepthOfField` and `ASDepthOfFieldMode`. Implementation is a change to `floater_as_depth_of_field.xml`. The Camera tab (§5A.1) also carries them built in (R15 allows built-in controls). |
| O15 | Viewer-only light overrides on any object | **Future** (decided 2026-10-25; §12.3 A12). Let the lighting UI change, hide or add lights on any object, with or without modify rights, in your view only. See §8.5. |
| O16 | Graphics preset save/load as a lighting preset | **Superseded by Lighting Presets** (§12.6, decided 2026-10-25). Quality (`RenderQualityPerformance`) and graphics presets, including Save preset… and Load preset…, are not surfaced in the KStorm lighting UI (decided 2026-10-25). The question to research: what a graphics preset actually saves and restores, and what a complete preset of the lighting controls in this design would need that a preset leaves out (for example environment and sky settings, Viewer Lights, per-sky values, settings outside the preset's list). |

---

## 8. Resolution notes

### 8.1 Sky Ambient Gain: physical units (O1)

There is no physical reference unit. The sky `ambient` field is a unitless RGB triple (validated 0–3 per channel). On its way to the screen it passes through non-photometric steps: cloud boost, `pow(x, 0.9) × 0.57`, sRGB-to-linear, reduction to Rec.709 luminance, × `RenderSkyAmbientScale` (`atmosphericsFuncs.glsl:113-164`), then exposure and tonemapping. The renderer works in arbitrary scene-referred units; the only absolute number in the chain is the 11.2 ceiling in `clampHDRRange` (`deferredUtil.glsl:83-93`), a safety clamp, not a calibration. No lux, nits or EV mapping exists.

Proposal: Sky Ambient Gain = Rec.709 luminance of the stored color (the weights the shader uses), 0–3 / 0.01. It scales all three channels together and stops before any channel passes 3. If the stored color is black, writing a value sets neutral gray of that luminance.

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

KS Sunlight Gain is 0–20 / 0.01 (Advanced Lighting is 0–50 / 0.1); the new UI uses 0–20. Max nearby lights 0–512.


### 8.5 Viewer-only light overrides (O15)

**Today.** The Selected object block (§5.3) is a Build tool. The light setters (`LLVOVolume::setLightIntensity`, `setLightRadius`, `setLightFalloff`, `setLightLinearColor`, `setIsLight`; `llvovolume.cpp:3343-3440`) write the object's `LLLightParams` and call `parameterChanged(PARAMS_LIGHT, true)`, which sends the change to the server. Without modify rights the server refuses it, and the next object update restores the original values.

**Precedent.** Viewer Lights already override rendering per object ID without touching the object: `ASLightRigRenderer::getLightBoost` and `isDarkLight` are looked up by `LLVOVolume` ID in the light loops (`pipeline.cpp:6712`, `6726`, `10170`, `10202`, `10284`, `10364`, `10761`), and `allowsLight` filters by ID for Only Viewer Lights (`aslightrigrenderer.cpp:124`).

**What overrides would cover.**

- *Adjust an existing server light:* override intensity, color, radius and falloff by object ID, applied where Boost is applied now. The object's own parameters are never written. No `ll*` changes beyond the existing `<KS:KS>` hooks.
- *Hide one server light:* a per-object form of Only Viewer Lights, at the same filter point.
- *Make a non-light object a light.* Two routes:
  1. **Anchor a Viewer Light to the object (recommended).** A viewer-only light, using the existing Viewer Lights code (`aslightrig.cpp`, `createObjectViewer`), follows the object's render position each frame. No `ll*` changes, and it gets the curve, spot and Dark features for free.
  2. **Fake the object's own light parameters.** Override `LLVOVolume::getIsLight()` and the light getters, and register the drawable with `gPipeline.setLight`. This needs several edits in `llvovolume.cpp`, and the update path (`llvovolume.cpp:4753`) recomputes `is_light` from the server's parameters whenever the object is updated, so it must also consult the override.

**Caveats.**

- **Persistence.** Object IDs change on re-rez and copy, and attachments get a new ID every time they are worn or at login. Overrides are session-only unless worn items are keyed differently (for example avatar plus attachment point).
- **Only you see the result.** The scene others see differs from yours.
- **Reflection probes** capture these lights too, so overrides appear in your reflections.
- **UI consequence.** Selected object is no longer in the lighting UI (2026-10-25). If O15 is adopted, it returns as a lighting tool: Selected object becomes a lighting tool that needs no modify rights. Editing the real object stays in Build › Features and leaves the lighting UI.

---

## 9. UI approaches

**Current direction (v4 mock-up, decided 2026-10-25): a single-pane launch list.** The KStorm Lighting floater holds no controls and no accordions, only one button per section; each button opens that section as its own floater. Order: Quick; Sun/Moon, Ambient, Haze, Atmosphere, Clouds, Water, Reflections, Environment Effects; All lights, Viewer Lights, Shadows, Occlusion, Exposure, Material glow; Depth of field, Vignette, Camera Effects. Environment Effects, Viewer Lights and Camera Effects open the AyaneStorm floaters (`as_environment_effects`, `as_my_lights`, `as_camera_effects`). The section tables in §2–§6 still describe the controls; their tab grouping is superseded by this list.

**Compact mode (mocked 2026-10-25, not yet decided).** Each list button carries a 16×16 icon. A toggle in the title bar switches between the full list (icon, label, status; 225 px) and an icon strip (about 44 px) with the name as the hover tooltip and no footer. XUI: buttons with `image_overlay` (left-aligned) plus a status `text`; compact mode hides the labels and status texts and reshapes the floater; the choice can persist in a setting. The bottom and side toolbars already offer the same pair of modes (`LLToolBar` icons-with-text / icons-only, `lltoolbar.h:122`); registering each pane as a command in `commands.xml` would also let users drag panes onto a toolbar. Icons are placeholders in the mock-up; real ones are needed (some can reuse existing skin textures).

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
| A12 | RLV and scripts | `@setenv` lets scripts drive the environment; show "locked by RLV" and gray out affected controls (the viewer already disables environment floaters under RLV). |

Highest value for portrait and photo work: A1–A2, A3 (per section) and A5.

Layout variant under evaluation (2026-10-24): accordions replaced by buttons that open each section as its own floater (mock-up `v3-floaters.html`, kept local and outside the repository).

---

## 10. As designed (v4, 2026-10-25)

**This section is the implementation reference.** It is generated from the v4 mock-up (`v4-list.html`, kept local and outside the repository) after all its regrouping. §2–§6 and their dated notes remain as the decision history; where they differ from this section, this section wins.

**Launch floater "KStorm Lighting".** No controls, only one button per pane; each opens its pane as its own movable floater. Each button shows a 16×16 icon, its label and a short status. A title-bar toggle switches between the full list (225 px) and an icon strip (about 44 px; the name is the hover tooltip). **For now (decided 2026-10-26) the launcher is always the icon strip and the toggle is hidden**; the full list and its toggle stay in the code (A10). **No footer** (removed 2026-10-26: the icon strip has no room for it). Sky origin and Probe Fill in use are shown on Sun/Moon and Ambient; Use Shared is on Quick. Exposure's list status shows the Exposure value only (A8). Opened by a toolbar button (icon TBD), the **KStorm › KStorm Lighting** menu item, and its hotkey **Alt+Shift+L** (A13; free in every menu and in code).

**Tooltip suffixes.** Every control Use KS Reference writes ends its tooltip with "(KS Reference)" (A3): Sun azimuth and elevation, Moon azimuth and elevation, Sun & Moon Light, Sky Ambient Gain, Haze Density, Haze Glow Brightness, Strength and Focus, Sky HDR Scale, Probe Fill Strength, Show avatar names, Subject sort, Dynamic exposure. Exposure, Dynamic exposure, Tone mapping, Tone mapping mix and Glow end with "No effect while Build › Options › No Post-processing is on and the Build floater is open." (D3).

Order: Quick; Sun/Moon, Atmosphere, Clouds, Water, Reflections, Environment Effects; All lights, Viewer Lights, Shadows, Exposure, Material glow; Depth of field, Vignette, Camera Effects. The Ambient pane is not in the list; Quick › Ambient occlusion › **Settings…** opens it.

Bindings: `sky …` = a value of the current sky, written through `KSEnvironment::getEditableSky()` / `commitSky()` (§11.2); `water …` = a value of the current water; other names are viewer settings (`control_name`).

**Toggles and their dependents (2026-10-27).** A control that has no effect while a toggle on its pane is off sits directly below that toggle and is dimmed (disabled) while it is off. The rule follows the value being drawn, so it also dims what Motion mode (§12.8) or KS Reference turned off. Dimming only reads settings: KS Reference, Motion mode and lighting presets (groups, captures, restore points) are unaffected, and a dimmed control still shows a value a preset applies. The list is `DEPENDENCIES` in `ksfloaterlightingpane.cpp` (child name → setting and minimum value; several entries must all hold); Hair Occlusion, a plain floater, uses `enabled_control="KSHairOcclusionEnabled"`. Each pane's dependents are in its *Enabling* note below.

#### 1. Quick

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| My Environments… | `Floater.Toggle my_environments` | floater button | | (Environment Effects… removed from Quick, A6) |
| Use Shared | Use Shared Environment (`World.EnvSettings` "region": clear ENV_LOCAL, instant, probe reset) | button | | Also turns Use KS Reference off (A4). |
| Dynamic exposure | `RenderDynamicExposureEnabled` | check box | true | Adapts exposure to scene brightness. Off and disabled while Use KS Reference is on. |
| Use KS Reference | `KSSetEnvironment` | check box, same row as Dynamic exposure | false | Sets Sky HDR Scale 0.1 (pins auto-exposure), Probe Fill Strength 0.01, Haze Density 0.1. |
| Exposure | `RenderExposure` | 0.5–4 / 0.1 | 1 | Overall scene exposure. |
| Shadows | `RenderShadowDetail` | None; Sun & Moon; Sun & Moon + Projectors | Sun & Moon + Projectors | |
| Sun azimuth | `sky sun_rotation` | 0–359.99 / 0.01 | 180 | |
| Sun elevation | `sky sun_rotation` | -90–90 / 0.01 | 45 | |
| Sunlight Gain | `RenderHDRSkySunlightScale` | 0–20 / 0.01 | 1 | Brightness of direct sun and moon light. (Renamed from Sunlight Scale 2026-10-25, from Sunlight Luminance 2026-10-26.) |
| Surface Ambient Gain | `RenderSkyAmbientScale` | -10–40 / 0.1 | 1 | Multiplies ambient fill on surfaces only, so it shows most up close. Does not change the sky, clouds, haze or water. At Probe Fill Strength 1 or more it no longer lights surfaces. Negative values darken lit areas. Loading a graphics preset resets it to 1. (Added 2026-10-25; renamed from Ambient Luminance 2026-10-26.) |
| Haze Density | `sky haze_density` | 0–5 / 0.01 | 0.7 | At 0, Haze Glow Strength, Haze Glow Focus and Haze Glow Brightness stop working, fog takes Air Glow Color only, and distance fog depends on Air Density's red channel. |
| Sky HDR Scale | `sky gamma` (EEP) / `RenderSkyAutoAdjustHDRScale` (legacy) | 0–20 / 0.01 | 1 | Brightness of sky and haze relative to the scene; also sets how far auto-exposure can adjust. At 0.25 or below, auto-exposure is fixed at 1. |
| Ambient occlusion + **Settings…** (opens Ambient) + **Hair…** | `RenderDeferredSSAO`; Hair… opens `ks_hair_occlusion` | check box, two buttons on its row | true | Hair… added 2026-10-26, matching Shadows. |
| Probe Fill Strength | `sky reflection_probe_ambiance` | 0.01–10 / 0.01 | 0.01 | <1: fill from sky ambient (uniform). >=1: fill from probes (bounce light, darker interiors). At a probe fill of 1 or more, probe irradiance fully replaces ambient wherever probes cover the surface. Reflections are unaffected. (Ambient Brightness moved to Sun/Moon as Sky Ambient Gain 2026-10-26.) |
| *note* | | | | Auto-exposure pinned at 1 (Sky HDR Scale ≤ 0.25). |
| Show avatars | render type `character` (`Advanced.ToggleRenderType`) | check box | true | Shows or hides all avatars. |
| Show avatar names | `AvatarNameTagMode` | check box | true | Name tags over avatars. |

#### 2. Sun/Moon

Row order (2026-10-26): Surface Ambient Gain; sky; sun; moon; star. Groups are not labeled.

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Surface Ambient Gain | `RenderSkyAmbientScale` | -10–40 / 0.1 | 1 | Multiplies ambient fill on surfaces only, so it shows most up close. Does not change the sky, clouds, haze or water. At Probe Fill Strength 1 or more it no longer lights surfaces. Negative values darken lit areas. Loading a graphics preset resets it to 1. (Added 2026-10-25; renamed from Ambient Luminance 2026-10-26.) |
| Sky Ambient Gain | `sky ambient` (Rec.709 luminance, hue kept) | 0–3 / 0.01 | 0.25 | Brightness of the sky's ambient color, saved with the sky (hue kept). Reaches the sky, clouds, haze and surface fill; water only through reflected sky. Surfaces use its brightness only. At Probe Fill Strength 1 or more it no longer lights surfaces. (Was Ambient Brightness on Quick, Shadows and Ambient; moved here 2026-10-26 as Sky Brightness, renamed the same day.) |
| Sky origin | read-only | "EEP" / "Legacy (auto-adjusted)" | | |
| Sun & Moon Light | `sky sunlight_color` | color + intensity 0–3 | #d0ddff × 0.9 | Light color of both the sun and the moon (color + intensity 0–3). |
| Sunlight Gain | `RenderHDRSkySunlightScale` | 0–20 / 0.01 | 1 | Brightness of direct sun and moon light. (Renamed from Sunlight Scale 2026-10-25, from Sunlight Luminance 2026-10-26.) |
| Sun azimuth | `sky sun_rotation` | 0–359.99 / 0.01 | 180 | |
| Sun elevation | `sky sun_rotation` | -90–90 / 0.01 | 45 | |
| Sun scale | `sky sun_scale` | 0.25–20 / 0.01 | 1 | Larger discs fade out later at the horizon. |
| Moon azimuth | `sky moon_rotation` | 0–359.99 / 0.01 | 0 | |
| Moon elevation | `sky moon_rotation` | -90–90 / 0.01 | -45 | |
| Moon scale | `sky moon_scale` | 0.25–20 / 0.01 | 1 | Larger discs fade out later at the horizon. |
| Moon Disk Brightness | `sky moon_brightness` | 0–1 / 0.01 | 0.5 | Brightness of the moon disc and night glow. Does not change moonlight on surfaces. |
| Partial moon below horizon | `ASRenderPartialMoonBelowHorizon` | check box | true | |
| Star brightness | `sky star_brightness` | 0–500 / 0.01 | 250 | |

#### 3. Atmosphere

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Atmosphere Height (m) | `sky max_y` | 0–10000 / 0.01 | 1605 | Height of the atmosphere: limits fog, haze and sun dimming. |
| Atmosphere Thickness | `sky density_multiplier` (stored = display × 0.001) | 0.0001–2 / 0.0001 | 0.1 | Scales haze thickness and sun extinction. |
| Fog Distance Scale | `sky distance_multiplier` | 0.05–1000 / 0.01 | 0.8 | Scales how fast distance fog builds. |
| Atmospheric pass | `RenderDeferredAtmospheric` | check box | true | Haze on scene geometry. |
| Moisture | `sky moisture_level` | 0–1 / 0.001 | 0 | Rainbow and halo only. |
| Droplet radius | `sky droplet_radius` | 5–1000 / 0.01 | 800 | |
| Ice | `sky ice_level` | 0–1 / 0.001 | 0 | |
| Haze Density | `sky haze_density` | 0–5 / 0.01 | 0.7 | (as Quick) |
| Haze Glow Brightness | `sky haze_horizon` | 0–5 / 0.01 | 0.19 | Brightness of white haze and its sun glow. |
| Haze Glow Strength | `sky glow.x` = (2 − v) · 20 | 0–1.99 / 0.01 | 1.75 | Strength of the haze glow around the sun; higher = brighter. Does not change its spread. |
| Haze Glow Focus | `sky glow.z` = v · −5 | -2–2 / 0.01 | 0.5 | Tightness of the haze glow; higher = tighter halo. |
| *note* | | | | Glow controls have no effect while Haze Density is 0. |
| Air Glow Color | `sky blue_horizon` | color + intensity 0–3 | #c6c6ff × 0.64 | Sky and distant-fog color from clear air. |
| *note* | | | | Sole fog color while Haze Density is 0. |
| Air Density | `sky blue_density` | color + intensity 0–3 | #5297ff × 0.76 | Clear-air amount per color: sunset reddening, fog fade rate and the air/haze mix. |
| *note* | | | | Fog strength follows the red component while Haze Density is 0. |

#### 4. Clouds

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Cloud color | `sky cloud_color` | color | #a8a8a8 | |
| Coverage | `sky cloud_shadow` | 0–1 / 0.01 | 0.27 | Also removes sun and adds fill. |
| Scale | `sky cloud_scale` | 0.01–3 / 0.01 | 0.42 | |
| Variance | `sky cloud_variance` | 0–1 / 0.01 | 0 | |
| Density X / Y / D | `sky cloud_pos_density1` x / y / z | 0–1, 0–1, 0–3 / 0.01 | 1, 0.53, 1 | |
| Detail X / Y / D | `sky cloud_pos_density2` x / y / z | 0–1 / 0.01 | 1, 0.53, 0.12 | |
| Scroll, map | XY pad, texture picker | | | |

#### 5. Water

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Water fog color | `water fog color` | color | #0f3355 | Tints what is seen below the water surface. |
| Water fog density | `water fog density` | 0.001–100 / 0.01 | 2.7 | How quickly things below the water surface fade. |
| Underwater fog multiplier | `water underwater fog mod` | 0–20 / 0.01 | 0.25 | Multiplies water fog density while the camera is underwater. |
| Fresnel scale / offset | `water fresnel scale` / `offset` | 0–1 / 0.01 | 0.4 / 0.5 | |
| Normal scale X / Y / Z | `water normal scale` | 0–10 / 0.01 | 2 | |
| Blur | `water blur multiplier` | 0–0.5 / 0.001 | 0.04 | |
| Transparent water | `RenderTransparentWater` | check box | true | |
| Refraction above / below | `water scale above` / `below` | 0–3 / 0.01 | 0.03 / 0.2 | |
| Normal map, waves | texture picker, 2 XY pads | | | |

*Enabling:* Refraction above and below are dimmed while Transparent water is off: opaque water draws no refraction (`waterF.glsl`, `underWaterF.glsl`; above still nudges where reflections are sampled, too little to tune).

#### 6. Reflections

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Coverage | `RenderReflectionProbeLevel` | Sky only; Manual only; Manual + terrain; Full | Full | |
| Probe detail | `RenderReflectionProbeDetail` | Static; Dynamic; Realtime | Dynamic | |
| Screen-space reflections | `RenderScreenSpaceReflections` | check box | false | |
| Probe count | `RenderReflectionProbeCount` | 32; 64; 128; 256 | 256 | |
| Probe resolution | `RenderReflectionProbeResolution` | 64; 128; 256; 512 | 128 | |
| Probe draw distance | `RenderReflectionProbeDrawDistance` | 0–4096 / 1 | 64 | |
| Mirrors | `RenderMirrors` | check box | false | |
| Hero resolution | `RenderHeroProbeResolution` | 256; 512; 1024; 2048 | 1024 | |
| Hero update rate | `RenderHeroProbeUpdateRate` | Every frame; Every 2 frames; Every 3 frames | Every 2 frames | |
| Show probe volumes | `RenderReflectionProbeVolumes` | check box | false | |

*Enabling:* Hero resolution and Hero update rate are dimmed while Mirrors is off: hero probes draw mirrors only.

#### 7. Environment Effects — opens `as_environment_effects`

AyaneStorm floater: Volumetric Lighting, Camera Effects, Color Grading, Sun, Horizon, Moon, Stars, Auroras, Weather. Sun/moon god rays live here (Volumetric Lighting).

#### 8. All lights

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Scope | read-only | | | Your view only: how server lights and Viewer Lights are rendered for you. |
| Max nearby lights | `RenderLocalLightCount` (global) | 0–512 / 1 | 256 | Counts nearby candidates, not only visible lights. Shared with Preferences and Viewer Lights. |
| Only Viewer Lights | `KSLightRigOnlyMyLights` | check box | false | Hides every server light; only Viewer Lights light the scene. |
| Worn lights | `RenderAttachedLights` | check box | true | Lights in attachments, yours and other people's. Off hides them all in your view only; others still see them. |
| Subject sort | `KSRenderLocalLightSubjectSort` | check box | false | Prioritize lights near the Alt-click focus. |
| Alpha masks lit as opaque | `RenderAutoMaskAlphaDeferred` | check box | true | Cut-out alpha (leaves, lace, hair tips) is drawn as opaque, so it gets every light, shadows and occlusion, and Viewer Lights spot cones. |
| Fullbright alpha masks as opaque | `RenderAutoMaskAlphaNonDeferred` | check box | false | The same for fullbright faces, which are not lit by the deferred pass. |
| Light bounding boxes | info display "lights" | check box | false | Boxes around every light object, server lights and Viewer Lights with the LLVOVolume backend (not the Shader backend). |
| Show selected light radius | `RenderLightRadius` | check box | false | Draws the reach of the light in the object selected with the Build tools, while they are open. |

*Enabling:* Worn lights is dimmed while Only Viewer Lights is on, which already hides worn lights.

#### 9. Viewer Lights — opens `as_my_lights`

AyaneStorm My Lights floater (viewer-only lights). Gains the Curve selector and an Eye mask radius control (§5.2, §8.3).

#### 10. Shadows

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Probe Fill Strength | (as Quick) | 0.01–10 / 0.01 | 0.01 | Same as Quick (A7). |
| Shadows | `RenderShadowDetail` | None; Sun & Moon; Sun & Moon + Projectors | Sun & Moon + Projectors | |
| Resolution | `RenderShadowResolutionScale` | 0–5 / 0.5 | 1 | |
| Clarity | `RenderShadowSplitExponent.y` | 0–10 / 0.025 | 3 | |
| Bias | `RenderShadowBias` | -0.002–0.001 / 0.000001 | -0.002 | |
| FOV cutoff | `RenderShadowFOVCutoff` | 0–100 / 0.1 | 0.8 | |
| Spot offset | `RenderSpotShadowOffset` | 0–0.04 / 0.0001 | 0.04 | |
| Ambient occlusion + **Settings…** (opens Ambient) + **Hair…** | `RenderDeferredSSAO`; Hair… = `Floater.Toggle ks_hair_occlusion` | check box, two buttons on its row | true | Same as Quick (A7); Hair… added 2026-10-25. |
| Soften | `RenderShadowGaussian.x` | 0.001–5 / 0.001 | 3 | |
| Blur size | `RenderShadowBlurSize` | 0–4 / 0.01 | 1.4 | |
| Blur distance | `RenderShadowBlurDistFactor` | 0–1 / 0.01 | 0 | |
| AO soften | `RenderShadowGaussian.y` | 0.001–3 / 0.001 | 2 | |

*Order and enabling (2026-10-27):* Probe Fill Strength, which depends on neither toggle, moved to the top. Resolution, Clarity, Bias and FOV cutoff are dimmed while Shadows is None; Spot offset unless Shadows is Sun & Moon + Projectors. The soften pass runs only while Ambient occlusion is on (`LLPipeline::renderDeferredLighting()`), so Soften, Blur size, Blur distance and AO soften sit below that check box and are dimmed while it is off; Soften also needs Shadows, and AO soften Legacy SSAO (GTAO bypasses the channel it blurs, `sunLightSSAOF.glsl`). With GTAO and no shadows, Blur size and distance stay enabled but have nothing to blur.

#### 11. Ambient (not in the list; opened from Quick › Ambient occlusion › Settings…)

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Probe Fill in use | read-only | | | |
| Surface Ambient Gain | `RenderSkyAmbientScale` | -10–40 / 0.1 | 1 | Multiplies ambient fill on surfaces only, so it shows most up close. Does not change the sky, clouds, haze or water. At Probe Fill Strength 1 or more it no longer lights surfaces. Negative values darken lit areas. (Renamed from Ambient Scale 2026-10-25, from Ambient Luminance 2026-10-26; also on Quick and at the top of Sun/Moon.) |
| Probe Fill Strength | `sky reflection_probe_ambiance` | 0.01–10 / 0.01 | 0.01 | (as Quick) |
| *note* | | | | Probes off: forced to 1.0, control disabled. |
| Ambient occlusion | `RenderDeferredSSAO` | check box | true | |
| Technique | `RenderAOTechnique` | Legacy SSAO; GTAO | GTAO | |
| Effect | `RenderSSAOEffect.x` | -1–1 / 0.01 | 0.8 | One control for the active technique. |
| White-material check | `RenderAODebugWhite` | check box | false | |
| Irradiance scale | `RenderSSAOIrradianceScale` | 0–2 / 0.01 | 0.6 | |
| Irradiance max | `RenderSSAOIrradianceMax` | 0–1 / 0.001 | 0.18 | |
| SSAO scale / max scale / factor | `RenderSSAOScale` / `RenderSSAOMaxScale` / `RenderSSAOFactor` | 0–10000 / 1; 0–10000 / 1; 0–250 / 1 | 500 / 200 / 0.3 | Legacy SSAO only. |
| GTAO quality | `RenderGTAOQuality` | Low; Medium; High; Ultra | Ultra | GTAO only. |
| GTAO radius | `RenderGTAORadius` | 0.05–5 / 0.05 | 1.7 | GTAO only. |
| GTAO power | `RenderGTAOFinalValuePower` | 0.5–5 / 0.05 | 2.2 | GTAO only. |
| GTAO denoise | `RenderGTAODenoisePasses` | 0; 1; 2; 3 | 3 | GTAO only. |
| GTAO bent normals | `RenderGTAOBentNormals` | check box | false | GTAO only. |
| Bent normal influence | `RenderGTAOBentNormalInfluence` | 0–1 / 0.01 | 1 | GTAO only. |

*Enabling:* everything below Ambient occlusion is dimmed while it is off; the SSAO rows also while Technique is GTAO, the GTAO rows while it is Legacy SSAO.

#### 12. Exposure

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Sky HDR Scale | (as Quick) | 0–20 / 0.01 | 1 | (as Quick) |
| Exposure | `RenderExposure` | 0.5–4 / 0.1 | 1 | Overall scene exposure. |
| Tone mapping | `RenderTonemapType` | Khronos Neutral; ACES | ACES | |
| Tone mapping mix | `RenderTonemapMix` | 0–1 / 0.05 | 0.7 | |
| Use sky exposure range | `RenderUseExposureSkySettings` | check box | false | Uses the sky's own exposure range (HDR offset ± min/max) instead of Sky HDR Scale. With Dynamic exposure off, exposure is held at the sky's HDR offset instead of 1. (Corrected 2026-10-25: was "fixed 0.5–3".) |
| Dynamic exposure | `RenderDynamicExposureEnabled` | check box | true | (as Quick); the four sliders below work only while it is on. |
| *note* | | | | Auto-exposure pinned at 1 (Sky HDR Scale ≤ 0.25). |
| Bright-scene level | `RenderDynamicExposureCoefficient` | 0.05–2 / 0.01 | 0.5 | Scene luminance treated as fully bright; lower = darkens sooner. |
| Settle accuracy | `RenderDynamicExposureSpeedError` | 0.01–0.5 / 0.01 | 0.1 | Fraction of the gap left after the settle time. |
| Settle time (s) | `RenderDynamicExposureSpeedTarget` | 0.1–10 / 0.1 | 2 | Time to get within that fraction. |
| Object weight | `RenderDiffuseLuminanceScale` | 0–2 / 0.05 | 0.5 | Weight of scene objects vs sky in metering. |

*Order and enabling (2026-10-27):* Dynamic exposure sits below the tone mapping rows, with Use sky exposure range just above it. Bright-scene level, Settle accuracy, Settle time and Object weight (child names `ks_dyn_*`) are dimmed while Dynamic exposure is off: `LLPipeline::generateExposure()` reads them only while it is on.

#### 13. Material glow

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Glow | `RenderGlow` | check box | true | Material glow (not haze glow). |
| Glow strength | `RenderGlowStrength` | 0–0.5 / 0.001 | 0.325 | |
| Glow width | `RenderGlowWidth` | 0–50 / 0.001 | 1.3 | |
| Glow resolution | `RenderGlowResolutionPow` | 8–10 / 1 | 9 | |
| Glow iterations | `RenderGlowIterations` | 0–200 / 1 | 2 | Blur rounds: higher is wider and smoother, and each round costs two passes. Preferences calls this Strength (0-3). (KS Reference) |

*Enabling:* the four sliders are dimmed while Glow is off.

#### 14. Depth of field

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Zoom | `CameraAngle` | 0.08–3.06 / 0.002 | 1.047 | |
| Depth of field | `RenderDepthOfField` | check box | false | Blurs the view except at the camera focus. |
| Focus follows pointer | `FSFocusPointFollowsPointer` | check box | false | |
| Focus locked (Alt-Shift-X) | `FSFocusPointLocked` | check box | false | |
| Show focus point | `FSFocusPointRender` | check box | false | |
| DoF mode | `ASDepthOfFieldMode`; items copied from AyaneStorm's `dof_mode` combo | currently Standard / Firestorm; Aperture-sampled (still images only); Live / AyaneStorm (new) | Live | Same list as the AyaneStorm Depth of Field floater. |
| F-number | `CameraFNumber` | 0.01–22 / 0.01 | 9 | |
| Focal length (mm) | `CameraFocalLength` | 1–300 / 1 | 50 | |
| Field of view (deg) | `CameraFieldOfView` | 0.1–180 / 0.1 | 60 | |
| Max blur (CoC) | `CameraMaxCoF` | 0–150 / 0.01 | 10 | |
| DoF resolution | `CameraDoFResScale` | 0–1 / 0.01 | 0.7 | |
| Depth of Field… | `Floater.Toggle as_depth_of_field` | floater button | | |

*Order and enabling (2026-10-27):* Zoom, the camera's own field of view, moved above Depth of field; everything between Depth of field and the Depth of Field… button is dimmed while it is off (Field of view only feeds the depth of field calculation).

#### 15. Vignette

| Label | Binding | Range / type | Default | Tooltip / note |
|---|---|---|---|---|
| Sharpening | `RenderCASSharpness` | 0–1 / 0.001 | 0.4 | |
| FS vignette amount | `FSRenderVignette.x` | 0–1 / 0.05 | 0 | |
| FS vignette power | `FSRenderVignette.y` | 0–40 / 0.1 | 1 | |
| FS vignette multiplier | `FSRenderVignette.z` | 0–8 / 0.1 | 1 | |

#### 16. Camera Effects — opens `as_camera_effects`

AyaneStorm floater: diffuse glow, lens flare, vignette, chromatic aberration.

---

## 11. Implementation plan (draft, 2026-10-25; incorporates the Advisor review)

Nothing here is built. Each branch is cut from `kstorm`, one feature per branch; the user builds and tests each. `ll*` / `fs*` edits stay minimal and `<KS:KS>`-tagged.

### 11.1 Architecture

- **Launch floater** `ks_lighting` (`floater_ks_lighting.xml`, class `KSFloaterLighting`): a `layout_stack` with a scroll list of rows (button with `image_overlay` icon, left-aligned, plus a status `text`). Compact toggle hides labels and statuses and reshapes the width; the choice persists in a new setting `KSLightingCompact`. For now `postBuild()` always calls `setCompact(true)` and the toggle (`ks_mode`) is `visible="false"` (A10). Statuses refresh from settings signals and from `LLEnvironment::setEnvironmentChanged` (pattern: `llfloaterenvironmentadjust.cpp:136`), not every frame.
- **Panes:** 13 floaters of our own (Quick, Sun/Moon, Atmosphere, Clouds, Water, Reflections, All lights, Shadows, Exposure, Material glow, Depth of field, Vignette, Ambient), named `ks_lighting_<pane>`. One generic class `KSFloaterLightingPane` driven by its XML:
  - viewer settings bind with `control_name` (no code);
  - sky and water values use a naming convention on the widget `name` (for example `sky_haze_density`) that the class maps to `LLSettingsSky` / `LLSettingsWater` getters and setters;
  - special controls get small handlers: Sky Ambient Gain (luminance edit with hue kept, §8.1), Sun & Moon Light and the air colors (color + intensity), sun and moon azimuth / elevation (quaternions, as `rotationFromDegrees` in `ksenvironment.cpp`), Haze Glow Strength and Focus (`glow` mapping), Atmosphere Thickness (× 0.001), Sky HDR Scale (sky `gamma` for EEP, `RenderSkyAutoAdjustHDRScale` for a legacy sky).
- **Launches** of existing floaters: Environment Effects, Viewer Lights, Camera Effects, My Environments, Depth of Field…, Hair Occlusion… use `Floater.Toggle` (R15).
- **Registration:** `llviewerfloaterreg.cpp` gains one `<KS:KS>` line per floater, as the existing `ks_environment_effects` line (`llviewerfloaterreg.cpp:695`).
- **KStorm menu** (`menu_viewer.xml`, KStorm's own top-level menu): **KStorm Lighting** (`shortcut="alt|shift|L"`) replaces both current items. **Retire** KStorm Environment Effects (`ks_environment_effects`: its menu item, `floater_ks_environment_effects.xml`, `panel_ks_lighting.xml`, `kspanellighting.cpp/.h` and its registration line); every one of its controls has a home in this design. The **Hair Occlusion** menu item goes too; the floater opens from Shadows › Hair… (decided 2026-10-25).

### 11.2 Sky and water editing

All sky and water edits go through `KSEnvironment::getEditableSky()` / `commitSky()` (and a water equivalent). Like Personal Lighting, the first edit creates a local fixed environment: **editing while a shared day cycle is running freezes time locally** until Use Shared. Every open pane and the launch-list statuses rebind on the environment-changed signal, so a sky change from anywhere (Use Shared, Use KS Reference, My Environments, region change) shows at once.

Legacy skies: `LLSettingsSky::setReflectionProbeAmbiance` clears `mCanAutoAdjust` (`llsettingssky.cpp:1517`), so any probe-fill edit, including Use KS Reference's, turns the local copy into an EEP sky. After the rebind, Sky HDR Scale and Probe Fill show the sky's own values. This resolves the legacy-sky mismatch recorded in §2.1.

### 11.3 Controls that need code

- **Use Shared:** `clearEnvironment(ENV_LOCAL)`, instant `setSelectedEnvironment`, `mReflectionMapManager.reset()` — call the same `World.EnvSettings` "region" handler.
- **Show avatars:** `Advanced.ToggleRenderType` / `Advanced.CheckRenderType` with `character`.
- **DoF mode:** at `postBuild`, read `floater_as_depth_of_field.xml` with `LLUICtrlFactory::getLayeredXMLNode` and copy the `dof_mode` combo items (label, value).
- **Dynamic exposure lock:** disabled while `KSSetEnvironment` is on (enable state from the setting's signal).
- **Readouts:** Sky origin, Probe Fill in use.
- **Settings…** next to Ambient occlusion: `Floater.Show ks_lighting_ambient`.

### 11.4 Use KS Reference (`ksenvironment.cpp`)

Add `RenderDynamicExposureEnabled` handling: save, set false, restore on off. While it is on, reapply its values after a graphics-preset load or a quality-slider change (D1, D2). Use Shared, and applying an environment from My Environments, turn it off without restoring the sky (A4). Editing a value it wrote leaves it on; turning it off restores the pre-KS values (A3). Panes show every value it writes (list in §2.1) through the environment-changed rebind; the code disables nothing else. Open interactions are A3 and A4 in §12.

### 11.5 Branches, in order

| # | Branch | Content | Notes |
|---|---|---|---|
| B1 | `feat-ks-lighting-ui` | Launch floater, 13 panes, registration, icons (placeholders until A10) | Largest. New files are `ks*`; only registration touches `ll*`. |
| B2 | `feat-ks-env-dynexp` | §11.4 | Small; independent of B1. |
| B3 | `feat-my-lights-curve` | Curve selector in `floater_as_my_lights.xml`, rig and shader code per §8.3 | Eye mask radius pending A11. |
| B4 | `feat-local-light-325` | Converge local-light constants on 3.25 (forward 3.0 → 3.25), as decided earlier | Shader change. |
| B5 | merge `fix-haze-zero-nan-guards` | Already committed on its branch | Shaders unvalidated: glslang is not in the repository. Validate by a build and a run with Haze Density 0, or install `glslangValidator` and run it on the three files. |
| B6 | (optional) toolbar commands | Register panes in `commands.xml` so they can sit on toolbars | Only if A13 says so. |
| B7 | `feat-ks-lighting-presets` | Lighting Presets (§12.6): preset files, Lighting Presets pane and dialogs, apply with restore point, KS Reference as the built-in preset (absorbs B2's `ksenvironment.cpp` work and the §12.1 settings once A14 is approved) | After B1. Save to My Environments postponed. |

### 11.6 Verification per pane (user, after each build)

1. Every binding exists (`settings.xml`, sky / water keys); the UI range matches the code range (§8.4).
2. Tooltips and notes match §10.
3. Status text updates when values change from another floater.
4. Use KS Reference on: every surfaced value it writes shows; off: all restored; Dynamic exposure disabled only while on.
5. Use Shared returns to the shared sky instantly and the panes follow.
6. The launcher opens as the icon strip with no mode toggle, and each button's hover tooltip is its pane label (while A10 hides the full list; with the toggle back, the toggle keeps its state across a restart).

### 11.7 Corrections found in review

- Exposure › "Use fixed range" was wrong: `RenderUseExposureSkySettings` uses the sky's own HDR offset, min and max (`pipeline.cpp:8067-8080`). Relabelled "Use sky exposure range" with a corrected tooltip (§10, pane 12).
- The design doc's spelling pass (R16) changed prose only; no backticked identifier or XUI name was affected.
- **Step 3 (Quick, 2026-10-25):**
  - My Environments… uses `KS.CycleFloater` (open, raise, close) like every other launch button, not `Floater.Toggle`.
  - Show avatar names is handled in code, not `control_name`: `AvatarNameTagMode` is 0 or a display mode, and checking the box again restores the last mode instead of forcing 1.
  - Quick's "pinned at 1" note sits on its own last row; while hidden, the pane drops that row and shrinks (its top stays put).
  - Quick's list status reads "KS Reference" while Use KS Reference is on.
  - DoF pane gained its DoF mode row; the items are read at run time from the `dof_mode` combo in `floater_as_depth_of_field.xml`.
- **B2 content done in B1:** Use KS Reference now saves `RenderDynamicExposureEnabled`, sets it off and restores it; the Dynamic exposure check box (Quick and Exposure) is disabled while KS is on. `KSEnvironment::init()` declares the three code-only exposure settings.
- **A4 implemented** (`ksenvironment.cpp`): clearing the local environment (Quick › Use Shared, World › Use Shared Environment, Personal Lighting Reset) and applying an environment set without a version (My Environments, inventory, World presets) turn KS off with its viewer settings restored and the sky left alone. Edits by KS, Personal Lighting and the celestial panels carry their own versions and leave it on (A3). One exception: an unversioned change keeps KS on while the new local sky still holds the KS Sky HDR Scale, Haze Density and Probe Fill (Personal Lighting's cloud texture picker replaces the sky with such a copy). **Ruled 2026-10-26:** KS Reference stays on across water edits and water-only items (My Environments, inventory), since they leave the KS sky in place (`LLEnvironment::setEnvironment` keeps the local sky when only water is given).
- **Pane header bars (2026-10-26):** every KStorm Lighting pane (`floater_ks_lighting_<pane>.xml`, including Lighting Presets) sets `header_height="38"`, the height of a launcher row; the launcher, the preset dialog and AyaneStorm floaters keep the default 25 px. LLFloater's header stretch keeps every control where it was below the header; the title and close button stay at the top of the bar.
- **Pane docking (2026-10-26):** the usage model is a launcher at the left or right screen edge with panes open beside it, each top aligned with its button, so the pane titles read as a list next to the icons; clicking a pane's header (or, when it is covered, its button) brings it to the front. Dragging a pane opened from the launcher to within `KSLightingPaneSnapRadius` pixels (default 40; 0 turns it off) of its slot snaps it there: top aligned with its button, against the launcher's right edge when the pane is right of the launcher's center, else against its left edge. Dragging it further away breaks it off; it then moves freely. Built on LLView's own drag snapping: `KSFloaterLightingPane` and `KSFloaterLightingPresets` override the virtual `findSnapRect()` and ask `KSFloaterLighting::findPaneSnap()` for the slot; no LL code changes. Ambient (no launcher button) and AyaneStorm floaters do not dock.
- **Dock button (2026-10-26):** a square button (`Icon_Restore_Foreground`) in the launcher's header, where the hidden open/closed toggle was. When any docked pane is open it closes the open docked panes; otherwise it opens every docked pane in its slot, collapsed or expanded as it was when it last closed (`KSLightingPaneStates`; a pane never closed opens collapsed), from top to bottom, each in front of the one above it so all headers stay visible. Undocked panes are never touched. Docked panes and their side persist in `KSLightingDockedPanes` (`name:R` / `name:L`), updated as a pane is dragged into or out of its slot; reopening places a pane in its slot even if the launcher moved. While docked, the alignment is enforced: every frame the launcher puts each open docked pane back in its slot if anything moved it (the launcher being dragged, a screen resize, a size change), except while the pane itself is being dragged.
- **Docking side near a screen edge (2026-10-26):** when the launcher is closer to the right screen edge than a pane's full width, the pane docks (and the dock button reopens it) on the launcher's left; closer to the left edge, on its right. In between, each pane docks on the side the user chose, and that side is kept (`KSLightingDockedPanes`) through dock open and close and when the launcher moves back from an edge (`KSFloaterLighting::sideFor()`). Proxy headers follow the same rule.
- **Proxy headers (2026-10-26):** the three AyaneStorm floaters too large to dock (Environment Effects, Viewer Lights, Camera Effects) get header-only stand-ins, `KSFloaterLightingProxy` (`floater_ks_lighting_proxy.xml`, registered as `ks_lighting_proxy_env_effects`, `_viewer_lights`, `_camera_effects`), always docked left of the launcher in their buttons' slots and not draggable. The launcher button opens the proxy and the floater in front (or closes both when the floater is already in front); a click on the proxy's header opens the floater in front when it is closed, brings it forward when it is open behind other floaters, and closes it when it is in front (revised 2026-10-26); the dock button opens and closes the proxies with the docked panes. The AyaneStorm floaters themselves are not changed or moved.
- **Collapsed width (2026-10-26):** a collapsed pane, and every proxy header, is half its width (150 px for our 300 px panes), keeping the edge that faces the launcher, so docked panes stay against it and an expanded pane sticks out beyond the collapsed ones. The dock button keeps each pane's last state, collapsed or expanded. Each pane records its full size once, from XUI, at the end of `postBuild()` (`initPaneSize()`); every frame its size is set from its collapsed state (full, or a half-width header bar), so a stale size cannot persist. (Fix, 2026-10-26: the first version took the expanded size from the current rect, so one wrong size kept repeating through dock, redock and close.) A proxy header click never closes the proxy itself, only its floater. While a pane is docked, its minimize button collapses it to its header (or expands it again) instead of minimizing it; undocked, it minimizes as usual (`KSFloaterLightingPaneBase::setMinimized()`).
- **Closing the launcher (2026-10-26)** by its close button, Alt+Shift+L, the menu or the toolbar button hides every open docked pane and proxy header (the dock button's state is kept per pane); undocked panes and the AyaneStorm floaters stay as they are.
- **Collapsing panes (2026-10-26):** a click on a pane's header bar (no visible button; not a drag, not the close or minimize button) collapses the pane to its header or expands it again, when the pane is in front. Behind other floaters, a collapsed pane expands and comes forward on the same click; an expanded one only comes forward. Launcher buttons for our panes now toggle between fully open (expanded, placed in its slot when docked, in front) and closed; AyaneStorm floaters keep open / bring forward / close. The dock button reopens docked panes as they were when they closed (collapsed or expanded). A collapsed pane expands before it closes, so its saved size stays the full one. Code: `KSFloaterLightingPaneBase` (in `ksfloaterlightingpane.h`), shared by `KSFloaterLightingPane` and `KSFloaterLightingPresets`; the collapsed pane keeps its controls in place and clips them with `LLLocalClipRect`.
- **KS Reference shadow resolution (2026-10-26):** `RenderShadowResolutionScale` = 1.5 while KS Reference is on (the prior value is restored when it is turned off). The scale multiplies the screen size: four sun/moon maps of screen size × scale and two square projector maps of screen width × scale (`LLPipeline::allocateShadowBuffer()`, `pipeline.cpp:1173`). At 3840 × 2160: 1.0 = 3840 × 2160 sun maps, about 0.25 GB of shadow maps (the same maps as 2.0 at 1080p); 1.5 = 5760 × 3240, about 0.56 GB, visibly crisper at 4K; 2.0 = 7680 × 4320, about 1 GB and four times the shadow-pass pixels of 1.0; 3.0 and above approach GPU texture limits for little visible gain. Clarity (`RenderShadowSplitExponent`) and Soften (`RenderShadowGaussian`) often change perceived sharpness more than the scale.
- **KS Reference background texture discard (2026-10-26):** `TextureDiscardBackgroundedTime` = 0 while KS Reference is on (default 60 s; the prior value is restored when it is turned off). At the default, after 60 s unfocused the viewer forces discard bias 5 on every texture (`llviewertexture.cpp` ~614); on refocus all textures reload from cache and re-decode, causing blur and FPS dips. 0 disables it; the viewer keeps its texture memory while in the background. Preferences > Graphics shows the same setting.
- **KS Reference glow iterations (2026-10-27):** `RenderGlowIterations` = 3 while KS Reference is on (default 2; the prior value is restored when it is turned off). Each iteration is a horizontal and a vertical blur pass over the glow buffer (`pipeline.cpp:8309`), so cost grows with the count; 3 is the most Preferences' "Strength" slider (the same setting) allows. Our Glow iterations slider still goes to 200.
- **Probe fill never 0 (2026-10-26):** a sky at probe ambiance 0 takes LL's classic path (legacy gamma, no post-processing), which turns off Probe Fill, Exposure and tone mapping. Two changes keep every sky at 0.01 or more: (1) `RenderSkyAutoAdjustLegacy` defaults to TRUE in KStorm (`settings.xml`, tagged), so legacy skies use `RenderSkyAutoAdjustProbeAmbiance` (0.01) and LL's conversion; Firestorm resets it to its default after a graphics preset load, which now means TRUE. (2) `LLSettingsSky::getReflectionProbeAmbiance()` returns at least `LLSettingsSky::sMinProbeAmbiance`, fed from the new setting `KSMinProbeAmbiance` (0.01; 0 restores LL behavior) in `llenvironment.cpp`, so skies saved at 0 (EEP items, region and parcel skies, other editors) render at 0.01. The stored value is not changed; the sky editor and Personal Lighting show 0.01 for such skies.
- **Findings for B7 (D1, D2):** a graphics preset load sends no event of its own (`LLPresetsManager::loadPreset`); `PresetGraphicActive` signals only when the name changes; the load resets `RenderSkyAutoAdjustLegacy` and `RenderSkyAmbientScale` to their defaults. The quality slider sets `RenderQualityPerformance` after the feature settings. Reapply on the next idle after either signal, writing only differing values.

### 11.8 Test scenarios: alpha masks lit as opaque (2026-10-25)

`RenderAutoMaskAlphaDeferred` (default on) and `RenderAutoMaskAlphaNonDeferred` (default off) decide whether a qualifying face is drawn as an **alpha mask** (opaque, deferred) or **alpha-blended** (forward pass). A face qualifies only if its texture is effectively cut-out (`getIsAlphaMask`), face transparency is 0, glow is 0, its material is not in alpha-blend mode, and it is not on a HUD (`llface.cpp:1235-1255`). `…Deferred` covers normal faces; `…NonDeferred` covers fullbright faces.

**Important:** the settings are read live, but each face decides mask or blend when its geometry is rebuilt. After changing either one, **teleport away and back, or relog**, before comparing; otherwise only objects rebuilt since then follow the new value.

**Test objects** (rez in a sandbox, or use a scene that has them):

- *T1, cut-out:* a plywood prim with no material (no normal or specular map), a texture whose alpha is only fully opaque or fully clear (a leaf, lace or chain-link image), transparency 0, glow 0.
- *T2, control:* the same prim with transparency 30 % (never masked).
- *T3, fullbright cut-out:* T1 with Full Bright on.
- *T4, layered foliage:* several T1 planes crossing each other (or a tree that uses such textures).

**Scene:** EEP sky, Shadows Sun & Moon + Projectors, Ambient occlusion on, a Viewer Light spotlight aimed at the objects, a few nearby point lights.

| # | Scenario | Setting change | What to look for |
|---|---|---|---|
| 1 | Spot cone on cut-out | `…Deferred` on vs off | On: T1 shows the Viewer Light's cone edge and falloff shaping. Off: T1 is lit evenly by the spot (cones are not applied to blended alpha); T2 behaves like "off" in both. |
| 2 | Contact shading | `…Deferred` on vs off | On: ambient occlusion darkens where T4's planes meet and where T1 touches the ground. Off: no occlusion on the cut-out faces. |
| 3 | Sorting | `…Deferred` on vs off | Orbit around T4. On: crossing planes stay in correct order. Off: layers can pop or show through each other as the camera moves. |
| 4 | Many local lights | `…Deferred` on vs off | Put 10 or more point lights around T1. On: every light contributes. Off: only the nearest few forward lights reach T1, so some lights seem missing on it. |
| 5 | Edges | `…Deferred` on vs off | Zoom close. On: hard, slightly aliased cut-out edges. Off: smoother blended edges. |
| 6 | Depth effects | `…Deferred` on vs off | With depth of field on, focus beyond T1. On: T1 blurs correctly with its depth. Off: blur around T1's edges can be wrong because blended faces do not write depth. Same check with dense haze. |
| 7 | Fullbright cut-out | `…NonDeferred` off vs on | T3 only. On: hard edges, correct sorting against T4 and correct depth of field around it. Off: blended look. Lighting does not change either way (fullbright is unlit). |
| 8 | Exclusions | either | T2 (transparency), a glowing T1 (glow > 0), a T1 with a material set to Alpha Blending, and a HUD with a cut-out texture never change: confirms the qualification rules. |
| 9 | Cost | `…Deferred` on vs off | In a dense foliage scene, compare frame time (Ctrl+Shift+1). Masks are usually cheaper than blended alpha. |

---

## 12. Items awaiting approval (revised 2026-10-25)

### 12.1 Candidates for Use KS Reference (not yet applied)

Use KS Reference would set each of these while it is on and restore the previous value when it is turned off, like the values it already sets (§2.1). Recommended value = the LL/FS/AS default unless marked otherwise; the point is a known reference state, not new behavior. Nothing here is implemented; `ksenvironment.cpp` is unchanged. Ten settings that cannot be set and restored safely, or have no effect, were removed on 2026-10-25 (§12.5); 54 remain.

**(a) Changeable only in Debug Settings**

| Setting | Default | Recommended | Note |
|---|---|---|---|
| `RenderHDREnabled` | TRUE | **TRUE** | "Enable HDR rendering" (HDR buffers, probes, sky values). Must stay TRUE (R1). |
| `RenderDefaultProbeUpdatePeriod` | 2.0 | default | Seconds between default-probe updates. |
| `RenderDesaturateIrradiance` | TRUE | default | Probe irradiance desaturation. |
| `RenderReflectionProbeMaxLocalLightAmbiance` | 4.0 | default | Cap on local-light energy in probes. |
| `RenderReflectionsEnabled` | TRUE | default | Off removes all probe reflections and probe fill. Quality slider also sets it. |
| `RenderHeroProbeConservativeUpdateMultiplier` | 16 | default | Hero probe update pacing. Quality slider also sets it. |
| `RenderScreenSpaceReflectionIterations` | 25 | default | SSR tuning (6 settings). |
| `RenderScreenSpaceReflectionRayStep` | 0.1 | default |  |
| `RenderScreenSpaceReflectionDistanceBias` | 0.015 | default |  |
| `RenderScreenSpaceReflectionDepthRejectBias` | 0.001 | default |  |
| `RenderScreenSpaceReflectionAdaptiveStepMultiplier` | 1.6 | default |  |
| `RenderScreenSpaceReflectionGlossySamples` | 4 | default |  |
| `RenderShadowSplits` | 3 | **2** | Shadow internals (7 settings). 2 skips the farthest sun cascade: at the 64 m draw distance shadows end about 16-27 m out and the costliest cascade is not drawn (since 2026-10-26). |
| `RenderShadowOffset` | 0.01 | default |  |
| `RenderShadowOffsetError` | 0.0 | default |  |
| `RenderShadowBiasError` | -0.007 | default |  |
| `RenderShadowErrorCutoff` | 5.0 | default |  |
| `RenderShadowNoise` | -0.0001 | default |  |
| `RenderSpotShadowBias` | -0.0002 | default |  |
| `RenderGTAOBackend` | 0 | default | AO implementation choice. |
| `RenderDeferredNoise` | 4.0 | default | Noise texture scale for SSAO and shadows. |
| `RenderEdgeDepthCutoff` | 0.01 | default | Depth of field edge detection. |
| `RenderEdgeNormCutoff` | 0.25 | default |  |
| `RenderEnableEmissiveBuffer` | TRUE | default | Off breaks auto-exposure metering, glow and several AyaneStorm sky effects. |
| `RenderGlowLumWeights` | 1.0 0.0 0.0 | default | Glow internals (6 settings). |
| `RenderGlowMinLuminance` | 0.8 | default |  |
| `RenderGlowMaxExtractAlpha` | 0.25 | default |  |
| `RenderGlowWarmthAmount` | 0.0 | default |  |
| `RenderGlowWarmthWeights` | 1.0 0.5 0.7 | default |  |
| `RenderGlowNoise` | TRUE | default |  |
| `RenderDeferredDisplayGamma` | 2.2 | default | Output gamma of the final image. |
| `RenderHDRIExposure` | 0.0 | default | HDRI preview (4 settings); only act while a preview is loaded. |
| `RenderHDRIRotation` | 0.0 | default |  |
| `RenderHDRISplitScreen` | 1.0 | default |  |
| `RenderHDRIIrradianceOnly` | FALSE | default |  |
| `RenderSkyAutoAdjustLegacy` | FALSE | **TRUE** | R2: legacy skies use LL's auto-adjust conversion. LL default is FALSE; KStorm's default is TRUE since 2026-10-26 (§11.7). |
| `RenderSkyAutoAdjustProbeAmbiance` | 0.01 | default | Probe fill for converted legacy skies; matches our minimum 0.01. |
| `RenderSkyAutoAdjustAmbientScale` | 1.0 | default | Legacy-sky conversion scales (4 settings). |
| `RenderSkyAutoAdjustBlueDensityScale` | 1.0 | default |  |
| `RenderSkyAutoAdjustBlueHorizonScale` | 1.0 | default |  |
| `RenderSkyAutoAdjustSunColorScale` | 1.0 | default |  |
| `RenderVolumetricLightingSampleCountOverride` | 0 | default | AyaneStorm god rays: 0 = use Quality. |
| `ASLightRigEyeMaskRadius` | 0.02 | default | Gains a control in My Lights with branch B3; candidate until then. |
| `KSHairOcclusionHeadOffset` | 0.01 0.0 1.0 | default | Hair occlusion head-sphere offset. |

**(b) Pre-EEP, non-HDR and non-PBR settings (generally unused)**

| Setting | Default | Recommended | Note |
|---|---|---|---|
| `RenderDisableVintageMode` | TRUE | **TRUE** | R1. Phototools and Preferences have the toggle; the quality slider's Low, LowMid, safe, GL3 and Unknown levels set it FALSE. |
| `WindLightUseAtmosShaders` | TRUE | **TRUE** | Deprecated; the PBR pipeline expects TRUE. Preferences toggle. |
| `RenderSkySunlightScale` | 1.0 | default | Sun scale when HDR is off. |
| `RenderSunDynamicRange` | 1.0 | default | Legacy forward lighting only. |
| `WaterGLFogDepthScale` | 50.0 | default | Legacy water fog. |
| `WaterGLFogDepthFloor` | 0.25 | default | Legacy water fog. |
| `RenderFogRatio` | 4.0 | default | Legacy fog distance (`llsky.cpp`); skipped by the feature manager. |
| `RenderSpecularResX` | 1024 | default | Legacy specular lookup texture (2 settings); a change rebuilds the texture. |
| `RenderSpecularResY` | 256 | default |  |
| `RenderSpotLightsInNondeferred` | FALSE | default | Non-deferred path only. |

### 12.2 Ignored: changeable elsewhere, no conflict with our controls (c)

Left exactly as LL, FS and AS have them:

- **AyaneStorm and KStorm floaters:** `RenderVolumetric*` (Volumetric Lighting), `KSHairOcclusion*` other than the head offset (Hair Occlusion floater), `ASLightRigRenderBackend`, `ASLightRigShowOtherAvatars`, `ASRenderLightBeacon` (My Lights, set by its code).
- **Values you set yourself:** `RenderGlowHDR` and `RenderNormalMapScale` (decided 2026-10-25: not touched by code).
- **Other UI, not lighting values we show:** `RenderFarClip`, `RenderAvatarMaxNonImpostors` (Use KS Reference already sets these two), `RenderAttachedParticles`, `UseOcclusion`, `RenderDynamicLOD`, `WLSkyDetail` (set independently), `SelectReflectionProbes`, `RenderReflectionProbeShowTransparent`, `CameraFocusTransitionTime`, `RenderDepthOfFieldInEditMode`, `FSEnvironmentManualTransitionTime`, `SkyPresetName`.
- **No effect in this viewer:** `RenderDeferred` (the code forces deferred rendering on).
- **Other editors of the same sky** (Personal Lighting, the sky and water editors, Phototools): they change the same values as our panes, but our panes follow every environment change (§11.2), so they coexist rather than conflict.

### 12.3 Decisions (2026-10-25)

General rule: do not change LL, FS or AS behavior, except for whatever competes or conflicts with our controls when it changes.

| # | Item | Decision |
|---|---|---|
| A1 | Forced settings | **Approved:** they are KS Reference candidates (§12.1). |
| D1 | Quality slider | **Approved:** while Use KS Reference is on, reapply its values after a quality change. |
| D2 | Graphics preset load | **Approved:** as D1. |
| D3 | Build no-post | **Leave as LL.** The note is added to the tooltips of Exposure, Dynamic exposure, Tone mapping, Tone mapping mix and Glow (§10). Tooltips are static text, so this needs no code. |
| A2 | Lighting preset (O16) | **Unified with A14 as Lighting Presets** (§12.6, decided 2026-10-25); compared with My Environments in §12.7. |
| A3 | Editing a KS value while KS is on | **Leave KS on** so the other values stay at the reference; turning it off restores the pre-KS values. Every control KS writes ends its tooltip with "(KS Reference)" (§10). |
| A4 | Use Shared / My Environments while KS is on | **Both turn KS off** (no sky restore: the new environment replaces it). |
| A5 | O14 | AyaneStorm's Depth of Field floater lacks all nine of our camera controls (F-number, Focal length, Field of view, Zoom, Focus follows pointer, Focus locked, Show focus point, Max blur, DoF resolution). **Keep our pane and its Depth of Field… button**; the O14 change to her floater is dropped. |
| A6 | Quick › Environment Effects… | **Removed** from Quick. |
| A7 | Ambient pane access | **Approved:** Ambient occlusion (with Settings…) and Probe Fill Strength are also at the bottom of Shadows (duplicates allowed). 2026-10-26: Sky Ambient Gain (was Ambient Brightness) is on Sun/Moon only. |
| A8 | Exposure entry status | **Exposure value only.** |
| A9 | Water labels | **Adopted:** Water fog color, Water fog density, Underwater fog multiplier. |
| A10 | Compact mode | **Adopted.** 2026-10-26: the launcher is locked to compact mode (icon strip; hover tooltip = pane label) and the mode toggle is hidden; the full list and toggle stay in the code for later. Icons: the mock-up's placeholders stand for now; real 16×16 icons (16, including Lighting Presets and the toolbar button) are the next task. |
| A11 | Eye mask radius | Not in AyaneStorm's My Lights floater today (`ASLightRigEyeMaskRadius`, default 0.02). **Decided 2026-10-25: add a control to her floater**, in its own session and branch (`feat-my-lights-eye-mask`). It stays a KS Reference value. |
| A12 | O15 viewer-only light overrides | **Future** (decided 2026-10-25). Details in §12.4. |
| A13 | Opening KStorm Lighting | **Decided 2026-10-25:** toolbar button (icon mocked; final icon with the others) and **hotkey Alt+Shift+L** (menu shortcut on a new menu item; free in every menu and in code). |
| A16 | KStorm menu | **Decided 2026-10-25:** keep KStorm's top-level menu (its own, so no LL menu edits) with **KStorm Lighting** (Alt+Shift+L). Retire KStorm Environment Effects. Remove the Hair Occlusion item; Hair Occlusion opens from a **Hair…** button to the right of Settings… on the Shadows pane's Ambient occlusion row, and leaves the Ambient pane. |
| A15 | Use sky exposure range | **Approved 2026-10-25:** the relabel of "Use fixed range" (§11.7). |
| A14 | §12.1 candidates | **Approved 2026-10-25:** KS Reference (the built-in preset, §12.6) sets the 54 settings of §12.1. **Every value it sets, sky and settings alike, must be restored to its prior value when it is turned off.** |

### 12.4 Restated items

**A2 — KStorm lighting preset (O16).** A named snapshot of every lighting value in this design, saved to a file and loaded back.

- *Contents:* the 76 viewer settings §10 surfaces (plus the §12.1 list if approved); the local sky and water as their settings data (`LLSettingsSky` / `LLSettingsWater` serialize themselves); the AyaneStorm floaters' settings (volumetric lighting, color grading, celestial, depth of field), as plain viewer settings; the Use KS Reference state.
- *Size:* new KStorm code only: one class (about 300–500 lines) and a small Save / Load / Delete panel (or two buttons in the launch floater), files under the user settings folder like graphics presets. Loading writes settings directly and applies the sky and water as a local environment through `KSEnvironment`, the same path the panes use.
- *Effect on LL / FS / AS code:* none required. Optional: one `<KS:KS>` line if it gets a menu entry. Graphics presets stay as they are.
- *Not saved:* Viewer Lights rigs (My Lights has its own presets), server lights, region and parcel environments.

**A12 — Viewer-only light overrides (O15, §8.5).** Let you change, hide or add lights on any object, with or without modify rights, in your view only; nothing is sent to the server.

- *Adjust a server light:* intensity, color, radius and falloff overrides stored by object ID and applied where Viewer Lights' Boost is applied now (`pipeline.cpp` light loops). The object itself is never written.
- *Hide one server light:* a per-object form of Only Viewer Lights at the same filter point.
- *Make any object a light:* route 1 (recommended) anchors a Viewer Light to the object's position, reusing My Lights; no `ll*` changes. Route 2 fakes the object's own light parameters and needs several `llvovolume.cpp` edits that the server's updates would keep undoing.
- *Limits:* overrides last for the session (object IDs change on re-rez, copy and each time an attachment is worn); only you see the result; reflection probes capture these lights too.
- *Effect on LL / FS / AS code:* adjust and hide need small `<KS:KS>` hooks in the existing light loops, where Boost and Dark already hook; route 1 needs a new anchor mode in My Lights (AyaneStorm code).
- *UI:* a "Selected object" lighting pane (no modify rights needed) with Override, Hide and Add light here.

**A14 — Use KS Reference candidates (§12.1).** Approving the list means a later `ksenvironment.cpp` change (branch B2) adds these settings to what Use KS Reference sets and restores.

- *While on:* each listed setting takes its recommended value; on off, its previous value comes back (as KS does today for draw distance and the impostor limit).
- *Values:* the LL/FS/AS default, so turning KS on changes nothing for a viewer that was never tuned; it only undoes stray Debug Settings edits. Five values differ from LL behavior or are policy: `RenderSkyAutoAdjustLegacy` TRUE (R2, LL default FALSE), `RenderShadowSplits` 2 (default 3, since 2026-10-26), and `RenderHDREnabled`, `RenderDisableVintageMode`, `WindLightUseAtmosShaders` TRUE (R1; equal to the defaults but now held while KS is on).
- *Interaction with D1 / D2:* these values are reapplied after a preset load or quality change while KS is on.
- *Which can be set and restored safely:* see §12.5.
- *Effect on LL / FS / AS code:* none; the change is in `ksenvironment.cpp` only.

### 12.5 Which §12.1 candidates can be set and restored safely

Checked against how each setting is read (`llviewercontrol.cpp` change handlers, `LLCachedControl`, `refreshCachedSettings`, one-off reads). Use KS Reference should write a value only when it differs from the current one, so a viewer already at the reference pays nothing.

**Safe: read live, takes effect at once and reverts cleanly (48).** `RenderDefaultProbeUpdatePeriod`, `RenderDesaturateIrradiance`, `RenderReflectionProbeMaxLocalLightAmbiance`, `RenderHeroProbeConservativeUpdateMultiplier`; the six `RenderScreenSpaceReflection*`; the seven shadow internals (`RenderShadowSplits`, `…Offset`, `…OffsetError`, `…BiasError`, `…ErrorCutoff`, `…Noise`, `RenderSpotShadowBias`); `RenderGTAOBackend`; `RenderEdgeDepthCutoff`, `RenderEdgeNormCutoff`; five glow internals (`RenderGlowLumWeights`, `…MinLuminance`, `…MaxExtractAlpha`, `…WarmthAmount`, `…WarmthWeights`); `RenderDeferredDisplayGamma`; the four `RenderHDRI*`; `RenderSkyAutoAdjustLegacy` with its four scales and its probe ambiance; `RenderVolumetricLightingSampleCountOverride`; `ASLightRigEyeMaskRadius`; `KSHairOcclusionHeadOffset`; `RenderSkySunlightScale`; `RenderSunDynamicRange`; `WaterGLFogDepthScale`, `WaterGLFogDepthFloor`; `RenderSpotLightsInNondeferred`; `WindLightUseAtmosShaders` (no handler; read when settings refresh). Also trivially safe but pointless, because they have no effect: `RenderDeferredSunWash`, `RenderReflectionProbeAmbiance`, `RenderGammaFull` — recommend dropping them from the list.

**Safe but costly: a change rebuilds GPU buffers or reloads shaders (a visible stall, about a second), each time KS turns on or off with a differing value.**

| Setting | What a change triggers |
|---|---|
| `RenderHDREnabled` | Probe reset, all GL buffers, all shaders (`handleEnableHDR`). |
| `RenderDisableVintageMode` | Sets `RenderHDREnabled` and `RenderEnableEmissiveBuffer`, so the same rebuild (`handleDisableVintageMode`). |
| `RenderEnableEmissiveBuffer` | All GL buffers and shaders (`handleEnableEmissiveChanged`). |
| `RenderReflectionsEnabled` | Probes, GL buffers and shaders (`handleReflectionProbeDetailChanged`). |
| `RenderDeferredNoise` | All GL buffers (`handleReleaseGLBufferChanged`). |
| `RenderGlowNoise` | Bump-map cache and shaders (`handleSetShaderChanged`). |
| `RenderSpecularResX`, `RenderSpecularResY` | Lookup-texture rebuild (`handleLUTBufferChanged`). |
| `RenderFogRatio` | Fog update (`handleFogRatioChanged`); cheap. |

Recommendation: keep them (they are at the reference in a normal viewer, so KS writes nothing), but never toggle them back and forth in a loop (for example D1/D2 reapply only on an actual difference).

**Not safe: not read live, so setting or restoring it leaves the viewer in a mixed state until some unrelated rebuild or a restart (6). Recommend removing from the candidates.**

| Setting | Why |
|---|---|
| `RenderReflectionProbeDynamicAllocation` | Read only in `LLReflectionMapManager::refreshSettings()`, which runs when other probe settings change. |
| `RenderHeroProbeDistance` | Read only in `LLHeroProbeManager::initReflectionMaps()`, on a probe reset. |
| `RenderShaderLightingMaxLevel` | Read only when shaders load (`llviewershadermgr.cpp:874`); no handler. |
| `SkyAmbientScale`, `SkyNightColorShift` | Read once, in the legacy atmospherics constructor (`lllegacyatmospherics.cpp:184-185`). |
| `RenderSpecularExponent` | Read only when the lookup texture is built; no handler of its own. |

**Also not recommended:** `RenderGamma`. Its handler (`handleGammaChanged`) sets the operating system's display gamma ramp; KS should not touch the display outside the viewer.

**Why `RenderGlowHDR` and `RenderNormalMapScale` were left out** (besides your decision to set them yourself):

- `RenderGlowHDR` chooses the glow buffer format. Every change releases and recreates all GL buffers (`handleReleaseGLBufferChanged`), the same stall as above, for a value that persists anyway.
- `RenderNormalMapScale` is read only when the viewer generates a normal map from a legacy bump map (`lldrawpoolbump.cpp:837`, `:950`). Maps already generated keep the old scale, so a change affects only bump maps loaded afterwards, and restoring it does not undo them: the scene ends up mixed. It cannot be set and restored safely.

Totals for the 64 candidates: 48 safe (45 once the three no-effect settings are dropped), 9 costly, 6 not safe, and `RenderGamma` not recommended. **Decided 2026-10-25:** the 6 not-safe settings, `RenderGamma` and the 3 no-effect settings are removed from §12.1, leaving 54 candidates (45 safe, 9 costly).

### 12.6 Lighting Presets: unified design (decided 2026-10-25)

A2 (lighting preset) and A14 (KS Reference candidates) become one feature. **Use KS Reference is the first preset**: built in and read-only. Lighting Presets you make can be created, updated, renamed, duplicated and deleted.

**Rules**

- *Partial presets.* A preset stores only the groups it names (Sun/Moon, Atmosphere, Clouds, Water, Reflections, All lights, Shadows, Ambient, Exposure, Material glow, Depth of field, Vignette, AyaneStorm floaters). KS Reference names a fixed subset: its sky values plus the 54 settings of §12.1.
- *Apply with a restore point.* Applying records the current values of everything the preset will change; **Turn off** restores them. One preset is active at a time; applying another first turns the active one off, so presets never stack.
- *Carried over from KS Reference:* while a preset is active its values are reapplied after a graphics-preset load or quality change (D1, D2); Use Shared and applying an environment from My Environments turn it off without restoring the sky (A4); editing one of its values leaves it active (A3); only values that differ are written (§12.5). The "(KS Reference)" tooltip suffix shows the active preset's name.
- *KS Reference stays special:* values defined in code (a KStorm update may change them), cannot be deleted, renamed or updated; Duplicate makes an editable copy; it keeps its Quick check box.
- *Storage:* one file per preset under the user settings folder, like graphics presets; sky and water are stored as their settings data (the same LLSD an EEP asset holds), viewer settings by name. No LL, FS or AS code changes.

**Where it is surfaced and how it looks (mock-up v4)**

- **Launch list:** a **Lighting Presets** entry directly under Quick (icon placeholder 🗂); its status shows the active preset or "none active".
- **Quick:** unchanged — the Use KS Reference check box is the shortcut for the built-in preset. Applying any preset from the Lighting Presets pane updates that check box.
- **Lighting Presets pane:**
  - a list (XUI `scroll_list`): lock icon for read-only, name, scope ("full", a list of groups, or "partial (built in)"); the active preset is bold with "● active". KS Reference is always first. Double-click applies or turns off.
  - buttons, row 1: **Apply** (becomes **Turn off** for the active preset), **Save current as…**
  - buttons, row 2: **Update**, **Rename…**, **Duplicate**, **Delete…**, **Contents…**. Update, Rename and Delete are disabled for KS Reference.
  - a note: "Double-click a preset to apply it or turn it off. One preset is active at a time; Use Shared and My Environments turn it off."
- **Dialogs** (small floaters, like Save Graphics Preset):
  - *Save current as…:* name field and Include check boxes for the 13 groups (all checked); Save / Cancel. An existing name asks to replace; "KS Reference" is reserved.
  - *Rename…:* name field; Rename / Cancel.
  - *Delete…:* confirmation ("This cannot be undone"; if active, its earlier values are restored first); Delete / Cancel. In the viewer this is a standard alert.
  - *Contents…:* for a user preset, the group check boxes (unchecking drops a group; checking one captures its current values); Save / Cancel. For KS Reference, a read-only list of its values.
- **Editing values:** there is no separate value editor. Apply the preset, adjust the panes, then **Update**.

**As built (2026-10-26, branch `feat-ks-lighting-presets`).** "Memos" was renamed **Lighting Presets** (the list entry, the pane and this design). Code: `kslightingpresets.cpp` (model, storage, apply and turn off, KS Reference, A4, D1/D2) and `ksfloaterlightingpresets.cpp` (pane and one dialog for Save current as…, Rename… and Contents…); `ksenvironment.cpp` keeps only the sky and water helpers and delegates to it.

- *Model:* a preset is three value maps (viewer settings, sky fields, water fields) plus the groups it includes. Groups are what Save current as… and Contents… capture or drop; apply and turn off work on the maps. KS Reference names no groups: it holds its 9 sky values, its 7 viewer settings (draw distance 64 m, impostors 4, name tags off, subject sort on, Dynamic exposure off, shadow resolution 1.5x, background texture discard off) and the 54 settings of §12.1 (each at its default except the five policy values).
- *Group contents:* a group's viewer settings are read at run time from the controls its pane shows (`control_name` and the `ksv_` vector names in `floater_ks_lighting_<pane>.xml`), so presets and panes cannot drift apart. The AyaneStorm group reads her nine Environment Effects and Camera Effects panels and the Depth of Field floater, plus every `ASColorGrade*` and `ASWeather*` setting (set in code by her color grading and weather panels). Sky and water values are stored raw (rotations as quaternions, colors as RGB), not as the panes' display values.
- *Files:* `user_settings/ks_lighting_presets/<escaped name>.xml` (LLSD: `groups`, `settings`, `sky`, `water`, and the full sky and water data as `sky_full` / `water_full` for the postponed export).
- *Restore point:* the prior values of exactly what the preset writes; only differing values are written, on apply, turn off and reapply. If the preset created the local environment, turning it off clears it (as Use KS Reference always did).
- *Rebuilds and Save current as… (2026-10-27):* applying, turning off and reapplying a preset batch the buffer, shader and probe rebuilds their setting writes trigger (`ks_begin_render_settings_batch`), so each operation rebuilds once instead of once per setting (a full rebuild is about 4 s at 4K without the shader cache). Save current as… no longer turns the active preset off and applies the new one (two rounds of rebuilds, about 90 s with most groups); the new preset becomes active without writing anything (`KSLightingPresets::activateSaved`), and the active preset's restore point carries over, so Turn off returns to the values from before either.
- *Shader cache (2026-10-27):* after each new build LL purges the shader cache, which deletes its folder, and nothing recreated it, so no shader binaries were saved in that session and every rebuild compiled all shaders. `LLShaderMgr::initShaderCache()` now recreates the folder after the purge (tagged).
- *A4:* only presets holding sky or water values react to environment changes; an unversioned change keeps the preset while the local sky and water still hold its values.
- *D1, D2:* `RenderQualityPerformance` and `PresetGraphicActive` (only when set to a name, which also ignores our own writes) schedule a reapply on the next idle; the values the load brought become the ones Turn off restores.
- *List:* a lock icon for KS Reference, the active preset in bold with "(active)", and a Contains column ("built in", "full", "custom" for a copy of KS Reference, or the group names).
- *Dropped:* the "(KS Reference)" tooltip suffix naming the active preset (tooltips are static text).
- *Layout:* the pane and its dialog keep the common 300 px width; the seven buttons wrap to three rows (rule, 2026-10-26: panes grow taller, up to the launcher's height, never wider; AyaneStorm floaters keep their widths).
- *Precision:* preset files store reals with the LLSD XML default precision (about 6 digits); settings are F32 anyway, and the A4 match uses a 0.001 tolerance.

### 12.7 Lighting Presets compared with My Environments

| | My Environments (EEP settings items) | Lighting Presets |
|---|---|---|
| What it is | Inventory items: Sky, Water, Day Cycle assets | Local files in the user settings folder |
| Where it lives | On the server, in your inventory | This computer only |
| Sharing | Can be given, sold, used for a parcel or region (with permissions) | Not shared |
| Sky | Every sky field: sun and moon rotation and scale, colors, ambient, haze, glow, density and distance multipliers, height (`max_y`), gamma (Sky HDR Scale), probe ambiance (Probe Fill), clouds (color, coverage, scale, variance, density, scroll), moisture, droplet, ice, star and moon brightness, textures (sun, moon, cloud, bloom, halo, rainbow), atmosphere physics (Rayleigh, Mie, absorption), legacy fields | The same sky values as a snapshot of the current (local) sky, when the preset includes the sky groups |
| Water | Every water field, including normal map and transparency texture | The same, when it includes Water |
| Time of day | Day cycles: sky and water keyframes over the day, with altitude tracks | None: a preset holds one frozen sky |
| Viewer settings | **None** | All the settings our panes surface: Exposure, Dynamic exposure and its tuning, tone mapping, Sunlight Gain, Surface Ambient Gain, shadows, ambient occlusion, reflections and probes, glow, depth of field, vignette, light budget and filters; plus AyaneStorm floater settings and the §12.1 reference settings |
| Applying | Replaces the environment (for you, or a parcel or region); no undo except Use Shared or another environment | Applies with a restore point; Turn off restores the previous values |
| Built-in content | Library environments | KS Reference |

**Consequences**

- They complement each other: My Environments carries the sky and water (and time of day) and can be shared; presets carry the viewer-side look that no environment stores.
- A preset that includes the sky duplicates what a sky asset holds. Because presets store the sky as the same settings data, a later option could be "Save sky to My Environments" from a preset (an inventory upload through the existing sky editor path; not planned).

**Is our local side a proper superset of My Environments? (checked 2026-10-25)** Not entirely:

- *Preset storage:* a superset of **one Sky item plus one Water item**, provided the preset stores the full sky and water data (as recommended below), plus all the viewer settings no environment holds.
- *Not covered:* **Day Cycles** — time-of-day keyframes, several skies and waters per day, altitude tracks, day length and offset. A preset holds one frozen sky. Also not covered: item name, permissions and sharing.
- *Our panes:* not a superset of what the sky and water editors can edit. Not surfaced: sun, moon, cloud, bloom, halo and rainbow textures; cloud scroll rate; atmosphere physics (Rayleigh, Mie, absorption, density profiles, Mie anisotropy); planet, sky-top and sky-bottom radii; dome offset and radius; sun arc radius; the legacy fields; water normal map, transparency texture and wave directions (shown as placeholders only). These stay editable in the sky and water editors (My Environments) and are preserved in a preset's stored data.
- *To close the gap* (not planned): a preset could reference a Day Cycle item and time instead of copying a frozen sky; the panes could add texture pickers and the physics settings in an expert block.

**Feasibility: "Save sky to My Environments" from a preset (evaluated 2026-10-25; postponed, decided 2026-10-25).**

- *Mechanism exists, public:* `LLSettingsVOSky::buildSky(LLSD)` rebuilds a sky from the preset's data, and `LLSettingsVOBase::createInventoryItem(settings, parent_id, name, callback)` creates the inventory item and uploads the asset, exactly as the sky editor's Save As does (`llsettingsvo.cpp:144-175`). Water is the same with `buildWater`. No LL, FS or AS code changes.
- *Effort:* small — about 100–150 lines in the preset code plus a **Save sky to My Environments…** button in the Lighting Presets pane (name dialog, target = the Settings folder), roughly a day including testing.
- *Permission rule (the real work):* the sky editor offers Save As only when the source can be copied (`mCanCopy`; settings flags `FLAG_NOCOPY` / `FLAG_NOSAVE`, `llfloatereditenvironmentbase.cpp:208-235`). A preset can capture any sky, including a region's or a no-copy item's, so exporting it unchecked would bypass those permissions. The preset must record where its sky came from when it is captured and allow the export only for skies you could copy anyway (your own copyable items, the Library, KS Reference, skies built from those). Tracking that origin through `LLEnvironment` without LL changes needs research (asset id of the local sky, edits made on top of a region sky).
- *Region support:* blocked where the region does not support settings inventory (`isInventoryEnabled()`, notification "SettingsUnsuported").
- *Content limits:* a preset holds one frozen sky, so the export is a Sky item, never a Day Cycle. AyaneStorm's viewer-local sky effects (horizon scattering, auroras, stars extras) are viewer settings, not sky fields, and do not go into the asset. For a complete export a preset that includes any sky group should store the full sky data, even though it applies only its included groups.

- Applying an environment from My Environments turns the active preset off (A4), so the two never fight over the sky.

### 12.8 Motion mode (built 2026-10-27, branch `feat-ks-motion-mode`)

Two ways of working: photo (full quality) and motion (frame rate first). **Motion mode** is one switch (`KSMotionMode`, not persisted) on Quick (check box beside Show avatars) and at the bottom of the launcher (a toggle button, pressed while on; status "on"). Code: `ksmotionmode.cpp`.

- *Turned off:* shadows (`RenderShadowDetail` 0) and god rays (`RenderVolumetricLighting`), ambient occlusion (`RenderDeferredSSAO`), screen space reflections, mirrors (`RenderMirrors`), glow (`RenderGlow`), depth of field (`RenderDepthOfField`, and `ASDepthOfFieldMode` to Standard), particles (`RenderMaxPartCount` 0), local lights (`RenderLocalLightCount` 0, which also stops My Lights), AyaneStorm Camera Effects (`ASCameraEffectsEnabled`) and the AyaneStorm Environment Effects (color grading, procedural sun, horizon scattering, moon halo, stars, Milky Way, aurora, snow).
- *Frozen:* reflection probes (`RenderReflectionProbeDetail` 0, static only; left alone when already 0 or -1). Disabling probes would remove all sky ambiance, so the lighting would change, not just its quality.
- *Left on:* antialiasing, HDR and the emissive buffer. HDR and emissive changes rebuild every buffer and shader, reset the probes, and change the look itself (no exposure, tone mapping or SSR; the legacy sky path; emissive textures and several AyaneStorm effects depend on the emissive buffer), for a small saving.
- *Save and restore:* turning it on saves each value it changes; turning it off restores only the values still at the Motion value, so a setting changed while in Motion mode keeps that change. Quitting with it on restores the values before settings are saved, so a session never starts in Motion mode.
- *Presets:* Motion mode is a layer, not a preset, and never part of one (excluded from the Quick group like `KSSetEnvironment`). Applying KS Reference or any lighting preset turns Motion mode off first, so the preset's restore point holds the real values. While Motion mode is on, the presets read and write the values outside it (`KSMotionMode::getValue` / `setValue`): Save current as… and Update capture the real values, and turning a preset off or reapplying it after a quality change updates the value Motion mode will restore instead of undoing Motion mode.
- *On teleport (2026-10-27):* every teleport turns Motion mode on: to another region or within the region (map, minimap, landmark, teleport offer, script, home). `LLAgent::setTeleportState()` (tagged) calls `KSMotionMode::onTeleportStarted()` on the states START, REQUESTED and LOCAL; Motion mode switches on at the next idle. It stays on after arrival; turn it off by hand. Setting `KSMotionModeOnTeleport` (default on) disables it.
- *One rebuild per switch:* shadows, ambient occlusion, glow, depth of field, SSR, mirrors and probe detail each rebuild buffers or shaders on change. Motion mode writes them in one batch (`ks_begin_render_settings_batch` / `ks_end_render_settings_batch`, `llviewercontrol.cpp`, tagged), so the rebuild runs once; switching still takes about a second.
