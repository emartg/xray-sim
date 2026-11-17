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

	// The name of each part
	const std::array<const char*, Tube_Display::NODE_COUNT> PART_NAMES{ "X-ray Tube",  "Detector",    "Beam Edge 1", "Beam Edge 2",
																		"Beam Edge 3", "Beam Edge 4", "Beam Volume" };
} // namespace

// Public Methods of Tube_Display_Style
// ------------------------------------
std::array<float, 4> Tube_Display_Style::get_edge_colour() const
{
	if (are_edges_linked_to_volume)
		return { volume.colour[0], volume.colour[1], volume.colour[2], 1.0f };
	return edges.colour;
}

// Public Methods
// --------------
void Tube_Display::update(const Xray_Setup& setup, bool is_shown, const Tube_Display_Style& style)
{
	Core* engine = Core::get_instance();
	if (!engine || !engine->get_node_manager())
		return;
	auto& node_manager = engine->get_node_manager();

	// hidden: the nodes that exist are hidden, and a selected one is no longer selected
	if (!is_shown)
	{
		if (was_shown)
			apply_look(false, style);
		was_shown = false;
		return;
	}

	// shown: a node that does not exist (never created, or deleted in the Engine with the Delete key) is created, and the nodes are
	// placed again and take their look again if one was created
	for (std::size_t part = 0; part < NODE_COUNT; ++part)
	{
		if (node_ids[part] == 0 || !node_manager->get_node_by_id(node_ids[part]))
		{
			create_node(part);
			is_placed       = false;
			is_look_applied = false;
		}
	}

	// the look: when the display was hidden, when a node was created, or when the style changed
	if (!was_shown || !is_look_applied || style != applied_style)
	{
		apply_look(true, style);
		is_look_applied = true;
		was_shown       = true;
	}

	// the placement: when a node was created, the geometry of the setup changed, or a size of the style changed
	const std::array<std::array<float, 3>, 4> corners = setup.get_detector_corners();
	const std::array<std::array<float, 3>, 5> points{ setup.source_position, corners[0], corners[1], corners[2], corners[3] };
	const bool                                are_sizes_changed = style.housing_radius != applied_style.housing_radius ||
		style.housing_length != applied_style.housing_length || style.plate_thickness != applied_style.plate_thickness ||
		style.edge_thickness != applied_style.edge_thickness;
	if (!is_placed || points != placed_points || are_sizes_changed)
	{
		place_nodes(setup, style);
		placed_points = points;
		is_placed     = true;
	}
	applied_style = style;
}

Tube_Display::Part_Placement Tube_Display::get_placement(const Xray_Setup& setup, const Tube_Display_Style& style, std::size_t part)
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
		placement.size   = { 2.0f * style.housing_radius, style.housing_length, 2.0f * style.housing_radius };
	}
	else if (part == PLATE)
	{ // the front face on the plane of the detector, the box behind it (right x up = direction)
		const std::array<float, 2> size = setup.get_detector_size();
		placement.centre                = to_array(to_vector(setup.get_detector_position()) + direction * (style.plate_thickness / 2.0f));
		placement.axes                  = { to_array(right), to_array(up), to_array(direction) };
		placement.size                  = { size[0], size[1], style.plate_thickness };
	}
	else if (part == VOLUME)
	{ // the pyramid from the source (its apex) to the plane of the detector (its base, at the size of the detector)
		const std::array<float, 2> size = setup.get_detector_size();
		placement.centre                = setup.source_position;
		placement.axes                  = { to_array(right), to_array(up), to_array(direction) };
		placement.size                  = { size[0], size[1], setup.source_detector_distance };
	}
	else
	{ // an edge: from the source to a corner of the detector, along the third axis
		const glm::vec3 corner = to_vector(setup.get_detector_corners()[part - FIRST_EDGE]);
		const glm::vec3 along  = glm::normalize(corner - source);
		const glm::vec3 across = glm::normalize(glm::cross(up, along)); // the edges are never along the up vector of the detector
		placement.centre       = to_array((source + corner) / 2.0f);
		placement.axes         = { to_array(across), to_array(glm::cross(along, across)), to_array(along) };
		placement.size         = { style.edge_thickness, style.edge_thickness, glm::length(corner - source) };
	}
	return placement;
}

bool Tube_Display::get_is_part_shown(const Tube_Display_Style& style, std::size_t part)
{
	if (part == HOUSING)
		return style.housing.is_shown;
	if (part == PLATE)
		return style.plate.is_shown;
	if (part == VOLUME)
		return style.volume.is_shown;
	return style.edges.is_shown;
}

std::array<float, 4> Tube_Display::get_part_colour(const Tube_Display_Style& style, std::size_t part)
{
	if (part == HOUSING)
		return style.housing.colour;
	if (part == PLATE)
		return style.plate.colour;
	if (part == VOLUME)
		return style.volume.colour;
	return style.get_edge_colour();
}

// Private Methods
// ---------------
void Tube_Display::create_node(std::size_t part)
{
	// a cylinder, a pyramid, or a cube of 1 cm, which the scale of the node stretches to the size of the part
	const Shape_Data shape = part == HOUSING ? Shape_Factory::make_cylinder(0.5f, 1.0f)
		: part == VOLUME                     ? Shape_Factory::make_pyramid()
											 : Shape_Factory::make_cube(1.0f);

	auto node = std::make_shared<Shape_Model>(PART_NAMES[part], shape.vertices, shape.indices);
	node->set_name(String_Utils::generate_id_prefixed_name(node));
	node_ids[part] = node->get_id();
	Core::get_instance()->get_node_manager()->add_node(std::move(node));
}

void Tube_Display::place_nodes(const Xray_Setup& setup, const Tube_Display_Style& style) const
{
	auto& node_manager = Core::get_instance()->get_node_manager();
	for (std::size_t part = 0; part < NODE_COUNT; ++part)
	{
		const std::shared_ptr<Node> node = node_manager->get_node_by_id(node_ids[part]);
		if (!node)
			continue;

		const Part_Placement placement = get_placement(setup, style, part);
		node->set_position(Scene_Builder::to_view(placement.centre));
		node->set_rotation(
			glm::quat_cast(glm::mat3{ to_vector(placement.axes[0]), to_vector(placement.axes[1]), to_vector(placement.axes[2]) }));
		node->set_scale(
			Scene_Builder::to_view(placement.size)); // the scale of the node also converts the vertices to the units of the view
	}
}

void Tube_Display::apply_look(bool is_shown, const Tube_Display_Style& style) const
{
	Core* engine            = Core::get_instance();
	auto& node_manager      = engine->get_node_manager();
	auto& selection_manager = engine->get_selection_manager();
	for (std::size_t part = 0; part < NODE_COUNT; ++part)
	{
		const std::shared_ptr<Node> node = node_ids[part] != 0 ? node_manager->get_node_by_id(node_ids[part]) : nullptr;
		if (!node)
			continue;

		const bool is_visible = is_shown && get_is_part_shown(style, part);
		node->set_is_visible(is_visible);
		const std::array<float, 4> colour = get_part_colour(style, part); // an opacity below 1 makes the Engine draw it transparent
		node->set_albedo(glm::vec4{ colour[0], colour[1], colour[2], colour[3] });
		if (!is_visible && selection_manager && selection_manager->get_selected_node_id() == node_ids[part])
			selection_manager->set_selected_node_id(0);
	}
}
