/**
 * @file ksshadernotice.h
 * @author Kissmy Spicoli
 * @brief Shader-compile notice: a "Compiling shaders" screen shown before a shader
 *        reload, so the pause that follows (seconds to a minute and more) is explained.
 *
 * Compiling runs on the main thread, which also draws the window, so nothing can be
 * drawn while it runs; the window keeps its last frame. A reload requested by a
 * render setting (requestReload()) is therefore held back: the next frames show the
 * notice on black instead of the world, then the reload runs with the notice on
 * screen, then rendering resumes. The world is not drawn while the reload waits:
 * the settings have already changed, and a newly enabled effect would bind a shader
 * that is not loaded yet (LLGLSLShader::bind() asserts in every build). Requests
 * made while one waits share its reload. Each reload logs its time (ShaderLoading).
 *
 * The reload runs at once, without a notice, before login, while quitting, while
 * the window is hidden or minimized, while the UI is hidden (Ctrl+Alt+F1, Clear
 * View), during the teleport or login screen, and when requested from inside a
 * frame (then the frame needs the shaders now).
 */
#ifndef KS_SHADERNOTICE_H
#define KS_SHADERNOTICE_H

namespace KSShaderNotice
{
    // In place of LLViewerShaderMgr::setShaders() where a render setting changed
    // (llviewercontrol.cpp).
    void requestReload();

    // Main loop, in place of the world frame (llappviewer.cpp): true when this frame
    // showed the notice or ran the held reload, so display() and the updates after it
    // are skipped.
    bool runFrame();

    // Marks the world frame (display() and the updates after it), during which a
    // reload is never held back.
    struct FrameScope
    {
        FrameScope();
        ~FrameScope();
    };
}

#endif // KS_SHADERNOTICE_H
