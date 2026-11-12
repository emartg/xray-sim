/*
 * Orbit_Camera.h
 * This file defines the Orbit_Camera struct, which moves the camera of the 3D view with the mouse: it orbits about a point (the pivot,
 * the selected object or the centre of the scene), pans across the view, and dollies towards the pivot. The camera is described as
 * the Engine describes it: a position, and a yaw and a pitch in degrees that give the direction of the view (a yaw of -90 degrees
 * and a pitch of 0 look along -z), with the axis y of the scene as the up of the world.
 * The orbit turns the camera rigidly about the pivot: its position and its direction turn together, so the pivot stays where it was
 * in the view (the view does not jump to the pivot when a drag starts) and only the scene turns about it.
 * The lengths are in centimetres, like the rest of the scene, and the angles in degrees. The camera is independent of the X-ray tube.
 * It has no dependency on OpenGL, on the Engine, or on the X-ray library.
 */

#pragma once

#include <array>

struct Orbit_Camera
{
	// Public Constants
	// ----------------
	// How far the mouse turns, pans, and dollies the camera, and its limits: a pitch that stops short of the vertical (as the Engine's
	// camera does, where the yaw would lose its meaning), and a distance to the pivot, along the view, that the wheel keeps between a
	// close look and the far plane of the Engine's view (100 cm from the camera; farther objects are not drawn)
	static constexpr float ORBIT_DEGREES_PER_PIXEL{ 0.25f };
	static constexpr float DOLLY_FACTOR_PER_NOTCH{ 0.85f }; // the distance to the pivot is multiplied by it for each notch of the wheel
	static constexpr float MIN_PITCH{ -89.0f }, MAX_PITCH{ 89.0f };     // in degrees
	static constexpr float MIN_DISTANCE{ 2.0f }, MAX_DISTANCE{ 90.0f }; // from the camera to the pivot along the view, in centimetres

	// Public Methods
	// --------------
	// Returns the direction of the view, a unit vector: (cos(yaw) cos(pitch), sin(pitch), sin(yaw) cos(pitch)), as the Engine's camera
	// computes it
	std::array<float, 3> get_front() const;

	// Returns the right of the view, a unit vector: the direction of the view times the up of the world (y), normalized
	std::array<float, 3> get_right() const;

	// Returns the up of the view, a unit vector perpendicular to the direction and to the right
	std::array<float, 3> get_up() const;

	// Returns the distance from the camera to a point along the direction of the view (negative if the point is behind the camera)
	float get_depth_of(const std::array<float, 3>& point) const;

	// Turns the camera about the pivot by the movement of the mouse, in pixels (x to the right, y down, as the mouse reports them).
	// The scene follows the mouse: a movement to the right turns the scene to the right (the yaw grows), and a movement down turns the
	// top of the scene towards the camera (the pitch falls), at ORBIT_DEGREES_PER_PIXEL. The pitch stays within its limits, and the
	// distance to the pivot does not change
	void orbit(const std::array<float, 3>& pivot, float delta_x, float delta_y);

	// Moves the camera across the view by the movement of the mouse, in pixels, so that a point at the depth of the pivot follows the
	// mouse. The field of view is the vertical angle of the view in degrees, and the view height is the height of the view in pixels.
	// The direction of the view does not change
	void pan(const std::array<float, 3>& pivot, float delta_x, float delta_y, float field_of_view, float view_height);

	// Moves the camera along the direction of the view by the turns of the wheel, in notches (positive forward): the distance to the
	// pivot along the view is multiplied by DOLLY_FACTOR_PER_NOTCH for each notch forward and divided by it for each notch back. The
	// camera stops at MIN_DISTANCE in front of the pivot and at MAX_DISTANCE from it, and does not move forward when the pivot is not
	// farther than MIN_DISTANCE in front of it
	void dolly(const std::array<float, 3>& pivot, float notches);

	// Public Attributes
	// -----------------
	std::array<float, 3> position{ 0.0f, 0.0f, 0.0f }; // in centimetres
	float                yaw{ -90.0f };                // in degrees
	float                pitch{ 0.0f };                // in degrees
};
