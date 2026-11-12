/*
 * XraySetupTest.cpp
 * This file is the test of the Xray_Setup struct. It checks the default setup (which is the one of the first X-ray view), the
 * conversion to the beam and the detector of the X-ray library, the free tube (the direction from the yaw and the pitch, the detector
 * that follows the beam, the roll, and aiming at a point), the projection of a point on the detector, the magnification, the corners of
 * the detector plate, and the limits of every value. It returns an error code if a check fails.
 */

#include <array>
#include <cmath>
#include <cstddef>
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

	bool is_near(const std::array<float, 3>& a, const std::array<float, 3>& b, double tolerance = 1e-4)
	{
		return is_near(a[0], b[0], tolerance) && is_near(a[1], b[1], tolerance) && is_near(a[2], b[2], tolerance);
	}

	double dot(const std::array<float, 3>& a, const std::array<float, 3>& b)
	{
		return static_cast<double>(a[0]) * b[0] + static_cast<double>(a[1]) * b[1] + static_cast<double>(a[2]) * b[2];
	}

	// Checks the default setup: a source 100 cm before the scene and a detector 50 cm behind it (magnification 1.5), 256 by 128 pixels
	// of 2 mm, as before the tube was free
	void test_default_setup()
	{
		const Xray_Setup setup;
		check(setup.energy == 80.0f, "the default energy is 80 keV");
		check(is_near(setup.get_magnification(), 1.5, 1e-6), "the default magnification is 1.5");
		check(setup.get_direction() == std::array<float, 3>{ 0.0f, 0.0f, 1.0f }, "the default beam runs along +z");
		check(setup.get_detector_position() == std::array<float, 3>{ 0.0f, 0.0f, 50.0f }, "the default detector is at z = +50 cm");
		check(setup.get_detector_up() == std::array<float, 3>{ 0.0f, 1.0f, 0.0f }, "the default up vector is y");
		check(is_near(setup.get_detector_right(), { 1.0f, 0.0f, 0.0f }), "the default right of the image is +x");
		check(setup.get_detector_size() == std::array<float, 2>{ 51.2f, 25.6f }, "the default detector is 51.2 by 25.6 cm");

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

		// the projection: a point of the plane z = 0 lands at 1.5 times its coordinates, in millimetres, with +x on the right and +y up
		double x{ 0.0 };
		double y{ 0.0 };
		check(setup.project({ 4.0f, -2.0f, 0.0f }, x, y), "a point in front of the source is projected");
		check(is_near(x, 60.0, 1e-3) && is_near(y, -30.0, 1e-3), "a point of the plane z = 0 is magnified 1.5 times, in millimetres");
		check(
			setup.project({ 0.0f, 0.0f, 20.0f }, x, y) && is_near(x, 0.0, 1e-3) && is_near(y, 0.0, 1e-3),
			"the axis projects on the centre");
		check(!setup.project({ 0.0f, 0.0f, -150.0f }, x, y), "a point behind the source is not projected");
		check(is_near(setup.get_magnification_at({ 0.0f, 0.0f, 25.0f }), 150.0 / 125.0, 1e-6), "the magnification falls with the depth");
		check(setup.get_magnification_at({ 0.0f, 0.0f, -150.0f }) == 0.0f, "a point behind the source has no magnification");
	}

	// Checks another setup: the values are converted from centimetres to millimetres, and the setups are compared by their values
	void test_conversion()
	{
		Xray_Setup other;
		other.energy                   = 40.0f;
		other.source_position          = { 0.0f, 0.0f, -150.0f };
		other.source_detector_distance = 180.0f;
		other.pixel_size               = 1.0f;
		other.width_in_pixels          = 64;
		other.height_in_pixels         = 32;
		check(is_near(other.get_magnification(), 1.2, 1e-6), "the magnification is 180 / 150");
		check(is_near(other.make_beam().source_position[2], -1500.0), "150 cm are 1500 mm");
		check(is_near(other.make_detector().position[2], 300.0, 1e-3), "the detector is 180 cm from the source: z = +300 mm");
		check(other.make_detector().width_in_pixels == 64, "the detector has 64 pixels");
		check(!(other == Xray_Setup{}) && Xray_Setup{} == Xray_Setup{}, "the setups are compared by their values");

		// a setup that is within the limits does not change
		Xray_Setup valid = other;
		valid.clamp();
		check(valid == other, "a setup within the limits is not changed by clamp");
	}

	// Checks the free tube: the direction, the detector that follows the beam and faces it, the roll, and aiming at a point
	void test_free_tube()
	{
		// yaw turns the beam about y (90 degrees: along +x), and pitch raises it (30 degrees: half of it is up)
		Xray_Setup setup;
		setup.yaw = 90.0f;
		check(is_near(setup.get_direction(), { 1.0f, 0.0f, 0.0f }), "a yaw of 90 degrees points the beam along +x");
		setup.yaw   = 0.0f;
		setup.pitch = 30.0f;
		check(is_near(setup.get_direction(), { 0.0f, 0.5f, 0.8660254f }), "a pitch of 30 degrees raises the beam");

		// any tube: the direction is a unit vector, the detector is at the distance along it, and the up vector is a unit vector
		// perpendicular to the beam
		setup.source_position                = { 40.0f, 25.0f, -80.0f };
		setup.yaw                            = -35.0f;
		setup.pitch                          = 20.0f;
		setup.roll                           = 50.0f;
		setup.source_detector_distance       = 200.0f;
		const std::array<float, 3> direction = setup.get_direction();
		const std::array<float, 3> up        = setup.get_detector_up();
		const std::array<float, 3> centre    = setup.get_detector_position();
		check(is_near(dot(direction, direction), 1.0, 1e-5), "the direction is a unit vector");
		check(is_near(dot(up, up), 1.0, 1e-5), "the up vector is a unit vector");
		check(is_near(dot(up, direction), 0.0, 1e-5), "the up vector is perpendicular to the beam");
		const std::array<float, 3> right = setup.get_detector_right();
		check(is_near(dot(right, right), 1.0, 1e-5), "the right vector is a unit vector");
		check(
			is_near(dot(right, direction), 0.0, 1e-5) && is_near(dot(right, up), 0.0, 1e-5),
			"the right vector is perpendicular to the beam and to up");
		check(
			is_near(centre, { 40.0f + 200.0f * direction[0], 25.0f + 200.0f * direction[1], -80.0f + 200.0f * direction[2] }, 1e-3),
			"the centre of the detector is on the axis of the beam, at the source-to-detector distance");
		const Xray_Detector detector = setup.make_detector();
		check(
			is_near(detector.position[0], centre[0] * 10.0, 1e-2) && is_near(detector.position[1], centre[1] * 10.0, 1e-2) &&
				is_near(detector.position[2], centre[2] * 10.0, 1e-2),
			"the detector of the library is the centre in millimetres");
		check(
			is_near(detector.up_vector[0], up[0], 1e-6) && is_near(detector.up_vector[1], up[1], 1e-6),
			"the detector of the library has the up vector");

		// the centre of the detector projects on the centre of the image, and a point above it (along the up vector) projects above
		double x{ 0.0 };
		double y{ 0.0 };
		check(
			setup.project(centre, x, y) && is_near(x, 0.0, 1e-2) && is_near(y, 0.0, 1e-2),
			"the centre of the detector projects on its centre");
		check(
			setup.project({ centre[0] + 3.0f * up[0], centre[1] + 3.0f * up[1], centre[2] + 3.0f * up[2] }, x, y) &&
				is_near(x, 0.0, 1e-2) && is_near(y, 30.0, 1e-2),
			"a point 3 cm along the up vector projects 30 mm above the centre");

		// the roll turns the detector about the beam: without roll the up vector has no sideways part, and 90 degrees lay it down
		Xray_Setup rolled;
		rolled.roll = 90.0f;
		check(is_near(rolled.get_direction(), { 0.0f, 0.0f, 1.0f }), "the roll does not move the beam");
		check(is_near(std::abs(rolled.get_detector_up()[0]), 1.0, 1e-5), "a roll of 90 degrees lays the up vector along x");
		check(rolled.get_detector_position() == Xray_Setup{}.get_detector_position(), "the roll does not move the detector");

		// aiming at a point: the beam passes through it, wherever the source is
		Xray_Setup aimed;
		aimed.source_position = { 70.0f, 40.0f, -60.0f };
		aimed.aim_at({ 0.0f, 0.0f, 0.0f });
		check(
			aimed.project({ 0.0f, 0.0f, 0.0f }, x, y) && is_near(x, 0.0, 1e-2) && is_near(y, 0.0, 1e-2),
			"the aimed beam passes through the point");
		const Xray_Setup before = aimed;
		aimed.aim_at(aimed.source_position);
		check(aimed == before, "aiming at the source itself changes nothing");
		aimed.source_position = { 0.0f, 200.0f, 0.0f };
		aimed.aim_at({ 0.0f, 0.0f, 0.0f });
		check(aimed.pitch == Xray_Setup::MIN_PITCH, "aiming straight down stops at the limit of the pitch");
	}

	// Checks the limits: the values outside of them go to the nearest limit, and a value that is not a number to the lower limit
	void test_limits()
	{
		Xray_Setup extreme;
		extreme.energy                   = 1000.0f;
		extreme.source_position          = { 1000.0f, -1000.0f, 0.0f };
		extreme.yaw                      = 500.0f;
		extreme.pitch                    = 90.0f;
		extreme.roll                     = -500.0f;
		extreme.source_detector_distance = 1.0f;
		extreme.pixel_size               = 0.0f;
		extreme.width_in_pixels          = 4;
		extreme.height_in_pixels         = 100000;
		extreme.clamp();
		check(extreme.energy == Xray_Setup::MAX_ENERGY, "an energy above the limit goes to the maximum");
		check(
			extreme.source_position == std::array<float, 3>{ Xray_Setup::MAX_SOURCE_POSITION, Xray_Setup::MIN_SOURCE_POSITION, 0.0f },
			"a source too far goes to the limits of its position");
		check(extreme.yaw == Xray_Setup::MAX_YAW && extreme.roll == Xray_Setup::MIN_ROLL, "the yaw and the roll go to their limits");
		check(extreme.pitch == Xray_Setup::MAX_PITCH, "a vertical beam goes to the limit of the pitch");
		check(
			extreme.source_detector_distance == Xray_Setup::MIN_SOURCE_DETECTOR_DISTANCE,
			"a detector too close goes to the minimum distance");
		check(extreme.pixel_size == Xray_Setup::MIN_PIXEL_SIZE, "a pixel without size goes to the minimum size");
		check(extreme.width_in_pixels == Xray_Setup::MIN_PIXELS, "a detector with too few pixels goes to the minimum");
		check(extreme.height_in_pixels == Xray_Setup::MAX_PIXELS, "a detector with too many pixels goes to the maximum");

		Xray_Setup not_a_number;
		not_a_number.energy                   = std::numeric_limits<float>::quiet_NaN();
		not_a_number.source_position[2]       = std::numeric_limits<float>::quiet_NaN();
		not_a_number.pitch                    = std::numeric_limits<float>::quiet_NaN();
		not_a_number.source_detector_distance = std::numeric_limits<float>::quiet_NaN();
		not_a_number.pixel_size               = -std::numeric_limits<float>::infinity();
		not_a_number.clamp();
		check(not_a_number.energy == Xray_Setup::MIN_ENERGY, "an energy that is not a number goes to the minimum");
		check(not_a_number.source_position[2] == Xray_Setup::MIN_SOURCE_POSITION, "a position that is not a number goes to the minimum");
		check(not_a_number.pitch == Xray_Setup::MIN_PITCH, "a pitch that is not a number goes to the minimum");
		check(
			not_a_number.source_detector_distance == Xray_Setup::MIN_SOURCE_DETECTOR_DISTANCE,
			"a distance that is not a number goes to the minimum");
		check(not_a_number.pixel_size == Xray_Setup::MIN_PIXEL_SIZE, "a negative infinite pixel size goes to the minimum");

		// the limits themselves are consistent: the pitch stops short of the vertical, where the up vector would not be defined
		check(Xray_Setup::MAX_PITCH < 90.0f && Xray_Setup::MIN_PITCH > -90.0f, "the beam cannot be vertical");
		check(Xray_Setup::MIN_ENERGY < Xray_Setup::MAX_ENERGY && Xray_Setup::MIN_PIXELS < Xray_Setup::MAX_PIXELS, "the limits are ordered");
		check(Xray_Setup::MIN_SOURCE_DETECTOR_DISTANCE > 0.0f, "the detector is never on the source");
	}
	// Checks the corners of the detector plate: the default ones, and, for a tube that is turned and rolled, corners that are on the plane
	// of the detector, at the size of the detector, and that project where the corners of the image are
	void test_detector_corners()
	{
		const Xray_Setup                          default_setup;
		const std::array<std::array<float, 3>, 4> corners = default_setup.get_detector_corners();
		check(is_near(corners[0], { -25.6f, 12.8f, 50.0f }), "the first corner is the top left of the image");
		check(is_near(corners[1], { 25.6f, 12.8f, 50.0f }), "the second corner is the top right");
		check(is_near(corners[2], { 25.6f, -12.8f, 50.0f }), "the third corner is the bottom right");
		check(is_near(corners[3], { -25.6f, -12.8f, 50.0f }), "the fourth corner is the bottom left");

		Xray_Setup setup;
		setup.source_position                                = { 30.0f, 40.0f, -60.0f };
		setup.yaw                                            = -25.0f;
		setup.pitch                                          = -35.0f;
		setup.roll                                           = 20.0f;
		const std::array<std::array<float, 3>, 4>  turned    = setup.get_detector_corners();
		const std::array<float, 3>                 centre    = setup.get_detector_position();
		const std::array<float, 3>                 direction = setup.get_direction();
		const std::array<float, 2>                 size      = setup.get_detector_size();
		const std::array<std::array<double, 2>, 4> expected{
			{ { -256.0, 128.0 }, { 256.0, 128.0 }, { 256.0, -128.0 }, { -256.0, -128.0 } }
		};
		for (std::size_t i = 0; i < turned.size(); ++i)
		{
			const std::array<float, 3> offset{ turned[i][0] - centre[0], turned[i][1] - centre[1], turned[i][2] - centre[2] };
			check(is_near(dot(offset, direction), 0.0, 1e-3), "a corner is on the plane of the detector");
			check(
				is_near(dot(offset, offset), (size[0] * size[0] + size[1] * size[1]) / 4.0, 1e-2),
				"a corner is half a diagonal from the centre");
			double x{ 0.0 };
			double y{ 0.0 };
			check(
				setup.project(turned[i], x, y) && is_near(x, expected[i][0], 1e-2) && is_near(y, expected[i][1], 1e-2),
				"a corner projects on the corner of the image");
		}
	}
} // namespace

int main()
{
	test_default_setup();
	test_conversion();
	test_free_tube();
	test_detector_corners();
	test_limits();

	if (failures != 0)
		return 1;

	std::cout << "[SUCCESS::XRAY_SETUP_TEST::main] The X-ray setup converts and limits its values, and its tube is free" << std::endl;
	return 0;
}
