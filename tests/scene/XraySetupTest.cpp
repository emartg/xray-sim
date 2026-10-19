/*
 * XraySetupTest.cpp
 * This file is the test of the Xray_Setup struct. It checks the default setup (which is the one of the first X-ray view), the
 * conversion to the beam and the detector of the X-ray library, the magnification, and the limits of every value. It returns an
 * error code if a check fails.
 */

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

#include "scene/Xray_Setup.h"

namespace
{
	int failures{ 0 };

	void check(bool condition, const std::string& description)
	{
		if (!condition)
		{
			std::cerr << "[ERROR::XRAY_SETUP_TEST::check] " << description << std::endl;
			++failures;
		}
	}

	bool is_near(double a, double b, double tolerance = 1e-9)
	{
		return std::abs(a - b) < tolerance;
	}
} // namespace

int main()
{
	// the default setup: a source 100 cm before the scene and a detector 50 cm behind it (magnification 1.5), 256 by 128 pixels of 2 mm
	const Xray_Setup setup;
	check(setup.energy == 80.0f, "the default energy is 80 keV");
	check(is_near(setup.get_magnification(), 1.5), "the default magnification is 1.5");

	const Xray_Beam beam = setup.make_beam();
	check(is_near(beam.energy, 80.0), "the beam has the energy of the setup");
	check(is_near(beam.source_position[0], 0.0) && is_near(beam.source_position[1], 0.0), "the source is on the axis z");
	check(is_near(beam.source_position[2], -1000.0), "the source is at z = -1000 mm");

	const Xray_Detector detector = setup.make_detector();
	check(is_near(detector.position[2], 500.0), "the detector is at z = +500 mm");
	check(is_near(detector.position[0], 0.0) && is_near(detector.position[1], 0.0), "the detector is centred on the axis z");
	check(
		is_near(detector.up_vector[0], 0.0) && is_near(detector.up_vector[1], 1.0) && is_near(detector.up_vector[2], 0.0),
		"the up vector is y");
	check(detector.width_in_pixels == 256 && detector.height_in_pixels == 128, "the detector has the pixels of the setup");
	check(is_near(detector.pixel_size, 2.0), "the pixels have the size of the setup");

	// another setup: the values are converted from centimetres to millimetres
	Xray_Setup other;
	other.energy            = 40.0f;
	other.source_distance   = 150.0f;
	other.detector_distance = 30.0f;
	other.pixel_size        = 1.0f;
	other.width_in_pixels   = 64;
	other.height_in_pixels  = 32;
	check(is_near(other.get_magnification(), 1.2, 1e-6), "the magnification is (150 + 30) / 150");
	check(is_near(other.make_beam().source_position[2], -1500.0), "150 cm are 1500 mm");
	check(is_near(other.make_detector().position[2], 300.0), "30 cm are 300 mm");
	check(other.make_detector().width_in_pixels == 64, "the detector has 64 pixels");
	check(!(other == setup) && Xray_Setup{} == setup, "the setups are compared by their values");

	// a setup that is within the limits does not change
	Xray_Setup valid = other;
	valid.clamp();
	check(valid == other, "a setup within the limits is not changed by clamp");

	// the values outside of the limits go to the nearest limit
	Xray_Setup extreme;
	extreme.energy            = 1000.0f;
	extreme.source_distance   = 1.0f;
	extreme.detector_distance = 1000.0f;
	extreme.pixel_size        = 0.0f;
	extreme.width_in_pixels   = 4;
	extreme.height_in_pixels  = 100000;
	extreme.clamp();
	check(extreme.energy == Xray_Setup::MAX_ENERGY, "an energy above the limit goes to the maximum");
	check(extreme.source_distance == Xray_Setup::MIN_SOURCE_DISTANCE, "a source too close goes to the minimum distance");
	check(extreme.detector_distance == Xray_Setup::MAX_DETECTOR_DISTANCE, "a detector too far goes to the maximum distance");
	check(extreme.pixel_size == Xray_Setup::MIN_PIXEL_SIZE, "a pixel without size goes to the minimum size");
	check(extreme.width_in_pixels == Xray_Setup::MIN_PIXELS, "a detector with too few pixels goes to the minimum");
	check(extreme.height_in_pixels == Xray_Setup::MAX_PIXELS, "a detector with too many pixels goes to the maximum");

	// a value that is not a number goes to the lower limit, and the source never gets inside of the scene
	Xray_Setup not_a_number;
	not_a_number.energy          = std::numeric_limits<float>::quiet_NaN();
	not_a_number.source_distance = std::numeric_limits<float>::quiet_NaN();
	not_a_number.pixel_size      = -std::numeric_limits<float>::infinity();
	not_a_number.clamp();
	check(not_a_number.energy == Xray_Setup::MIN_ENERGY, "an energy that is not a number goes to the minimum");
	check(not_a_number.source_distance == Xray_Setup::MIN_SOURCE_DISTANCE, "a distance that is not a number goes to the minimum");
	check(not_a_number.pixel_size == Xray_Setup::MIN_PIXEL_SIZE, "a negative infinite pixel size goes to the minimum");

	// the limits themselves are consistent: the minimum source distance is outside of the scene (objects within 20 cm of z = 0)
	check(Xray_Setup::MIN_SOURCE_DISTANCE > 20.0f, "the source cannot be inside of the scene");
	check(Xray_Setup::MIN_ENERGY < Xray_Setup::MAX_ENERGY && Xray_Setup::MIN_PIXELS < Xray_Setup::MAX_PIXELS, "the limits are ordered");

	if (failures != 0)
		return 1;

	std::cout << "[SUCCESS::XRAY_SETUP_TEST::main] The X-ray setup converts and limits its values" << std::endl;
	return 0;
}
