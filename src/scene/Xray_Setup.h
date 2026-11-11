/*
 * Xray_Setup.h
 * This file defines the Xray_Setup struct, which describes how the scene is imaged: the energy of the beam, where the point
 * source (the X-ray tube) is and where it points, and the flat detector. The tube is free: its position is any point of the scene,
 * and its direction is given by two angles (yaw and pitch). The detector follows the beam: its centre is on the axis of the beam, at
 * the source-to-detector distance, it faces the source, and a third angle (roll) turns it about the beam. With the default values the
 * source is on the axis z, 100 cm before the plane z = 0 where the objects of the default scene are, and the detector is 50 cm behind
 * it, as in the first versions of the simulator.
 * The lengths are in centimetres, like the rest of the scene, and the angles in degrees; the setup converts them to the millimetres
 * of the X-ray library. The limits only keep the geometry valid: they carry no clinical meaning.
 * It has no dependency on OpenGL, on the Engine, or on the X-ray library (it only uses the plain types of the adapter).
 */

#pragma once

#include <array>

#include "xray/Xray_Types.h"

struct Xray_Setup
{
	// Public Constants
	// ----------------
	// The limits of each value: the energies of the tables of the library, a tube anywhere within 3 m of the centre of the scene, a
	// pitch that stops short of the vertical (where the yaw would lose its meaning), a detector at a reasonable distance of the
	// source, and a detector with a reasonable size
	static constexpr float        MIN_ENERGY{ 20.0f }, MAX_ENERGY{ 150.0f };                     // in keV
	static constexpr float        MIN_SOURCE_POSITION{ -300.0f }, MAX_SOURCE_POSITION{ 300.0f }; // on each axis, in centimetres
	static constexpr float        MIN_YAW{ -180.0f }, MAX_YAW{ 180.0f };                         // in degrees
	static constexpr float        MIN_PITCH{ -89.0f }, MAX_PITCH{ 89.0f };                       // in degrees
	static constexpr float        MIN_ROLL{ -180.0f }, MAX_ROLL{ 180.0f };                       // in degrees
	static constexpr float        MIN_SOURCE_DETECTOR_DISTANCE{ 60.0f }, MAX_SOURCE_DETECTOR_DISTANCE{ 500.0f }; // in centimetres
	static constexpr float        MIN_PIXEL_SIZE{ 0.5f }, MAX_PIXEL_SIZE{ 5.0f };                                // in millimetres
	static constexpr unsigned int MIN_PIXELS{ 32 }, MAX_PIXELS{ 512 };                                           // per side of the detector

	// Public Methods
	// --------------
	// Puts every value within its limits (a value that is not a number goes to the lower limit)
	void clamp();

	// Turns the tube towards a point of the scene (in centimetres): sets the yaw and the pitch, within their limits. Does nothing if the
	// point is the position of the source
	void aim_at(const std::array<float, 3>& point);

	// Returns the direction of the beam, a unit vector: (sin(yaw) cos(pitch), sin(pitch), cos(yaw) cos(pitch)), which is +z for the
	// default angles
	std::array<float, 3> get_direction() const;

	// Returns the centre of the detector, in centimetres: the source plus the direction times the source-to-detector distance
	std::array<float, 3> get_detector_position() const;

	// Returns the up vector of the detector, a unit vector perpendicular to the beam: the axis y of the scene made perpendicular to the
	// direction, and turned about the direction by the roll
	std::array<float, 3> get_detector_up() const;

	// Projects a point of the scene (in centimetres) on the detector from the source. Returns false if the point is not in front of
	// the source. The coordinates are in millimetres from the centre of the detector: x_on_detector grows to the right of the image
	// as the simulator shows it (seen from the detector towards the source, which is +x of the scene for the default angles), and
	// y_on_detector grows along the up vector
	bool project(const std::array<float, 3>& point, double& x_on_detector, double& y_on_detector) const;

	// Returns the magnification of a point of the scene on the detector: the source-to-detector distance over the distance from the
	// source to the point along the beam. Returns 0 if the point is not in front of the source
	float get_magnification_at(const std::array<float, 3>& point) const;

	// Returns the magnification of the centre of the scene (the origin): 1.5 for the default values
	float get_magnification() const;

	// Returns the beam and the detector for the X-ray library, in millimetres and keV (the values are used as they are: call
	// clamp() before if they come from the user)
	Xray_Beam     make_beam() const;
	Xray_Detector make_detector() const;

	bool operator==(const Xray_Setup&) const = default;

	// Public Attributes
	// -----------------
	float                energy{ 80.0f };                        // in keV (monochromatic beam)
	std::array<float, 3> source_position{ 0.0f, 0.0f, -100.0f }; // in centimetres
	float                yaw{ 0.0f };                            // the direction of the beam turned about the axis y, in degrees
	float                pitch{ 0.0f };                          // the direction of the beam raised towards +y, in degrees
	float                roll{ 0.0f };                           // the detector turned about the beam, in degrees
	float                source_detector_distance{ 150.0f };     // from the source to the centre of the detector, in centimetres
	float                pixel_size{ 2.0f };                     // side of a pixel of the detector, in millimetres
	unsigned int         width_in_pixels{ 256 };
	unsigned int         height_in_pixels{ 128 };
};
