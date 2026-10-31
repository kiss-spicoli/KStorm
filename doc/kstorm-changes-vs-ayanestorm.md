# KStorm: changes compared with AyaneStorm

A plain-language list of everything KStorm adds to AyaneStorm, grouped by feature, with the
commits for each. Kept up to date as KStorm work is merged into the `kstorm` branch.

- **Base:** AyaneStorm `master` as of 2026-10-06 (`c1b790e08bd`, `upstream/master`). KStorm
  contains all of it.
- **Last updated:** 2026-10-31, with the merge of `feat-ks-ci-mac-linux`.
- **KStorm's own work:** 176 commits (Kissmy Spicoli, Oct 18 to Oct 31), 299 files, about 17,000
  lines added.
- **Not yet in KStorm:** 19 newer commits of AyaneStorm's own on its `ayanestorm-dev` branch (for
  example VBAO, an improved ambient occlusion), plus a merge of newer Firestorm and Linden Lab
  code.

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
  of at a joint. (`f76666a5021`, `e412527f421`)
- **Only My Lights.** One switch hides every other light in the scene, including other people's
  worn lights. A "Max lights" slider limits how many lights draw at once. (`beafaf4d788`)
- **Boost.** Each light can be up to 4x brighter than Second Life normally allows.
  (`165a67322ee`)
- **Dark spots.** A spotlight can darken where it points instead of lighting it, like a
  photographer's flag. (`ff0b99896c1`)
- **Eye mask radius.** A slider for this setting, which was reachable only in Debug Settings.
  (`b910c70b95b`)

Details: `doc/my-lights.md`.

## 2. Smarter choice of which lights draw

When there are more lights than the viewer draws, AyaneStorm keeps the ones closest to the
camera. KStorm can optionally keep the ones closest to what you are photographing (whatever you
Alt-clicked), which suits portraits. (`76c8b73a103`)

Details: `doc/ks-local-light-sort.md`.

## 3. Hair occlusion

Your own semi-transparent (alpha) hair gets realistic shadowing from your head, neck and
shoulders. Before, it could look flat or glow unnaturally. (`ed6e424eff9`, fixes
`fa0862ca461`, `c40889ac44c`)

Details: `doc/ks-hair-occlusion.md`.

## 4. KStorm Lighting: one home for lighting controls

- **The early version** was a "KS" tab inside Environment Effects, then its own floater under a
  new **KStorm** menu. (`f4c4bceda95`, `a7ad889681f`, `b50834bf4e1`)
- **It became KStorm Lighting (Alt+Shift+L).** A narrow launcher with one button per topic opens
  13 small panes, such as Quick, Sun/Moon, Ambient and Shadows. They gather controls that were
  spread across Preferences, Personal Lighting and AyaneStorm's floaters. (`58765c91a30`)
- **Clearer names.** Controls say what they do, for example Sunlight Gain, Sky Ambient Gain and
  Surface Ambient Gain. (`4cce546195d`, `3a63198baa7`)
- **Docking.** Panes snap beside the launcher and line up with their buttons. They switch sides
  near a screen edge and stay aligned when the launcher moves. They collapse to a header bar with
  a click on the header or the minimize button. One dock button opens or closes them all, as you
  left them. Closing the launcher hides them. (`83fef239cf6`, `72c5e9387cc`, `eed9edc7ed8`,
  `5d5cdfab790`, `fcd8f11814b`, `65c22cf2df0`, `453cb84d733`, `c5bb6dd225a`, `73c57ed882c`,
  `83a6cbc387a`, `e9dbbbf771a`, `95b050b644b`)
- **AyaneStorm's large floaters stay unchanged.** Environment Effects, Viewer Lights and Camera
  Effects get a small header stand-in that docks beside the launcher and opens or closes the
  real floater. (`8832f0b7343`, `ef6f51c8d3c`, `d8aa7fe555d`)
- **Controls that would do nothing are dimmed.** Each on/off switch has the controls it governs
  indented directly below it, and they grey out while it is off: for example the shadow sliders
  under Shadows, the glow sliders under Glow, the Dynamic exposure tuning under Dynamic exposure,
  and Worn lights under Only Viewer Lights. This also shows what Motion mode turned off. Hair
  Occlusion works the same way. (`1a8d4134002`, `e4b692235af`)
- **Glow & bloom.** The Glow pane is now "Glow & bloom" and says what it really controls: the
  viewer's one glow pass, which blurs glowing faces, anything brighter than a threshold (the
  sun and the sky around it included) and AyaneStorm's bright-surface bloom. Two new sliders,
  Bloom threshold and Bloom amount, set how much of the bright scene blooms, and the tooltips
  explain that a wide glow at low resolution turns blocky. (`1a8d4134002`, `0c76b76f294`)
- **Its own icons.** Every launcher button now has a KStorm icon, drawn as one consistent set:
  the same light grey, similar size and similar brightness on the button. New: Quick (a
  lightning bolt), Sun/Moon (a sky dome over the horizon), Atmosphere (haze layers), Clouds,
  Water (waves), Reflections (a mirror), Viewer Lights (a bulb), Shadows (a light, a card and
  its shadow), Exposure (the camera's +/- symbol), Glow & bloom (a glowing filament), Depth of
  field (a lens aperture) and Vignette. Look Inventory, Camera Effects, All lights and
  Environment Effects keep their pictures, adjusted to match. (`61f4a04bff8`, `f9f873a1060`,
  `10bac4f86ef`, `eea48454397`, `0cbbea227e7`, `0783eb39df8`, `4e5a529b8f2`, `7463cb52f5f`,
  set v2 `08e0b0d8cfa`)
- **Layout.** The launcher is always the compact icon strip, with no footer. Pane headers are
  taller and panes are 300 px wide. A Hair... button sits on Quick. (`7b5783c2f25`,
  `2b061db5011`, `f849cb87a61`, `47e14bc389a`, `34405ce728c`)
- **First open.** The first time the launcher opens, it goes near the right edge of the screen,
  halfway up, or higher if a pane would otherwise hang below the screen. Every pane and proxy is
  docked and closed on its left, and the launcher is narrow. The dock button then opens every
  pane collapsed. AyaneStorm's own windows are not moved. (`9dbc85c5e28`, `5128d9a831f`)

Details: `doc/kstorm2-lighting-UI-design.md`.

## 5. Look Inventory and KS Reference

- **Look Inventory** (named Lighting Presets until Oct 27). Save your lighting settings as a
  named look, then apply or turn it off. Turning a look off puts everything back as it was.
  The pane also has rename, duplicate and contents views. (`b82da93d72f`, `645b0f057bd`,
  `b39075135f0`; renamed `c91f38523eb`)
- **KS Reference** is the Reference look, and the viewer labels it so: a fixed, known setup of
  sky, lighting and rendering. Builders use it as a base for textures, lighting and looks, and
  make their changes as deltas from it. It cannot be edited; Duplicate makes a copy to vary
  (`f5222ecbe4f`).
  It sets the sky and about 54 settings. Its values were tuned along the way:
  - draw distance 64 m (`ac3ade93b19`)
  - shadow resolution 1.5x (`7269fb7495b`, `480e7032c2a`, tooltip `fe6d3dc0140`)
  - textures no longer reload when you switch back to the viewer, and two shadow layers
    instead of three (`0e93d03982a`)
  - glow blurred 3 times, the most Preferences allows, instead of the default 2
    (`1a8d4134002`), at full glow resolution, 1024 rows instead of 512 (`0c76b76f294`)
  - keeps `RenderDeferredAtmospheric` on. This setting controls all sun and ambient light on
    normal surfaces, and turning it off leaves them black except for local lights. Its misleading
    Atmospheric pass box is gone from the Atmosphere pane. It can only be turned off in Debug
    Settings, and every startup turns it back on, so that lasts one session. (`c9896848df9`,
    `22b1fcc7346`)
- **Saving a look makes it the active one** (`e52a1f164be`). The save took about 90 seconds
  and is now near-instant. Each look switch does one graphics rebuild instead of dozens. A
  Second Life bug that kept the shader cache from saving after each new build is fixed.
  (`9cad1bafe9c`)

## 6. Motion mode: fast frame rate while moving

- **One switch.** Turns off the expensive effects: shadows, god rays, ambient occlusion,
  reflections, mirrors, glow, depth of field, particles, local lights, and AyaneStorm's
  environment and camera effects. Turning it off restores them. Antialiasing and HDR stay on.
  Measured about 8 to about 65 FPS. (`732a59c26c5`)
- **Looks.** Looks never save Motion mode, and applying a look turns it off first.
  (`dda20e86190`)
- **Teleports and login.** It turns on by itself whenever you teleport, to another region or
  within one (for example a minimap click), and at login, before the world is first drawn, so
  loading starts fast and nothing renders at full quality first. The `KSMotionModeOnTeleport`
  setting turns both off. (`d7d55424dd4`, login `2f43e5feb32`)
- **Icon shows the state.** On the launcher, the Motion mode gauge reads fast while on and is
  mirrored to read slow while off. (`c573815cd86`)

## 7. Camera Positioning: saved camera views and Lock Camera

- **Camera Positioning floater.** KStorm menu > Camera Positioning opens a 300 px floater laid
  out like Look Inventory. It saves the current camera view under a name: position, yaw, pitch,
  roll and zoom (the view angle, shown as a factor where 1.00x is normal). You can apply,
  update, rename and delete views. Double-click or Apply moves the camera there, roll
  included. (`9d8dc8f3832`)
- **Per sim.** Views are stored per sim, the way My Lights stores lights left in place. A view
  belongs to the sim the camera was over when it was saved or updated, which can be a neighbour
  of yours. Other sims' views are dimmed and show the sim's name. They cannot be edited from
  elsewhere, but they can be deleted from anywhere, and reached by Teleport (below).
  (`9d8dc8f3832`, `43b3acef559`; camera's sim `d3e84b1a461`)
- **Lock Camera.** Freezes the camera and the avatar. While on, the camera keys, the mouse, scripted
  and sit cameras, movement keys, click-to-walk, the joystick and mouselook do nothing.
  - Pressing Esc three times always unlocks and returns the camera to the avatar.
  - Closing the floater, a teleport, a disconnect or quitting also unlock.
  - Turning the toggle off leaves the camera where it is.

  (`9d8dc8f3832`, joystick reset on unlock `2441b385ef2`)
- **Disable Camera Constraints** check box in the floater, and **on by default** (AyaneStorm:
  off). The camera can go beyond draw distance from the avatar and below the ground, so Apply
  lands exactly, also across a sim boundary. It is the same setting as Preferences and the
  Advanced menu. (`3121c13e9fc`)
- **Connected sims.** Views of a sim the viewer is connected to (one it has loaded, within
  draw distance of the camera) can be applied, updated and renamed from where you are. Their
  Sim column reads "Connected:" and the sim's name. (`65a36e9b9c1`, editing `d3e84b1a461`)
- **Teleport to other sims' views.** On a dimmed view the Apply button reads Teleport: after a
  confirmation it teleports you to the view and, once you land, moves the camera to it. You stay
  where you land. Each view keeps its sim's position on the grid; older views look their sim up
  on the world map. (`8ae2d9b8873`, Teleport `618a05233f4`, observed region connections
  `55dee732401`)
- **Show disconnected, requires teleport** check box, on by default and remembered: off, the
  list shows only this sim's and connected sims' views. (`d3e84b1a461`, label `618a05233f4`)
- **Floater.** It resizes in height only and keeps its size across closing and sessions. View
  names have no tooltip, so nothing covers the list. The Lock toggle is labeled Lock Camera.
  (`d3e84b1a461`)
- **Camera Controls** toggle beside Lock Camera. It opens and closes the Camera Controls floater
  (orbit, pan and zoom) as Ctrl+K does, the small one when that option is on, and shows pressed
  while it is open. (Committed on `kstorm` with this entry, so no hash here.)
- **Draw Distance** slider below Lock Camera, 8 to 1024 m. It is the same setting as
  Preferences and follows every other setter, such as the Reference look and `/dd`.
  (`b76c5eb6089`, range `7914e82ebae`)

Details: `doc/kstorm-camera-positions.md`.

## 8. Performance check: the `ksperf` chat command

Type `ksperf` in nearby chat (the command word is the `KSCmdLinePerf` setting). The answer
shows in your own nearby chat only; it is never said aloud or sent to a script channel.
(`1b79e96884c`)

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
- **Delta.** `ksperf delta` compares with the last `ksperf` or `ksperf delta` report and
  shows how long ago that was, then only what changed: the look, the mode, toggles, and KS
  Reference values that moved away, moved again or came back (each with what it was). The
  figures now always show, each with its signed change, e.g. `FPS: 42.1 (+6.3)`. The last
  report is kept in memory until logout; timed samples do not replace it. With no report yet,
  it gives the full report.
- **End marker.** Each report ends with a row of `=` (the sample's after its figures, not
  when it starts).
- Any other word after `ksperf` gets a one-line usage reminder.

## 9. Clear View: only the world on screen

KStorm menu > KS Clear View, or **Alt+Shift+U**, hides everything but the world, for screen
shots taken outside the viewer: menus, bars, toolbars, floaters, chat, notifications, avatar name
tags, hover text and HUD attachments. The world fills the whole window. Press it again to bring
everything back; a HUD or UI you had already hidden stays hidden. It always starts off, and the
shortcut works while you type in chat. (`5bca4f2e6be`)

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
screen first, so the pause is explained, then compiles and draws the world again. (`aab646f2c25`)

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
is built on. (`3b2b3c33241`)

- **Name and channel.** The window title, About, the exe's file details and the name sent to
  the grid at login say KStorm. The channel is `KStorm-Research`: an unsupported research viewer.
- **Second Life only.** OpenSim support is off: no grid selector (turned off at every launch,
  even if saved settings turn it on, and its Preferences checkbox is hidden), no OpenSim tab in
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
- **App icon.** KStorm has its own icon: an orange firebird, wings spread and head thrown back,
  with no background behind it (the small Windows/Linux bitmap is on black). It is the window,
  taskbar and exe icon, and the macOS and installer icons in the same folder. AyaneStorm's
  unused icon files there are gone. The installer, crash reporter and other copies elsewhere
  still show AyaneStorm's art. (`59ebef89371`)
- **Ayane's names stay.** Features Ayane created keep their AyaneStorm names: the AyaneStorm
  menu and Preferences tab, AyaneStorm Depth of Field, AyaneStorm stars and the rest.
- **README.** The KStorm home page: the mission (Looks and Scenes, and research), what KStorm
  adds, credit to Ayane with links to her repository, status and build. AyaneStorm's README is
  kept as `doc/ayanestorm-README.md`. (`8972fffd967`)
- **Login screen.** Drawn by the viewer itself, with no web page, so the built-in browser does
  not even start before you log in and nothing is sent anywhere. One background fills the
  screen (a purple gradient, `ks_login_bg`). The KStorm logo sits centred near 40% of the
  height: "KStorm" in heavy type, an orange beam (the Log In button's colour, `#F97D00`) running
  past its end into a starburst, and "Research" spaced out beneath. A small label at the bottom
  right shows the version, build number and build time. The username, password, location and
  Log In controls sit centred, their bottom on the lower-third line. The firebird logo, Mode
  selector, Create an account and Forgot password? are gone, and so is Firestorm's page with
  its tiles, blogs and splash-screen settings. Maximizing no longer letterboxes or flashes.
  The pictures are redrawn with `scripts/content_tools/ks_render_login_art.ps1`. A first
  version used a local web page (`721ecc549f3`); this one replaced it. (`f2ee5e838a6`,
  `494dea04776`)
- **No Report Problem.** Help > Report Problem, which files bugs with Firestorm, is hidden on
  the login screen and in world. (`721ecc549f3`)
- **No password eye.** The login page's show and hide password buttons did not work and are
  removed. (`063c3ade3e0`)
- **No contact with Firestorm's servers.** The viewer used to fetch Firestorm's messages, version
  blocks, team list, asset list, client tags, grid list and grid news at every start. All of that
  is gone from the code and settings, so startup no longer waits for it. The grid news checkbox is
  gone from Preferences. Firestorm's team flags and `/reqsysinfo` no longer apply, and neither does
  its global asset list. (`5726da43aa1`, `431889dbdee`)
- **Links.** Help and F1 open KStorm's README, whose new Troubleshooting section replaces the wiki
  links in messages and Preferences. Downloads go to KStorm's Releases page; sign-up and account
  support go to Second Life. Messages about a broken install point to the README. Firestorm's
  troubleshooting, support group, classes, events and social menu items are gone, and text like
  `FIRE-123` in chat no longer links to Firestorm's JIRA. (`c12d8898d9a`, `431889dbdee`,
  `87ea69ddb5f`)
- **Primfeed hidden.** Its toolbar button, menu items and snapshot button are gone, and Firestorm's
  API key is deleted from the settings. (`82c2d9cf3fc`)
- **RLVa off.** RLVa can't be switched on, a saved "on" is cleared at startup, and its menu and
  Preferences controls are gone. Items that depend on RestrainedLove won't respond. (`be0e9d5a3f3`)
- **Installer.** No `Phoenix` in the file names, a `SOFTWARE\KStorm` registry key, KStorm as the
  publisher in the installed apps list, and no prompt to download another build. Uninstalling no
  longer deletes Firestorm's folder in `ProgramData`, and answering Yes to removing data now clears
  the settings folder the viewer really uses, keeping chat logs.
  (`9a3a7ed93f9`, `5007ac0e996`, `837bea300fa`, `ba82056b291`)
- **AVX2 and current hardware.** Builds assume AVX2 and 202x hardware, with a display of at least
  1K and 4K preferred. There is no build for older processors. (`4066bfbe5fe`)
- **Releases.** The GitHub workflow can make a draft pre-release, using the release page in
  `.github/kstorm-release-page.md`, which carries the disclosures and privacy statement Linden
  Lab's viewer policy requires. (`527fa9878f4`, `34ebe455a19`)
- **Notification icon.** `kstorm_icon.png`, used for Growl notifications, has new art, and the
  README's title now shows it too, replacing the old `doc/kstorm_256.png`.
  (`092ac4c97ea`, `e6c2934413d`, `c58e4ed6039`)

Details: `doc/kstorm-rebrand-plan.md`.

## 12. Fixes to existing behavior

- **Advanced Lighting.** Three atmosphere sliders (Density, Distance, Maximum Altitude) were
  hidden in AyaneStorm; they are shown and working. (`afa5c90931e`)
- **Haze bug.** Setting Haze Density to 0 could break the sky math (a divide-by-zero).
  (`491e2c4b964`)
- **Older skies.** They no longer look darker than they should, because probe fill can never
  drop to zero. (`532ab810f21`)
- **Conversations window.** Its minimum height is halved to 141 px, so it can be made much
  shorter. Torn-off windows keep their own limits. (`c8b73f87333`)

## 13. Renames and documentation

- **Naming.** All KStorm code uses `ks*` names and `<KS:KS>` tags, so it is easy to tell apart
  from AyaneStorm's. (`51295134d8e`)
- **Design and research docs.** Lighting factors, UI design and decisions; no code. (`148ca6d0c3f`, `bc9e1ec3fc3`, `6481f69ea73`, `ac19c718818`, `8cf26fc8201`,
  `213d02378ce`, `12b3036fe26`)
- **This document.** (`161a0c9d973`)
- **KStorm's own GitHub build.** A workflow started by hand checks changed shaders in the variants
  the viewer compiles and builds an unsigned Windows installer, with no secrets. Optional Mac and
  Linux jobs build a disk image and an archive the same way; these aren't released, and neither has
  been run or tested yet. Firestorm's inherited workflows stay in the repo. (`fd5e6b6b825`,
  `d97e886eaaf`, `e33ee113494`)

## Keyboard shortcuts

Every key KStorm adds or changes. KStorm adds no Preferences > Controls key bindings.

| Keys | What it does | Where |
|---|---|---|
| Alt+Shift+L | KStorm menu > KStorm Lighting: opens the launcher, or closes it and its docked panes (`58765c91a30`) | section 4 |
| Alt+Shift+U | KStorm menu > KS Clear View: hides all UI, name tags and HUDs, and shows them again (`5bca4f2e6be`) | section 9 |
| Alt+Shift+Y | View > Show User Interface, moved from Alt+Shift+U (`5bca4f2e6be`) | section 9 |
| Esc three times | Turns Lock Camera off and returns the camera to the avatar (`9d8dc8f3832`) | section 7 |

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
- for the login screen: the login panel (`fspanellogin.cpp`, `panel_fs_nui_login.xml`), the icon
  list (`textures.xml`) and the Help menus (`menu_login.xml`, `menu_viewer.xml`)
- for release preparation: the Firestorm feed and grid fetches (`fsdata.cpp`, `llstartup.cpp`,
  `llviewernetwork.cpp`, `settings.xml`), RLVa (`rlvhandler.cpp`, `llappviewer.cpp`), chat links
  (`llurlregistry.cpp`), error messages (`llappviewer.cpp`, `lltransutil.cpp`, `llerror.cpp`), the
  installer and packaging scripts, Preferences panels, and links in the skin files, including the
  translated ones
- for the Conversations window's minimum height: `fsfloaterimcontainer.cpp`
- for turning `RenderDeferredAtmospheric` on at startup: `llappviewer.cpp`
- for removing the login password eye: `panel_fs_nui_login.xml`, `panel_fs_login.xml`
