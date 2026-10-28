/**
 * @file kscamerapositions.h
 * @author Kissmy Spicoli
 * @brief KStorm Camera Positioning: labeled camera views per sim, and Lock Camera
 *        (doc/kstorm-camera-positions.md).
 *
 * A record is a named camera view in one region: the camera position (region-local),
 * yaw / pitch / roll and the view angle. Records live in one per-account file, each
 * tagged with its region name and grid, the way My Lights tags fixed lights. A record's
 * region is the one the camera was over when it was saved or updated, which can be a
 * neighbour of the agent's. Records of a connected region (the agent's, or a neighbour the
 * viewer has loaded, in LLWorld's region list) can be applied and edited; any can be
 * deleted. Names are unique within a region.
 *
 * Each record also keeps its region's global origin, taken whenever the region is connected.
 * With it, a record of a disconnected region on this grid can be applied too: the camera goes
 * to the global position, and the simulator connects the regions near the camera (it receives
 * the camera position and draw distance in every AgentUpdate).
 *
 * Applying a record moves the camera through LLAgentCamera (focus off the avatar, roll,
 * CameraAngle), so the camera can be moved on from there as usual.
 *
 * Lock Camera pins the rendered camera every frame (holdCamera(), from the idle loop) and makes
 * avatar and camera key / mouse bindings, the joystick and mouselook do nothing. Pressing
 * Esc three times, closing the Camera Positioning floater, a teleport, a disconnect or
 * quitting turns it off and resets the view; the Lock Camera toggle turns it off and leaves the
 * camera where it is. Lock Camera is never saved.
 */
#ifndef KS_CAMERAPOSITIONS_H
#define KS_CAMERAPOSITIONS_H

#include "v3dmath.h"
#include "v3math.h"

#include <boost/signals2.hpp>
#include <string>
#include <vector>

namespace KSCameraPositions
{
    struct View
    {
        LLVector3 mPosition;    // region-local, meters
        F32 mYaw = 0.f;         // degrees, 0 = east, 90 = north
        F32 mPitch = 0.f;       // degrees, positive looks up
        F32 mRoll = 0.f;        // degrees, LLAgentCamera roll
        F32 mViewAngle = 0.f;   // vertical view angle (CameraAngle), degrees
    };

    struct Record
    {
        std::string mName;
        std::string mRegion;
        std::string mGrid;
        View mView;
        LLVector3d mOrigin;         // the region's global origin when last connected
        bool mHasOrigin = false;    // false for records saved before origins were kept, until connected
    };

    // Every record: the agent's region's first (by name), then connected regions', then the
    // others (each by region, name).
    std::vector<Record> records();
    // Whether a record belongs to the agent's region and grid.
    bool isHere(const Record& record);
    // Whether a record belongs to another region the viewer is connected to (same grid).
    bool isConnected(const Record& record);
    // Whether a record belongs to the agent's region or a connected one: it can be applied and edited.
    bool isAvailable(const Record& record);
    // Whether a record can be applied: available, or of this grid with a known region origin.
    bool canApply(const Record& record);
    // Names of the connected regions other than the agent's, sorted, one per line: to notice changes.
    std::string connectedRegions();
    bool exists(const std::string& name, const std::string& region);
    std::string currentRegion();               // the agent's; "" when there is none
    // The connected region the camera is over (else the agent's); "" when there is none.
    std::string cameraRegion();

    // The current camera view, local to cameraRegion(); false without a region.
    bool currentView(View& view);

    // Saves the current view under a name in cameraRegion() (replacing one of the same name
    // there). False if the name is empty or there is no region.
    bool saveCurrent(const std::string& name);
    // Replaces a record's view with the current one; the record moves to cameraRegion().
    // False if the record is gone, not available, or another view there has its name.
    bool update(const Record& record);
    // Renames an available record within its region; false if the name is empty or taken there.
    bool rename(const Record& record, const std::string& to);
    // Deletes a record of any region (matched by name, region and grid).
    bool remove(const Record& record);
    // Moves the camera to a record canApply() accepts (the held view while locked). A
    // disconnected region's record goes by its stored origin; the region appears once the
    // simulator connects it.
    bool apply(const Record& record);

    // Zoom shown for a view angle: the default view angle (60 degrees) over it.
    F32 zoomFactor(F32 view_angle_degrees);

    bool isLocked();
    // Locks at the current view, or unlocks leaving the camera there.
    void setLocked(bool locked);
    // Unlocks (if locked) and resets the view to the avatar.
    void unlockAndReset();

    // Called from the idle loop before the camera update; while locked, pins the camera and
    // returns true (the caller then skips its own camera update).
    bool holdCamera();
    // Whether a key / mouse binding function is dead while locked (llviewerinput.cpp).
    bool blocksAction(const std::string& function_name);
    // Every Esc key press (llviewerwindow.cpp); three within a short time unlock and reset.
    void noteEscapeKey();

    // Records or the lock state changed.
    boost::signals2::signal<void()>& changedSignal();
}

#endif // KS_CAMERAPOSITIONS_H
