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
#include "llframetimer.h"
#include "llgl.h"
#include "llimagegl.h"
#include "llmemory.h"
#include "llvertexbuffer.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llviewertexture.h"
#include "pipeline.h"

#include <cmath>
#include <map>
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
    constexpr size_t TOGGLE_COUNT = sizeof(TOGGLES) / sizeof(TOGGLES[0]);

    const char* onOff(bool on)
    {
        return on ? "on" : "off";
    }

    // Nearby chat on this viewer only; the command line's announce channel is not used.
    void say(const std::string& line)
    {
        FSCommon::report_to_nearby_chat(line);
    }

    // The last line of a report, so its end is plain among other chat.
    void sayEnd()
    {
        say("==================");
    }

    // A toggle's state as shown after its label: on or off, its KS Reference state and what
    // Motion mode holds.
    std::string toggleValue(const Toggle& toggle)
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
            return llformat("no setting %s", toggle.mSetting);
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
        return llformat("%s (%s)%s", onOff(on), annotation.c_str(), motion.c_str());
    }

    std::string lookName(const std::string& look)
    {
        return look.empty() ? "none" : look;
    }

    const char* modeName(bool motion)
    {
        return motion ? "Motion (fast)" : "Photo (slow)";
    }

    std::string lookAndMode()
    {
        return llformat("Look: %s. Mode: %s.", lookName(KSLightingPresets::active()).c_str(), modeName(KSMotionMode::isOn()));
    }

    // Everything the state report shows, kept so a later delta can compare with it.
    struct State
    {
        std::string mLook;
        bool mMotion = false;
        std::string mToggles[TOGGLE_COUNT];     // toggleValue() of each TOGGLES entry
        std::vector<KSLightingPresets::ReferenceDifference> mMoved;
        S32 mTotal = 0;
        bool mSky = false;
    };

    State readState()
    {
        State state;
        state.mLook = KSLightingPresets::active();
        state.mMotion = KSMotionMode::isOn();
        for (size_t t = 0; t < TOGGLE_COUNT; ++t)
        {
            state.mToggles[t] = toggleValue(TOGGLES[t]);
        }
        state.mMoved = KSLightingPresets::referenceDifferences(state.mTotal);
        state.mSky = KSEnvironment::getDisplaySky() != nullptr;
        return state;
    }

    const char* skyScope(bool sky)
    {
        return sky ? "" : " (no sky yet: settings only)";
    }

    std::string differenceLine(const KSLightingPresets::ReferenceDifference& difference)
    {
        std::string line = "  " + difference.mName + ": " + difference.mCurrent + " (Reference " + difference.mReference + ")";
        if (!difference.mOutsideMotion.empty())
        {
            line += " [Motion mode; " + difference.mOutsideMotion + " outside it]";
        }
        return line;
    }

    void sayState(const State& state)
    {
        say(llformat("KStorm performance check. Look: %s. Mode: %s.", lookName(state.mLook).c_str(), modeName(state.mMotion)));

        say("Toggles:");
        for (size_t t = 0; t < TOGGLE_COUNT; ++t)
        {
            say(llformat("  %s: %s", TOGGLES[t].mLabel, state.mToggles[t].c_str()));
        }

        if (state.mMoved.empty())
        {
            say(llformat("All %d KS Reference values conform%s.", state.mTotal, skyScope(state.mSky)));
            return;
        }
        say(llformat("Moved from KS Reference: %d of %d values%s:", (S32)state.mMoved.size(), state.mTotal, skyScope(state.mSky)));
        for (const KSLightingPresets::ReferenceDifference& difference : state.mMoved)
        {
            say(differenceLine(difference));
        }
    }

    // Memory and VRAM.
    struct Memory
    {
        F64 mViewer;        // MB
        F64 mFree;          // MB of system memory
        F64 mMax;           // MB
        F64 mVRAMUsed;      // MB, estimated
        U32 mVRAM;          // MB
        F64 mTextureFree;   // MB left in the texture budget
    };

    Memory readMemory()
    {
        LLMemory::updateMemoryInfo();
        Memory memory;
        memory.mViewer = LLMemory::getAllocatedMemKB().value() / 1024.0;
        memory.mFree = LLMemory::getAvailableMemKB().value() / 1024.0;
        memory.mMax = LLMemory::getMaxMemKB().value() / 1024.0;
        // The viewer's own VRAM estimate (llviewertexture.cpp): its byte counts miss about half
        // of what is used, so it doubles them.
        memory.mVRAMUsed = (LLImageGL::getTextureBytesAllocated() + LLVertexBuffer::getBytesAllocated()) / 1024.0 / 512.0;
        memory.mVRAM = gGLManager.mVRAM;
        memory.mTextureFree = LLViewerTexture::sFreeVRAMMegabytes;
        return memory;
    }

    // lowest_free_vram < 0 when there is no window to report a low for.
    void sayMemory(const Memory& memory, F32 lowest_free_vram)
    {
        say(llformat("  Memory: viewer %.0f MB, system %.0f of %.0f MB free", memory.mViewer, memory.mFree, memory.mMax));
        std::string line = llformat("  VRAM: about %.0f of %u MB used, texture budget %.0f MB free",
                                    memory.mVRAMUsed, memory.mVRAM, memory.mTextureFree);
        if (lowest_free_vram >= 0.f)
        {
            line += llformat(" (lowest %.0f)", lowest_free_vram);
        }
        say(line);
    }

    // The figures now. FPS over the frame recording's last 200 frames, as the statistics bar shows it.
    struct Figures
    {
        F64 mFPS;
        F64 mPing;          // ms
        F64 mSimFPS;
        F64 mDilation;
        Memory mMemory;
    };

    Figures readFigures()
    {
        LLTrace::PeriodicRecording& frames = LLTrace::get_frame_recording();
        LLTrace::Recording& last = frames.getLastRecording();
        const F64Milliseconds ping = last.getLastValue(LLStatViewer::SIM_PING);
        Figures figures;
        figures.mFPS = frames.getPeriodMeanPerSec(LLStatViewer::FPS);
        figures.mPing = ping.value();
        figures.mSimFPS = last.getLastValue(LLStatViewer::SIM_FPS);
        figures.mDilation = last.getLastValue(LLStatViewer::SIM_TIME_DILATION);
        figures.mMemory = readMemory();
        return figures;
    }

    void sayFigures(const Figures& figures)
    {
        say("Now:");
        say(llformat("  FPS: %.1f (last 200 frames). Ping: %.0f ms", figures.mFPS, figures.mPing));
        say(llformat("  Sim: %.1f FPS, time dilation %.2f", figures.mSimFPS, figures.mDilation));
        sayMemory(figures.mMemory, -1.f);
    }

    // ---------------------------------------------------------------------
    // Delta: the last report (ksperf or ksperf delta), for the next delta to compare with.
    // Timed samples do not set it; it lasts until logout.

    struct Report
    {
        State mState;
        Figures mFigures;
        F64 mTime;          // LLFrameTimer::getElapsedSeconds() when it was read, s
    };
    std::unique_ptr<Report> sLast;

    void keep(State state, const Figures& figures)
    {
        sLast = std::make_unique<Report>(Report{ std::move(state), figures, LLFrameTimer::getElapsedSeconds().value() });
    }

    // Signed change rounded as the figure is shown, "+0" when the rounding hides it.
    std::string change(F64 now, F64 before, S32 decimals)
    {
        const F64 scale = std::pow(10.0, decimals);
        F64 difference = std::round((now - before) * scale) / scale;
        if (difference == 0.0)
        {
            difference = 0.0;   // no "-0"
        }
        return llformat("%+.*f", decimals, difference);
    }

    std::string ago(F64 seconds)
    {
        const S32 whole = (S32)std::lround(seconds);
        return whole < 60 ? llformat("%d s", whole) : llformat("%d min %d s", whole / 60, whole % 60);
    }

    // The state lines that changed; false when none did.
    bool sayStateChanges(const State& before, const State& now)
    {
        bool changed = false;
        if (now.mLook != before.mLook)
        {
            say(llformat("  Look: %s (was %s)", lookName(now.mLook).c_str(), lookName(before.mLook).c_str()));
            changed = true;
        }
        if (now.mMotion != before.mMotion)
        {
            say(llformat("  Mode: %s (was %s)", modeName(now.mMotion), modeName(before.mMotion)));
            changed = true;
        }

        bool header = false;
        for (size_t t = 0; t < TOGGLE_COUNT; ++t)
        {
            if (now.mToggles[t] == before.mToggles[t])
            {
                continue;
            }
            if (!header)
            {
                say("Toggles:");
                header = true;
            }
            say(llformat("  %s: %s, was %s", TOGGLES[t].mLabel, now.mToggles[t].c_str(), before.mToggles[t].c_str()));
        }
        changed |= header;

        // KS Reference values moved away, moved again, or back.
        std::map<std::string, const KSLightingPresets::ReferenceDifference*> earlier;
        for (const KSLightingPresets::ReferenceDifference& difference : before.mMoved)
        {
            earlier[difference.mName] = &difference;
        }
        std::vector<std::string> lines;
        for (const KSLightingPresets::ReferenceDifference& difference : now.mMoved)
        {
            auto found = earlier.find(difference.mName);
            if (found == earlier.end())
            {
                lines.push_back(differenceLine(difference) + ", was Reference");
                continue;
            }
            const KSLightingPresets::ReferenceDifference& old = *found->second;
            if (difference.mCurrent != old.mCurrent || difference.mReference != old.mReference
                || difference.mOutsideMotion != old.mOutsideMotion)
            {
                lines.push_back(differenceLine(difference) + ", was " + old.mCurrent);
            }
            earlier.erase(found);
        }
        for (const auto& entry : earlier)
        {
            const KSLightingPresets::ReferenceDifference& old = *entry.second;
            lines.push_back("  " + old.mName + ": back to Reference " + old.mReference + ", was " + old.mCurrent);
        }
        if (!lines.empty() || now.mTotal != before.mTotal || now.mSky != before.mSky)
        {
            say(llformat("Moved from KS Reference: %d of %d values%s (was %d of %d):", (S32)now.mMoved.size(), now.mTotal,
                         skyScope(now.mSky), (S32)before.mMoved.size(), before.mTotal));
            for (const std::string& line : lines)
            {
                say(line);
            }
            changed = true;
        }
        return changed;
    }

    void sayFigureChanges(const Figures& before, const Figures& now)
    {
        const Memory& was = before.mMemory;
        const Memory& is = now.mMemory;
        say("Now (change):");
        say(llformat("  FPS: %.1f (%s, last 200 frames). Ping: %.0f ms (%s)", now.mFPS, change(now.mFPS, before.mFPS, 1).c_str(),
                     now.mPing, change(now.mPing, before.mPing, 0).c_str()));
        say(llformat("  Sim: %.1f FPS (%s), time dilation %.2f (%s)", now.mSimFPS, change(now.mSimFPS, before.mSimFPS, 1).c_str(),
                     now.mDilation, change(now.mDilation, before.mDilation, 2).c_str()));
        say(llformat("  Memory: viewer %.0f MB (%s), system %.0f MB free (%s)", is.mViewer, change(is.mViewer, was.mViewer, 0).c_str(),
                     is.mFree, change(is.mFree, was.mFree, 0).c_str()));
        say(llformat("  VRAM: about %.0f MB used (%s), texture budget %.0f MB free (%s)", is.mVRAMUsed,
                     change(is.mVRAMUsed, was.mVRAMUsed, 0).c_str(), is.mTextureFree,
                     change(is.mTextureFree, was.mTextureFree, 0).c_str()));
    }

    // The state report and the figures now; kept for the next delta.
    void report()
    {
        State state = readState();
        const Figures figures = readFigures();
        sayState(state);
        sayFigures(figures);
        sayEnd();
        keep(std::move(state), figures);
    }

    // Only what changed since the last report; a full report when there is none yet.
    void reportDelta()
    {
        if (!sLast)
        {
            say("No earlier KStorm report this session to compare with; the full report:");
            report();
            return;
        }
        State state = readState();
        const Figures figures = readFigures();
        say(llformat("KStorm changes since the report %s ago.", ago(LLFrameTimer::getElapsedSeconds().value() - sLast->mTime).c_str()));
        if (!sayStateChanges(sLast->mState, state))
        {
            say("  Look, mode, toggles and KS Reference values: no change.");
        }
        sayFigureChanges(sLast->mFigures, figures);
        sayEnd();
        keep(std::move(state), figures);
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
        sayMemory(readMemory(), sample.mLowestFreeVRAM);
        sayEnd();
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

    void startSample(S32 seconds)
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
        sayState(readState());
        sSample = std::make_unique<Sample>(seconds);
        say(llformat("Sampling for %d s.", seconds));
        doPeriodically(onSampleSecond, 1.f);
    }
}

void KSPerfCheck::run(const std::string& argument)
{
    S32 seconds;
    if (argument.empty())
    {
        report();
    }
    else if (LLStringUtil::compareInsensitive(argument, "delta") == 0)
    {
        reportDelta();
    }
    else if (LLStringUtil::convertToS32(argument, seconds))
    {
        startSample(seconds);
    }
    else
    {
        say(llformat("%s takes no word, a number of seconds (1 to %d), or delta.",
                     gSavedSettings.getString("KSCmdLinePerf").c_str(), MAX_SAMPLE_SECONDS));
    }
}
