/**
 * @file ksshadernotice.cpp
 * @author Kissmy Spicoli
 * @brief Shader-compile notice: holds a shader reload back while a notice is shown.
 */

#include "llviewerprecompiledheaders.h"

#include "ksshadernotice.h"

#include "llapp.h"
#include "llappviewer.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llprogressview.h"
#include "llrender.h"
#include "llstartup.h"
#include "lltimer.h"
#include "llvertexbuffer.h"
#include "llviewerdisplay.h"
#include "llviewershadermgr.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "pipeline.h"

namespace
{
    // Notice frames swapped before the reload runs. One shows it; the second makes
    // sure it is the frame on screen with a queued or triple-buffered swap chain.
    constexpr S32 NOTICE_FRAMES = 2;

    bool sPending = false;
    S32 sNoticeFrames = 0;
    S32 sRequests = 0;
    S32 sFrameDepth = 0;

    bool windowShown()
    {
        LLWindow* window = gViewerWindow ? gViewerWindow->getWindow() : nullptr;
        return window && gViewerWindow->getActive() && window->getVisible() && !window->getMinimized();
    }

    // Whether a notice can be shown now; otherwise the reload runs at once.
    bool canHoldBack()
    {
        if (sFrameDepth > 0 || LLApp::isExiting() || gNonInteractive ||
            LLStartUp::getStartupState() < STATE_STARTED || !windowShown() ||
            !gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI) ||
            gTeleportDisplay)
        {
            return false;
        }
        LLProgressView* progress_view = gViewerWindow->getProgressView();
        return !(progress_view && progress_view->getVisible());
    }

    void reload(S32 requests)
    {
        LLTimer timer;
        LLViewerShaderMgr::instance()->setShaders();
        LL_INFOS("ShaderLoading") << "KS shader reload took " << (S32)(timer.getElapsedTimeF32() * 1000.f)
                                  << " ms (" << requests << " request" << (requests == 1 ? "" : "s") << ")" << LL_ENDL;
    }

    // One frame of black with the notice centered, as display_startup() draws a UI-only frame.
    void drawNotice()
    {
        LLGLSDefault gls_default;
        LLVertexBuffer::unbind();
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        {
            LLGLSUIDefault gls_ui;
            gPipeline.disableLights();
            gViewerWindow->setup2DRender();

            gUIProgram.bind();
            gGL.pushMatrix();
            gGL.pushUIMatrix();
            {
                const LLVector2& scale = gViewerWindow->getDisplayScale();
                gGL.scaleUI(scale.mV[VX], scale.mV[VY], 1.f);
                const LLRect window_rect = gViewerWindow->getWindowRectScaled();
                const F32 x = (F32)window_rect.getCenterX();
                const F32 y = (F32)window_rect.getCenterY();
                const LLColor4 text_color(0.9f, 0.9f, 0.9f, 1.f);

                LLFontGL::getFontSansSerifHuge()->renderUTF8("Compiling shaders...", 0, x, y + 4.f, text_color,
                    LLFontGL::HCENTER, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW);
                LLFontGL::getFontSansSerif()->renderUTF8("The viewer pauses until this finishes.", 0, x, y - 4.f,
                    text_color, LLFontGL::HCENTER, LLFontGL::TOP, LLFontGL::NORMAL, LLFontGL::NO_SHADOW);
            }
            gGL.popUIMatrix();
            gGL.popMatrix();
            gGL.flush();
            gUIProgram.unbind();
        }
        LLVertexBuffer::unbind();
        gViewerWindow->getWindow()->swapBuffers();
    }
}

void KSShaderNotice::requestReload()
{
    if (!canHoldBack())
    {
        // A reload waiting for its notice is done here too.
        const S32 requests = sRequests + 1;
        sPending = false;
        sNoticeFrames = sRequests = 0;
        reload(requests);
        return;
    }
    sPending = true;
    ++sRequests;
}

bool KSShaderNotice::runFrame()
{
    if (!sPending)
    {
        return false;
    }
    if (sNoticeFrames < NOTICE_FRAMES && canHoldBack())
    {
        drawNotice();
        ++sNoticeFrames;
        return true;
    }
    // The notice is on screen (or can no longer be shown): reload now, before any world frame.
    // The pause is expected, so the main loop timeout does not count it.
    const S32 requests = sRequests;
    sPending = false;
    sNoticeFrames = sRequests = 0;
    LLAppViewer::instance()->pauseMainloopTimeout();
    reload(requests);
    LLAppViewer::instance()->resumeMainloopTimeout("Main:Display");
    return true;
}

KSShaderNotice::FrameScope::FrameScope()
{
    ++sFrameDepth;
}

KSShaderNotice::FrameScope::~FrameScope()
{
    --sFrameDepth;
}
