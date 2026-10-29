# KStorm shader-compile notice (KSShaderNotice)

When a render setting needs the shaders reloaded (shadows, ambient occlusion, reflections, the
lighting looks, Motion mode, most Graphics preferences), the viewer pauses for seconds to over a
minute while it compiles. KStorm shows **"Compiling shaders... The viewer pauses until this
finishes."** on a black screen first, so the pause is explained.

## Why a black screen

Compiling runs on the main thread, which also draws the window, so nothing can be drawn once it
starts; the window keeps its last frame (and after about 5 seconds Windows dims it as "Not
Responding"). The notice must therefore be on screen before the reload starts:

1. A render setting changes and asks for a reload (`KSShaderNotice::requestReload()`). The
   reload is held back.
2. The next two frames draw only the notice on black (`runFrame()`), and swap.
3. The frame after that runs the reload (`LLViewerShaderMgr::setShaders()`) with the notice on
   screen. Rendering resumes on the next frame.

The world is not drawn while the reload waits. The settings have already changed, and a newly
enabled effect would bind a shader that is not loaded yet: `LLGLSLShader::bind()` has
`llassert_always(mProgramObject != 0)`, which aborts in Release builds too. The frame step after
`display()` in the main loop (`gPipeline.mReflectionMapManager.update()`, the snapshot floaters)
also binds shaders, so the whole world frame is skipped, not only `display()`. `idle()` only
marks shader uniforms dirty (`LLEnvironment::update()`), so it keeps running.

Keeping the last world frame on screen instead of black would need a blit of the pipeline's final
render target; the probe-detail path has just recreated those buffers, so it is not done.

## When the reload runs at once (no notice)

Before login (`STATE_STARTED`), while quitting, while the window is hidden or minimized, while
the UI is hidden (Ctrl+Alt+F1 or KS Clear View, so screen shots stay clean), during the teleport
or login progress screen, and when requested from inside a world frame (`FrameScope`: the frame
needs the shaders now). A reload waiting for its notice is done then too.

## Coalescing and timing

Requests made while a reload waits share it, so changing several settings at once (Graphics
preferences OK, a look applied outside the KStorm batch) compiles once instead of once per
setting. Each reload logs its time and request count:

```
ShaderLoading: KS shader reload took 23410 ms (3 requests)
```

Search `AyaneStorm.log` (`%APPDATA%\AyaneStorm_x64\logs`) for `KS shader reload` to see which
changes cause the long pauses. The main loop timeout is paused around a held reload.

## Hooks

- `indra/newview/ksshadernotice.cpp/.h`: the module.
- `llviewercontrol.cpp`: `handleSetShaderChanged()`, `handleRenderTransparentWaterChanged()` and
  `handleReflectionProbeDetailChanged()` call `KSShaderNotice::requestReload()` instead of
  `setShaders()`. These are the settings-driven reloads, including the KStorm batch
  (`ks_end_render_settings_batch()`).
- `llappviewer.cpp` main loop: `KSShaderNotice::runFrame()` before the world frame;
  `FrameScope` around `display()` and the updates after it.

Not hooked, because they need the shaders at once or run while the GL state is being rebuilt:
`LLPipeline::init()`, `LLPipeline::restoreGL()`, `LLViewerWindow::checkSettings()` (inside
`display()`), `LLFeatureManager::setGraphicsLevel()` (graphics quality presets) and the GLTF
region feature. `LLPipeline::handleShadowDetailChanged()` has no callers.
