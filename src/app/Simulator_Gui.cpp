/*
 * Simulator_Gui.cpp
 * This file implements the Simulator_Gui class, which is the graphical user interface of the simulator.
 */

#include "Simulator_Gui.h"

#include <array>
#include <optional>

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Scene_Sync.h"
#include "core/Core.h"
#include "core/Node.h"
#include "core/managers/Node_Manager.h"
#include "core/managers/Selection_Manager.h"
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

	// Checks that what the adapter changes in place gives the image of a new adapter: the material and the transform of a mesh, and the
	// beam and the detector (with the same number of pixels; another number is refused). Returns an empty string if it does, or the
	// reason of the failure. No other adapter may be alive: the library keeps one graphics context
	std::string check_in_place_updates(const Xray_Beam& beam, const Xray_Detector& detector)
	{
		const Xray_Material aluminium{ "Al", 2.699 };

		// a translation of 30 mm along x, in the column-major order of OpenGL
		const std::array<float, 16> translation{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
												 0.0f, 0.0f, 1.0f, 0.0f, 30.0f, 0.0f, 0.0f, 1.0f };

		Xray_Beam changed_beam       = beam;
		changed_beam.energy          = 60.0;
		changed_beam.source_position = { 0.0, 0.0, -1200.0 };

		Xray_Detector changed_detector = detector;
		changed_detector.position      = { 0.0, 0.0, 600.0 };
		changed_detector.pixel_size    = 2.5;

		Xray_Image first;
		Xray_Image moved;
		Xray_Image updated;
		Xray_Image fresh;
		{
			Gvxr_Adapter adapter;
			std::string  error = adapter.initialise(beam, detector);
			if (error.empty())
				error = adapter.add_mesh(make_cube("Cube", 100.0f, Xray_Material{ "H2O", 1.0 }));
			if (error.empty())
				error = adapter.compute_image(first);
			if (error.empty())
				error = adapter.set_mesh_material(0, aluminium);
			if (error.empty())
				error = adapter.set_mesh_transform(0, translation);
			if (error.empty())
				error = adapter.compute_image(moved);
			if (error.empty())
				error = adapter.update_beam(changed_beam);
			if (error.empty())
				error = adapter.update_detector(changed_detector);
			if (error.empty())
				error = adapter.compute_image(updated);
			if (!error.empty())
				return "In place: " + error;

			Xray_Detector other_detector   = detector;
			other_detector.width_in_pixels = 64;
			if (adapter.update_detector(other_detector).empty())
				return "A detector with another number of pixels was accepted in place";
			if (adapter.set_mesh_transform(1, translation).empty() || adapter.set_mesh_material(1, aluminium).empty())
				return "A mesh that does not exist was accepted";
		}

		{ // the same state, built from scratch: an aluminium cube moved 30 mm along x in its positions, and the changed beam and detector
			Xray_Mesh cube = make_cube("Cube", 100.0f, aluminium);
			for (std::size_t i = 0; i < cube.positions.size(); i += 3) cube.positions[i] += 30.0f;

			Gvxr_Adapter adapter;
			std::string  error = adapter.initialise(changed_beam, changed_detector);
			if (error.empty())
				error = adapter.add_mesh(cube);
			if (error.empty())
				error = adapter.compute_image(fresh);
			if (!error.empty())
				return "New adapter: " + error;
		}

		const auto largest_difference = [](const Xray_Image& a, const Xray_Image& b) {
			float difference{ 0.0f };
			for (std::size_t i = 0; i < a.pixels.size() && i < b.pixels.size(); ++i)
				difference = std::max(difference, std::abs(a.pixels[i] - b.pixels[i]));
			return difference;
		};
		if (first.pixels.size() != moved.pixels.size() || updated.pixels.size() != fresh.pixels.size() || fresh.pixels.empty())
			return "The images have different sizes";

		const float beam_value       = *std::max_element(fresh.pixels.begin(), fresh.pixels.end());
		const float first_beam_value = *std::max_element(first.pixels.begin(), first.pixels.end());
		std::cout << "[INFO::SIMULATOR_GUI::run_xray_self_test] the changes in place differ from a new adapter by "
				  << largest_difference(updated, fresh) << " (the unattenuated beam is " << beam_value
				  << "), and the change of the material and of the transform changes the first image by "
				  << largest_difference(first, moved) << " (the unattenuated beam was " << first_beam_value << ")" << std::endl;
		if (!(largest_difference(first, moved) > 0.05f * first_beam_value))
			return "The change of the material and of the transform did not change the image";
		if (!(largest_difference(updated, fresh) <= 1.0e-3f * beam_value))
			return "The image updated in place is not the one of a new adapter";

		return "";
	}
} // namespace
#endif

// Constructors
// ------------
Simulator_Gui::Simulator_Gui(std::shared_ptr<Scene> scene, bool run_xray_self_test, bool run_xray_view_test) :
	scene{ std::move(scene) },
	scene_sync{ this->scene ? std::make_unique<Scene_Sync>(this->scene) : nullptr },
	is_xray_self_test_requested{ run_xray_self_test },
	is_scene_change_test_requested{ run_xray_view_test && this->scene }
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
	ImGui::TextWrapped("Projectional radiography simulator"); // wraps at the edge of the window, whatever its size
	ImGui::End();

	draw_scene_window();

#ifdef XRSIM_HAS_XRAY
	// the X-ray image of the scene: a window next to the 3D view, or the background of the whole window
	if (xray_view)
	{
		xray_view->draw();

		// after the checks of the first image, the changes of the scene are checked once
		if (is_scene_change_test_requested && xray_view->get_is_check_passed())
		{
			is_scene_change_test_requested = false;
			run_scene_change_test();
			selection_test_stage = 1;
		}
		if (selection_test_stage > 0)
			run_selection_test();
	}
#endif
}

void Simulator_Gui::set_node_ids(std::vector<std::uint32_t> node_ids)
{
	if (scene_sync)
		scene_sync->set_node_ids(std::move(node_ids));
}

// Private Methods
// ---------------
void Simulator_Gui::draw_scene_window()
{
	if (!scene)
		return;

	// the nodes follow the scene (and the scene follows the nodes that the user deleted in the Engine)
	if (scene_sync)
		scene_sync->update();
	const std::optional<std::size_t> selected = scene_sync ? scene_sync->get_selected_object() : std::nullopt;

	ImGui::SetNextWindowPos(ImVec2{ 20.0f, 110.0f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2{ 520.0f, 380.0f }, ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene");

	// the objects of the scene: the name, the shape, the material, and the position in centimetres; the row of the selected object has a
	// colour
	const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
	if (ImGui::BeginTable("Objects", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Shape");
		ImGui::TableSetupColumn("Material");
		ImGui::TableSetupColumn("Position (cm)");
		ImGui::TableHeadersRow();

		const std::vector<Scene_Object>& objects = scene->get_objects();
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			const Scene_Object& object = objects[i];
			ImGui::TableNextRow();
			if (selected && *selected == i)
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(0, 130, 150, 120));
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

	// the selected object: its data, and the combo of its material (a click on an object of the 3D view selects it)
	ImGui::Separator();
	if (!selected)
	{
		ImGui::TextWrapped("No object is selected. Click an object in the 3D view to select it.");
	}
	else
	{
		const Scene_Object& object = scene->get_objects()[*selected];
		ImGui::TextWrapped("Selected: %s (%s)", object.name.c_str(), Scene::get_shape_type_name(object.type));

		std::size_t material_index = object.material_index < materials.size() ? object.material_index : 0;
		if (ImGui::BeginCombo("Material", materials[material_index].name.c_str()))
		{
			for (std::size_t i = 0; i < materials.size(); ++i)
			{
				if (ImGui::Selectable(materials[i].name.c_str(), i == material_index))
				{
					scene->set_object_material(*selected, i); // the node and the X-ray image follow: see Scene_Sync and Xray_View
					scene_sync->update();
				}
				if (i == material_index)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// the object may have changed (the combo): it is read again
		const Scene_Object& current = scene->get_objects()[*selected];
		material_index              = current.material_index < materials.size() ? current.material_index : 0;
		ImGui::TextWrapped(
			"X-ray material: %s, %.3g g/cm3",
			materials[material_index].xray.compound.c_str(),
			materials[material_index].xray.density);

		// the transform of the object: drag fields with a fine step per pixel (1 mm, 0.2 degrees, and 0.005 of scale; Shift drags 10 times
		// faster, Alt 100 times slower, and a double click or Ctrl+click types a value) that keep every value within the limits of the
		// scene; the nodes and the X-ray image follow the scene (see Scene_Sync and Xray_View)
		const int            clamp_flag = ImGuiSliderFlags_AlwaysClamp;
		std::array<float, 3> position   = current.position;
		std::array<float, 3> rotation   = current.rotation;
		std::array<float, 3> scale      = current.scale;
		if (ImGui::DragFloat3("Position (cm)", position.data(), 0.1f, Scene::MIN_POSITION, Scene::MAX_POSITION, "%.1f", clamp_flag))
			scene->set_object_position(*selected, position);
		if (ImGui::DragFloat3("Rotation (degrees)", rotation.data(), 0.2f, Scene::MIN_ROTATION, Scene::MAX_ROTATION, "%.1f", clamp_flag))
			scene->set_object_rotation(*selected, rotation);
		if (ImGui::DragFloat3("Scale", scale.data(), 0.005f, Scene::MIN_SCALE, Scene::MAX_SCALE, "%.3f", clamp_flag))
			scene->set_object_scale(*selected, scale);
	}

	ImGui::End();
}

void Simulator_Gui::run_scene_change_test()
{
#ifdef XRSIM_HAS_XRAY
	const auto fail = [](const std::string& reason) {
		std::cerr << "[ERROR::SIMULATOR_GUI::run_scene_change_test] " << reason << std::endl;
	};

	Core* engine = Core::get_instance();
	if (!scene || !scene_sync || !xray_view || !engine || !engine->get_node_manager() || !engine->get_selection_manager())
	{
		fail("The scene, the nodes, or the X-ray view are not ready");
		return;
	}

	const auto find_object = [this](const std::string& name) {
		const std::vector<Scene_Object>& objects = scene->get_objects();
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			if (objects[i].name == name)
				return i;
		}
		return objects.size();
	};
	const std::size_t water     = find_object("Water Sphere");
	const std::size_t iron      = find_object("Iron Sphere");
	const std::size_t count     = scene->get_objects().size();
	const std::size_t aluminium = Scene_Material_Table::find("Aluminium");
	if (water == count || iron == count || aluminium == Scene_Material_Table::get_materials().size())
	{
		fail("The default scene has not the water sphere, the iron sphere, or the aluminium");
		return;
	}

	// a new material: the node takes its colour and the X-ray image takes it in place (aluminium attenuates more than water)
	const float  water_x      = scene->get_objects()[water].position[0];
	const float  water_y      = scene->get_objects()[water].position[1];
	const double water_before = xray_view->get_transmission_at(water_x, water_y);
	scene->set_object_material(water, aluminium);
	scene_sync->update();
	xray_view->update_scene();

	const std::shared_ptr<Node> water_node = engine->get_node_manager()->get_node_by_id(scene_sync->get_node_id(water));
	if (!water_node)
	{
		fail("The node of the water sphere does not exist");
		return;
	}
	const std::array<float, 4>& albedo = Scene_Material_Table::get_materials()[aluminium].albedo;
	if (water_node->get_albedo() != glm::vec4{ albedo[0], albedo[1], albedo[2], albedo[3] })
	{
		fail("The node of the water sphere has not the colour of aluminium");
		return;
	}
	const double water_after = xray_view->get_transmission_at(water_x, water_y);
	std::cout << "[INFO::SIMULATOR_GUI::run_scene_change_test] transmission behind the sphere: " << water_before << " as water, "
			  << water_after << " as aluminium" << std::endl;
	if (!(water_before > 0.0 && water_after >= 0.0 && water_after < 0.5 * water_before))
	{
		fail("The X-ray image did not take the new material (aluminium should attenuate more than water)");
		return;
	}

	// a transform: the node moves, turns, and scales with the object, and the X-ray image follows in place
	const std::size_t cube = find_object("Aluminium Cube");
	if (cube == count)
	{
		fail("The default scene has not the aluminium cube");
		return;
	}
	const float                cube_x        = scene->get_objects()[cube].position[0];
	const float                cube_y        = scene->get_objects()[cube].position[1];
	const double               cube_before   = xray_view->get_transmission_at(cube_x, cube_y);
	const std::array<float, 3> cube_rotation = scene->get_objects()[cube].rotation;
	scene->set_object_position(cube, { cube_x, cube_y + 5.0f, 0.0f });
	scene->set_object_rotation(cube, { 0.0f, 60.0f, 15.0f });
	scene->set_object_scale(water, { 0.5f, 0.5f, 0.5f });
	scene_sync->update();
	xray_view->update_scene();

	const std::shared_ptr<Node> cube_node = engine->get_node_manager()->get_node_by_id(scene_sync->get_node_id(cube));
	if (!cube_node || !water_node)
	{
		fail("A node of the cube or of the sphere does not exist");
		return;
	}
	const glm::quat expected_rotation{ glm::radians(glm::vec3{ 0.0f, 60.0f, 15.0f }) };
	if (cube_node->get_position() != glm::vec3{ cube_x, cube_y + 5.0f, 0.0f } ||
		std::abs(glm::dot(cube_node->get_rotation(), expected_rotation)) < 0.9999f || water_node->get_scale() != glm::vec3{ 0.5f })
	{
		fail("The nodes did not take the position, the rotation, and the scale");
		return;
	}
	const double cube_after         = xray_view->get_transmission_at(cube_x, cube_y);
	const double cube_moved         = xray_view->get_transmission_at(cube_x, cube_y + 5.0f);
	const double water_when_smaller = xray_view->get_transmission_at(water_x, water_y);
	std::cout << "[INFO::SIMULATOR_GUI::run_scene_change_test] transmission where the cube was: " << cube_before << " before, "
			  << cube_after << " after it moved up 5 cm (" << cube_moved
			  << " at its new place); behind the sphere at half its size: " << water_when_smaller << std::endl;
	if (!(cube_after > 0.5 && cube_after > 10.0 * cube_before && cube_moved < 0.5))
	{
		fail("The X-ray image did not follow the movement of the cube");
		return;
	}
	if (!(water_when_smaller > 3.0 * water_after))
	{
		fail("The X-ray image did not follow the scale of the sphere");
		return;
	}
	const double difference = xray_view->measure_difference_with_new_adapter();
	std::cout << "[INFO::SIMULATOR_GUI::run_scene_change_test] the image updated in place differs from the one of a new adapter by "
			  << difference << " of the unattenuated beam" << std::endl;
	if (!(difference >= 0.0 && difference < 1.0e-3))
	{
		fail("The image updated in place is not the one of a new adapter");
		return;
	}

	// the detector plate through an object: no image is computed, the last one stays, and the reason is given; the object back
	// out of the way, the image is computed again
	scene->set_object_position(water, { 0.0f, 0.0f, 49.5f }); // the detector is 50 cm behind the plane z = 0
	scene_sync->update();
	xray_view->update_scene();
	const double      cube_while_stopped = xray_view->get_transmission_at(cube_x, cube_y + 5.0f);
	const std::string problem            = xray_view->get_geometry_problem();
	std::cout << "[INFO::SIMULATOR_GUI::run_scene_change_test] with the sphere on the detector plate: '" << problem << "'" << std::endl;
	if (problem.empty() || cube_while_stopped != cube_moved)
	{
		fail("The sphere around the detector was not refused, or the last image did not stay");
		return;
	}
	scene->set_object_position(water, { water_x, water_y, 0.0f });
	scene_sync->update();
	xray_view->update_scene();
	if (!xray_view->get_geometry_problem().empty() || !(xray_view->get_transmission_at(water_x, water_y) > 0.0))
	{
		fail("The image was not computed again when the sphere left the detector");
		return;
	}

	// the cube and the sphere are put back, so that the click of the next test finds the cube at the centre of the 3D view
	scene->set_object_position(cube, { cube_x, cube_y, 0.0f });
	scene->set_object_rotation(cube, cube_rotation);
	scene->set_object_scale(water, { 1.0f, 1.0f, 1.0f });
	scene_sync->update();
	xray_view->update_scene();

	// a node deleted in the Engine, as the Delete key does for the selected node: the object leaves the scene and the X-ray image
	const float  iron_x      = scene->get_objects()[iron].position[0];
	const float  iron_y      = scene->get_objects()[iron].position[1];
	const double iron_before = xray_view->get_transmission_at(iron_x, iron_y);
	engine->get_selection_manager()->set_selected_node_id(scene_sync->get_node_id(iron));
	engine->get_selection_manager()->delete_selected(engine->get_node_manager().get());
	scene_sync->update();
	xray_view->update_scene();

	const double iron_after = xray_view->get_transmission_at(iron_x, iron_y);
	std::cout << "[INFO::SIMULATOR_GUI::run_scene_change_test] transmission where the iron sphere was: " << iron_before << " before, "
			  << iron_after << " after the node was deleted" << std::endl;
	if (scene->get_objects().size() != count - 1 || find_object("Iron Sphere") != count - 1)
	{
		fail("The deleted node did not remove its object from the scene");
		return;
	}
	if (scene_sync->get_selected_object().has_value())
	{
		fail("An object is still selected after the deletion");
		return;
	}
	if (!(iron_after > 0.5 && iron_after > 10.0 * iron_before))
	{
		fail("The X-ray image still shows the deleted object");
		return;
	}

	std::cout << "[SUCCESS::SIMULATOR_GUI::run_scene_change_test] The changes of a material and of a transform and the deletion of a "
				 "node reach the node, the scene, and the X-ray image"
			  << std::endl;
#endif
}

void Simulator_Gui::run_selection_test()
{
#ifdef XRSIM_HAS_XRAY
	const auto fail = [this](const std::string& reason) {
		selection_test_stage = 0;
		std::cerr << "[ERROR::SIMULATOR_GUI::run_selection_test] " << reason << std::endl;
	};

	Core* engine = Core::get_instance();
	if (!scene || !scene_sync || !engine || !engine->get_selection_manager())
	{
		fail("The scene or the selection are not ready");
		return;
	}

	if (selection_test_stage == 1)
	{ // the click: the centre of the window, in screen coordinates, and the size of the framebuffer
		const ImGuiIO& io = ImGui::GetIO();
		engine->get_selection_manager()->queue_pick(
			io.DisplaySize.x / 2.0f,
			io.DisplaySize.y / 2.0f,
			static_cast<GLsizei>(io.DisplaySize.x * io.DisplayFramebufferScale.x),
			static_cast<GLsizei>(io.DisplaySize.y * io.DisplayFramebufferScale.y));
		selection_test_stage = 2;
		return;
	}

	// the click is resolved in the frame after it was queued; it is looked for in two frames
	const std::optional<std::size_t> selected = scene_sync->get_selected_object();
	if (!selected)
	{
		if (++selection_test_tries >= 2)
			fail("The click at the centre of the 3D view did not select an object");
		return;
	}

	const std::string name = scene->get_objects()[*selected].name;
	std::cout << "[INFO::SIMULATOR_GUI::run_selection_test] the click at the centre of the 3D view selected '" << name << "'" << std::endl;
	if (name != "Aluminium Cube")
	{
		fail("The click at the centre of the 3D view selected '" + name + "', not the aluminium cube");
		return;
	}

	selection_test_stage = 0;
	std::cout << "[SUCCESS::SIMULATOR_GUI::run_selection_test] A click in the 3D view selects the object under it" << std::endl;
#endif
}

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

	auto          adapter_owner = std::make_unique<Gvxr_Adapter>(); // destroyed before the check of the updates in place
	Gvxr_Adapter& adapter       = *adapter_owner;
	std::string   error         = adapter.initialise(beam, detector);
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

	// what the adapter changes in place gives the image of a new adapter (the first adapter goes first: the library keeps one context)
	adapter_owner.reset();
	error = check_in_place_updates(beam, detector);
	if (!error.empty())
	{
		fail(error);
		return;
	}

	if (is_state_the_same)
	{
		std::cout << "[SUCCESS::SIMULATOR_GUI::run_xray_self_test] The X-ray image of the cube is valid, the state of OpenGL is unchanged, "
					 "and the changes in place give the image of a new adapter"
				  << std::endl;
	}
#endif
}
