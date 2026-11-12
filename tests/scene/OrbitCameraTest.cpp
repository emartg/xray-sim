/*
 * OrbitCameraTest.cpp
 * This file is the test of the Orbit_Camera struct. It checks the vectors of the view (as the Engine's camera computes them), the orbit
 * about a pivot (the direction that the mouse turns the scene, the distance and the place of the pivot in the view that it keeps, and
 * the limits of the pitch), the pan (a point at the depth of the pivot follows the mouse), and the dolly (the factor of each notch and
 * the limits of the distance). It returns an error code if a check fails.
 */

#include <array>
#include <cmath>
#include <iostream>
#include <string>

#include "scene/Orbit_Camera.h"

namespace
{
	int failures{ 0 };

	void check(bool condition, const std::string& description)
	{
		if (!condition)
		{
			std::cerr << "[ERROR::ORBIT_CAMERA_TEST::check] " << description << std::endl;
			++failures;
		}
	}

	bool is_near(double a, double b, double tolerance = 1e-4)
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

	double distance(const std::array<float, 3>& a, const std::array<float, 3>& b)
	{
		const std::array<float, 3> difference{ a[0] - b[0], a[1] - b[1], a[2] - b[2] };
		return std::sqrt(dot(difference, difference));
	}

	// Returns where a point is in the frame of the view: its coordinates along the right, the up, and the direction of the view
	std::array<float, 3> to_view(const Orbit_Camera& camera, const std::array<float, 3>& point)
	{
		const std::array<float, 3> offset{ point[0] - camera.position[0], point[1] - camera.position[1], point[2] - camera.position[2] };
		return { static_cast<float>(dot(offset, camera.get_right())),
				 static_cast<float>(dot(offset, camera.get_up())),
				 static_cast<float>(dot(offset, camera.get_front())) };
	}

	// A camera 40 cm in front of the origin, looking at it along -z
	Orbit_Camera make_front_camera()
	{
		Orbit_Camera camera;
		camera.position = { 0.0f, 0.0f, 40.0f };
		camera.yaw      = -90.0f;
		camera.pitch    = 0.0f;
		return camera;
	}

	// Checks the vectors of the view: a yaw of -90 degrees looks along -z with +x on the right, and the three vectors are orthonormal
	void test_vectors()
	{
		const Orbit_Camera camera = make_front_camera();
		check(is_near(camera.get_front(), { 0.0f, 0.0f, -1.0f }), "a yaw of -90 degrees looks along -z");
		check(is_near(camera.get_right(), { 1.0f, 0.0f, 0.0f }), "the right of the view is +x");
		check(is_near(camera.get_up(), { 0.0f, 1.0f, 0.0f }), "the up of the view is +y");
		check(is_near(camera.get_depth_of({ 3.0f, -2.0f, 0.0f }), 40.0), "the depth of a point of the plane z = 0 is 40 cm");

		Orbit_Camera tilted;
		tilted.yaw                       = 30.0f;
		tilted.pitch                     = -40.0f;
		const std::array<float, 3> front = tilted.get_front();
		const std::array<float, 3> right = tilted.get_right();
		const std::array<float, 3> up    = tilted.get_up();
		check(
			is_near(dot(front, front), 1.0) && is_near(dot(right, right), 1.0) && is_near(dot(up, up), 1.0),
			"the vectors are unit vectors");
		check(
			is_near(dot(front, right), 0.0) && is_near(dot(front, up), 0.0) && is_near(dot(right, up), 0.0),
			"the vectors are perpendicular");
		check(is_near(right[1], 0.0), "the right of the view is horizontal");
		check(up[1] > 0.0f, "the up of the view points upwards");
	}

	// Checks the orbit: the direction that the mouse turns the scene, the distance and the place of the pivot that it keeps, and the
	// limits of the pitch
	void test_orbit()
	{
		// a movement of 360 pixels to the right turns the scene 90 degrees to the right: the camera goes to the left of the origin
		Orbit_Camera camera = make_front_camera();
		camera.orbit({ 0.0f, 0.0f, 0.0f }, 360.0f, 0.0f);
		check(is_near(camera.yaw, 0.0), "360 pixels to the right add 90 degrees to the yaw");
		check(is_near(camera.position, { -40.0f, 0.0f, 0.0f }, 1e-3), "the camera goes to the left of the scene");
		check(is_near(camera.get_front(), { 1.0f, 0.0f, 0.0f }), "and still looks at the pivot");

		// a movement down turns the top of the scene towards the camera: the camera rises and looks down at the pivot
		camera = make_front_camera();
		camera.orbit({ 0.0f, 0.0f, 0.0f }, 0.0f, 100.0f);
		check(is_near(camera.pitch, -25.0), "100 pixels down take 25 degrees from the pitch");
		check(camera.position[1] > 0.0f, "the camera rises");
		check(is_near(to_view(camera, { 0.0f, 0.0f, 0.0f }), { 0.0f, 0.0f, 40.0f }, 1e-3), "the pivot stays at the centre of the view");

		// a pivot that is not at the centre of the view stays where it is in the view, and at the same distance
		camera.position = { 0.0f, 12.0f, 40.0f };
		camera.yaw      = -90.0f;
		camera.pitch    = -15.0f;
		const std::array<float, 3> pivot{ 5.0f, 3.0f, -2.0f };
		const std::array<float, 3> before = to_view(camera, pivot);
		const double               radius = distance(camera.position, pivot);
		camera.orbit(pivot, 123.0f, -45.0f);
		check(is_near(to_view(camera, pivot), before, 1e-3), "the pivot stays where it was in the view");
		check(is_near(distance(camera.position, pivot), radius, 1e-3), "the distance to the pivot does not change");
		check(is_near(camera.yaw, -90.0 + 123.0 * 0.25) && is_near(camera.pitch, -15.0 + 45.0 * 0.25), "the angles follow the mouse");

		// the pitch stops at its limits, and the distance still does not change
		camera = make_front_camera();
		camera.orbit({ 0.0f, 0.0f, 0.0f }, 0.0f, -1000.0f);
		check(is_near(camera.pitch, Orbit_Camera::MAX_PITCH), "the pitch stops at 89 degrees");
		check(is_near(distance(camera.position, { 0.0f, 0.0f, 0.0f }), 40.0, 1e-3), "at the same distance");
		check(is_near(to_view(camera, { 0.0f, 0.0f, 0.0f }), { 0.0f, 0.0f, 40.0f }, 1e-3), "and still looking at the pivot");
		camera.orbit({ 0.0f, 0.0f, 0.0f }, 0.0f, 2000.0f);
		check(is_near(camera.pitch, Orbit_Camera::MIN_PITCH), "the pitch stops at -89 degrees");

		// the yaw stays between -180 and 180 degrees
		camera = make_front_camera();
		camera.orbit({ 0.0f, 0.0f, 0.0f }, -480.0f, 0.0f);
		check(is_near(camera.yaw, 150.0), "the yaw is kept between -180 and 180 degrees");
		check(is_near(to_view(camera, { 0.0f, 0.0f, 0.0f }), { 0.0f, 0.0f, 40.0f }, 1e-3), "and the camera still looks at the pivot");
	}

	// Checks the pan: a point at the depth of the pivot follows the mouse, and the direction of the view does not change
	void test_pan()
	{
		Orbit_Camera               camera = make_front_camera();
		const std::array<float, 3> pivot{ 0.0f, 0.0f, 0.0f };
		const double               centimetres_per_pixel = 2.0 * 40.0 * std::tan(22.5 * 3.14159265358979 / 180.0) / 800.0;

		camera.pan(pivot, 100.0f, 0.0f, 45.0f, 800.0f);
		check(is_near(camera.position[0], -100.0 * centimetres_per_pixel), "the camera goes left when the mouse goes right");
		check(is_near(to_view(camera, pivot)[0], 100.0 * centimetres_per_pixel), "so the pivot goes to the right of the view");
		check(is_near(camera.get_depth_of(pivot), 40.0), "the depth of the pivot does not change");

		camera.pan(pivot, 0.0f, 100.0f, 45.0f, 800.0f);
		check(is_near(camera.position[1], 100.0 * centimetres_per_pixel), "the camera goes up when the mouse goes down");
		check(is_near(camera.get_front(), { 0.0f, 0.0f, -1.0f }), "the direction of the view does not change");

		// a pivot behind the camera pans as if it were MIN_DISTANCE in front
		camera = make_front_camera();
		camera.pan({ 0.0f, 0.0f, 50.0f }, 100.0f, 0.0f, 45.0f, 800.0f);
		check(
			is_near(camera.position[0], -100.0 * centimetres_per_pixel * Orbit_Camera::MIN_DISTANCE / 40.0),
			"a pivot behind pans slowly");

		// a view without a height does not move the camera
		camera = make_front_camera();
		camera.pan(pivot, 100.0f, 100.0f, 45.0f, 0.0f);
		check(is_near(camera.position, { 0.0f, 0.0f, 40.0f }), "a view without a height does not pan");
	}

	// Checks the dolly: the factor of each notch, the way back, and the limits of the distance
	void test_dolly()
	{
		Orbit_Camera               camera = make_front_camera();
		const std::array<float, 3> pivot{ 0.0f, 0.0f, 0.0f };

		camera.dolly(pivot, 1.0f);
		check(is_near(camera.get_depth_of(pivot), 40.0 * 0.85, 1e-3), "a notch forward multiplies the distance by 0.85");
		camera.dolly(pivot, -1.0f);
		check(is_near(camera.get_depth_of(pivot), 40.0, 1e-3), "a notch back divides it by 0.85");
		check(is_near(camera.position[0], 0.0) && is_near(camera.position[1], 0.0), "the camera moves along the view");

		camera.dolly(pivot, 100.0f);
		check(is_near(camera.get_depth_of(pivot), Orbit_Camera::MIN_DISTANCE, 1e-3), "the camera stops 2 cm in front of the pivot");
		camera.dolly(pivot, 1.0f);
		check(is_near(camera.get_depth_of(pivot), Orbit_Camera::MIN_DISTANCE, 1e-3), "and goes no closer");
		camera.dolly(pivot, -100.0f);
		check(is_near(camera.get_depth_of(pivot), Orbit_Camera::MAX_DISTANCE, 1e-3), "the camera stops 90 cm from the pivot");
		camera.dolly(pivot, -1.0f);
		check(is_near(camera.get_depth_of(pivot), Orbit_Camera::MAX_DISTANCE, 1e-3), "and goes no farther");

		// a pivot behind the camera: the camera does not go forward, and goes back by the step of MIN_DISTANCE
		camera = make_front_camera();
		camera.dolly({ 0.0f, 0.0f, 50.0f }, 1.0f);
		check(is_near(camera.position, { 0.0f, 0.0f, 40.0f }), "the camera does not go forward when the pivot is behind it");
		camera.dolly({ 0.0f, 0.0f, 50.0f }, -1.0f);
		check(camera.position[2] > 40.0f, "and goes back");
	}
} // namespace

int main()
{
	test_vectors();
	test_orbit();
	test_pan();
	test_dolly();

	if (failures > 0)
	{
		std::cerr << "[ERROR::ORBIT_CAMERA_TEST::main] " << failures << " check(s) failed" << std::endl;
		return 1;
	}

	std::cout << "[SUCCESS::ORBIT_CAMERA_TEST::main] All the checks of the orbit camera passed" << std::endl;
	return 0;
}
