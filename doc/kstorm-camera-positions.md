# KStorm Camera Positioning: per-sim camera views and camera Lock

Author: Kissmy Spicoli. Branch `feat-ks-camera-positions`.

## Request

KStorm menu > **Camera Positioning** toggles a floater modeled on Look Inventory. It saves,
lists, updates, renames and deletes labeled camera views: position x/y/z, yaw/pitch/roll and an
absolute zoom, per sim (as My Lights does). Views of other sims show disabled and labeled with
their sim. Double-click (or select + **Apply**) moves the camera to the view. **Lock** makes the
usual camera and avatar motion do nothing; Esc pressed three times always unlocks and returns
the camera to the avatar.

## Decisions (user, 2026-10-27)

- **Roll**: feasible without new camera code. AyaneStorm carries Alchemy's camera roll
  (`LLAgentCamera::mRollAngle`, `setRollAngle()`, applied at the end of `updateCamera()`).
- **Zoom** = the view angle (`CameraAngle`, the vertical FOV that Ctrl+0/8/9 and right-click
  zoom change). It is stored in degrees and shown as a factor: 60 degrees (`DEFAULT_FIELD_OF_VIEW`)
  divided by the angle, so 1.00x is normal. Apply sets `CameraAngle`.
- **Lock is a full freeze**: the rendered camera is pinned every frame, and avatar/camera key and
  mouse bindings, the joystick and mouselook do nothing.
- **Closing the floater** unlocks and resets the view, like Esc x3. Teleport, disconnect and quit
  also unlock.
- **The Lock toggle off** leaves the camera at the held view, movable from there. Esc x3 is the
  way back to the avatar.

## Files

| File | Role |
|---|---|
| `kscamerapositions.cpp/.h` | Records (load/save, per-sim), Apply, Lock state, hooks' entry points |
| `ksfloatercamerapositions.cpp/.h` | Floater `ks_camera_positions` and name dialog `ks_camera_position_edit` |
| `floater_ks_camera_positions.xml` | 300 px wide: list (View, Sim), details line, Apply / Save current as... / Update / Rename... / Delete... / Lock |
| `floater_ks_camera_position_edit.xml` | Name dialog (Save, Rename) |

`<KS:KS>`-tagged upstream edits: `menu_viewer.xml` (menu item), `llviewerfloaterreg.cpp`
(registration), `CMakeLists.txt`, `llappviewer.cpp` (hold branch, joystick skip),
`llviewerinput.cpp` (binding filter), `llviewerwindow.cpp` (Esc count) and
`llagentcamera.cpp` (no mouselook while locked).

## Records and storage

- One file, `ks_camera_positions.xml`, in `LL_PATH_PER_SL_ACCOUNT`. It holds an LLSD array of
  `{name, region, grid, position[3], yaw, pitch, roll, view_angle}`.
- This is the My Lights approach (`as_my_lights_autosave.xml`, where `ASLightRig` stores
  `mRegionName` + `mGrid`): records are tagged with the region name and the grid id
  (`LLGridManager::getGridId()`), and their positions are region-local.
- A record belongs **here** when its region and grid match the agent's region. Names are unique
  per sim, so the same label can exist on several sims.
- The list puts this sim's records first, sorted by name, then other sims' records sorted by sim
  and name. Other sims' rows are `enabled=false`, so `LLScrollListCtrl` never selects them, and
  their Sim column shows the sim's name. Every row's tooltip shows its values.
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

- **Unlocked** (`moveAgentCamera()`):
  1. Turn flycam off and leave mouselook.
  2. Call `unlockView()` to take the focus off the avatar.
  3. Call `setFocusGlobal()` 5 m ahead. A focus with no object zeroes the object FOV zoom, so
     `CameraAngle` alone sets the view.
  4. Call `setCameraPosAndFocusGlobal()`. Its usual camera animation is at most 1 s.
  5. Call `setRollAngle()`, then set `CameraAngle`.

  The camera then behaves as after any camera move: moving the avatar resets it, as usual.
- **Locked**: Apply replaces the held view (and sets `CameraAngle` for after the lock).

## Lock

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

  Closing the floater unlocks and resets. Lock is a static flag, never saved, so every session
  starts unlocked.

## Not covered / follow-ups

- The Move floater's on-screen buttons call `LLAgent` directly and are not blocked while locked.
- There is no bulk delete for other sims' records: they cannot be selected. Delete them from the
  sim they belong to.
- Applying a view while unlocked goes through `LLAgentCamera`, whose ground clamp
  (`getCameraMinOffGround()`) can lift a view saved below it, for example from flycam or a
  locked view. The clamp is off when `DisableCameraConstraints` is on. A locked Apply is exact.
- Esc x3 does not restore the view angle (`CameraAngle`) that an Apply set.
- A single Esc while locked still runs the menu's Reset View. With `ResetViewTurnsAvatar` on,
  the avatar turns once on the way to Esc x3.
- Moving the avatar by other means while locked (a scripted push, or a sit target moving a
  vehicle) still moves it; only the camera stays.
