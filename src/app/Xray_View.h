/*
 * Xray_View.h
 * This file defines the Xray_View class, which shows the scene as an X-ray image. It owns the adapter of gVirtualXRay, gives it the
 * meshes of the scene once, computes the image when it is requested (the first time it is drawn, and when the user presses the
 * button), converts it to grey values (see Xray_Display), and draws it with ImGui as a window next to the 3D view or as the
 * background of the whole window (the single X-ray view).
 * Everything here needs the OpenGL context of the window to be current: the Simulator_Gui calls draw() from the frame of the
 * Engine. This file is part of the X-ray part of the simulator (it is not built without it).
 */

#pragma once

#include <memory>
#include <string>

#include "display/Xray_Display.h"
#include "scene/Scene.h"
#include "xray/Xray_Types.h"

class Gvxr_Adapter;

class Xray_View
{
public:
	// Constructors
	// ------------
	// The scene is the one that the image shows. If should_check is true, the first image is checked (see check_image) and the
	// result is printed with the format of the Engine ("[SUCCESS::..." or "[ERROR::...")
	explicit Xray_View(std::shared_ptr<const Scene> scene, bool should_check = false);
	Xray_View(const Xray_View&) = delete; // copy constructor (the view owns an OpenGL texture and the adapter)

	// Operator overloads
	// ------------------
	Xray_View& operator=(const Xray_View&) = delete; // copy assignment operator

	// Destructor
	// ----------
	// Releases the adapter and the texture (the OpenGL context has to be current)
	~Xray_View();

	// Public Methods
	// --------------
	// Draws the X-ray window and, in the single view, the image as the background. Called every frame by the Simulator_Gui
	// between the start of the ImGui frame and its rendering. The first call starts the adapter and computes the image
	void draw();

	// Public Constants
	// ----------------
	// Geometry of the X-ray view, in millimetres: the point source is before the scene and the flat detector behind it, both on the
	// axis z, so that the scene (between z = -100 and z = 100) is magnified by 1.5. The detector covers 512 by 256 mm, and the shadow
	// of the default scene is 390 by 150 mm
	static constexpr double       SOURCE_Z{ -1000.0 };
	static constexpr double       DETECTOR_Z{ 500.0 };
	static constexpr unsigned int DETECTOR_WIDTH_IN_PIXELS{ 256 };
	static constexpr unsigned int DETECTOR_HEIGHT_IN_PIXELS{ 128 };
	static constexpr double       PIXEL_SIZE{ 2.0 };
	static constexpr double       BEAM_ENERGY{ 80.0 }; // in keV

private:
	// Private Methods
	// ---------------
	// Creates the adapter with the beam and the detector, and gives it the meshes of the scene. Sets the error on failure
	void initialise();

	// Computes the X-ray image of the scene, and measures the time it takes. Sets the error on failure
	void compute();

	// Converts the image to grey values with the display settings and uploads them to the texture (it is created the first time)
	void update_texture();

	// Draws the controls, the status, and the image (if it is not the single view) in the "X-ray" window
	void draw_window();

	// Draws the image as the background of the whole window, with its aspect ratio, on a black background
	void draw_background() const;

	// Checks that the image is valid and physically plausible: the corners (no object) are nearly unattenuated, each object of
	// the scene attenuates the beam at the projection of its centre, and the iron sphere attenuates more than the water sphere.
	// Prints the result with the format of the Engine
	void check_image() const;

	// Returns the mean of the image in the block of 3 by 3 pixels at the projection of a point of the scene (in centimetres) on the
	// detector, relative to the unattenuated beam. Returns -1 if the projection is outside of the detector
	double get_transmission_at(float x, float y) const;

	// Private Attributes
	// ------------------
	std::shared_ptr<const Scene>  scene;
	std::unique_ptr<Gvxr_Adapter> adapter;
	std::string                   error; // the reason the adapter or the image failed; empty if there is none
	bool                          is_check_requested{ false };

	Xray_Image            image;
	float                 reference{ 0.0f }; // value of the unattenuated beam in the image
	double                compute_time_ms{ 0.0 };
	bool                  is_image_valid{ false };
	Xray_Display_Settings display_settings;
	bool                  is_single_view{ false };
	bool                  was_single_view{ false }; // the view of the previous frame, to resize the window when it changes

	unsigned int texture{ 0 }; // OpenGL name of the texture of the grey values (0 if it has not been created)
	bool         is_texture_dirty{ false };
};
