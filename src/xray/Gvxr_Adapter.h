/*
 * Gvxr_Adapter.h
 * This file defines the Gvxr_Adapter class, which is the entry point of the simulator to the gVirtualXRay
 * library. It is the first piece of the adapter: for now it only reports the version of the library and checks
 * that the library works. The header includes no header of gVirtualXRay or of the Engine, so that the
 * simulator can use it without the C++ standard, the preprocessor definitions, or the OpenGL loader of the library.
 */

#pragma once

#include <string>

class Gvxr_Adapter
{
public:
	// Public Static Methods
	// ---------------------
	// Returns the version of the gVirtualXRay library as "major.minor.patch"
	static std::string get_version();

	// Checks that the gVirtualXRay library works without an OpenGL context (it creates a mixture of chemical
	// elements and verifies its label and its molar mass). Returns an empty string if the check succeeds, or
	// the reason of the failure
	static std::string run_self_check();
};
