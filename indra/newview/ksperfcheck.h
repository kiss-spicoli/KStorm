/**
 * @file ksperfcheck.h
 * @author Kissmy Spicoli
 * @brief Performance check chat command (KSCmdLinePerf, default "ksperf").
 *
 * Reports to nearby chat, on this viewer only: the active look, Photo or Motion mode,
 * the Quick pane's toggles with their KS Reference state, and every KS Reference value
 * the current one has moved from. Values that conform are left out.
 *
 * "ksperf" adds a snapshot of frame rate, ping, sim and memory figures; "ksperf N" instead
 * records N one-second periods and then reports frame rate (mean, worst and best second),
 * frame time, network, sim and memory over that window.
 */
#ifndef KS_PERFCHECK_H
#define KS_PERFCHECK_H

#include "stdtypes.h"

namespace KSPerfCheck
{
    // The state report and a snapshot of the current figures.
    void report();

    // The state report, then a sample of the given number of seconds (1 to 600), reported
    // when it ends. One sample runs at a time.
    void startSample(S32 seconds);
}

#endif // KS_PERFCHECK_H
