/*
 * Gvxr_AdapterTest.cpp
 * This file is the test of the Gvxr_Adapter class. It checks that the simulator can use the gVirtualXRay library:
 * that the version is reported and that the library works without an OpenGL context. It returns an error code
 * if a check fails.
 */

#include <iostream>
#include <string>

#include "xray/Gvxr_Adapter.h"

int main()
{
	const std::string version = Gvxr_Adapter::get_version();
	if (version.empty())
	{
		std::cerr << "[ERROR::GVXR_ADAPTER_TEST::main] The version of gVirtualXRay is empty" << std::endl;
		return 1;
	}

	const std::string error = Gvxr_Adapter::run_self_check();
	if (!error.empty())
	{
		std::cerr << "[ERROR::GVXR_ADAPTER_TEST::main] " << error << std::endl;
		return 1;
	}

	std::cout << "[SUCCESS::GVXR_ADAPTER_TEST::main] gVirtualXRay " << version << " works" << std::endl;
	return 0;
}
