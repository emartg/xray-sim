/*
 * Xray_Setup.cpp
 * This file implements the Xray_Setup struct, which describes how the scene is imaged.
 */

#include "Xray_Setup.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

namespace
{
	constexpr double MILLIMETRES_PER_CENTIMETRE{ 10.0 };

	// Returns the value within the limits (the lower limit for a value that is not a number)
	template<typename T>
	T clamp_value(T value, T minimum, T maximum)
	{
		if (!(value >= minimum))
			return minimum;
		return std::min(value, maximum);
	}

	glm::vec3 to_vector(const std::array<float, 3>& array)
	{
		return glm::vec3{ array[0], array[1], array[2] };
	}

	std::array<float, 3> to_array(const glm::vec3& vector)
	{
		return { vector.x, vector.y, vector.z };
	}

} // namespace

// Public Methods
// --------------
void Xray_Setup::clamp()
{
	energy = clamp_value(energy, MIN_ENERGY, MAX_ENERGY);
	for (float& coordinate : source_position) coordinate = clamp_value(coordinate, MIN_SOURCE_POSITION, MAX_SOURCE_POSITION);
	yaw                      = clamp_value(yaw, MIN_YAW, MAX_YAW);
	pitch                    = clamp_value(pitch, MIN_PITCH, MAX_PITCH);
	roll                     = clamp_value(roll, MIN_ROLL, MAX_ROLL);
	source_detector_distance = clamp_value(source_detector_distance, MIN_SOURCE_DETECTOR_DISTANCE, MAX_SOURCE_DETECTOR_DISTANCE);
	pixel_size               = clamp_value(pixel_size, MIN_PIXEL_SIZE, MAX_PIXEL_SIZE);
	width_in_pixels          = clamp_value(width_in_pixels, MIN_PIXELS, MAX_PIXELS);
	height_in_pixels         = clamp_value(height_in_pixels, MIN_PIXELS, MAX_PIXELS);
}

void Xray_Setup::aim_at(const std::array<float, 3>& point)
{
	const glm::vec3 to_point = to_vector(point) - to_vector(source_position);
	const float     distance = glm::length(to_point);
	if (!(distance > 1.0e-4f))
		return;

	// the inverse of get_direction: the pitch from the height, and the yaw from the horizontal part (kept when the beam is vertical)
	const glm::vec3 direction = to_point / distance;
	pitch                     = clamp_value(glm::degrees(std::asin(std::clamp(direction.y, -1.0f, 1.0f))), MIN_PITCH, MAX_PITCH);
	if (std::abs(direction.x) > 1.0e-6f || std::abs(direction.z) > 1.0e-6f)
		yaw = clamp_value(glm::degrees(std::atan2(direction.x, direction.z)), MIN_YAW, MAX_YAW);
}

std::array<float, 3> Xray_Setup::get_direction() const
{
	const float yaw_in_radians   = glm::radians(yaw);
	const float pitch_in_radians = glm::radians(pitch);
	return { std::sin(yaw_in_radians) * std::cos(pitch_in_radians),
			 std::sin(pitch_in_radians),
			 std::cos(yaw_in_radians) * std::cos(pitch_in_radians) };
}

std::array<float, 3> Xray_Setup::get_detector_position() const
{
	return to_array(to_vector(source_position) + to_vector(get_direction()) * source_detector_distance);
}

std::array<float, 3> Xray_Setup::get_detector_up() const
{
	const glm::vec3 direction = to_vector(get_direction());

	// the axis y made perpendicular to the beam (the axis z if the beam is vertical, which the limits of the pitch do not allow)
	glm::vec3 up = glm::vec3{ 0.0f, 1.0f, 0.0f } - direction * direction.y;
	if (glm::length(up) < 1.0e-4f)
		up = glm::vec3{ 0.0f, 0.0f, -1.0f } - direction * -direction.z;
	up = glm::normalize(up);

	// turned about the beam by the roll (the up vector is perpendicular to the beam, so the rotation has two terms)
	const float roll_in_radians = glm::radians(roll);
	return to_array(glm::normalize(up * std::cos(roll_in_radians) + glm::cross(direction, up) * std::sin(roll_in_radians)));
}

std::array<float, 3> Xray_Setup::get_detector_right() const
{
	// up x direction: the right of the image seen from the detector towards the source
	return to_array(glm::normalize(glm::cross(to_vector(get_detector_up()), to_vector(get_direction()))));
}

std::array<float, 2> Xray_Setup::get_detector_size() const
{
	const float millimetres_per_centimetre = static_cast<float>(MILLIMETRES_PER_CENTIMETRE);
	return { static_cast<float>(width_in_pixels) * pixel_size / millimetres_per_centimetre,
			 static_cast<float>(height_in_pixels) * pixel_size / millimetres_per_centimetre };
}

std::array<std::array<float, 3>, 4> Xray_Setup::get_detector_corners() const
{
	const glm::vec3            centre = to_vector(get_detector_position());
	const glm::vec3            right  = to_vector(get_detector_right());
	const glm::vec3            up     = to_vector(get_detector_up());
	const std::array<float, 2> size   = get_detector_size();
	const glm::vec3            half_width{ right * (size[0] / 2.0f) };
	const glm::vec3            half_height{ up * (size[1] / 2.0f) };
	return { to_array(centre - half_width + half_height),
			 to_array(centre + half_width + half_height),
			 to_array(centre + half_width - half_height),
			 to_array(centre - half_width - half_height) };
}

bool Xray_Setup::project(const std::array<float, 3>& point, double& x_on_detector, double& y_on_detector) const
{
	const glm::vec3 direction = to_vector(get_direction());
	const glm::vec3 to_point  = to_vector(point) - to_vector(source_position);
	const float     depth     = glm::dot(to_point, direction); // the distance from the source to the point along the beam
	if (!(depth > 1.0e-4f))
		return false;

	// the ray from the source through the point meets the plane of the detector at the source-to-detector distance along the beam
	const glm::vec3 on_detector = to_point * (source_detector_distance / depth) - direction * source_detector_distance;
	x_on_detector               = static_cast<double>(glm::dot(on_detector, to_vector(get_detector_right()))) * MILLIMETRES_PER_CENTIMETRE;
	y_on_detector               = static_cast<double>(glm::dot(on_detector, to_vector(get_detector_up()))) * MILLIMETRES_PER_CENTIMETRE;
	return true;
}

float Xray_Setup::get_magnification_at(const std::array<float, 3>& point) const
{
	const float depth = glm::dot(to_vector(point) - to_vector(source_position), to_vector(get_direction()));
	return depth > 1.0e-4f ? source_detector_distance / depth : 0.0f;
}

float Xray_Setup::get_magnification() const
{
	return get_magnification_at({ 0.0f, 0.0f, 0.0f });
}

Xray_Beam Xray_Setup::make_beam() const
{
	Xray_Beam beam;
	beam.source_position = { static_cast<double>(source_position[0]) * MILLIMETRES_PER_CENTIMETRE,
							 static_cast<double>(source_position[1]) * MILLIMETRES_PER_CENTIMETRE,
							 static_cast<double>(source_position[2]) * MILLIMETRES_PER_CENTIMETRE };
	beam.energy          = energy;
	return beam;
}

Xray_Detector Xray_Setup::make_detector() const
{
	const std::array<float, 3> position = get_detector_position();
	const std::array<float, 3> up       = get_detector_up();

	Xray_Detector detector;
	detector.position         = { static_cast<double>(position[0]) * MILLIMETRES_PER_CENTIMETRE,
								  static_cast<double>(position[1]) * MILLIMETRES_PER_CENTIMETRE,
								  static_cast<double>(position[2]) * MILLIMETRES_PER_CENTIMETRE };
	detector.up_vector        = { up[0], up[1], up[2] };
	detector.width_in_pixels  = width_in_pixels;
	detector.height_in_pixels = height_in_pixels;
	detector.pixel_size       = pixel_size;
	return detector;
}
