/*
 * Xray_Display.h
 * This file defines how an X-ray image is shown on the screen. The image holds the energy fluence that reaches each pixel of the
 * detector; the screen needs a grey value between 0 and 255. The conversion is relative to the unattenuated beam (the brightest
 * pixel of the image, where the beam crossed no object): the transmission of a pixel is its value divided by that reference, and
 * it is shown as it is (linear) or by its logarithm (the objects of very different attenuation are all visible), and optionally
 * inverted (negative, as a radiograph is usually read). It has no dependency on OpenGL, on the Engine, or on the X-ray library,
 * so it is unit tested.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "xray/Xray_Types.h"

enum class Xray_Display_Mode
{
	LINEAR,     // the grey value is the transmission: 0 black (nothing reaches the pixel), 1 white (the beam is not attenuated)
	LOGARITHMIC // the grey value is 1 + log10(transmission) / DECADES, so the range from 10^-DECADES to 1 goes from black to white
};

struct Xray_Display_Settings
{
	Xray_Display_Mode mode{ Xray_Display_Mode::LOGARITHMIC };
	bool              is_negative{ false }; // true: the grey value is inverted (the attenuating objects are bright)
};

namespace Xray_Display
{
	// Number of decades of transmission shown by the logarithmic mode (the transmission of 0.01 is black)
	constexpr float DECADES{ 2.0f };

	// Returns the value of the unattenuated beam: the largest finite value of the image, or 0 if there is none
	float find_reference_value(const Xray_Image& image);

	// Returns the transmission of a value (the value divided by the reference), between 0 and 1. A value that is not finite,
	// negative, or zero, and any value if the reference is not positive, has transmission 0
	float get_transmission(float value, float reference);

	// Returns the grey value (0 to 255) of a transmission (between 0 and 1) for the settings
	std::uint8_t get_grey_value(float transmission, const Xray_Display_Settings& settings);

	// Converts the image to grey values, one byte per pixel, in the order of the image (row by row, the first row is the bottom of
	// the detector). The reference is the value of the unattenuated beam (see find_reference_value)
	std::vector<std::uint8_t> convert_to_grey(const Xray_Image& image, float reference, const Xray_Display_Settings& settings);
} // namespace Xray_Display
