# KStorm Clear View (KSClearView, Alt+Shift+U)

Clear View leaves only the world on screen, for screen shots taken outside the viewer (Windows
Snipping Tool, Print Screen, OBS). It hides what a snapshot without UI and HUD hides.

## Use

- **KStorm menu > KS Clear View**, shortcut **Alt+Shift+U**. A check item; press the shortcut
  again to bring everything back.
- The shortcut works while the menus are hidden and while the chat bar or a floater has keyboard
  focus (menu accelerators with Alt or Ctrl run before the focused control,
  `LLViewerWindow::handleKey()`). Like every menu shortcut it does nothing during a teleport to
  another region.
- Setting `KSClearView` (Boolean, default off, `Persist` 0): every session starts with the UI
  shown, so the login screen can never come up blank. It can also be flipped in Debug Settings.

## What it hides

| Hidden | How |
|---|---|
| All 2D UI: menu bar, navigation and status bars, toolbars, floaters, chat, IM and nearby chat toasts, notifications, tooltips, the mouselook crosshair, the teleport screen | `RENDER_DEBUG_FEATURE_UI` off: `render_ui_2d()` is skipped (`llviewerdisplay.cpp`) |
| Avatar name tags, object hover text (`LLHUDNameTag`, `LLHUDText`) | `LLHUDObject::renderAll()` runs only under the same UI flag |
| Beacons, property lines, tracking overlays, debug text | same UI flag (`render_hud_elements()`, `pipeline.cpp`, `LLViewerWindow::updateDebugText()`) |
| HUD attachments | `LLPipeline::sShowHUDAttachments = false` (`render_hud_attachments()`) |
| AyaneStorm DoF aperture progress counter | `ASDoFRenderer::drawProgress()` returns when the UI flag is off (`<KS:KS>` edit) |

With the UI flag off the world view also fills the whole window (`updateWorldViewRect`), the UI
receives no mouse or hover events, and keyboard focus is cleared each frame.

Still drawn, as in snapshots: selection outlines of edited objects (`renderSelections()` non-HUD
call), and the chromatic aberration center marker and chat-range spheres when their own toggles
are on. The OS mouse cursor is outside the viewer's control.

## Implementation

- `indra/newview/ksclearview.cpp/.h`: `KSClearView::onSettingChanged()`. On: records whether the
  UI flag and HUD attachments were shown, then hides them, using the same switches as
  `LLViewerWindow::rawSnapshot()`. Off: shows only what it hid, so a HUD hidden with
  View > Show HUD Attachments or a UI hidden with Ctrl+Alt+F1 stays hidden. The UI flag is
  toggled with `LLPipeline::toggleRenderDebugFeature()`, never with
  `setRenderDebugFeatureControl(bit, false)`, which clears every feature bit (`&= !bit`, stock
  bug).
- `llviewercontrol.cpp` `settings_setup_listeners()`: connects the `KSClearView` listener.
- `menu_viewer.xml`: KStorm menu item `KS Clear View` (`CheckControl` / `ToggleControl`).
- `app_settings/settings.xml`: `KSClearView`.

## Changes to stock behavior

- **View > Show User Interface** (`View.ToggleUI`, `setUIVisibility()`) moved from Alt+Shift+U to
  **Alt+Shift+Y**. Menu accelerators match the exact mask, and the first item with Alt+Shift+U
  would take the key. Its confirmation notice (`ConfirmHideUI`, `[SHORTCUT]` substitution in
  `LLViewToggleUI`, `llviewermenu.cpp`) names Alt+Shift+Y.
- **Snapshots.** `LLViewerWindow::rawSnapshot()` turned the UI flag back on after every snapshot,
  even when it had been off. It now restores it only if it was on before (as `simpleSnapshot()`
  and `cubeSnapshot()` already did), so a snapshot taken in Clear View, or with the UI off by
  Ctrl+Alt+F1, leaves it off.

## Not used: a Preferences > Controls key binding

A `ks_clear_view` action in `llviewerinput.cpp` with a row in Preferences > Controls was tried
and dropped in favour of the menu item. Notes for anyone revisiting it:

- Menu accelerators handle a key first; a binding is skipped when the UI handled the key
  (`mKeyHandledByUI`), so the stock Alt+Shift+U menu item had to move either way.
- Global bindings (`REGISTER_KEYBOARD_GLOBAL_ACTION`) ignore key repeat (holding the key toggles
  repeatedly) and match a superset mask, so Alt+Shift+U also fires on Ctrl+Alt+Shift+U.
- Regular bindings do nothing while a text field has focus.
- A user `key_bindings.xml` in `user_settings` replaces the app defaults entirely
  (`LLAppViewer::loadKeyBindings()`); new default bindings never reach an existing user.
