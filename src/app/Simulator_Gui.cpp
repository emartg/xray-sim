/*
 * Simulator_Gui.cpp
 * This file implements the Simulator_Gui class, which is the graphical user interface of the simulator.
 */

#include "Simulator_Gui.h"

#include <imgui.h>

#include "scene/Scene_Material_Table.h"

#ifdef XRSIM_HAS_XRAY
	#include <algorithm>
	#include <cmath>
	#include <iostream>
	#include <string>
	#include <utility>
	#include <vector>

	#include <glad/glad.h>

	#include "Xray_View.h"
	#include "xray/Gvxr_Adapter.h"

namespace
{
	// Transmission of the beam through 10 cm of water at 80 keV: exp(-0.1837 cm2/g * 1 g/cm3 * 10 cm)
	const double EXPECTED_TRANSMISSION{ 0.1593 };

	// Captures the values of the OpenGL state that the X-ray library may change, with their names
	std::vector<std::pair<std::string, GLint>> capture_gl_state()
	{
		std::vector<std::pair<std::string, GLint>> state;
		const auto                                 capture = [&state](const std::string& name, GLenum parameter, int count = 1) {
			GLint values[4]{};
			glGetIntegerv(parameter, values);
			for (int i = 0; i < count; ++i) state.emplace_back(name + (count > 1 ? "[" + std::to_string(i) + "]" : ""), values[i]);
		};

		capture("draw framebuffer", GL_DRAW_FRAMEBUFFER_BINDING);
		capture("read framebuffer", GL_READ_FRAMEBUFFER_BINDING);
		capture("viewport", GL_VIEWPORT, 4);
		capture("scissor box", GL_SCISSOR_BOX, 4);
		capture("program", GL_CURRENT_PROGRAM);
		capture("vertex array", GL_VERTEX_ARRAY_BINDING);
		capture("array buffer", GL_ARRAY_BUFFER_BINDING);
		capture("active texture", GL_ACTIVE_TEXTURE);
		capture("depth function", GL_DEPTH_FUNC);
		capture("cull face mode", GL_CULL_FACE_MODE);
		capture("front face", GL_FRONT_FACE);
		capture("blend source (RGB)", GL_BLEND_SRC_RGB);
		capture("blend destination (RGB)", GL_BLEND_DST_RGB);
		capture("polygon mode", GL_POLYGON_MODE, 2);
		for (const GLenum capability : { GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST, GL_STENCIL_TEST })
			state.emplace_back("capability " + std::to_string(capability), glIsEnabled(capability));
		GLboolean depth_mask{ GL_TRUE };
		glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_mask);
		state.emplace_back("depth mask", depth_mask);

		// the 2D texture bound to each of the first texture units
		GLint active_texture{ 0 };
		glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
		for (GLuint unit = 0; unit < 8; ++unit)
		{
			GLint texture{ 0 };
			glActiveTexture(GL_TEXTURE0 + unit);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
			state.emplace_back("texture 2D of unit " + std::to_string(unit), texture);
		}
		glActiveTexture(static_cast<GLenum>(active_texture));

		return state;
	}

	// Cube of the given side centred at the origin, with the triangles in counter-clockwise order seen from outside
	Xray_Mesh make_cube(const std::string& name, float side, const Xray_Material& material)
	{
		const float h{ side / 2.0f };

		Xray_Mesh mesh;
		mesh.name      = name;
		mesh.material  = material;
		mesh.positions = { -h, -h, -h, h, -h, -h, h, h, -h, -h, h, -h, -h, -h, h, h, -h, h, h, h, h, -h, h, h };
		mesh.indices   = { 0, 2, 1, 0, 3, 2,   // back (-z)
						   4, 5, 6, 4, 6, 7,   // front (+z)
						   0, 4, 7, 0, 7, 3,   // left (-x)
						   1, 2, 6, 1, 6, 5,   // right (+x)
						   0, 1, 5, 0, 5, 4,   // bottom (-y)
						   3, 7, 6, 3, 6, 2 }; // top (+y)
		return mesh;
	}

	// Mean of the pixels of the square of the given side whose lower left corner is (x, y)
	double mean_of_block(const Xray_Image& image, unsigned int x, unsigned int y, unsigned int side)
	{
		double sum{ 0.0 };
		for (unsigned int row = y; row < y + side; ++row)
			for (unsigned int column = x; column < x + side; ++column)
				sum += image.pixels[static_cast<std::size_t>(row) * image.width + column];
		return sum / (static_cast<double>(side) * side);
	}
} // namespace
#endif

// Constructors
// ------------
Simulator_Gui::Simulator_Gui(std::shared_ptr<const Scene> scene, bool run_xray_self_test, bool run_xray_view_test) :
	scene{ std::move(scene) },
	is_xray_self_test_requested{ run_xray_self_test }
{
#ifdef XRSIM_HAS_XRAY
	xray_view = std::make_unique<Xray_View>(this->scene, run_xray_view_test);
#else
	static_cast<void>(run_xray_view_test);
#endif
}

// Destructor
// ----------
Simulator_Gui::~Simulator_Gui() = default;

// Public Methods
// --------------
void Simulator_Gui::draw()
{
#ifdef XRSIM_HAS_XRAY
	if (is_xray_self_test_requested)
	{ // the self-test runs once, in the first frame, when the context of the window is current
		is_xray_self_test_requested = false;
		run_xray_self_test();
	}
#endif

	// set the position and size explicitly, so that the window is visible from the first frame
	ImGui::SetNextWindowPos(ImVec2{ 20.0f, 20.0f });
	ImGui::SetNextWindowSize(ImVec2{ 360.0f, 80.0f });
	ImGui::Begin("XRSim");
	ImGui::TextUnformatted("Projectional radiography simulator");
	ImGui::End();

	// the objects of the scene: the name, the shape, the material, and the position in centimetres
	if (scene)
	{
		ImGui::SetNextWindowPos(ImVec2{ 20.0f, 110.0f }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2{ 520.0f, 160.0f }, ImGuiCond_FirstUseEver);
		ImGui::Begin("Scene");
		if (ImGui::BeginTable("Objects", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Shape");
			ImGui::TableSetupColumn("Material");
			ImGui::TableSetupColumn("Position (cm)");
			ImGui::TableHeadersRow();

			const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
			for (const Scene_Object& object : scene->get_objects())
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(object.name.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(Scene::get_shape_type_name(object.type));
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(object.material_index < materials.size() ? materials[object.material_index].name.c_str() : "?");
				ImGui::TableNextColumn();
				ImGui::Text("%.1f, %.1f, %.1f", object.position[0], object.position[1], object.position[2]);
			}
			ImGui::EndTable();
		}
		ImGui::End();
	}

#ifdef XRSIM_HAS_XRAY
	// the X-ray image of the scene: a window next to the 3D view, or the background of the whole window
	if (xray_view)
		xray_view->draw();
#endif
}

// Private Methods
// ---------------
void Simulator_Gui::run_xray_self_test()
{
#ifdef XRSIM_HAS_XRAY
	const auto fail = [](const std::string& reason) {
		std::cerr << "[ERROR::SIMULATOR_GUI::run_xray_self_test] " << reason << std::endl;
	};

	// point source 1000 mm before the cube and flat detector 500 mm behind it: the 128 by 128 pixels of 2 mm cover the
	// shadow of the cube (150 mm wide on the detector, with a magnification of 1.5) and its surroundings
	Xray_Beam beam;
	beam.source_position = { 0.0, 0.0, -1000.0 };
	beam.energy          = 80.0;

	Xray_Detector detector;
	detector.position         = { 0.0, 0.0, 500.0 };
	detector.up_vector        = { 0.0, 1.0, 0.0 };
	detector.width_in_pixels  = 128;
	detector.height_in_pixels = 128;
	detector.pixel_size       = 2.0;

	const std::vector<std::pair<std::string, GLint>> state_before = capture_gl_state();

	Gvxr_Adapter adapter;
	std::string  error = adapter.initialise(beam, detector);
	if (!error.empty())
	{
		fail("The adapter could not be initialised: " + error);
		return;
	}

	error = adapter.add_mesh(make_cube("Water cube", 100.0f, Xray_Material{ "H2O", 1.0 }));
	if (!error.empty())
	{
		fail("The mesh could not be added: " + error);
		return;
	}

	Xray_Image image;
	error = adapter.compute_image(image);
	if (!error.empty())
	{
		fail("The image could not be computed: " + error);
		return;
	}

	const std::vector<std::pair<std::string, GLint>> state_after = capture_gl_state();

	// the times of the calls: one mesh, and the parts of the initialisation are not longer than the whole
	const Xray_Timings& timings = adapter.get_timings();
	std::cout << "[INFO::SIMULATOR_GUI::run_xray_self_test] times in ms: initialise " << timings.initialise_ms << " (GLEW "
			  << timings.glew_ms << ", renderer " << timings.renderer_initialise_ms << "), add meshes " << timings.add_meshes_ms
			  << ", computation " << timings.computation_ms << ", read-back " << timings.read_back_ms << ", state guard "
			  << timings.guard_ms << std::endl;
	if (timings.mesh_count != 1 || !(timings.initialise_ms > 0.0) || !(timings.computation_ms > 0.0) || !(timings.read_back_ms >= 0.0) ||
		timings.glew_ms + timings.renderer_initialise_ms > timings.initialise_ms || !(timings.guard_ms > 0.0))
	{
		fail("The times of the adapter are not valid");
		return;
	}

	// the state of OpenGL must be the same after the computation
	bool is_state_the_same{ true };
	for (std::size_t i = 0; i < state_before.size(); ++i)
	{
		if (state_before[i].second != state_after[i].second)
		{
			is_state_the_same = false;
			fail(
				"The state of OpenGL changed: " + state_before[i].first + " was " + std::to_string(state_before[i].second) +
				" and is now " + std::to_string(state_after[i].second));
		}
	}

	// the image must have the size of the detector and valid values
	if (image.width != detector.width_in_pixels || image.height != detector.height_in_pixels ||
		image.pixels.size() != static_cast<std::size_t>(image.width) * image.height)
	{
		fail("The image is " + std::to_string(image.width) + " by " + std::to_string(image.height) + " pixels");
		return;
	}
	if (!std::all_of(image.pixels.begin(), image.pixels.end(), [](float value) { return std::isfinite(value) && value >= 0.0f; }))
	{
		fail("The image has negative or non-finite values");
		return;
	}

	// the beam is attenuated behind the cube (centre) and not beside it (corners), by exp(-mu x): the mass attenuation
	// coefficient of water at 80 keV is 0.1837 cm2/g (NIST), so the 10 cm of water transmit exp(-1.837), 15.9 percent
	const double background = (mean_of_block(image, 0, 0, 8) + mean_of_block(image, 120, 0, 8) + mean_of_block(image, 0, 120, 8) +
							   mean_of_block(image, 120, 120, 8)) /
		4.0;
	const double behind_cube = mean_of_block(image, 60, 60, 8);
	if (!(background > 0.0))
	{
		fail("The background of the image is empty");
		return;
	}
	const double transmission = behind_cube / background;
	std::cout << "[INFO::SIMULATOR_GUI::run_xray_self_test] gVirtualXRay " << Gvxr_Adapter::get_version() << ": background " << background
			  << ", behind the cube " << behind_cube << ", transmission " << transmission << " (expected 0.159)" << std::endl;
	if (!(std::abs(transmission - EXPECTED_TRANSMISSION) < 0.03 * EXPECTED_TRANSMISSION))
	{
		fail(
			"The transmission through the cube is " + std::to_string(transmission) + ", not " + std::to_string(EXPECTED_TRANSMISSION) +
			" (within 3 percent)");
		return;
	}

	if (is_state_the_same)
	{
		std::cout
			<< "[SUCCESS::SIMULATOR_GUI::run_xray_self_test] The X-ray image of the cube is valid and the state of OpenGL is unchanged"
			<< std::endl;
	}
#endif
}
