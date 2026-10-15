/*
 * XrayDisplayTest.cpp
 * This file is the test of the conversion of an X-ray image to grey values (Xray_Display). It checks the reference value, the
 * transmission, the linear and logarithmic modes, the negative, and the values that are not valid. It returns an error code if a
 * check fails.
 */

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "display/Xray_Display.h"

namespace
{
	int failures{ 0 };

	void check(bool condition, const std::string& description)
	{
		if (!condition)
		{
			std::cerr << "[ERROR::XRAY_DISPLAY_TEST::check] " << description << std::endl;
			++failures;
		}
	}

	Xray_Image make_image(unsigned int width, unsigned int height, std::vector<float> pixels)
	{
		Xray_Image image;
		image.width  = width;
		image.height = height;
		image.pixels = std::move(pixels);
		return image;
	}
} // namespace

int main()
{
	const float                 infinity     = std::numeric_limits<float>::infinity();
	const float                 not_a_number = std::numeric_limits<float>::quiet_NaN();
	const Xray_Display_Settings linear{ Xray_Display_Mode::LINEAR, false };
	const Xray_Display_Settings logarithmic{ Xray_Display_Mode::LOGARITHMIC, false };
	const Xray_Display_Settings linear_negative{ Xray_Display_Mode::LINEAR, true };
	const Xray_Display_Settings logarithmic_negative{ Xray_Display_Mode::LOGARITHMIC, true };

	// the reference is the largest finite value
	check(Xray_Display::find_reference_value(make_image(2, 2, { 1.0f, 4.0f, 2.0f, 3.0f })) == 4.0f, "the reference is the largest value");
	check(
		Xray_Display::find_reference_value(make_image(2, 2, { 1.0f, infinity, not_a_number, 3.0f })) == 3.0f,
		"the reference ignores the values that are not finite");
	check(Xray_Display::find_reference_value(make_image(0, 0, {})) == 0.0f, "the reference of an empty image is 0");
	check(Xray_Display::find_reference_value(make_image(1, 2, { -1.0f, 0.0f })) == 0.0f, "the reference with no positive value is 0");

	// the transmission is the value over the reference, between 0 and 1
	check(Xray_Display::get_transmission(2.0f, 4.0f) == 0.5f, "the transmission is the value over the reference");
	check(Xray_Display::get_transmission(5.0f, 4.0f) == 1.0f, "the transmission is at most 1");
	check(Xray_Display::get_transmission(-1.0f, 4.0f) == 0.0f, "a negative value has transmission 0");
	check(Xray_Display::get_transmission(not_a_number, 4.0f) == 0.0f, "a value that is not a number has transmission 0");
	check(Xray_Display::get_transmission(infinity, 4.0f) == 0.0f, "an infinite value has transmission 0");
	check(Xray_Display::get_transmission(1.0f, 0.0f) == 0.0f, "any value has transmission 0 if the reference is 0");

	// linear mode: the grey value is the transmission
	check(Xray_Display::get_grey_value(0.0f, linear) == 0, "linear: transmission 0 is black");
	check(Xray_Display::get_grey_value(1.0f, linear) == 255, "linear: transmission 1 is white");
	check(Xray_Display::get_grey_value(0.5f, linear) == 128, "linear: transmission 0.5 is 128");
	check(Xray_Display::get_grey_value(2.0f, linear) == 255, "linear: a transmission above 1 is clamped");
	check(Xray_Display::get_grey_value(-1.0f, linear) == 0, "linear: a negative transmission is clamped");

	// logarithmic mode: 1 is white, 10^-DECADES and below are black, 10^(-DECADES / 2) is half way
	check(Xray_Display::get_grey_value(1.0f, logarithmic) == 255, "logarithmic: transmission 1 is white");
	check(Xray_Display::get_grey_value(0.1f, logarithmic) == 128, "logarithmic: transmission 0.1 is half way");
	check(Xray_Display::get_grey_value(0.01f, logarithmic) == 0, "logarithmic: transmission 0.01 is black");
	check(Xray_Display::get_grey_value(0.0001f, logarithmic) == 0, "logarithmic: a transmission below the range is black");
	check(Xray_Display::get_grey_value(0.0f, logarithmic) == 0, "logarithmic: transmission 0 is black");

	// negative: the grey value is inverted
	check(Xray_Display::get_grey_value(1.0f, linear_negative) == 0, "linear negative: transmission 1 is black");
	check(Xray_Display::get_grey_value(0.0f, linear_negative) == 255, "linear negative: transmission 0 is white");
	check(Xray_Display::get_grey_value(0.0f, logarithmic_negative) == 255, "logarithmic negative: transmission 0 is white");
	check(Xray_Display::get_grey_value(1.0f, logarithmic_negative) == 0, "logarithmic negative: transmission 1 is black");

	// the grey value does not decrease when the transmission increases
	for (const Xray_Display_Settings& settings : { linear, logarithmic })
	{
		std::uint8_t previous{ 0 };
		bool         is_monotonic{ true };
		for (int i = 0; i <= 100; ++i)
		{
			const std::uint8_t grey = Xray_Display::get_grey_value(static_cast<float>(i) / 100.0f, settings);
			if (grey < previous)
				is_monotonic = false;
			previous = grey;
		}
		check(is_monotonic, "the grey value does not decrease when the transmission increases");
	}

	// the whole image: the same size and order, the unattenuated beam is white, the objects are darker
	const Xray_Image image     = make_image(3, 2, { 8.0f, 4.0f, 0.0f, 8.0f, 0.08f, 8.0f });
	const float      reference = Xray_Display::find_reference_value(image);
	const auto       grey      = Xray_Display::convert_to_grey(image, reference, logarithmic);
	check(reference == 8.0f, "the reference of the image is 8");
	check(grey.size() == 6, "the converted image has one byte per pixel");
	if (grey.size() == 6)
	{
		check(grey[0] == 255 && grey[3] == 255 && grey[5] == 255, "the unattenuated pixels are white");
		check(grey[1] < grey[0] && grey[1] > grey[4], "the order of the pixels is kept (8 > 4 > 0.08)");
		check(grey[2] == 0, "the pixel with no fluence is black");
		check(grey[4] == 0, "the pixel with a transmission of 0.01 is black");
	}
	check(Xray_Display::convert_to_grey(make_image(0, 0, {}), 1.0f, linear).empty(), "an empty image gives an empty result");
	check(Xray_Display::convert_to_grey(image, 0.0f, linear) == std::vector<std::uint8_t>(6, 0), "a reference of 0 gives black");

	if (failures != 0)
		return 1;

	std::cout << "[SUCCESS::XRAY_DISPLAY_TEST::main] The conversion of the X-ray images to grey values works" << std::endl;
	return 0;
}
