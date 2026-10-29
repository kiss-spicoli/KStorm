# KStorm: changes compared with AyaneStorm

A plain-language list of everything KStorm adds to AyaneStorm, grouped by feature, with the
commits for each. Kept up to date as KStorm work is merged into the `kstorm` branch.

- **Base:** AyaneStorm `master` as of 2026-10-06 (`c1b790e08bd`, `upstream/master`). KStorm
  contains all of it.
- **Last updated:** 2026-10-29, at `kstorm` commit `6893457c26b`.
- **KStorm's own work:** 119 commits (Kissmy Spicoli, Oct 18 to Oct 29), 173 files, about 15,340
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
- **Glow & bloom.** The Glow pane is now "Glow & bloom" and says what it really controls: the
  viewer's one glow pass, which blurs glowing faces, anything brighter than a threshold (the
  sun and the sky around it included) and AyaneStorm's bright-surface bloom. Two new sliders,
  Bloom threshold and Bloom amount, set how much of the bright scene blooms, and the tooltips
  explain that a wide glow at low resolution turns blocky. (`096c0a97a40`, `5ec4b3d7f6e`)
- **Its own icons.** Every launcher button now has a KStorm icon, drawn as one consistent set:
  the same light grey, similar size and similar brightness on the button. New: Quick (a
  lightning bolt), Sun/Moon (a sky dome over the horizon), Atmosphere (haze layers), Clouds,
  Water (waves), Reflections (a mirror), Viewer Lights (a bulb), Shadows (a light, a card and
  its shadow), Exposure (the camera's +/- symbol), Glow & bloom (a glowing filament), Depth of
  field (a lens aperture) and Vignette. Look Inventory, Camera Effects, All lights and
  Environment Effects keep their pictures, adjusted to match. (`88c36e93f05`, `96aa43f58ee`,
  `9281361b4a4`, `26d77b6959c`, `04a6fcd486c`, `16d84a6eca5`, `6d796f226c7`, `121459cced7`,
  set v2 `b6d11547d45`)
- **Layout.** The launcher is always the compact icon strip, with no footer. Pane headers are
  taller and panes are 300 px wide. A Hair... button sits on Quick. (`53138cb89d8`,
  `68d6e528bd7`, `a6785359376`, `5485d81b9f0`, `4a31d052c89`)

Details: `doc/kstorm2-lighting-UI-design.md`.

## 5. Look Inventory and KS Reference

- **Look Inventory** (named Lighting Presets until Oct 27). Save your lighting settings as a
  named look, then apply or turn it off. Turning a look off puts everything back as it was.
  The pane also has rename, duplicate and contents views. (`79a3ce0ee5b`, `d8ffd8d668b`,
  `47ec7d2fa1d`; renamed `d6a1cd28377`)
- **KS Reference** is the Reference look, and the viewer labels it so: a fixed, known setup of
  sky, lighting and rendering. Builders use it as a base for textures, lighting and looks, and
  make their changes as deltas from it. It cannot be edited; Duplicate makes a copy to vary
  (`f471888a20e`).
  It sets the sky and about 54 settings. Its values were tuned along the way:
  - draw distance 64 m (`e5d1fe07c2c`)
  - shadow resolution 1.5x (`ec7c5ce1f88`, `3b28af09464`, tooltip `f68c8bcea06`)
  - textures no longer reload when you switch back to the viewer, and two shadow layers
    instead of three (`177dfbf963a`)
  - glow blurred 3 times, the most Preferences allows, instead of the default 2
    (`096c0a97a40`), at full glow resolution, 1024 rows instead of 512 (`5ec4b3d7f6e`)
- **Saving a look makes it the active one** (`aba9a069ceb`). The save took about 90 seconds
  and is now near-instant. Each look switch does one graphics rebuild instead of dozens. A
  Second Life bug that kept the shader cache from saving after each new build is fixed.
  (`226663fd6c0`)

## 6. Motion mode: fast frame rate while moving

- **One switch.** Turns off the expensive effects: shadows, god rays, ambient occlusion,
  reflections, mirrors, glow, depth of field, particles, local lights, and AyaneStorm's
  environment and camera effects. Turning it off restores them. Antialiasing and HDR stay on.
  Measured about 8 to about 65 FPS. (`b43fa8df854`)
- **Looks.** Looks never save Motion mode, and applying a look turns it off first.
  (`dcb5db0440f`)
- **Teleports and login.** It turns on by itself whenever you teleport, to another region or
  within one (for example a minimap click), and at login, before the world is first drawn, so
  loading starts fast and nothing renders at full quality first. The `KSMotionModeOnTeleport`
  setting turns both off. (`183b9769220`, login `0e5a1f17218`)
- **Icon shows the state.** On the launcher, the Motion mode gauge reads fast while on and is
  mirrored to read slow while off. (`c927b6a87c1`)

## 7. Camera Positioning: saved camera views and Lock Camera

- **Camera Positioning floater.** KStorm menu > Camera Positioning opens a 300 px floater laid
  out like Look Inventory. It saves the current camera view under a name: position, yaw, pitch,
  roll and zoom (the view angle, shown as a factor where 1.00x is normal). You can apply,
  update, rename and delete views. Double-click or Apply moves the camera there, roll
  included. (`9ec2c4ca485`)
- **Per sim.** Views are stored per sim, the way My Lights stores lights left in place. A view
  belongs to the sim the camera was over when it was saved or updated, which can be a neighbour
  of yours. Other sims' views are dimmed and show the sim's name. They cannot be edited from
  elsewhere, but they can be deleted from anywhere, and reached by Teleport (below).
  (`9ec2c4ca485`, `06b21643389`; camera's sim `d504d0cafe1`)
- **Lock Camera.** Freezes the camera and the avatar. While on, the camera keys, the mouse, scripted
  and sit cameras, movement keys, click-to-walk, the joystick and mouselook do nothing.
  - Pressing Esc three times always unlocks and returns the camera to the avatar.
  - Closing the floater, a teleport, a disconnect or quitting also unlock.
  - Turning the toggle off leaves the camera where it is.

  (`9ec2c4ca485`, joystick reset on unlock `ff67860fccf`)
- **Disable Camera Constraints** check box in the floater, and **on by default** (AyaneStorm:
  off). The camera can go beyond draw distance from the avatar and below the ground, so Apply
  lands exactly, also across a sim boundary. It is the same setting as Preferences and the
  Advanced menu. (`87dc7d7eafd`)
- **Connected sims.** Views of a sim the viewer is connected to (one it has loaded, within
  draw distance of the camera) can be applied, updated and renamed from where you are. Their
  Sim column reads "Connected:" and the sim's name. (`65dd3a3654e`, editing `d504d0cafe1`)
- **Teleport to other sims' views.** On a dimmed view the Apply button reads Teleport: after a
  confirmation it teleports you to the view and, once you land, moves the camera to it. You stay
  where you land. Each view keeps its sim's position on the grid; older views look their sim up
  on the world map. (`484637bfcd1`, Teleport `9bb45147996`, observed region connections
  `c773353e82b`)
- **Show disconnected, requires teleport** check box, on by default and remembered: off, the
  list shows only this sim's and connected sims' views. (`d504d0cafe1`, label `9bb45147996`)
- **Floater.** It resizes in height only and keeps its size across closing and sessions. View
  names have no tooltip, so nothing covers the list. The Lock toggle is labeled Lock Camera.
  (`d504d0cafe1`)
- **Draw Distance** slider below Lock Camera, 8 to 1024 m. It is the same setting as
  Preferences and follows every other setter, such as the Reference look and `/dd`.
  (`d83720c41ec`, range `56bdf6f78a8`)

Details: `doc/kstorm-camera-positions.md`.

## 8. Performance check: the `ksperf` chat command

Type `ksperf` in nearby chat (the command word is the `KSCmdLinePerf` setting). The answer
shows in your own nearby chat only; it is never said aloud or sent to a script channel.
(`7b342044043`)

- **State.** Which look is on (or none), and Photo (slow) or Motion (fast) mode.
- **Toggles.** Every Quick pane check box, and Motion mode at login and teleport, on or off,
  each marked as matching KS Reference, what KS Reference has instead, or not part of it.
- **Moved from KS Reference.** Only the KS Reference values that the current ones differ from,
  with the Reference value beside each; the rest are counted as conforming. A value Motion mode
  is holding says what it goes back to. Values are compared by the type each setting declares,
  so a setting saved as 1 matches "on".
- **Figures now.** Frame rate over the last 200 frames, ping, the sim's frame rate and time
  dilation, the viewer's memory, free system memory and VRAM.
- **Timed sample.** `ksperf 10` (1 to 600 seconds) measures for that long and then reports
  once: average frame rate with the worst and best second, average frame time and the worst
  single frame, packets per second, packet loss, data received, ping, the sim's frame rate and
  time dilation, and memory and VRAM, with the lowest free texture memory during the sample.
  It says so if the look or Motion mode changed while it ran.

## 9. Clear View: only the world on screen

KStorm menu > KS Clear View, or **Alt+Shift+U**, hides everything but the world, for screen
shots taken outside the viewer: menus, bars, toolbars, floaters, chat, notifications, avatar name
tags, hover text and HUD attachments. The world fills the whole window. Press it again to bring
everything back; a HUD or UI you had already hidden stays hidden. It always starts off, and the
shortcut works while you type in chat. (`397213d4450`)

- **Show User Interface moved.** View > Show User Interface, which had Alt+Shift+U, is now
  **Alt+Shift+Y**, and its confirmation names the new key.
- **Snapshots keep the UI hidden.** A snapshot taken while the UI is hidden (Clear View or
  Ctrl+Alt+F1) no longer turns it back on.
- **Depth of field.** The aperture progress counter hides along with the UI.

Details: `doc/kstorm-clear-view.md`.

## 10. Shader-compile notice

Turning on a setting that needs new shaders (shadows, ambient occlusion, reflections, transparent
water, or a look, Motion mode or KStorm Lighting change that includes one) can freeze the viewer
for 10 to 90 seconds while they compile. KStorm now shows "Compiling shaders..." on a black
screen first, so the pause is explained, then compiles and draws the world again. (`9510c258777`)

- **Black, not over the scene.** The world can't be drawn while it waits: a frame with the new
  settings and the old shaders could crash the viewer.
- **One compile per batch.** Several settings changed together, such as applying Graphics
  preferences, compile once.
- **Timing in the log.** Each compile writes a line such as `KS shader reload took 23410 ms
  (3 requests)` to the log; search for "KS shader reload".
- **No notice when it can't help.** Before login, while quitting or minimized, while the UI is
  hidden (Clear View or Ctrl+Alt+F1), during teleport, for the Preferences quality slider, and
  when the viewer itself reloads shaders, they compile at once as before.

Details: `doc/kstorm-shader-notice.md`.

## 11. The KStorm name

KStorm now calls itself KStorm wherever the viewer's own name shows, and credits the viewers it
is built on. (`97d6a932d1f`)

- **Name and channel.** The window title, About, the exe's file details and the name sent to
  the grid at login say KStorm. The channel is `KStorm-Research`: an unsupported research viewer.
- **Second Life only.** OpenSim support is off: no grid selector, no OpenSim tab in
  Preferences, no grid list downloaded at startup, and no Vivox voice (Second Life uses WebRTC).
- **New data folders.** Settings, logins and chat logs live in `%APPDATA%\KStorm_x64`, the cache
  in `%LOCALAPPDATA%\KStorm_x64`. The AyaneStorm settings were copied over once; the AyaneStorm
  folders are kept as a backup.
- **About.** A new KStorm Credits tab tells the lineage: AyaneStorm by Chanayane, Firestorm,
  and Linden Lab's Second Life viewer. The tab AyaneStorm had named after itself, which holds
  Firestorm's credits, is now Firestorm Credits. Release Notes open the KStorm GitHub home.
- **No support links.** The About Info line and the crash, freeze and upload-error messages say
  KStorm is an unsupported research viewer, instead of sending you to Firestorm support.
- **Search.** The Search window is titled KStorm Search.
- **Icons.** The `kstorm_*` icons hold AyaneStorm's art for now, until KStorm artwork replaces
  them.
- **Ayane's names stay.** Features Ayane created keep their AyaneStorm names: the AyaneStorm
  menu and Preferences tab, AyaneStorm Depth of Field, AyaneStorm stars and the rest.
- **README.** Rewritten for KStorm; AyaneStorm's README is kept as `doc/ayanestorm-README.md`.

Details: `doc/kstorm-rebrand-plan.md`.

## 12. Fixes to existing behavior

- **Advanced Lighting.** Three atmosphere sliders (Density, Distance, Maximum Altitude) were
  hidden in AyaneStorm; they are shown and working. (`9b783fa6bf6`)
- **Haze bug.** Setting Haze Density to 0 could break the sky math (a divide-by-zero).
  (`2c8a7e9d438`)
- **Older skies.** They no longer look darker than they should, because probe fill can never
  drop to zero. (`456bd1193ec`)

## 13. Renames and documentation

- **Naming.** All KStorm code uses `ks*` names and `<KS:KS>` tags, so it is easy to tell apart
  from AyaneStorm's. (`230af14da33`)
- **Design and research docs.** Lighting factors, UI design, decisions and an AI-render idea; no
  code. (`a24c8275c11`, `a9dcf435756`, `78125eea578`, `fc115c89c70`, `ff95fe874e5`,
  `29f7828dc63`, `1fc45fbd589`)
- **This document.** (`7f8dcd24e29`)

## Keyboard shortcuts

Every key KStorm adds or changes. KStorm adds no Preferences > Controls key bindings.

| Keys | What it does | Where |
|---|---|---|
| Alt+Shift+L | KStorm menu > KStorm Lighting: opens the launcher, or closes it and its docked panes (`435f28b859a`) | section 4 |
| Alt+Shift+U | KStorm menu > KS Clear View: hides all UI, name tags and HUDs, and shows them again (`397213d4450`) | section 9 |
| Alt+Shift+Y | View > Show User Interface, moved from Alt+Shift+U (`397213d4450`) | section 9 |
| Esc three times | Turns Lock Camera off and returns the camera to the avatar (`9ec2c4ca485`) | section 7 |

While Lock Camera is on, the movement and camera keys do nothing; chat, gestures, media and
voice keys still work.

## Where AyaneStorm's own files were touched

Most KStorm work lives in new `ks*` files. AyaneStorm's and Second Life's own files were changed
only where hooking in was unavoidable, with small edits tagged `<KS:KS>`:

- the renderer (`pipeline.cpp`), the shader manager and a few shaders
- My Lights (`aslightrig*`, `asfloatermylights`)
- teleport and login handling (`llagent.cpp`, `llstartup.cpp`), settings, menus, the icon list (`textures.xml`) and Advanced
  Lighting
- the chat command line (`chatbar_as_cmdline.cpp`), for `ksperf`
- camera and input, for Camera Positioning's Lock Camera: the idle camera update (`llappviewer.cpp`),
  key and mouse bindings (`llviewerinput.cpp`), Esc (`llviewerwindow.cpp`) and mouselook
  (`llagentcamera.cpp`)
- for Clear View: the settings listener (`llviewercontrol.cpp`), the snapshot UI restore
  (`llviewerwindow.cpp`), the Show User Interface shortcut notice (`llviewermenu.cpp`) and the
  depth-of-field progress counter (`asdofrenderer.cpp`)
- for the shader-compile notice: the three settings handlers that reload shaders
  (`llviewercontrol.cpp`) and the main loop, which draws the notice in place of a frame
  (`llappviewer.cpp`)
- for the KStorm name: the app name (`indra_constants.h`, `strings.xml`), the build and
  packaging scripts (`llmanifest.py`, `fs_viewer_manifest.py`, `configure_firestorm.sh`,
  `CMakeLists.txt`, the Windows installer), the exe resources (`viewerRes.rc`), About and Search
  (`floater_about.xml`, `floater_fs_search.xml`), Growl notifications, and the macOS and Linux
  display text
