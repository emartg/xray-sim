/*
 * Gvxr_Adapter.h
 * This file defines the Gvxr_Adapter class, which is the entry point of the simulator to the gVirtualXRay
 * library. It owns the X-ray renderer, the beam, the detector, and the meshes, and computes the X-ray image of the
 * meshes on the OpenGL context that is current when it is used (the context of the Engine's window).
 * The header includes no header of gVirtualXRay or of the Engine, so that the simulator can use it without the C++
 * standard, the preprocessor definitions, or the OpenGL loader of the library. The methods report failures with a
 * message instead of throwing exceptions.
 * This is a prototype: the interface will change.
 */

#pragma once

#include <memory>
#include <string>

#include "xray/Xray_Types.h"

class Gvxr_Adapter
{
public:
	// Constructors
	// ------------
	Gvxr_Adapter();
	Gvxr_Adapter(const Gvxr_Adapter&) = delete; // copy constructor (the adapter owns the OpenGL objects of the renderer)

	// Operator overloads
	// ------------------
	Gvxr_Adapter& operator=(const Gvxr_Adapter&) = delete; // copy assignment operator

	// Destructor
	// ----------
	// Releases the renderer (the OpenGL context has to be current, or the objects are leaked by the driver)
	~Gvxr_Adapter();

	// Public Static Methods
	// ---------------------
	// Returns the version of the gVirtualXRay library as "major.minor.patch"
	static std::string get_version();

	// Checks that the gVirtualXRay library works without an OpenGL context (it creates a mixture of chemical
	// elements and verifies its label and its molar mass). Returns an empty string if the check succeeds, or
	// the reason of the failure
	static std::string run_self_check();

	// Public Methods
	// --------------
	// Loads the OpenGL functions of the library on the current context (the Engine has loaded its own already),
	// and initialises the renderer with the beam and the detector. Returns an empty string if it succeeds, or the
	// reason of the failure
	std::string initialise(const Xray_Beam& beam, const Xray_Detector& detector);

	// Adds a mesh to the X-ray scene (its data is copied, and uploaded to the GPU once). Returns an empty string if
	// it succeeds, or the reason of the failure
	std::string add_mesh(const Xray_Mesh& mesh);

	// Computes the X-ray image of the meshes. The state of OpenGL (framebuffers, viewport, program, vertex array,
	// buffers, textures, and the depth, blend, cull, and scissor settings) is the same before and after the call.
	// Returns an empty string if it succeeds, or the reason of the failure
	std::string compute_image(Xray_Image& image);

	// Getters
	bool get_is_initialised() const;

	// The times that the calls of the adapter took (see Xray_Timings)
	const Xray_Timings& get_timings() const;

private:
	// Private Attributes
	// ------------------
	struct Implementation; // the objects of the library, hidden from the header
	std::unique_ptr<Implementation> implementation;
};
