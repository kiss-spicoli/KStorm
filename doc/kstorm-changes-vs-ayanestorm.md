# KStorm: changes compared with AyaneStorm

A plain-language list of everything KStorm adds to AyaneStorm, grouped by feature, with the
commits for each. Kept up to date as KStorm work is merged into the `kstorm` branch.

- **Base:** AyaneStorm `master` as of 2026-10-06 (`c1b790e08bd`, `upstream/master`). KStorm
  contains all of it.
- **Last updated:** 2026-10-27, at `kstorm` merge `3d38bbee364`.
- **KStorm's own work:** 82 commits (Kissmy Spicoli, Oct 18 to Oct 27), 107 files, about 11,600
  lines added.
- **Not yet in KStorm:** 11 newer commits on AyaneStorm's `ayanestorm-dev` branch (for example
  VBAO, an improved ambient occlusion).

To refresh the numbers:

```
git log --reverse --no-merges --format="%h %ad %s" --date=short upstream/master..kstorm
git diff --shortstat upstream/master kstorm
git rev-list --count upstream/master..upstream/ayanestorm-dev
```

## 1. My Lights: your personal lights, much expanded

AyaneStorm's My Lights only hung simple lights on your avatar. KStorm adds:

- **Spotlights that track a body part.** A light can be a spotlight that keeps pointing at a
  joint, such as your neck, as you move. You can set the beam width, softness and an optional
  projector image. (`49e8ead6e5f`)
- **Lights left in place.** A light can stay at a spot in a region instead of following you,
  so each region keeps its own lighting setup. These lights switch off while you are away and
  come back when you return. Lights also autosave every few seconds. (`95500628365`)
- **Fixed direction.** A fixed spotlight can point in a set compass direction and angle instead
  of at a joint. (`a0fcd0bfbde`, `a0b6ed8737f`)
- **Only My Lights.** One switch hides every other light in the scene, including other people's
  worn lights. A "Max lights" slider limits how many lights draw at once. (`beafaf4d788`)
- **Boost.** Each light can be up to 4x brighter than Second Life normally allows.
  (`165a67322ee`)
- **Dark spots.** A spotlight can darken where it points instead of lighting it, like a
  photographer's flag. (`7d3ff22f4bc`)
- **Eye mask radius.** A slider for this setting, which was reachable only in Debug Settings.
  (`185f5d7a020`)

Details: `doc/my-lights.md`.

## 2. Smarter choice of which lights draw

When there are more lights than the viewer draws, AyaneStorm keeps the ones closest to the
camera. KStorm can optionally keep the ones closest to what you are photographing (whatever you
Alt-clicked), which suits portraits. (`76c8b73a103`)

Details: `doc/ks-local-light-sort.md`.

## 3. Hair occlusion

Your own semi-transparent (alpha) hair gets realistic shadowing from your head, neck and
shoulders. Before, it could look flat or glow unnaturally. (`ce5968a5112`, fixes
`07ea66ec85e`, `a802aa03bc0`)

Details: `doc/ks-hair-occlusion.md`.

## 4. KStorm Lighting: one home for lighting controls

- **The early version** was a "KS" tab inside Environment Effects, then its own floater under a
  new **KStorm** menu. (`5d25edd5bd9`, `622f591182f`, `ef6f7178d56`)
- **It became KStorm Lighting (Alt+Shift+L).** A narrow launcher with one button per topic opens
  13 small panes, such as Quick, Sun/Moon, Ambient and Shadows. They gather controls that were
  spread across Preferences, Personal Lighting and AyaneStorm's floaters. (`435f28b859a`)
- **Clearer names.** Controls say what they do, for example Sunlight Gain, Sky Ambient Gain and
  Surface Ambient Gain. (`4c5c8bf967e`, `b5c7271685f`)
- **Docking.** Panes snap beside the launcher and line up with their buttons. They switch sides
  near a screen edge and stay aligned when the launcher moves. They collapse to a header bar with
  a click on the header or the minimize button. One dock button opens or closes them all, as you
  left them. Closing the launcher hides them. (`1890bf513da`, `4a351f236cf`, `e189f9b6d59`,
  `a985c238a36`, `bcfa9b8c829`, `7728a821232`, `eca710f5508`, `fb85a2cff4a`, `d597cf4ebe9`,
  `86f7625cff8`, `5d3981aa505`, `6bef1529b1f`)
- **AyaneStorm's large floaters stay unchanged.** Environment Effects, Viewer Lights and Camera
  Effects get a small header stand-in that docks beside the launcher and opens or closes the
  real floater. (`0ca3ba92b77`, `189dbf95bb4`, `ec9768e8a89`)
- **Controls that would do nothing are dimmed.** Each on/off switch has the controls it governs
  indented directly below it, and they grey out while it is off: for example the shadow sliders
  under Shadows, the glow sliders under Glow, the Dynamic exposure tuning under Dynamic exposure,
  and Worn lights under Only Viewer Lights. This also shows what Motion mode turned off. Hair
  Occlusion works the same way. (`096c0a97a40`, `66b4ce8e533`)
- **Clearer pane names.** The Glow pane is now "Material glow", so it is not confused with the
  sky's haze glow or AyaneStorm's bloom. (`096c0a97a40`)
- **Its own icons.** Launcher buttons are getting KStorm icons instead of borrowed ones:
  Vignette (a frame that darkens toward the corners, `88c36e93f05`), Depth of field (a lens
  aperture, `96aa43f58ee`), Material glow (a glowing filament) and Viewer Lights (a light bulb)
  (`9281361b4a4`), Exposure (the camera's +/- exposure symbol, `26d77b6959c`), Water
  (three waves, `04a6fcd486c`), Atmosphere (haze layers, `16d84a6eca5`), Sun/Moon (a sky dome
  rising over the horizon) and Quick (a lightning bolt) (`6d796f226c7`). Motion mode now shows the
  speedometer that Exposure had, and Camera Effects the camera that Depth of field had
  (`6d796f226c7`). Reflections shows a mirror and a reflected object (`121459cced7`). Still to come:
  Clouds, Shadows and the KStorm Lighting toolbar button.
- **Layout.** The launcher is always the compact icon strip, with no footer. Pane headers are
  taller and panes are 300 px wide. A Hair... button sits on Quick. (`53138cb89d8`,
  `68d6e528bd7`, `a6785359376`, `5485d81b9f0`, `4a31d052c89`)

Details: `doc/kstorm2-lighting-UI-design.md`.

## 5. Lighting presets and KS Reference

- **Lighting presets.** Save your lighting settings as a named preset, then apply or turn it
  off. Turning a preset off puts everything back as it was. The pane also has rename, duplicate
  and contents views. (`79a3ce0ee5b`, `d8ffd8d668b`, `47ec7d2fa1d`)
- **KS Reference** is the built-in preset: a neutral, predictable lighting setup to work from.
  It sets the sky and about 54 settings. Its values were tuned along the way:
  - draw distance 64 m (`e5d1fe07c2c`)
  - shadow resolution 1.5x (`ec7c5ce1f88`, `3b28af09464`, tooltip `f68c8bcea06`)
  - textures no longer reload when you switch back to the viewer, and two shadow layers
    instead of three (`177dfbf963a`)
  - material glow blurred 3 times, the most Preferences allows, instead of the default 2
    (`096c0a97a40`)
- **Saving a preset makes it the active one** (`aba9a069ceb`). The save took about 90 seconds
  and is now near-instant. Each preset switch does one graphics rebuild instead of dozens. A
  Second Life bug that kept the shader cache from saving after each new build is fixed.
  (`226663fd6c0`)

## 6. Motion mode: fast frame rate while moving

- **One switch.** Turns off the expensive effects: shadows, god rays, ambient occlusion,
  reflections, mirrors, glow, depth of field, particles, local lights, and AyaneStorm's
  environment and camera effects. Turning it off restores them. Antialiasing and HDR stay on.
  Measured about 8 to about 65 FPS. (`b43fa8df854`)
- **Presets.** Presets never save Motion mode, and applying a preset turns it off first.
  (`dcb5db0440f`)
- **Teleports.** It turns on by itself whenever you teleport, to another region or within one
  (for example a minimap click). The `KSMotionModeOnTeleport` setting turns this off.
  (`183b9769220`)

## 7. Fixes to existing behavior

- **Advanced Lighting.** Three atmosphere sliders (Density, Distance, Maximum Altitude) were
  hidden in AyaneStorm; they are shown and working. (`9b783fa6bf6`)
- **Haze bug.** Setting Haze Density to 0 could break the sky math (a divide-by-zero).
  (`2c8a7e9d438`)
- **Older skies.** They no longer look darker than they should, because probe fill can never
  drop to zero. (`456bd1193ec`)

## 8. Renames and documentation

- **Naming.** All KStorm code uses `ks*` names and `<KS:KS>` tags, so it is easy to tell apart
  from AyaneStorm's. (`230af14da33`)
- **Design and research docs.** Lighting factors, UI design, decisions and an AI-render idea; no
  code. (`a24c8275c11`, `a9dcf435756`, `78125eea578`, `fc115c89c70`, `ff95fe874e5`,
  `29f7828dc63`, `1fc45fbd589`)
- **This document.** (`7f8dcd24e29`)

## Where AyaneStorm's own files were touched

Most KStorm work lives in new `ks*` files. AyaneStorm's and Second Life's own files were changed
only where hooking in was unavoidable, with small edits tagged `<KS:KS>`:

- the renderer (`pipeline.cpp`), the shader manager and a few shaders
- My Lights (`aslightrig*`, `asfloatermylights`)
- teleport handling (`llagent.cpp`), settings, menus, the icon list (`textures.xml`) and Advanced
  Lighting
