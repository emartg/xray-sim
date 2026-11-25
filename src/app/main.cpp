/*
 * main.cpp
 * This file is the entry point of the simulator. It follows these steps:
 * - It reads the command line (--frames N renders N frames and exits, e.g., for automated smoke tests,
 *   --view X Y Z TX TY TZ places the camera of the 3D view at the start, at X Y Z looking at TX TY TZ, in centimetres,
 *   --xray-self-test computes the X-ray image of a test object in the first frame and checks it, and
 *   --xray-view-test checks the X-ray image of the scene that is computed in the first frame).
 * - It creates a GLFW_Renderer with the simulator's GUI, and initializes the Core engine.
 * - It creates the default scene of the simulator and fills the Engine's scene from it (camera, light, and shapes).
 * - It runs the main loop of the engine and shuts it down.
 */

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

#include "core/Core.h"

#include "platform/renderer/GLFW_Renderer.h"

#include "Scene_Builder.h"
#include "Simulator_Gui.h"
#include "scene/Orbit_Camera.h"
#include "scene/Scene.h"

// Reads the six values of --view, the position of the camera and the point that it looks at, from the arguments that follow the option.
// Returns the view, or nothing (after printing the reason) if a value is missing or is not a number, or if the view is not valid
std::optional<Orbit_Camera> parse_view(int argc, char** argv, int option_index)
{
	std::array<float, 6> values{};
	if (option_index + static_cast<int>(values.size()) >= argc)
	{ // if there are fewer than six arguments after the option, print an error message and return nothing
		std::cerr << "[ERROR::MAIN::parse_view] --view needs six values: the position X Y Z and the target TX TY TZ, in centimetres"
				  << std::endl;
		return std::nullopt;
	}

	for (std::size_t k{ 0 }; k < values.size(); k++)
	{ // convert each value to a number (the whole value must be a number)
		const std::string_view value{ argv[option_index + 1 + static_cast<int>(k)] };
		auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), values[k]);
		if (error != std::errc{} || end != value.data() + value.size())
		{ // if the value is not a number, print an error message and return nothing
			std::cerr << "[ERROR::MAIN::parse_view] Invalid value of --view: '" << value << "' (expected a number)" << std::endl;
			return std::nullopt;
		}
	}

	std::optional<Orbit_Camera> view = Orbit_Camera::look_at({ values[0], values[1], values[2] }, { values[3], values[4], values[5] });
	if (!view)
	{ // if the camera cannot take the view, print an error message and return nothing
		std::cerr << "[ERROR::MAIN::parse_view] Invalid view: each coordinate must be within " << Orbit_Camera::MAX_COORDINATE
				  << " cm, the position must differ from the target, and the pitch must be between " << Orbit_Camera::MIN_PITCH << " and "
				  << Orbit_Camera::MAX_PITCH << " degrees" << std::endl;
	}
	return view;
}

// Reads the command line. Returns false (after printing the reason) if an argument is not valid.
// The number of frames is left at 0 (no limit, the simulator runs until the window is closed) unless --frames is given
bool parse_command_line(
	int                          argc,
	char**                       argv,
	std::uint64_t&               max_frames,
	bool&                        xray_self_test,
	bool&                        xray_view_test,
	std::optional<Orbit_Camera>& start_view)
{
	start_view.reset();
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

		if (argument == "--view")
		{ // the camera of the 3D view at the start: the six values that follow the option
			start_view = parse_view(argc, argv, i);
			if (!start_view)
				return false;
			i += 6;
			continue;
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
	std::uint64_t               max_frames{ 0 };
	bool                        xray_self_test{ false };
	bool                        xray_view_test{ false };
	std::optional<Orbit_Camera> start_view;
	if (!parse_command_line(argc, argv, max_frames, xray_self_test, xray_view_test, start_view))
	{ // if the command line is not valid, exit with an error code
		return -1;
	}

	Core* engine = Core::get_instance(); // retrieve the singleton instance of the Core class

	// the scene that the simulator shows, drawn by the Engine and imaged by the X-ray library
	const auto scene = std::make_shared<Scene>(Scene::make_default_scene());

	// create a GLFW_Renderer instance with the simulator's GUI, and set it as the renderer for the engine (the renderer owns the GUI)
	GLFW_Renderer* renderer = new GLFW_Renderer();
	auto           gui      = std::make_unique<Simulator_Gui>(scene, xray_self_test, xray_view_test);
	Simulator_Gui* gui_view = gui.get();
	renderer->set_gui_layer(std::move(gui));
	engine->set_renderer(renderer);

	if (!engine->init())
	{ // if the initialization fails, shut down the engine and exit with an error code
		engine->shutdown();
		return -1;
	}

	// fill the Engine's scene with the camera, the light, and the shapes of the scene
	// and tell the GUI which node is which object
	gui_view->set_node_ids(Scene_Builder::build(*scene, engine, start_view));

	engine->run(max_frames); // run the main loop of the engine (0 means until the window is closed)
	engine->shutdown();      // clean up resources in the correct order and shut down the engine

	return 0;
}
