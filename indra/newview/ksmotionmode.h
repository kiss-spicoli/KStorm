/**
 * @file ksmotionmode.h
 * @author Kissmy Spicoli
 * @brief Motion mode: one switch (KSMotionMode) that trades render quality for
 *        frame rate while moving, and restores the previous values when turned off.
 *
 * Motion mode sits on top of the lighting presets rather than being one: a preset
 * stays active underneath it, and applying a preset turns Motion mode off first.
 * While it is on, a setting it set and still holds is "owned": reads and writes
 * made through getValue() and setValue() see the value it will restore, so the
 * lighting presets capture and restore the real values, not the Motion ones.
 */
#ifndef KS_MOTIONMODE_H
#define KS_MOTIONMODE_H

#include "llcontrol.h"

namespace KSMotionMode
{
    // Connects the KSMotionMode setting (not persisted: every session starts off).
    // Safe to call repeatedly.
    void init();

    bool isOn();
    void setOn(bool on);

    // The control's value outside Motion mode: the value Motion mode will restore if it
    // owns the control, otherwise its current value.
    LLSD getValue(LLControlVariable& control);
    // Writes a value outside Motion mode: into the value Motion mode will restore if it
    // owns the control, otherwise to the control (only when it differs).
    void setValue(LLControlVariable& control, const LLSD& value);

    // Restores the previous values before settings are saved at quit.
    void restoreForShutdown();
}

#endif // KS_MOTIONMODE_H
