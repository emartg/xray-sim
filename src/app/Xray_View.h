/*
 * Xray_View.h
 * This file defines the Xray_View class, which shows the scene as an X-ray image. It owns the adapter of gVirtualXRay, gives it the
 * meshes of the scene, computes the image when it is requested (the first time it is drawn, when the user presses the button, and
 * when the setup of the source and the detector changes), converts it to grey values (see Xray_Display), and draws it with ImGui as a
 * window next to the 3D view or as the background of the whole window (the single X-ray view). A second window has the controls of
 * the setup (see Xray_Setup).
 * Everything here needs the OpenGL context of the window to be current: the Simulator_Gui calls draw() from the frame of the
 * Engine. This file is part of the X-ray part of the simulator (it is not built without it).
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "display/Xray_Display.h"
#include "scene/Scene.h"
#include "scene/Xray_Setup.h"
#include "xray/Xray_Types.h"

class Gvxr_Adapter;

class Xray_View
{
public:
	// Constructors
	// ------------
	// The scene is the one that the image shows. If should_check is true, the first image is checked, and then the setup is changed
	// to check that the changes take effect (see check_image); the results are printed with the format of the Engine
	// ("[SUCCESS::..." or "[ERROR::..."). The times of each image are printed to the console only if the user turns the option on
	// in the setup window, or if the first image is checked (should_check)
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
	// Draws the windows of the X-ray view and, in the single view, the image as the background. Called every frame by the
	// Simulator_Gui between the start of the ImGui frame and its rendering. The first call starts the adapter and computes the image
	void draw();

private:
	// Private Methods
	// ---------------
	// Puts the setup within its limits, creates the adapter again with the beam and the detector of the setup and the meshes of the
	// scene, and computes the image. The adapter is created again because the library takes its beam and its detector when it is
	// initialised. Sets the error on failure (and clears it first, so that the user can try again). Measures the time of each stage,
	// prints it, and counts the images (see get_images_in_last_seconds)
	void apply_setup();

	// Computes the X-ray image of the scene with the adapter, and measures the time it takes. Sets the error on failure
	void compute();

	// Converts the image to grey values with the display settings and uploads them to the texture (it is created the first time,
	// and again if the image has another size)
	void update_texture();

	// Draws the controls of the setup and the times of the last image. The changes are applied when they are made, or with the button
	// if the option is off
	void draw_setup_window();

	// Returns how many images apply_setup computed in the last few seconds, and keeps the highest number
	unsigned int get_images_in_last_seconds();

	// Draws the controls of the display, the status, and the image (if it is not the single view) in the "X-ray" window
	void draw_window();

	// Draws the image as the background of the whole window, with its aspect ratio, on a black background
	void draw_background() const;

	// Returns an empty string if the current image is valid and physically plausible, or the reason it is not: the corners (no
	// object) are nearly unattenuated, and each object of the scene attenuates the beam at the projection of its centre. The
	// transmissions behind the water sphere and the iron sphere (-1 if the scene has none) are returned, and the transmissions behind
	// each object are printed
	std::string check_current_image(double& water_transmission, double& iron_transmission) const;

	// Checks the first image of the scene and that the iron sphere attenuates more than the water sphere; then changes the setup (a
	// lower energy must attenuate more, and another geometry and detector must still give a valid image of the right size) and checks
	// the images, and restores the setup. Prints the result with the format of the Engine
	void check_image();

	// Returns the mean of the image in the block of 3 by 3 pixels at the projection of a point of the scene (in centimetres) on the
	// detector, relative to the unattenuated beam. Returns -1 if the projection is outside of the detector
	double get_transmission_at(float x, float y) const;

	// Private Attributes
	// ------------------
	std::shared_ptr<const Scene>  scene;
	std::unique_ptr<Gvxr_Adapter> adapter;
	std::string                   error; // the reason the adapter or the image failed; empty if there is none
	bool                          is_check_requested{ false };
	bool is_console_output_enabled{ false }; // true: every image prints its times to the console (the user's option, and the check's)

	Xray_Setup setup;                     // the values that the user chooses
	Xray_Setup image_setup;               // the values of the current image
	bool       is_auto_recompute{ true }; // true: the image is computed again whenever a value of the setup changes
	bool       is_setup_dirty{ false };   // true: a value of the setup changed and is not applied yet

	// Times of the last image, in milliseconds (the ones of the adapter are in timings)
	Xray_Timings timings;
	double       apply_ms{ 0.0 };          // the whole apply_setup
	double       adapter_reset_ms{ 0.0 };  // the destruction of the old adapter
	double       adapter_create_ms{ 0.0 }; // the construction of the new adapter (the objects of the library, before initialise)
	double       scene_ms{ 0.0 };          // the meshes of the scene in the units of the library
	double       mirror_ms{ 0.0 };         // the mirror of the image and the search of the reference value
	double       grey_ms{ 0.0 };           // the conversion to grey values
	double       upload_ms{ 0.0 };         // the creation of the texture, if it is new, and the copy of the grey values to it

	std::vector<std::chrono::steady_clock::time_point> image_times; // when the images of the last few seconds were computed
	unsigned int                                       most_images_in_last_seconds{ 0 };

	Xray_Image            image;
	float                 reference{ 0.0f }; // value of the unattenuated beam in the image
	double                compute_time_ms{ 0.0 };
	bool                  is_image_valid{ false };
	Xray_Display_Settings display_settings;
	bool                  is_single_view{ false };
	bool                  was_single_view{ false }; // the view of the previous frame, to resize the window when it changes

	unsigned int texture{ 0 }; // OpenGL name of the texture of the grey values (0 if it has not been created)
	unsigned int texture_width{ 0 }, texture_height{ 0 };
	bool         is_texture_dirty{ false };
};
