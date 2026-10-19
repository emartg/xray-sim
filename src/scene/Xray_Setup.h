/*
 * Xray_Setup.h
 * This file defines the Xray_Setup struct, which describes how the scene is imaged: the energy of the beam, where the point
 * source and the flat detector are, and the size of the detector. The source and the detector are on the axis z, before and
 * behind the plane z = 0 where the objects of the scene are, and look at each other (the up vector of the detector is y). The
 * distances are in centimetres, like the rest of the scene, and the setup converts them to the millimetres of the X-ray library.
 * It has no dependency on OpenGL, on the Engine, or on the X-ray library (it only uses the plain types of the adapter).
 */

#pragma once

#include "xray/Xray_Types.h"

struct Xray_Setup
{
	// Public Constants
	// ----------------
	// The limits of each value: the energies of the tables of the library, a source that is outside of the scene (its objects are
	// within 20 cm of the plane z = 0), and a detector with a reasonable size
	static constexpr float        MIN_ENERGY{ 20.0f }, MAX_ENERGY{ 150.0f };                       // in keV
	static constexpr float        MIN_SOURCE_DISTANCE{ 50.0f }, MAX_SOURCE_DISTANCE{ 300.0f };     // in centimetres
	static constexpr float        MIN_DETECTOR_DISTANCE{ 10.0f }, MAX_DETECTOR_DISTANCE{ 200.0f }; // in centimetres
	static constexpr float        MIN_PIXEL_SIZE{ 0.5f }, MAX_PIXEL_SIZE{ 5.0f };                  // in millimetres
	static constexpr unsigned int MIN_PIXELS{ 32 }, MAX_PIXELS{ 512 };                             // per side of the detector

	// Public Methods
	// --------------
	// Puts every value within its limits (a value that is not a number goes to the lower limit)
	void clamp();

	// Returns the magnification of the plane z = 0 on the detector: (source distance + detector distance) / source distance
	float get_magnification() const;

	// Returns the beam and the detector for the X-ray library, in millimetres and keV (the values are used as they are: call
	// clamp() before if they come from the user)
	Xray_Beam     make_beam() const;
	Xray_Detector make_detector() const;

	bool operator==(const Xray_Setup&) const = default;

	// Public Attributes
	// -----------------
	float        energy{ 80.0f };            // in keV (monochromatic beam)
	float        source_distance{ 100.0f };  // from the plane z = 0 to the source (at z < 0), in centimetres
	float        detector_distance{ 50.0f }; // from the plane z = 0 to the detector (at z > 0), in centimetres
	float        pixel_size{ 2.0f };         // side of a pixel of the detector, in millimetres
	unsigned int width_in_pixels{ 256 };
	unsigned int height_in_pixels{ 128 };
};
