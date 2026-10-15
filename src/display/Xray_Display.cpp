/*
 * Xray_Display.cpp
 * This file implements the conversion of an X-ray image to the grey values that the screen shows.
 */

#include "Xray_Display.h"

#include <algorithm>
#include <cmath>

namespace Xray_Display
{
	float find_reference_value(const Xray_Image& image)
	{
		float reference{ 0.0f };
		for (const float value : image.pixels)
		{
			if (std::isfinite(value) && value > reference)
				reference = value;
		}
		return reference;
	}

	float get_transmission(float value, float reference)
	{
		if (!(reference > 0.0f) || !std::isfinite(value) || !(value > 0.0f))
			return 0.0f;
		return std::min(value / reference, 1.0f);
	}

	std::uint8_t get_grey_value(float transmission, const Xray_Display_Settings& settings)
	{
		transmission = std::clamp(transmission, 0.0f, 1.0f);

		float grey{ transmission };
		if (settings.mode == Xray_Display_Mode::LOGARITHMIC)
		{ // the logarithm of 0 is not finite: the transmissions below the range are black
			grey = transmission > 0.0f ? std::clamp(1.0f + std::log10(transmission) / DECADES, 0.0f, 1.0f) : 0.0f;
		}
		if (settings.is_negative)
			grey = 1.0f - grey;

		return static_cast<std::uint8_t>(std::lround(grey * 255.0f));
	}

	std::vector<std::uint8_t> convert_to_grey(const Xray_Image& image, float reference, const Xray_Display_Settings& settings)
	{
		std::vector<std::uint8_t> grey;
		grey.reserve(image.pixels.size());
		for (const float value : image.pixels) grey.push_back(get_grey_value(get_transmission(value, reference), settings));
		return grey;
	}
} // namespace Xray_Display
