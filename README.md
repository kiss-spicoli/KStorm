<img align="left" width="100" height="100" src="doc/kstorm_256.png" alt="Logo of KStorm viewer"/>

# KStorm - A research viewer for photographers

KStorm is a personal research viewer for Second Life by Kissmy Spicoli, built on
**[AyaneStorm](https://github.com/AyaneStorm/ayanestorm)** by Ayane (Chanayane). It is an
unsupported experiment: there is no support from KStorm, AyaneStorm or Firestorm. If you want a
supported viewer for photography, use [Firestorm](https://www.firestormviewer.org).

<br clear="left"/>

## Mission

**Looks and Scenes.** Photography is a memory of the experience. A look is a creative act,
and creativity grows from practice with your tools. To practice with them you have to
understand them, so KStorm gives you tools you can understand. Find a scene, then come back to
it with different looks until you find the one with heart.

- **A scene is where you shoot:** the place, the camera, the lights. Camera Positioning saves
  your camera views per sim and takes you back to them, and My Lights left in place come back
  when you return. Named Scenes that bring the whole setup back at once are planned.
- **A look is how it is lit and what effects are applied.** Look Inventory saves your lighting
  and effects (glow, depth of field, vignette, color grading and more) as a named look. Apply
  one, try another, or turn it off to get back exactly what you had. KS Reference is a known place
  to start from.
- **Tools you can understand.** One place for lighting instead of controls spread across
  Preferences, Personal Lighting and several floaters. Plain names and tooltips that say what a
  control really does, and controls dimmed when they would do nothing.
- **Nothing in the way of the shot.** Clear View hides everything but the world, Motion mode
  keeps moving around fast, and the full-quality look comes back with one click.

**Research the viewer, and write down what is found.** KStorm is also a test bed:

- **A known baseline.** The KS Reference look is a fixed, documented setup of sky, lighting and
  rendering. Builders and testers can work from it and describe their changes as differences
  from it.
- **Measure, don't guess.** The `ksperf` chat command reports frame rate, frame times, network
  and memory, and which settings differ from KS Reference, so changes can be compared.
- **Findings in the open.** Design decisions, lighting maps and notes on each finished feature
  are kept in [`doc/`](doc/), written so others can reuse them.

## What KStorm adds to AyaneStorm

KStorm contains all of AyaneStorm `master` as of 2026-10-06. Its own work, in brief:

- **Look Inventory**: save your lighting as a named look, apply it, or turn it off to get back
  exactly what you had. **KS Reference** is the fixed reference look.
- **Camera Positioning**: save camera views per sim and return to them, teleporting if needed;
  Lock Camera freezes camera and avatar for a shot.
- **My Lights, expanded**: spotlights that track a body part, lights left in place per region,
  fixed directions, Only My Lights, a 4x boost, and dark spots that work like a photographer's
  flag.
- **KStorm Lighting** (Alt+Shift+L): a compact launcher with 13 docking panes (Quick, Sun/Moon,
  Ambient, Shadows, Glow & bloom and more), with clearer control names and its own icons.
- **Clear View** (Alt+Shift+U): hides all UI, name tags and HUDs, leaving only the world.
- **Motion mode**: one switch that turns off the expensive effects while you move (about 8 to
  about 65 FPS in one measurement), on by itself at login and teleport.
- **Smarter light choice**: when there are too many lights, keep the ones nearest your subject
  rather than the camera.
- **Hair occlusion**: alpha hair gets shadowing from your head, neck and shoulders.
- **Shader-compile notice**: a "Compiling shaders..." screen instead of an unexplained freeze.
- **`ksperf`**: performance and settings report in your own nearby chat.
- **The KStorm name and a native login screen**, with no web page and nothing sent anywhere
  before you log in.
- **Fixes**: three hidden Advanced Lighting sliders shown again, a Haze Density divide-by-zero,
  and older skies looking too dark.

Every change, with its commits and the AyaneStorm files it touches, is listed in
[doc/kstorm-changes-vs-ayanestorm.md](doc/kstorm-changes-vs-ayanestorm.md).

## Credit and lineage

- **[AyaneStorm](https://github.com/AyaneStorm/ayanestorm)** by Ayane (Chanayane), a
  Firestorm-based viewer for photographers. Most of KStorm's rendering and photography features
  are Ayane's: depth of field, the night sky, color grading and LUTs, the transparency modes,
  god rays, procedural sunsets, moon and auroras, snow, camera effects, the original My Lights
  and more. Her features keep their AyaneStorm names in KStorm. Her README is kept in
  [doc/ayanestorm-README.md](doc/ayanestorm-README.md).
- **[Firestorm](https://www.firestormviewer.org)** by The Phoenix Firestorm Project, Inc.
- **The [Second Life viewer](https://github.com/secondlife/viewer)** source code released by
  Linden Lab.

KStorm is distributed under the GNU LGPL 2.1, like its upstream projects. All upstream copyright
notices are retained.

## Status

- Second Life only; OpenSim support is off.
- Windows is the build that is used and tested.
- There are no public builds yet.

## Build (Windows)

Prerequisites: see [doc/building_windows.md](doc/building_windows.md). KStorm builds the open (no
FMOD, no Havok) configuration with OpenSim support off:

```
autobuild configure -A 64 -c ReleaseFS_open -- --chan Research --no-opensim -DLL_TESTS:BOOL=FALSE
autobuild build -A 64 -c ReleaseFS_open --no-configure
```

The channel is `KStorm-Research`.

## Community respect

KStorm follows the [Third Party Viewer Policy](https://secondlife.com/corporate/third-party-viewers)
and the [Second Life Code of Conduct](https://github.com/secondlife/viewer?tab=coc-ov-file).
