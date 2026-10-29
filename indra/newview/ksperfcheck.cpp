/**
 * @file ksperfcheck.cpp
 * @author Kissmy Spicoli
 * @brief Performance check chat command: look, mode, toggles and KS Reference differences.
 */

#include "llviewerprecompiledheaders.h"

#include "ksperfcheck.h"

#include "fscommon.h"
#include "ksenvironment.h"
#include "kslightingpresets.h"
#include "ksmotionmode.h"
#include "llcallbacklist.h"
#include "llgl.h"
#include "llimagegl.h"
#include "llmemory.h"
#include "llvertexbuffer.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llviewertexture.h"
#include "pipeline.h"

#include <memory>
#include <string>

namespace
{
    // The Quick pane's check boxes, and Motion mode's login and teleport switch.
    struct Toggle
    {
        const char* mLabel;
        const char* mSetting;   // nullptr for Show avatars, a render type
        // KS Reference's state of the two switches a look never holds: it is Use KS Reference,
        // and applying it turns Motion mode off. Undefined: read from KS Reference's settings.
        LLSD mReference;
    };

    const Toggle TOGGLES[] =
    {
        { "Use KS Reference",                  "KSSetEnvironment",             LLSD(true) },
        { "Motion mode",                       "KSMotionMode",                 LLSD(false) },
        { "Motion mode at login and teleport", "KSMotionModeOnTeleport",       LLSD() },
        { "Dynamic exposure",                  "RenderDynamicExposureEnabled", LLSD() },
        { "Ambient occlusion",                 "RenderDeferredSSAO",           LLSD() },
        { "Show avatars",                      nullptr,                        LLSD() },
        // AvatarNameTagMode: 0 is off, any other value a display mode.
        { "Show avatar names",                 "AvatarNameTagMode",            LLSD() },
    };

    const char* onOff(bool on)
    {
        return on ? "on" : "off";
    }

    // Nearby chat on this viewer only; the command line's announce channel is not used.
    void say(const std::string& line)
    {
        FSCommon::report_to_nearby_chat(line);
    }

    std::string toggleLine(const Toggle& toggle)
    {
        bool on = false;
        std::string motion;
        LLSD reference = toggle.mReference;
        if (!toggle.mSetting)
        {
            // Same as Advanced > Rendering Types > Avatars.
            on = LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR);
        }
        else if (LLControlVariable* control = gSavedSettings.getControl(toggle.mSetting))
        {
            // In the setting's own type: AvatarNameTagMode can hold a String "0", which is TRUE as LLSD.
            on = KSLightingPresets::controlValue(*control, control->getValue()).asBoolean();
            const bool outside = KSLightingPresets::controlValue(*control, KSMotionMode::getValue(*control)).asBoolean();
            if (outside != on)
            {
                motion = llformat(" [Motion mode; %s outside it]", onOff(outside));
            }
            if (reference.isUndefined())
            {
                reference = KSLightingPresets::referenceSetting(toggle.mSetting);
            }
            if (reference.isDefined())
            {
                reference = KSLightingPresets::controlValue(*control, reference);
            }
        }
        else
        {
            return llformat("  %s: no setting %s", toggle.mLabel, toggle.mSetting);
        }

        std::string annotation;
        if (reference.isUndefined())
        {
            annotation = "not in Reference";
        }
        else if (reference.asBoolean() == on)
        {
            annotation = "Reference";
        }
        else
        {
            annotation = llformat("Reference: %s", onOff(reference.asBoolean()));
        }
        return llformat("  %s: %s (%s)%s", toggle.mLabel, onOff(on), annotation.c_str(), motion.c_str());
    }

    std::string lookAndMode()
    {
        const std::string& look = KSLightingPresets::active();
        return llformat("Look: %s. Mode: %s.", look.empty() ? "none" : look.c_str(),
                        KSMotionMode::isOn() ? "Motion (fast)" : "Photo (slow)");
    }

    void stateReport()
    {
        say("KStorm performance check. " + lookAndMode());

        say("Toggles:");
        for (const Toggle& toggle : TOGGLES)
        {
            say(toggleLine(toggle));
        }

        S32 total = 0;
        const std::vector<KSLightingPresets::ReferenceDifference> differences = KSLightingPresets::referenceDifferences(total);
        const char* scope = KSEnvironment::getDisplaySky() ? "" : " (no sky yet: settings only)";
        if (differences.empty())
        {
            say(llformat("All %d KS Reference values conform%s.", total, scope));
            return;
        }
        say(llformat("Moved from KS Reference: %d of %d values%s:", (S32)differences.size(), total, scope));
        for (const KSLightingPresets::ReferenceDifference& difference : differences)
        {
            std::string line = "  " + difference.mName + ": " + difference.mCurrent + " (Reference " + difference.mReference + ")";
            if (!difference.mOutsideMotion.empty())
            {
                line += " [Motion mode; " + difference.mOutsideMotion + " outside it]";
            }
            say(line);
        }
    }

    // Memory and VRAM now; lowest_free_vram < 0 when there is no window to report a low for.
    void sayMemory(F32 lowest_free_vram)
    {
        LLMemory::updateMemoryInfo();
        say(llformat("  Memory: viewer %.0f MB, system %.0f of %.0f MB free",
                     LLMemory::getAllocatedMemKB().value() / 1024.0,
                     LLMemory::getAvailableMemKB().value() / 1024.0,
                     LLMemory::getMaxMemKB().value() / 1024.0));
        // The viewer's own VRAM estimate (llviewertexture.cpp): its byte counts miss about half
        // of what is used, so it doubles them.
        const F64 vram_used = (LLImageGL::getTextureBytesAllocated() + LLVertexBuffer::getBytesAllocated()) / 1024.0 / 512.0;
        std::string line = llformat("  VRAM: about %.0f of %u MB used, texture budget %.0f MB free",
                                    vram_used, gGLManager.mVRAM, LLViewerTexture::sFreeVRAMMegabytes);
        if (lowest_free_vram >= 0.f)
        {
            line += llformat(" (lowest %.0f)", lowest_free_vram);
        }
        say(line);
    }

    // ---------------------------------------------------------------------
    // Timed sample

    constexpr S32 MAX_SAMPLE_SECONDS = 600;

    struct Sample
    {
        S32 mSeconds;
        S32 mElapsed = 0;
        // One period per second: the last mSeconds completed periods are the window (the one
        // extra slot is the period running when the sample ends, left empty).
        LLTrace::PeriodicRecording mPeriods;
        LLTrace::Recording mTotal;
        F32 mLowestFreeVRAM;
        // Set when the look or Motion mode changed during the window (its figures mix both).
        bool mChanged = false;
        boost::signals2::scoped_connection mLookConnection;
        boost::signals2::scoped_connection mMotionConnection;

        explicit Sample(S32 seconds)
        :   mSeconds(seconds),
            mPeriods(seconds + 1, LLTrace::PeriodicRecording::STARTED),
            mLowestFreeVRAM(LLViewerTexture::sFreeVRAMMegabytes)
        {
            mTotal.start();
            mLookConnection = KSLightingPresets::changedSignal().connect([this]() { mChanged = true; });
            if (LLControlVariable* control = gSavedSettings.getControl("KSMotionMode"))
            {
                mMotionConnection = control->getSignal()->connect(
                    [this](LLControlVariable*, const LLSD&, const LLSD&) { mChanged = true; });
            }
        }
    };
    std::unique_ptr<Sample> sSample;

    void reportSample(Sample& sample)
    {
        LLTrace::PeriodicRecording& periods = sample.mPeriods;
        LLTrace::Recording& total = sample.mTotal;
        const size_t n = static_cast<size_t>(sample.mSeconds);

        say(llformat("KStorm sample, %d s. %s%s", sample.mSeconds, lookAndMode().c_str(),
                     sample.mChanged ? " The look or Motion mode changed during the sample." : ""));
        say(llformat("  FPS: %.1f mean, worst second %.1f, best second %.1f",
                     total.getPerSec(LLStatViewer::FPS),
                     periods.getPeriodMinPerSec(LLStatViewer::FPS, n),
                     periods.getPeriodMaxPerSec(LLStatViewer::FPS, n)));
        const F64Milliseconds frame_mean = total.getMean(LLStatViewer::FRAMETIME);
        const F64Milliseconds frame_max = total.getMax(LLStatViewer::FRAMETIME);
        say(llformat("  Frame time: %.1f ms mean, worst frame %.1f ms", frame_mean.value(), frame_max.value()));

        const F64 packets_in = total.getSum(LLStatViewer::PACKETS_IN);
        const F64 packets_lost = total.getSum(LLStatViewer::PACKETS_LOST);
        const F64Kilobits data_in_per_sec = total.getPerSec(LLStatViewer::ACTIVE_MESSAGE_DATA_RECEIVED);
        say(llformat("  Packets: %.1f/s in, %.1f/s out, %.0f lost (%.1f%%), %.0f kbit/s in",
                     total.getPerSec(LLStatViewer::PACKETS_IN),
                     total.getPerSec(LLStatViewer::PACKETS_OUT),
                     packets_lost, packets_in > 0.0 ? 100.0 * packets_lost / packets_in : 0.0,
                     data_in_per_sec.value()));
        const F64Milliseconds ping_mean = total.getMean(LLStatViewer::SIM_PING);
        const F64Milliseconds ping_max = total.getMax(LLStatViewer::SIM_PING);
        say(llformat("  Ping: %.0f ms mean, %.0f ms worst", ping_mean.value(), ping_max.value()));
        say(llformat("  Sim: %.1f FPS mean, time dilation %.2f mean, %.2f lowest",
                     total.getMean(LLStatViewer::SIM_FPS),
                     total.getMean(LLStatViewer::SIM_TIME_DILATION),
                     total.getMin(LLStatViewer::SIM_TIME_DILATION)));
        sayMemory(sample.mLowestFreeVRAM);
    }

    // Once a second: closes a period, and reports when the window is done.
    bool onSampleSecond()
    {
        if (!sSample)
        {
            return true;
        }
        Sample& sample = *sSample;
        sample.mLowestFreeVRAM = llmin(sample.mLowestFreeVRAM, LLViewerTexture::sFreeVRAMMegabytes);
        sample.mPeriods.nextPeriod();
        if (++sample.mElapsed < sample.mSeconds)
        {
            return false;
        }
        sample.mPeriods.stop();
        sample.mTotal.stop();
        reportSample(sample);
        sSample.reset();
        return true;
    }
}

void KSPerfCheck::report()
{
    stateReport();

    // Figures now. FPS over the frame recording's last 200 frames, as the statistics bar shows it.
    LLTrace::PeriodicRecording& frames = LLTrace::get_frame_recording();
    LLTrace::Recording& last = frames.getLastRecording();
    const F64Milliseconds ping = last.getLastValue(LLStatViewer::SIM_PING);
    say("Now:");
    say(llformat("  FPS: %.1f (last 200 frames). Ping: %.0f ms", frames.getPeriodMeanPerSec(LLStatViewer::FPS), ping.value()));
    say(llformat("  Sim: %.1f FPS, time dilation %.2f",
                 last.getLastValue(LLStatViewer::SIM_FPS), last.getLastValue(LLStatViewer::SIM_TIME_DILATION)));
    sayMemory(-1.f);
}

void KSPerfCheck::startSample(S32 seconds)
{
    if (sSample)
    {
        say(llformat("A KStorm sample is already running: %d s left.", sSample->mSeconds - sSample->mElapsed));
        return;
    }
    if (seconds < 1 || seconds > MAX_SAMPLE_SECONDS)
    {
        say(llformat("Sample length is 1 to %d seconds.", MAX_SAMPLE_SECONDS));
        return;
    }
    stateReport();
    sSample = std::make_unique<Sample>(seconds);
    say(llformat("Sampling for %d s.", seconds));
    doPeriodically(onSampleSecond, 1.f);
}
