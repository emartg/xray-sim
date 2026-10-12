/*
 * main.cpp
 * This file is the entry point of the simulator. It follows these steps:
 * - It reads the command line (--frames N renders N frames and exits, e.g., for automated smoke tests).
 * - It creates a GLFW_Renderer with the simulator's GUI, and initializes the Core engine.
 * - It sets up a minimal scene (a camera).
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
#include "core/camera/Camera.h"
#include "core/managers/Node_Manager.h"
#include "core/managers/Scene_Manager.h"

#include "platform/renderer/GLFW_Renderer.h"

#include "Simulator_Gui.h"

// Reads the command line. Returns false (after printing the reason) if an argument is not valid.
// The number of frames is left at 0 (no limit, the simulator runs until the window is closed) unless --frames is given
bool parse_command_line(int argc, char** argv, std::uint64_t& max_frames)
{
	max_frames = 0;

	for (int i{ 1 }; i < argc; i++)
	{ // iterate through the arguments (the first one is the program name)
		const std::string_view argument{ argv[i] };
		if (argument != "--frames")
		{ // if the argument is unknown, print an error message and return false
			std::cerr << "[ERROR::main::parse_command_line] Unknown argument: " << argument << std::endl;
			return false;
		}

		if (i + 1 >= argc)
		{ // if there is no next argument, print an error message and return false
			std::cerr << "[ERROR::main::parse_command_line] Missing value for --frames" << std::endl;
			return false;
		}

		// convert the value to a positive integer (the whole value must be a number)
		const std::string_view value{ argv[++i] };
		auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), max_frames);
		if (error != std::errc{} || end != value.data() + value.size() || max_frames == 0)
		{ // if the value is not a positive integer, print an error message and return false
			std::cerr << "[ERROR::main::parse_command_line] Invalid number of frames: '" << value << "' (expected a positive integer)"
					  << std::endl;
			return false;
		}
	}

	return true;
}

int main(int argc, char** argv)
{
	std::uint64_t max_frames{ 0 };
	if (!parse_command_line(argc, argv, max_frames))
	{ // if the command line is not valid, exit with an error code
		return -1;
	}

	Core* engine = Core::get_instance(); // retrieve the singleton instance of the Core class

	// create a GLFW_Renderer instance with the simulator's GUI, and set it as the renderer for the engine
	GLFW_Renderer* renderer = new GLFW_Renderer();
	renderer->set_gui_layer(std::make_unique<Simulator_Gui>());
	engine->set_renderer(renderer);

	if (!engine->init())
	{ // if the initialization fails, shut down the engine and exit with an error code
		engine->shutdown();
		return -1;
	}

	// create a camera, since the renderer requires one to render the scene
	auto camera = std::make_shared<Camera>("Camera");
	engine->get_scene_manager()->set_camera(camera);
	engine->get_node_manager()->add_node(std::move(camera));

	engine->run(max_frames); // run the main loop of the engine (0 means until the window is closed)
	engine->shutdown();      // clean up resources in the correct order and shut down the engine

	return 0;
}
