/*
 * Xray_Setup.cpp
 * This file implements the Xray_Setup struct, which describes how the scene is imaged.
 */

#include "Xray_Setup.h"

#include <algorithm>

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
} // namespace

// Public Methods
// --------------
void Xray_Setup::clamp()
{
	energy            = clamp_value(energy, MIN_ENERGY, MAX_ENERGY);
	source_distance   = clamp_value(source_distance, MIN_SOURCE_DISTANCE, MAX_SOURCE_DISTANCE);
	detector_distance = clamp_value(detector_distance, MIN_DETECTOR_DISTANCE, MAX_DETECTOR_DISTANCE);
	pixel_size        = clamp_value(pixel_size, MIN_PIXEL_SIZE, MAX_PIXEL_SIZE);
	width_in_pixels   = clamp_value(width_in_pixels, MIN_PIXELS, MAX_PIXELS);
	height_in_pixels  = clamp_value(height_in_pixels, MIN_PIXELS, MAX_PIXELS);
}

float Xray_Setup::get_magnification() const
{
	return (source_distance + detector_distance) / source_distance;
}

Xray_Beam Xray_Setup::make_beam() const
{
	Xray_Beam beam;
	beam.source_position = { 0.0, 0.0, -static_cast<double>(source_distance) * MILLIMETRES_PER_CENTIMETRE };
	beam.energy          = energy;
	return beam;
}

Xray_Detector Xray_Setup::make_detector() const
{
	Xray_Detector detector;
	detector.position         = { 0.0, 0.0, static_cast<double>(detector_distance) * MILLIMETRES_PER_CENTIMETRE };
	detector.up_vector        = { 0.0, 1.0, 0.0 };
	detector.width_in_pixels  = width_in_pixels;
	detector.height_in_pixels = height_in_pixels;
	detector.pixel_size       = pixel_size;
	return detector;
}
