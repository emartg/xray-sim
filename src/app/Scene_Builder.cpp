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
void Scene_Builder::build(const Scene& scene, Core* engine)
{
	auto& node_manager  = engine->get_node_manager();
	auto& scene_manager = engine->get_scene_manager();

	// the camera: 40 cm in front of the objects, 12 cm above them, looking slightly down (the default yaw looks along -z)
	auto camera = std::make_shared<Camera>("Camera", glm::vec3{ 0.0f, 12.0f, 40.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, -90.0f, -15.0f);
	camera->set_name(String_Utils::generate_id_prefixed_name(camera));
	scene_manager->set_camera(camera);
	node_manager->add_node(std::move(camera));

	// a directional light from above, in front, and to the right
	auto light = std::make_shared<Directional_Light>(
		"Directional Light",
		glm::vec3{ 0.25f },                   // ambient colour
		glm::vec4{ 1.0f, 1.0f, 0.95f, 1.0f }, // diffuse colour
		glm::vec3{ 1.0f },                    // specular colour
		glm::vec3{ 10.0f, 30.0f, 20.0f },     // position
		glm::vec3{ -0.3f, -0.8f, -0.5f });    // direction
	light->set_name(String_Utils::generate_id_prefixed_name(light));
	node_manager->add_node(std::move(light));

	// a node for each object, with the geometry and the transform of the scene
	const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
	for (const Scene_Object& object : scene.get_objects())
	{
		const Scene_Material& material = materials[object.material_index < materials.size() ? object.material_index : 0];

		auto node = std::make_shared<Shape_Model>(
			object.name,
			object.shape.vertices,
			object.shape.indices,
			glm::vec4{ material.albedo[0], material.albedo[1], material.albedo[2], material.albedo[3] },
			glm::vec3{ object.position[0], object.position[1], object.position[2] },
			glm::quat{ glm::radians(glm::vec3{ object.rotation[0], object.rotation[1], object.rotation[2] }) },
			glm::vec3{ object.scale[0], object.scale[1], object.scale[2] });
		node->set_name(String_Utils::generate_id_prefixed_name(node));
		node_manager->add_node(std::move(node));
	}
}
