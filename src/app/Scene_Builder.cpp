/*
 * Scene_Builder.cpp
 * This file implements the Scene_Builder class, which fills the Engine's scene from the simulator's description of a scene.
 */

#include "Scene_Builder.h"

#include <memory>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/Core.h"
#include "core/camera/Camera.h"
#include "core/light/Directional_Light.h"
#include "core/managers/Node_Manager.h"
#include "core/managers/Scene_Manager.h"
#include "core/model/Shape_Model.h"
#include "core/utils/string/String_Utils.h"

#include "scene/Scene_Material_Table.h"

// Public Static Methods
// ---------------------
std::vector<std::uint32_t> Scene_Builder::build(const Scene& scene, Core* engine)
{
	auto& node_manager  = engine->get_node_manager();
	auto& scene_manager = engine->get_scene_manager();

	// the camera: 40 cm in front of the objects, 12 cm above them, looking slightly down (the default yaw looks along -z)
	auto camera = std::make_shared<Camera>("Camera", to_view({ 0.0f, 12.0f, 40.0f }), glm::vec3{ 0.0f, 1.0f, 0.0f }, -90.0f, -15.0f);
	camera->set_name(String_Utils::generate_id_prefixed_name(camera));
	scene_manager->set_camera(camera);
	node_manager->add_node(std::move(camera));

	// a directional light from above, in front, and to the right
	auto light = std::make_shared<Directional_Light>(
		"Directional Light",
		glm::vec3{ 0.25f },                   // ambient colour
		glm::vec4{ 1.0f, 1.0f, 0.95f, 1.0f }, // diffuse colour
		glm::vec3{ 1.0f },                    // specular colour
		to_view({ 10.0f, 30.0f, 20.0f }),     // position
		glm::vec3{ -0.3f, -0.8f, -0.5f });    // direction
	light->set_name(String_Utils::generate_id_prefixed_name(light));
	node_manager->add_node(std::move(light));

	// a node for each object, with the geometry of the scene (in centimetres) and its transform in the units of the view: the scale of the
	// node also converts the vertices to decimetres
	const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
	std::vector<std::uint32_t>         node_ids;
	node_ids.reserve(scene.get_objects().size());
	for (const Scene_Object& object : scene.get_objects())
	{
		const Scene_Material& material = materials[object.material_index < materials.size() ? object.material_index : 0];

		auto node = std::make_shared<Shape_Model>(
			object.name,
			object.shape.vertices,
			object.shape.indices,
			glm::vec4{ material.albedo[0], material.albedo[1], material.albedo[2], material.albedo[3] },
			to_view(object.position),
			glm::quat{ glm::radians(glm::vec3{ object.rotation[0], object.rotation[1], object.rotation[2] }) },
			to_view(object.scale));
		node->set_name(String_Utils::generate_id_prefixed_name(node));
		node_ids.push_back(node->get_id());
		node_manager->add_node(std::move(node));
	}

	return node_ids;
}

glm::vec3 Scene_Builder::to_view(const std::array<float, 3>& values)
{
	return glm::vec3{ values[0], values[1], values[2] } * VIEW_UNITS_PER_CENTIMETRE;
}

std::array<float, 3> Scene_Builder::from_view(const glm::vec3& position)
{
	const glm::vec3 centimetres = position / VIEW_UNITS_PER_CENTIMETRE;
	return { centimetres.x, centimetres.y, centimetres.z };
}
