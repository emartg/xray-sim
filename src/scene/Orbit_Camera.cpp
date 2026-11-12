/*
 * Orbit_Camera.cpp
 * This file implements the Orbit_Camera struct, which moves the camera of the 3D view with the mouse.
 */

#include "Orbit_Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace
{
	glm::vec3 to_vec3(const std::array<float, 3>& values)
	{
		return glm::vec3{ values[0], values[1], values[2] };
	}

	std::array<float, 3> to_array(const glm::vec3& vector)
	{
		return { vector.x, vector.y, vector.z };
	}

	// The up of the world, as the camera of the simulator's scene has it
	const glm::vec3 WORLD_UP{ 0.0f, 1.0f, 0.0f };
} // namespace

// Public Methods
// --------------
std::array<float, 3> Orbit_Camera::get_front() const
{
	const float yaw_radians   = glm::radians(yaw);
	const float pitch_radians = glm::radians(pitch);
	return to_array(
		glm::normalize(
			glm::vec3{ std::cos(yaw_radians) * std::cos(pitch_radians),
					   std::sin(pitch_radians),
					   std::sin(yaw_radians) * std::cos(pitch_radians) }));
}

std::array<float, 3> Orbit_Camera::get_right() const
{
	return to_array(glm::normalize(glm::cross(to_vec3(get_front()), WORLD_UP)));
}

std::array<float, 3> Orbit_Camera::get_up() const
{
	return to_array(glm::normalize(glm::cross(to_vec3(get_right()), to_vec3(get_front()))));
}

float Orbit_Camera::get_depth_of(const std::array<float, 3>& point) const
{
	return glm::dot(to_vec3(point) - to_vec3(position), to_vec3(get_front()));
}

void Orbit_Camera::orbit(const std::array<float, 3>& pivot, float delta_x, float delta_y)
{
	// the angles: the pitch stays within its limits, so the turn about the right is what is left of it
	const float yaw_change   = delta_x * ORBIT_DEGREES_PER_PIXEL;
	const float new_pitch    = std::clamp(pitch - delta_y * ORBIT_DEGREES_PER_PIXEL, MIN_PITCH, MAX_PITCH);
	const float pitch_change = new_pitch - pitch;

	// the position turns about the pivot as the direction of the view turns: first about the right of the view (the pitch), then about
	// the up of the world (the yaw). A larger yaw turns the direction from +x towards +z, which is a turn of minus the angle about +y
	const glm::quat pitch_turn = glm::angleAxis(glm::radians(pitch_change), to_vec3(get_right()));
	const glm::quat yaw_turn   = glm::angleAxis(glm::radians(-yaw_change), WORLD_UP);
	const glm::vec3 offset     = yaw_turn * (pitch_turn * (to_vec3(position) - to_vec3(pivot)));
	position                   = to_array(to_vec3(pivot) + offset);

	// the yaw is kept between -180 and 180 degrees (the direction is the same)
	yaw   = std::remainder(yaw + yaw_change, 360.0f);
	pitch = new_pitch;
}

void Orbit_Camera::pan(const std::array<float, 3>& pivot, float delta_x, float delta_y, float field_of_view, float view_height)
{
	if (view_height <= 0.0f)
		return;

	// the size of a pixel at the depth of the pivot (at least MIN_DISTANCE, so that a pivot beside or behind the camera still pans)
	const float depth                 = std::max(get_depth_of(pivot), MIN_DISTANCE);
	const float centimetres_per_pixel = 2.0f * depth * std::tan(glm::radians(field_of_view) / 2.0f) / view_height;

	// the scene follows the mouse, so the camera moves the other way: left when the mouse goes right, up when it goes down
	const glm::vec3 movement =
		to_vec3(get_right()) * (-delta_x * centimetres_per_pixel) + to_vec3(get_up()) * (delta_y * centimetres_per_pixel);
	position = to_array(to_vec3(position) + movement);
}

void Orbit_Camera::dolly(const std::array<float, 3>& pivot, float notches)
{
	if (notches == 0.0f)
		return;

	const float depth = get_depth_of(pivot);

	// the movement forward that multiplies the distance by the factor for each notch
	float movement = std::max(depth, MIN_DISTANCE) * (1.0f - std::pow(DOLLY_FACTOR_PER_NOTCH, notches));
	if (movement > 0.0f) // forward: stop at MIN_DISTANCE in front of the pivot
		movement = std::min(movement, std::max(depth - MIN_DISTANCE, 0.0f));
	else // back: stop at MAX_DISTANCE from the pivot
		movement = std::max(movement, std::min(depth - MAX_DISTANCE, 0.0f));

	position = to_array(to_vec3(position) + to_vec3(get_front()) * movement);
}
