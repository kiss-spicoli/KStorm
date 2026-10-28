/**
 * @file kscamerapositions.cpp
 * @author Kissmy Spicoli
 * @brief KStorm Camera Positioning: labeled camera views per sim, and the camera Lock.
 */

#include "llviewerprecompiledheaders.h"

#include "kscamerapositions.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llapp.h"
#include "llfile.h"
#include "lldir.h"
#include "llframetimer.h"
#include "llsdserialize.h"
#include "llstartup.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"
#include "llviewermenu.h"
#include "llviewernetwork.h"
#include "llviewerregion.h"
#include "llworld.h"

#include <algorithm>
#include <set>

using KSCameraPositions::Record;
using KSCameraPositions::View;

namespace
{
    const std::string FILE_NAME = "ks_camera_positions.xml";
    const F32 FOCUS_DISTANCE = 5.f;     // meters: focus point ahead of an applied camera
    const F32 MAX_PITCH = 89.5f;        // degrees: the unrolled frame needs a horizontal at axis
    const F32 ESCAPE_WINDOW = 1.5f;     // seconds for the three Esc presses

    std::vector<Record> sRecords;
    bool sLoaded = false;

    bool sLocked = false;
    LLVector3d sHeldPosition;           // global
    LLVector3 sHeldAt;
    LLVector3 sHeldLeft;
    LLVector3 sHeldUp;
    F32 sHeldView = 0.f;                // radians

    S32 sEscapeCount = 0;
    LLFrameTimer sEscapeTimer;

    std::string currentGrid()
    {
        return LLGridManager::getInstance()->getGridId();
    }

    std::string filename()
    {
        return gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILE_NAME);
    }

    LLSD toLLSD(const Record& record)
    {
        LLSD data;
        data["name"] = record.mName;
        data["region"] = record.mRegion;
        data["grid"] = record.mGrid;
        data["position"] = record.mView.mPosition.getValue();
        data["yaw"] = record.mView.mYaw;
        data["pitch"] = record.mView.mPitch;
        data["roll"] = record.mView.mRoll;
        data["view_angle"] = record.mView.mViewAngle;
        return data;
    }

    Record fromLLSD(const LLSD& data)
    {
        Record record;
        record.mName = data["name"].asString();
        record.mRegion = data["region"].asString();
        record.mGrid = data["grid"].asString();
        record.mView.mPosition.setValue(data["position"]);
        record.mView.mYaw = (F32)data["yaw"].asReal();
        record.mView.mPitch = (F32)data["pitch"].asReal();
        record.mView.mRoll = (F32)data["roll"].asReal();
        record.mView.mViewAngle = (F32)data["view_angle"].asReal();
        return record;
    }

    void load()
    {
        if (sLoaded)
        {
            return;
        }
        sLoaded = true;
        llifstream file(filename().c_str());
        if (!file.is_open())
        {
            return;
        }
        LLSD data;
        if (LLSDSerialize::fromXML(data, file) == LLSDParser::PARSE_FAILURE || !data.isArray())
        {
            LL_WARNS("KSCamera") << "Could not read " << filename() << LL_ENDL;
            return;
        }
        for (LLSD::array_const_iterator it = data.beginArray(); it != data.endArray(); ++it)
        {
            Record record = fromLLSD(*it);
            if (!record.mName.empty() && !record.mRegion.empty())
            {
                sRecords.push_back(record);
            }
        }
    }

    void save()
    {
        LLSD data = LLSD::emptyArray();
        for (const Record& record : sRecords)
        {
            data.append(toLLSD(record));
        }
        llofstream file(filename().c_str());
        if (!file.is_open())
        {
            LL_WARNS("KSCamera") << "Could not write " << filename() << LL_ENDL;
            return;
        }
        LLSDSerialize::toPrettyXML(data, file);
    }

    void changed()
    {
        KSCameraPositions::changedSignal()();
    }

    // The connected region (the agent's included) a record belongs to, or null.
    LLViewerRegion* regionOf(const Record& record)
    {
        if (record.mGrid != currentGrid())
        {
            return nullptr;
        }
        for (LLViewerRegion* region : LLWorld::getInstance()->getRegionList())
        {
            if (region && region->getName() == record.mRegion)
            {
                return region;
            }
        }
        return nullptr;
    }

    // Sort rank: this sim, connected sims, the others.
    int rank(const Record& record)
    {
        return KSCameraPositions::isHere(record) ? 0 : KSCameraPositions::isConnected(record) ? 1 : 2;
    }

    std::vector<Record>::iterator findHere(const std::string& name)
    {
        load();
        return std::find_if(sRecords.begin(), sRecords.end(),
            [&name](const Record& record) { return record.mName == name && KSCameraPositions::isHere(record); });
    }

    // The unrolled camera frame looking along yaw / pitch, the one LLViewerCamera::updateCameraLocation()
    // builds from a world up vector.
    void levelAxes(F32 yaw, F32 pitch, LLVector3& at, LLVector3& left, LLVector3& up)
    {
        at.set(cosf(pitch) * cosf(yaw), cosf(pitch) * sinf(yaw), sinf(pitch));
        left = LLVector3::z_axis % at;
        left.normalize();
        up = at % left;
    }

    // The camera frame for a view; roll is applied as LLAgentCamera::updateCamera() applies
    // mRollAngle (rows of LLMatrix3(roll, 0, 0) times the unrolled frame).
    void viewAxes(const View& view, LLVector3& at, LLVector3& left, LLVector3& up)
    {
        const F32 pitch = llclamp(view.mPitch, -MAX_PITCH, MAX_PITCH) * DEG_TO_RAD;
        LLVector3 level_left;
        LLVector3 level_up;
        levelAxes(view.mYaw * DEG_TO_RAD, pitch, at, level_left, level_up);
        const F32 c = cosf(view.mRoll * DEG_TO_RAD);
        const F32 s = sinf(view.mRoll * DEG_TO_RAD);
        left = c * level_left + s * level_up;
        up = c * level_up - s * level_left;
    }

    // Yaw, pitch and roll (degrees) of a camera frame.
    void frameAngles(const LLVector3& at, const LLVector3& left, F32& yaw, F32& pitch, F32& roll)
    {
        yaw = atan2f(at.mV[VY], at.mV[VX]) * RAD_TO_DEG;
        pitch = asinf(llclamp(at.mV[VZ], -1.f, 1.f)) * RAD_TO_DEG;
        roll = 0.f;
        if (fabsf(at.mV[VZ]) < 0.9999f)
        {
            LLVector3 level_at;
            LLVector3 level_left;
            LLVector3 level_up;
            levelAxes(yaw * DEG_TO_RAD, pitch * DEG_TO_RAD, level_at, level_left, level_up);
            roll = atan2f(left * level_up, left * level_left) * RAD_TO_DEG;
        }
    }

    F32 clampView(F32 radians)
    {
        const LLViewerCamera* camera = LLViewerCamera::getInstance();
        return llclamp(radians, camera->getMinView(), camera->getMaxView());
    }

    // Puts LLAgentCamera at a view (global position, at axis, roll and view angle in radians),
    // focus off the avatar, so the camera moves on from there as usual.
    void moveAgentCamera(const LLVector3d& position, const LLVector3& at, F32 roll, F32 view, bool animate)
    {
        if (LLViewerJoystick::getInstance()->getOverrideCamera())
        {
            LLViewerJoystick::getInstance()->toggleFlycam();
        }
        if (gAgentCamera.cameraMouselook())
        {
            gAgentCamera.changeCameraToDefault();
        }
        gAgentCamera.unlockView();
        const LLVector3d focus = position + LLVector3d(at * FOCUS_DISTANCE);
        // A focus without an object clears the object FOV zoom, so CameraAngle is the view angle.
        gAgentCamera.setFocusGlobal(focus);
        gAgentCamera.setCameraPosAndFocusGlobal(position, focus, LLUUID::null);
        if (!animate)
        {
            gAgentCamera.stopCameraAnimation();
        }
        gAgentCamera.setRollAngle(roll);
        gSavedSettings.setF32("CameraAngle", clampView(view));
    }

    void unlock(bool reset)
    {
        if (!sLocked)
        {
            return;
        }
        // Off first: while locked the camera update is skipped, so a reset would not show.
        sLocked = false;
        sEscapeCount = 0;
        // The joystick was not scanned while locked: no spurious motion from stale deltas.
        LLViewerJoystick::getInstance()->setNeedsReset();
        if (reset)
        {
            handle_reset_view();
        }
        changed();
    }
}

std::vector<Record> KSCameraPositions::records()
{
    load();
    std::vector<Record> list = sRecords;
    std::stable_sort(list.begin(), list.end(), [](const Record& a, const Record& b)
        {
            const int a_rank = rank(a);
            const int b_rank = rank(b);
            if (a_rank != b_rank)
            {
                return a_rank < b_rank;
            }
            if (a.mRegion != b.mRegion)
            {
                return LLStringUtil::compareDict(a.mRegion, b.mRegion) < 0;
            }
            return LLStringUtil::compareDict(a.mName, b.mName) < 0;
        });
    return list;
}

bool KSCameraPositions::isHere(const Record& record)
{
    const LLViewerRegion* region = gAgent.getRegion();
    return region && record.mRegion == region->getName() && record.mGrid == currentGrid();
}

bool KSCameraPositions::isConnected(const Record& record)
{
    return !isHere(record) && regionOf(record) != nullptr;
}

std::string KSCameraPositions::connectedRegions()
{
    const LLViewerRegion* here = gAgent.getRegion();
    std::vector<std::string> names;
    for (const LLViewerRegion* region : LLWorld::getInstance()->getRegionList())
    {
        if (region && region != here)
        {
            names.push_back(region->getName());
        }
    }
    std::sort(names.begin(), names.end());
    std::string joined;
    for (const std::string& name : names)
    {
        joined += name + "\n";
    }
    return joined;
}

bool KSCameraPositions::exists(const std::string& name)
{
    return findHere(name) != sRecords.end();
}

std::string KSCameraPositions::currentRegion()
{
    const LLViewerRegion* region = gAgent.getRegion();
    return region ? region->getName() : std::string();
}

bool KSCameraPositions::currentView(View& view)
{
    LLViewerRegion* region = gAgent.getRegion();
    if (!region)
    {
        return false;
    }
    const LLViewerCamera* camera = LLViewerCamera::getInstance();
    view.mPosition = region->getPosRegionFromGlobal(gAgent.getPosGlobalFromAgent(camera->getOrigin()));
    frameAngles(camera->getAtAxis(), camera->getLeftAxis(), view.mYaw, view.mPitch, view.mRoll);
    view.mViewAngle = camera->getView() * RAD_TO_DEG;
    return true;
}

bool KSCameraPositions::saveCurrent(const std::string& name)
{
    std::string label = name;
    LLStringUtil::trim(label);
    View view;
    if (label.empty() || !currentView(view))
    {
        return false;
    }
    auto it = findHere(label);
    if (it == sRecords.end())
    {
        Record record;
        record.mName = label;
        record.mRegion = currentRegion();
        record.mGrid = currentGrid();
        sRecords.push_back(record);
        it = sRecords.end() - 1;
    }
    it->mView = view;
    save();
    changed();
    return true;
}

bool KSCameraPositions::update(const std::string& name)
{
    auto it = findHere(name);
    return it != sRecords.end() && saveCurrent(name);
}

bool KSCameraPositions::rename(const std::string& from, const std::string& to)
{
    std::string label = to;
    LLStringUtil::trim(label);
    auto it = findHere(from);
    if (it == sRecords.end() || label.empty() || exists(label))
    {
        return false;
    }
    it->mName = label;
    save();
    changed();
    return true;
}

bool KSCameraPositions::remove(const Record& record)
{
    load();
    auto it = std::find_if(sRecords.begin(), sRecords.end(), [&record](const Record& other)
        {
            return other.mName == record.mName && other.mRegion == record.mRegion && other.mGrid == record.mGrid;
        });
    if (it == sRecords.end())
    {
        return false;
    }
    sRecords.erase(it);
    save();
    changed();
    return true;
}

bool KSCameraPositions::apply(const Record& record)
{
    load();
    auto it = std::find_if(sRecords.begin(), sRecords.end(), [&record](const Record& other)
        {
            return other.mName == record.mName && other.mRegion == record.mRegion && other.mGrid == record.mGrid;
        });
    // Positions are local to the record's own region, which may be a connected neighbour.
    LLViewerRegion* region = it != sRecords.end() ? regionOf(*it) : nullptr;
    if (!region)
    {
        return false;
    }
    const View& view = it->mView;
    const LLVector3d position = region->getPosGlobalFromRegion(view.mPosition);
    LLVector3 at;
    LLVector3 left;
    LLVector3 up;
    viewAxes(view, at, left, up);
    if (sLocked)
    {
        sHeldPosition = position;
        sHeldAt = at;
        sHeldLeft = left;
        sHeldUp = up;
        sHeldView = clampView(view.mViewAngle * DEG_TO_RAD);
        // Kept for after the lock, like a zoom made before it.
        gSavedSettings.setF32("CameraAngle", sHeldView);
    }
    else
    {
        moveAgentCamera(position, at, view.mRoll * DEG_TO_RAD, view.mViewAngle * DEG_TO_RAD, true);
    }
    return true;
}

F32 KSCameraPositions::zoomFactor(F32 view_angle_degrees)
{
    return view_angle_degrees > 0.f ? DEFAULT_FIELD_OF_VIEW * RAD_TO_DEG / view_angle_degrees : 1.f;
}

bool KSCameraPositions::isLocked()
{
    return sLocked;
}

void KSCameraPositions::setLocked(bool locked)
{
    if (locked == sLocked)
    {
        return;
    }
    if (!locked)
    {
        // Hand the held view to LLAgentCamera, without animation: the camera stays put.
        F32 yaw;
        F32 pitch;
        F32 roll;
        frameAngles(sHeldAt, sHeldLeft, yaw, pitch, roll);
        unlock(false);
        moveAgentCamera(sHeldPosition, sHeldAt, roll * DEG_TO_RAD, sHeldView, false);
        return;
    }
    if (!gAgent.getRegion())
    {
        return;
    }
    const LLViewerCamera* camera = LLViewerCamera::getInstance();
    sHeldPosition = gAgent.getPosGlobalFromAgent(camera->getOrigin());
    sHeldAt = camera->getAtAxis();
    sHeldLeft = camera->getLeftAxis();
    sHeldUp = camera->getUpAxis();
    sHeldView = camera->getView();
    sEscapeCount = 0;
    sLocked = true;
    if (gAgent.getAutoPilot())
    {
        gAgent.stopAutoPilot();
    }
    if (gAgentCamera.cameraMouselook())
    {
        // Mouselook steers the avatar with the mouse; the held view is the mouselook one.
        gAgentCamera.changeCameraToDefault();
    }
    changed();
}

void KSCameraPositions::unlockAndReset()
{
    unlock(true);
}

bool KSCameraPositions::holdCamera()
{
    if (!sLocked)
    {
        return false;
    }
    if (LLApp::isExiting() || LLStartUp::getStartupState() != STATE_STARTED || !gAgent.getRegion())
    {
        unlock(false);
        return false;
    }
    if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
    {
        // The held view belongs to the region being left.
        unlock(true);
        return false;
    }
    LLViewerCamera* camera = LLViewerCamera::getInstance();
    camera->setView(sHeldView);
    camera->setOrigin(gAgent.getPosAgentFromGlobal(sHeldPosition));
    camera->mXAxis = sHeldAt;
    camera->mYAxis = sHeldLeft;
    camera->mZAxis = sHeldUp;
    return true;
}

bool KSCameraPositions::blocksAction(const std::string& function_name)
{
    if (!sLocked)
    {
        return false;
    }
    // Avatar movement and camera bindings (llviewerinput.cpp); chat, gestures, media and voice stay.
    static const std::set<std::string> BLOCKED = {
        "jump", "push_down", "push_forward", "push_backward", "look_up", "look_down", "toggle_fly",
        "turn_left", "turn_right", "slide_left", "slide_right",
        "spin_around_ccw", "spin_around_cw", "spin_around_ccw_sitting", "spin_around_cw_sitting",
        "spin_over", "spin_under", "spin_over_sitting", "spin_under_sitting",
        "move_forward", "move_backward", "move_forward_sitting", "move_backward_sitting",
        "pan_up", "pan_down", "pan_left", "pan_right", "pan_in", "pan_out", "roll_left", "roll_right",
        "move_forward_fast", "move_backward_fast",
        "edit_avatar_spin_ccw", "edit_avatar_spin_cw", "edit_avatar_spin_over", "edit_avatar_spin_under",
        "edit_avatar_move_forward", "edit_avatar_move_backward",
        "run_forward", "run_backward", "run_left", "run_right", "toggle_run", "toggle_sit",
        "teleport_to", "walk_to" };
    return BLOCKED.count(function_name) > 0;
}

void KSCameraPositions::noteEscapeKey()
{
    if (!sLocked)
    {
        sEscapeCount = 0;
        return;
    }
    if (sEscapeCount == 0 || sEscapeTimer.getElapsedTimeF32() > ESCAPE_WINDOW)
    {
        sEscapeCount = 0;
        sEscapeTimer.reset();
    }
    if (++sEscapeCount >= 3)
    {
        unlock(true);
    }
}

boost::signals2::signal<void()>& KSCameraPositions::changedSignal()
{
    static boost::signals2::signal<void()> signal;
    return signal;
}
