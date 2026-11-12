/*
 * Tube_Display.cpp
 * This file implements the Tube_Display class, which draws the X-ray setup in the Engine's 3D view.
 */

#include "Tube_Display.h"

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Scene_Builder.h"
#include "core/Core.h"
#include "core/Node.h"
#include "core/managers/Node_Manager.h"
#include "core/managers/Selection_Manager.h"
#include "core/model/Shape_Model.h"
#include "core/utils/string/String_Utils.h"
#include "scene/Shape_Factory.h"

namespace
{
	glm::vec3 to_vector(const std::array<float, 3>& values)
	{
		return glm::vec3{ values[0], values[1], values[2] };
	}

	std::array<float, 3> to_array(const glm::vec3& vector)
	{
		return { vector.x, vector.y, vector.z };
	}

	// The name and the colour of each part
	struct Part_Style
	{
		const char* name;
		glm::vec4   albedo;
	};
	const std::array<Part_Style, Tube_Display::NODE_COUNT> PART_STYLES{ {
		{ "X-ray Tube", glm::vec4{ 0.30f, 0.30f, 0.34f, 1.0f } },
		{ "Detector", glm::vec4{ 0.55f, 0.62f, 0.72f, 1.0f } },
		{ "Beam Edge 1", glm::vec4{ 1.0f, 0.85f, 0.2f, 1.0f } },
		{ "Beam Edge 2", glm::vec4{ 1.0f, 0.85f, 0.2f, 1.0f } },
		{ "Beam Edge 3", glm::vec4{ 1.0f, 0.85f, 0.2f, 1.0f } },
		{ "Beam Edge 4", glm::vec4{ 1.0f, 0.85f, 0.2f, 1.0f } },
	} };
} // namespace

// Public Methods
// --------------
void Tube_Display::update(const Xray_Setup& setup, bool is_shown)
{
	Core* engine = Core::get_instance();
	if (!engine || !engine->get_node_manager())
		return;
	auto& node_manager = engine->get_node_manager();

	// hidden: the nodes that exist are hidden, and a selected one is no longer selected
	if (!is_shown)
	{
		if (was_shown)
		{
			auto& selection_manager = engine->get_selection_manager();
			for (const std::uint32_t id : node_ids)
			{
				if (const std::shared_ptr<Node> node = id != 0 ? node_manager->get_node_by_id(id) : nullptr)
					node->set_is_visible(false);
				if (id != 0 && selection_manager && selection_manager->get_selected_node_id() == id)
					selection_manager->set_selected_node_id(0);
			}
		}
		was_shown = false;
		return;
	}

	// shown: a node that does not exist (never created, or deleted in the Engine with the Delete key) is created, and the nodes are
	// placed again if one was created or the geometry changed
	for (std::size_t part = 0; part < NODE_COUNT; ++part)
	{
		if (node_ids[part] == 0 || !node_manager->get_node_by_id(node_ids[part]))
		{
			create_node(part);
			is_placed = false;
		}
	}
	if (!was_shown)
	{
		for (const std::uint32_t id : node_ids)
			if (const std::shared_ptr<Node> node = node_manager->get_node_by_id(id))
				node->set_is_visible(true);
		was_shown = true;
	}

	const std::array<std::array<float, 3>, 4> corners = setup.get_detector_corners();
	const std::array<std::array<float, 3>, 5> points{ setup.source_position, corners[0], corners[1], corners[2], corners[3] };
	if (!is_placed || points != placed_points)
	{
		place_nodes(setup);
		placed_points = points;
		is_placed     = true;
	}
}

Tube_Display::Part_Placement Tube_Display::get_placement(const Xray_Setup& setup, std::size_t part)
{
	const glm::vec3 source    = to_vector(setup.source_position);
	const glm::vec3 direction = to_vector(setup.get_direction());
	const glm::vec3 right     = to_vector(setup.get_detector_right());
	const glm::vec3 up        = to_vector(setup.get_detector_up());

	Part_Placement placement;
	if (part == HOUSING)
	{ // the axis of the cylinder (its second axis) along the right of the image; the three axes are right-handed (up x right = -direction)
		placement.centre = setup.source_position;
		placement.axes   = { to_array(up), to_array(right), to_array(-direction) };
		placement.size   = { 2.0f * HOUSING_RADIUS, HOUSING_LENGTH, 2.0f * HOUSING_RADIUS };
	}
	else if (part == PLATE)
	{ // the front face on the plane of the detector, the box behind it (right x up = direction)
		const std::array<float, 2> size = setup.get_detector_size();
		placement.centre                = to_array(to_vector(setup.get_detector_position()) + direction * (PLATE_THICKNESS / 2.0f));
		placement.axes                  = { to_array(right), to_array(up), to_array(direction) };
		placement.size                  = { size[0], size[1], PLATE_THICKNESS };
	}
	else
	{ // an edge: from the source to a corner of the detector, along the third axis
		const glm::vec3 corner = to_vector(setup.get_detector_corners()[part - FIRST_EDGE]);
		const glm::vec3 along  = glm::normalize(corner - source);
		const glm::vec3 across = glm::normalize(glm::cross(up, along)); // the edges are never along the up vector of the detector
		placement.centre       = to_array((source + corner) / 2.0f);
		placement.axes         = { to_array(across), to_array(glm::cross(along, across)), to_array(along) };
		placement.size         = { EDGE_THICKNESS, EDGE_THICKNESS, glm::length(corner - source) };
	}
	return placement;
}

// Private Methods
// ---------------
void Tube_Display::create_node(std::size_t part)
{
	// a cylinder or a cube of 1 cm, which the scale of the node stretches to the size of the part
	const Shape_Data shape = part == HOUSING ? Shape_Factory::make_cylinder(0.5f, 1.0f) : Shape_Factory::make_cube(1.0f);

	auto node = std::make_shared<Shape_Model>(PART_STYLES[part].name, shape.vertices, shape.indices, PART_STYLES[part].albedo);
	node->set_name(String_Utils::generate_id_prefixed_name(node));
	node_ids[part] = node->get_id();
	Core::get_instance()->get_node_manager()->add_node(std::move(node));
}

void Tube_Display::place_nodes(const Xray_Setup& setup) const
{
	auto& node_manager = Core::get_instance()->get_node_manager();
	for (std::size_t part = 0; part < NODE_COUNT; ++part)
	{
		const std::shared_ptr<Node> node = node_manager->get_node_by_id(node_ids[part]);
		if (!node)
			continue;

		const Part_Placement placement = get_placement(setup, part);
		node->set_position(Scene_Builder::to_view(placement.centre));
		node->set_rotation(
			glm::quat_cast(glm::mat3{ to_vector(placement.axes[0]), to_vector(placement.axes[1]), to_vector(placement.axes[2]) }));
		node->set_scale(
			Scene_Builder::to_view(placement.size)); // the scale of the node also converts the vertices to the units of the view
	}
}
