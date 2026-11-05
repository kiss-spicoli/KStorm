# KStorm Camera Positioning: per-sim camera views and Lock Camera

Author: Kissmy Spicoli. Branch `feat-ks-camera-positions`.

## Request

KStorm menu > **Camera Positioning** toggles a floater modeled on Look Inventory. It saves,
lists, updates, renames and deletes labeled camera views: position x/y/z, yaw/pitch/roll and an
absolute zoom, per sim (as My Lights does). Views of other sims show disabled and labeled with
their sim; they can still be deleted. Double-click (or select + **Apply**) moves the camera to the view. **Lock Camera** makes the
usual camera and avatar motion do nothing; Esc pressed three times always unlocks and returns
the camera to the avatar.

## Decisions (user, 2026-10-27)

- **Roll**: feasible without new camera code. AyaneStorm carries Alchemy's camera roll
  (`LLAgentCamera::mRollAngle`, `setRollAngle()`, applied at the end of `updateCamera()`).
- **Zoom** = the view angle (`CameraAngle`, the vertical FOV that Ctrl+0/8/9 and right-click
  zoom change). It is stored in degrees and shown as a factor: 60 degrees (`DEFAULT_FIELD_OF_VIEW`)
  divided by the angle, so 1.00x is normal. Apply sets `CameraAngle`.
- **Lock Camera is a full freeze**: the rendered camera is pinned every frame, and avatar/camera key and
  mouse bindings, the joystick and mouselook do nothing.
- **Disable Camera Constraints** (check box under Draw Distance, 2026-10-28) is bound to the
  `DisableCameraConstraints` setting, the same one as Preferences > Move & View > View and
  Advanced > Disable Camera Constraints. KStorm turns it on by default (`app_settings/settings.xml`,
  tagged `<KS:KS>`). While on, `LLAgentCamera` does not pull the camera back to within draw
  distance of the avatar or above the ground, so an unlocked Apply lands exactly, also across a
  sim boundary. A locked Apply is exact either way. Only regions the viewer is connected to have
  anything to show (see Connected sims below: the simulator connects the regions near the camera).
- **Connected sims** (2026-10-28): views of a sim the viewer is connected to can be applied
  from a neighbour. Their Sim column reads "Connected: <sim>".
- **A view belongs to the sim the camera is over** (user, 2026-10-28: a view saved with the
  camera over a neighbour showed "this sim"). Save and Update tag the record with the connected
  region containing the camera (`LLWorld::getRegionFromPosGlobal()` of the camera origin, else
  the agent's region), with the position local to it. Views of this sim and of connected sims
  can be applied, updated and renamed. Update moves a view to the sim the camera is over now; if
  another view there has its name, Update refuses and says so.
- **No coordinate tooltips** (user, 2026-10-28): the names have no tooltip and the list has
  none, because a tooltip covered the names below. The details line under the list shows the
  selected view's values.
- **Show disconnected, requires teleport** (user, 2026-10-28): a check box under the list, on
  by default and remembered (setting `KSCameraPositionsShowDisconnected`, `<KS:KS>` in
  `app_settings/settings.xml`). Off, the list holds only this sim's and connected sims' views.
- **Height-only resize** (user, 2026-10-28): the floater resizes in height only
  (`enableResizeCtrls(true, false, true)`: no side bars or corner handles, as in
  `llfloatersettingscolor.cpp`). The list grows, and the controls below it follow the bottom
  edge. `save_rect` keeps the size across closing and sessions. `min_height` is the XUI height.
- **"Lock Camera"** (user, 2026-10-28): the Lock toggle is labeled Lock Camera, and every
  mention follows.
- **Camera Controls toggle** (user, 2026-10-29): Lock Camera shares its row with a
  **Camera Controls** toggle (each 137 px), so the floater keeps its height. It opens and closes
  the viewer's Camera Controls floater (orbit, pan and zoom) as Ctrl+K does
  (`FSToggleCameraFloater`): `fs_camera_small` while `FSUseSmallCameraFloater` is on, else
  `camera`, through `LLFloaterReg::toggleInstance()`. The toolbar's
  `toggleInstanceOrBringToFront()` was dropped (user, 2026-10-29): clicking the button takes
  focus from Camera Controls, so each click only raised it. `draw()` sets the button pressed
  while that floater is open, however it was opened. While Lock Camera is on, its controls have no visible
  effect: the idle update pins the rendered camera instead of running `updateCamera()`.
- **Roll row** (user, 2026-10-31): the bottom row shows the roll keys as a hint (`Roll: R / T`), with the
  Cameratools roll buttons to its left and right, same images and same click and hold speed. The hint
  reads the bindings, so it follows Preferences > Controls, and the buttons move to sit against it however
  long the key names are (up to 244 px of text). The floater grew 18 px to 412. Both buttons are
  off while Lock Camera is on, which also blocks the keys. A saved view already stores its roll.
- **Draw Distance** (user, 2026-10-28): a slider just below Lock Camera, bound to `RenderFarClip`
  (`control_name`), so it follows every other setter: Preferences > Graphics > General and
  Advanced, looks such as the Reference look, the Performance floaters, the `/dd` chat command.
  Nothing in code clamps the setting. The range is 8 to 1024 m in steps of 8 (user, 2026-10-28):
  wider at the low end than any other viewer control (Preferences > Graphics > General allows 32
  to 1024, Advanced 64 to 512).
- **Apply reaches disconnected sims** (user, 2026-10-28). Moving only the camera near a sim
  boundary connects sims several regions away from the avatar. The viewer never asks for a
  region (it adds one when the simulator sends EnableSimulator). Every AgentUpdate carries the
  camera position (`CameraCenter`) and the draw distance (`Far`) (`llviewermessage.cpp`), so the
  simulator connects the regions near the camera, not the avatar; this is inferred from that
  behaviour, as the server code is not available. Each record therefore keeps its region's
  global origin (`origin`, `LLViewerRegion::getOriginGlobal()`), set on Save and Update and
  refreshed whenever the region is connected (`refreshOrigins()`, also for records saved
  before origins were kept, and for a region moved on the map).
  The first version applied a disconnected record by moving the camera to origin + position
  and waiting for the simulator to connect the region. It is replaced by Teleport (below): a
  camera jump to a region no connected region borders may never get it connected (inferred, not
  verified), and the grid
  is too large for a hop-by-hop camera travel (user, 2026-10-28).
- **Teleport for disconnected sims** (user, 2026-10-28). Every row stays active; disconnected
  rows stay dimmed. On a disconnected row of this grid the Apply button reads **Teleport**
  (double-click does the same), after a confirmation. `KSCameraPositions::teleportTo()`:
  1. teleports the avatar to the view's camera position (`gAgent.teleportViaLocation()`),
     global = the stored origin + the region-local position. Without an origin it first asks the
     world map for the region by name (`LLWorldMapMessage::sendNamedRegionRequest()`), stores the
     origin from the reply's region handle, then teleports; no reply within 15 s cancels it with
     a message;
  2. once the teleport finishes (`LLViewerParcelMgr` teleport-finished signal) and 2 s more,
     after the arrival camera reset, applies the view as Apply does (unlocked: through
     `LLAgentCamera`). The avatar stays where it landed: a landing point or telehub may place it
     away from the camera;
  3. a failed teleport (the viewer shows why) drops the pending view. A landing in another,
     unconnected region says the view was not applied.

  Lock Camera turns off with the teleport, as on any teleport. Update and Rename stay with
  connected sims.
- **Region connections, as observed in the viewer** (user, 2026-10-28, the world map showing the
  camera's position):
  - The camera moves as expected, also several regions from the avatar.
  - When the camera is within draw distance of a region, the simulator connects that region and
    sends its content, also across several region hops away from the avatar.
  - A region connected a short while ago stays visible after the camera leaves, until the
    simulator stops sending it. The viewer keeps a region until the simulator disables it; it
    never drops one because the camera moved away.
  - A teleport resets the connected set to the regions around the landing point, and the
    earlier connections time out. Lock Camera turns off on any teleport too.
- **Closing the floater** unlocks and resets the view, like Esc x3. Teleport, disconnect and quit
  also unlock.
- **The Lock Camera toggle off** leaves the camera at the held view, movable from there. Esc x3 is the
  way back to the avatar.

## Files

| File | Role |
|---|---|
| `kscamerapositions.cpp/.h` | Records (load/save, per-sim), Apply, Lock Camera state, hooks' entry points |
| `ksfloatercamerapositions.cpp/.h` | Floater `ks_camera_positions` and name dialog `ks_camera_position_edit` |
| `floater_ks_camera_positions.xml` | 300 px wide, height-resizable: list (View, Sim), Show disconnected (requires teleport), details line, Apply / Save current as... / Update / Rename... / Delete... / Lock Camera, Camera Controls, Draw Distance, Disable Camera Constraints |
| `floater_ks_camera_position_edit.xml` | Name dialog (Save, Rename) |

`<KS:KS>`-tagged upstream edits: `menu_viewer.xml` (menu item), `llviewerfloaterreg.cpp`
(registration), `CMakeLists.txt`, `llappviewer.cpp` (hold branch, joystick skip),
`llviewerinput.cpp` (binding filter), `llviewerwindow.cpp` (Esc count) and
`llagentcamera.cpp` (no mouselook while locked).

## Records and storage

- One file, `ks_camera_positions.xml`, in `LL_PATH_PER_SL_ACCOUNT`. It holds an LLSD array of
  `{name, region, grid, position[3], yaw, pitch, roll, view_angle, origin[3]}`. `origin`, the
  region's global origin, is missing from records saved before 2026-10-28 until their region is
  connected again.
- This is the My Lights approach (`as_my_lights_autosave.xml`, where `ASLightRig` stores
  `mRegionName` + `mGrid`): records are tagged with the region name and the grid id
  (`LLGridManager::getGridId()`), and their positions are region-local.
- The grid id is kept and compared in lower case. OpenSim builds (`fsgridhandler.cpp`) return the grid
  nick `agni`; `--no-opensim` builds (`llviewernetwork.cpp`, KStorm's) return `Agni`. Records saved by
  either build match in both. Older files keep their spelling until the next save.
- A record belongs **here** when its region and grid match the agent's region. Names are unique
  per sim, so the same label can exist on several sims. A record's sim is the one the camera was
  over when it was saved or last updated.
- A record is **connected** when its grid is the current one and its region is another region in
  `LLWorld::getRegionList()`: one the simulator has connected the viewer to, in Second Life a
  region within draw distance of the camera (see "Apply reaches disconnected sims"). Only these
  regions have anything loaded to show. The set changes as the camera and the avatar move, as
  draw distance changes and as regions go up or down; the
  floater's `draw()` compares `KSCameraPositions::connectedRegions()` (and the agent's region)
  with the last list and rebuilds the list on a change.
- The list puts this sim's records first, then connected sims' records, then the others; each
  group is sorted by sim and name. Connected sims' rows are drawn normally, their Sim column reads
  "Connected: <sim>", and **Apply** (and double-click), **Update**, **Rename...** and
  **Delete...** are enabled on them.
  Other sims' rows are dimmed (each cell is drawn in `ScrollDisabledColor`) but stay selectable,
  and their Sim column shows the sim's name. On such a row **Delete...** is enabled (the user,
  2026-10-27: deleting must not need a visit to the sim), and **Teleport** (the Apply button,
  relabeled) on any record of this grid;
  `KSCameraPositions::remove(record)` matches name, region and grid. With **Show disconnected**
  off, these rows are left out. The details line shows the selected view's values; rows have no
  tooltip.
- Angles are in degrees:
  - yaw = atan2(at.y, at.x), where 0 is east and 90 is north;
  - pitch = asin(at.z), positive looks up;
  - roll is measured from the unrolled frame (`frameAngles()`).
- Saving reads the rendered camera (`LLViewerCamera`: origin, at/left axes, `getView()`), so it
  also captures a locked or flycam view.

## Roll math

`LLAgentCamera::updateCamera()` builds an unrolled frame from a world up vector, then applies
`LLQuaternion(LLMatrix3(mRollAngle, 0, 0)) * frame`. Row 1 of `LLMatrix3(r, 0, 0)` is
`(0, cos r, sin r)` (`m3math.cpp` `setRot`), so the rolled axes are:

- left = cos r * left0 + sin r * up0
- up = cos r * up0 - sin r * left0

`viewAxes()` builds these axes for the held (locked) view. `frameAngles()` inverts them:
roll = atan2(left . up0, left . left0). Pitch is clamped to +/-89.5 degrees, where the unrolled
frame stays defined.

## Apply

`KSCameraPositions::apply(record)` finds the record's region among the connected regions
(`regionOf()`) and converts its region-local position with that region's origin
(`getPosGlobalFromRegion()`), so a connected neighbour's view lands where it was saved.

- **Unlocked** (`moveAgentCamera()`):
  1. Turn flycam off and leave mouselook.
  2. Call `unlockView()` to take the focus off the avatar.
  3. Call `setFocusGlobal()` 5 m ahead. A focus with no object zeroes the object FOV zoom, so
     `CameraAngle` alone sets the view.
  4. Call `setCameraPosAndFocusGlobal()`. Its usual camera animation is at most 1 s.
  5. Call `setRollAngle()`, then set `CameraAngle`.

  The camera then behaves as after any camera move: moving the avatar resets it, as usual.
- **Locked**: Apply replaces the held view (and sets `CameraAngle` for after the lock).

## Lock Camera

- **On**: the current rendered frame is held (global position, axes, view angle). Autopilot
  stops, and mouselook is left.
- **Pinning**: `llappviewer.cpp` idle calls `KSCameraPositions::holdCamera()` before the agent
  pilot, flycam and `gAgentCamera.updateCamera()` branches. While locked it sets
  `LLViewerCamera` view, origin and axes, the way `LLViewerJoystick::moveFlycam()` does, and the
  others are skipped. The pin wins over:
  - mouse orbit and alt-zoom;
  - the scroll wheel;
  - scripted (`llSetCameraParams`) and sit cameras;
  - camera animation.

  `LLAgentCamera` state may drift underneath; it is discarded on unlock.
- **Avatar and camera bindings**: `LLViewerInput::scanKey()` / `scanMouse()` swallow the
  bindings in `KSCameraPositions::blocksAction()`:
  - walking, running, flying, jumping, turning, sit toggle;
  - every camera spin/pan/move/roll;
  - `walk_to` and `teleport_to`.

  Chat, gestures, media, voice and script mouse controls still work.
- **Joystick**: `scanJoystick()` is skipped while locked.
- **Mouselook**: `changeCameraToMouselook()` returns early while locked, because mouselook
  steers the avatar with the mouse.
- **Off with the toggle**: the held view is handed to `LLAgentCamera` without animation, so the
  camera stays put.
- **Esc x3**: `LLViewerWindow::handleKey()` reports every Esc before focus dispatch, so it works
  with any floater focused. Three presses within 1.5 s unlock, then run `handle_reset_view()`.
  The flag is cleared first, because a reset would not show while the camera update is skipped.
  Holding Esc also counts, since key repeats are presses.
- **Safety unlocks** (polled in `holdCamera()`):
  - any teleport state other than `TELEPORT_NONE` unlocks and resets, because the held view
    belongs to the region being left;
  - no region, startup state not `STATE_STARTED`, or the app exiting unlocks without a reset.

  Closing the floater unlocks and resets. Lock Camera is a static flag, never saved, so every session
  starts unlocked.

## Not covered / follow-ups

- The Move floater's on-screen buttons call `LLAgent` directly and are not blocked while locked.
- Applying a view while unlocked goes through `LLAgentCamera`, whose ground clamp
  (`getCameraMinOffGround()`) can lift a view saved below it, for example from flycam or a
  locked view. The clamp is off when `DisableCameraConstraints` is on, KStorm's default and the
  floater's Disable Camera Constraints check box. A locked Apply is exact.
- Esc x3 does not restore the view angle (`CameraAngle`) that an Apply set.
- A single Esc while locked still runs the menu's Reset View. With `ResetViewTurnsAvatar` on,
  the avatar turns once on the way to Esc x3.
- Moving the avatar by other means while locked (a scripted push, or a sit target moving a
  vehicle) still moves it; only the camera stays.
