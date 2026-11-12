/*
 * Scene_Sync.cpp
 * This file implements the Scene_Sync class, which keeps the Engine's nodes in step with the simulator's Scene.
 */

#include "Scene_Sync.h"

#include <array>
#include <iostream>
#include <utility>

#include "Scene_Builder.h"
#include "core/Core.h"
#include "core/Node.h"
#include "core/managers/Node_Manager.h"
#include "core/managers/Selection_Manager.h"
#include "scene/Scene_Material_Table.h"

// Constructors
// ------------
Scene_Sync::Scene_Sync(std::shared_ptr<Scene> scene) : scene{ std::move(scene) } {}

// Private Static Methods
// ----------------------
Scene_Sync::Applied_State Scene_Sync::make_applied_state(const Scene_Object& object)
{
	return Applied_State{ object.material_index, object.position, object.rotation, object.scale };
}

// Public Methods
// --------------
void Scene_Sync::set_node_ids(std::vector<std::uint32_t> ids)
{
	node_ids = std::move(ids);

	// the nodes were built from the objects, so they show what the objects have now
	applied_state.clear();
	if (scene)
	{
		for (const Scene_Object& object : scene->get_objects()) applied_state.push_back(make_applied_state(object));
	}
}

void Scene_Sync::update()
{
	Core* engine = Core::get_instance();
	if (!scene || !engine || !engine->get_node_manager() || node_ids.size() != scene->get_objects().size())
		return;

	const std::shared_ptr<Node_Manager>& node_manager = engine->get_node_manager();

	// the nodes that the user deleted in the Engine: their objects go, from the last to the first so that the indices stay valid
	for (std::size_t i = node_ids.size(); i-- > 0;)
	{
		if (node_manager->get_node_by_id(node_ids[i]))
			continue;

		std::cout << "[WARNING::SCENE_SYNC::update] The node of '" << scene->get_objects()[i].name
				  << "' was deleted in the Engine: the object is removed from the scene" << std::endl;
		scene->remove_object(i);
		node_ids.erase(node_ids.begin() + static_cast<std::ptrdiff_t>(i));
		applied_state.erase(applied_state.begin() + static_cast<std::ptrdiff_t>(i));
	}

	// the nodes whose objects changed: the colour of the new material, and the new position, rotation, and scale
	const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
	for (std::size_t i = 0; i < node_ids.size(); ++i)
	{
		const Scene_Object& object = scene->get_objects()[i];
		Applied_State&      state  = applied_state[i];

		const std::shared_ptr<Node> node = node_manager->get_node_by_id(node_ids[i]);
		if (!node)
			continue;

		if (object.material_index != state.material_index && object.material_index < materials.size())
		{
			const std::array<float, 4>& albedo = materials[object.material_index].albedo;
			node->set_albedo(glm::vec4{ albedo[0], albedo[1], albedo[2], albedo[3] });
			state.material_index = object.material_index;
		}
		if (object.position != state.position)
		{
			node->set_position(Scene_Builder::to_view(object.position));
			state.position = object.position;
		}
		if (object.rotation != state.rotation)
		{ // the angles are set as the constructor of the node took them: Euler angles in degrees
			node->set_rotation_in_euler_angles(glm::vec3{ object.rotation[0], object.rotation[1], object.rotation[2] });
			state.rotation = object.rotation;
		}
		if (object.scale != state.scale)
		{
			node->set_scale(
				Scene_Builder::to_view(object.scale)); // the scale of the node also converts the vertices to the units of the view
			state.scale = object.scale;
		}
	}
}

std::uint32_t Scene_Sync::get_node_id(std::size_t object_index) const
{
	return object_index < node_ids.size() ? node_ids[object_index] : 0;
}

std::optional<std::size_t> Scene_Sync::get_selected_object() const
{
	Core* engine = Core::get_instance();
	if (!engine || !engine->get_selection_manager())
		return std::nullopt;

	const std::uint32_t selected_id = engine->get_selection_manager()->get_selected_node_id();
	if (selected_id == 0)
		return std::nullopt;

	for (std::size_t i = 0; i < node_ids.size(); ++i)
	{
		if (node_ids[i] == selected_id)
			return i;
	}
	return std::nullopt;
}
