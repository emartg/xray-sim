/*
 * Gvxr_Adapter.cpp
 * This file implements the Gvxr_Adapter class, which is the entry point of the simulator to the gVirtualXRay library.
 * It is the only place that includes the headers of the library.
 */

#include "xray/Gvxr_Adapter.h"

#include <map>

#include "gVirtualXRay/Mixture.h"
#include "gVirtualXRay/gVirtualXRayConfig.h"

// Public Static Methods
// ---------------------
std::string Gvxr_Adapter::get_version()
{
	return std::string{ gVirtualXRay_VERSION_MAJOR } + "." + gVirtualXRay_VERSION_MINOR + "." + gVirtualXRay_VERSION_PATCH;
}

std::string Gvxr_Adapter::run_self_check()
{
	// hydrogen, helium, lithium, and beryllium, with the weights used by the tests of the library
	const std::map<int, double> weights{ { 1, 1.0 }, { 2, 2.0 }, { 3, 1.0 }, { 4, 2.0 } };
	const gVirtualXRay::Mixture mixture{ weights };

	if (mixture.getLabel() != "H17He33Li17Be33")
	{
		return "Unexpected label of the mixture: " + mixture.getLabel();
	}

	if (!(mixture.getMolarMass() > 0.0))
	{
		return "The molar mass of the mixture is not positive";
	}

	return "";
}
