/*
 * Scene.cpp
 * This file implements the Scene class, which describes what the simulator shows.
 */

#include "scene/Scene.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "scene/Scene_Material_Table.h"
#include "scene/Shape_Factory.h"

namespace
{
	// Creates an object of the default scene
	Scene_Object make_object(
		const std::string&   name,
		Shape_Type           type,
		Shape_Data           shape,
		std::array<float, 3> position,
		std::array<float, 3> rotation,
		const std::string&   material_name)
	{
		Scene_Object object;
		object.name           = name;
		object.type           = type;
		object.shape          = std::move(shape);
		object.position       = position;
		object.rotation       = rotation;
		object.material_index = Scene_Material_Table::find(material_name);
		return object;
	}

	// Returns the model matrix of an object: the same composition as the Engine's nodes (translation, rotation, scale), with the
	// rotation built from the Euler angles in degrees as the Engine's constructors of shapes do
	glm::mat4 make_model_matrix(const Scene_Object& object)
	{
		const glm::quat rotation{ glm::radians(glm::vec3{ object.rotation[0], object.rotation[1], object.rotation[2] }) };
		return glm::translate(glm::mat4{ 1.0f }, glm::vec3{ object.position[0], object.position[1], object.position[2] }) *
			glm::mat4_cast(rotation) * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ object.scale[0], object.scale[1], object.scale[2] });
	}
} // namespace

// Public Static Methods
// ---------------------
const char* Scene::get_shape_type_name(Shape_Type type)
{
	switch (type)
	{
		case Shape_Type::CUBE: return "Cube";
		case Shape_Type::SPHERE: return "Sphere";
		case Shape_Type::CYLINDER: return "Cylinder";
	}
	return "Unknown";
}

Scene Scene::make_default_scene()
{
	Scene scene;
	scene.add_object(make_object(
		"Water Sphere",
		Shape_Type::SPHERE,
		Shape_Factory::make_sphere(4.0f),
		{ -9.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		"Water"));
	scene.add_object(make_object(
		"Aluminium Cube",
		Shape_Type::CUBE,
		Shape_Factory::make_cube(6.0f),
		{ -2.0f, 0.0f, 0.0f },
		{ 0.0f, 25.0f, 0.0f },
		"Aluminium"));
	scene.add_object(make_object(
		"Bone Cylinder",
		Shape_Type::CYLINDER,
		Shape_Factory::make_cylinder(2.0f, 10.0f),
		{ 5.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		"Bone (calcium)"));
	scene.add_object(make_object(
		"Iron Sphere",
		Shape_Type::SPHERE,
		Shape_Factory::make_sphere(1.5f),
		{ 10.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		"Iron"));
	return scene;
}

// Public Methods
// --------------
std::vector<Xray_Mesh> Scene::make_xray_meshes() const
{
	std::vector<Xray_Mesh> meshes;
	meshes.reserve(objects.size());

	for (const Scene_Object& object : objects)
	{
		const glm::mat4 model = make_model_matrix(object);

		Xray_Mesh mesh;
		mesh.name = object.name;
		mesh.positions.reserve(object.shape.get_vertex_count() * 3);
		for (std::size_t i = 0; i < object.shape.get_vertex_count(); ++i)
		{
			const float*    vertex = &object.shape.vertices[i * Shape_Data::FLOATS_PER_VERTEX];
			const glm::vec3 world  = glm::vec3{ model * glm::vec4{ vertex[0], vertex[1], vertex[2], 1.0f } } * MILLIMETRES_PER_UNIT;
			mesh.positions.insert(mesh.positions.end(), { world.x, world.y, world.z });
		}
		mesh.indices = object.shape.indices;

		const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
		mesh.material                                = materials[object.material_index < materials.size() ? object.material_index : 0].xray;

		meshes.push_back(std::move(mesh));
	}

	return meshes;
}
