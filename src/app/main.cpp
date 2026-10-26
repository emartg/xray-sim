/*
 * main.cpp
 * This file is the entry point of the simulator. It follows these steps:
 * - It reads the command line (--frames N renders N frames and exits, e.g., for automated smoke tests,
 *   --xray-self-test computes the X-ray image of a test object in the first frame and checks it, and
 *   --xray-view-test checks the X-ray image of the scene that is computed in the first frame).
 * - It creates a GLFW_Renderer with the simulator's GUI, and initializes the Core engine.
 * - It creates the default scene of the simulator and fills the Engine's scene from it (camera, light, and shapes).
 * - It runs the main loop of the engine and shuts it down.
 */

#include <charconv>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

#include "core/Core.h"

#include "platform/renderer/GLFW_Renderer.h"

#include "Scene_Builder.h"
#include "Simulator_Gui.h"
#include "scene/Scene.h"

// Reads the command line. Returns false (after printing the reason) if an argument is not valid.
// The number of frames is left at 0 (no limit, the simulator runs until the window is closed) unless --frames is given
bool parse_command_line(int argc, char** argv, std::uint64_t& max_frames, bool& xray_self_test, bool& xray_view_test)
{
	max_frames     = 0;
	xray_self_test = false;
	xray_view_test = false;

	for (int i{ 1 }; i < argc; i++)
	{ // iterate through the arguments (the first one is the program name)
		const std::string_view argument{ argv[i] };
		if (argument == "--xray-self-test" || argument == "--xray-view-test")
		{ // the tests of the X-ray library and of the X-ray view, which need the X-ray part of the simulator
#ifdef XRSIM_HAS_XRAY
			(argument == "--xray-self-test" ? xray_self_test : xray_view_test) = true;
			continue;
#else
			std::cerr << "[ERROR::MAIN::parse_command_line] This build of the simulator has no X-ray part (" << argument << ")"
					  << std::endl;
			return false;
#endif
		}

		if (argument != "--frames")
		{ // if the argument is unknown, print an error message and return false
			std::cerr << "[ERROR::MAIN::parse_command_line] Unknown argument: " << argument << std::endl;
			return false;
		}

		if (i + 1 >= argc)
		{ // if there is no next argument, print an error message and return false
			std::cerr << "[ERROR::MAIN::parse_command_line] Missing value for --frames" << std::endl;
			return false;
		}

		// convert the value to a positive integer (the whole value must be a number)
		const std::string_view value{ argv[++i] };
		auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), max_frames);
		if (error != std::errc{} || end != value.data() + value.size() || max_frames == 0)
		{ // if the value is not a positive integer, print an error message and return false
			std::cerr << "[ERROR::MAIN::parse_command_line] Invalid number of frames: '" << value << "' (expected a positive integer)"
					  << std::endl;
			return false;
		}
	}

	return true;
}

int main(int argc, char** argv)
{
	std::uint64_t max_frames{ 0 };
	bool          xray_self_test{ false };
	bool          xray_view_test{ false };
	if (!parse_command_line(argc, argv, max_frames, xray_self_test, xray_view_test))
	{ // if the command line is not valid, exit with an error code
		return -1;
	}

	Core* engine = Core::get_instance(); // retrieve the singleton instance of the Core class

	// the scene that the simulator shows, drawn by the Engine and imaged by the X-ray library
	const auto scene = std::make_shared<const Scene>(Scene::make_default_scene());

	// create a GLFW_Renderer instance with the simulator's GUI, and set it as the renderer for the engine
	GLFW_Renderer* renderer = new GLFW_Renderer();
	renderer->set_gui_layer(std::make_unique<Simulator_Gui>(scene, xray_self_test, xray_view_test));
	engine->set_renderer(renderer);

	if (!engine->init())
	{ // if the initialization fails, shut down the engine and exit with an error code
		engine->shutdown();
		return -1;
	}

	// fill the Engine's scene with the camera, the light, and the shapes of the scene
	Scene_Builder::build(*scene, engine);

	engine->run(max_frames); // run the main loop of the engine (0 means until the window is closed)
	engine->shutdown();      // clean up resources in the correct order and shut down the engine

	return 0;
}
