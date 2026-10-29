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
 * frame time, network, sim and memory over that window. "ksperf delta" shows only what
 * changed since the last "ksperf" or "ksperf delta", and the figures with their signed
 * change. Each report ends with a row of "=".
 */
#ifndef KS_PERFCHECK_H
#define KS_PERFCHECK_H

#include <string>

namespace KSPerfCheck
{
    // The command's argument: "" for the report, "delta", or a number of seconds (1 to 600)
    // for a timed sample (one runs at a time).
    void run(const std::string& argument);
}

#endif // KS_PERFCHECK_H
