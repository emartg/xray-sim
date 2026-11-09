/*
 * Scene.cpp
 * This file implements the Scene class, which describes what the simulator shows.
 */

#include "scene/Scene.h"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

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

	// Returns the transform of an object for the X-ray library: the model matrix with the translation in millimetres, as the vertices
	// that it moves are
	glm::mat4 make_xray_model_matrix(const Scene_Object& object)
	{
		const glm::quat rotation{ glm::radians(glm::vec3{ object.rotation[0], object.rotation[1], object.rotation[2] }) };
		return glm::translate(
				   glm::mat4{ 1.0f },
				   glm::vec3{ object.position[0], object.position[1], object.position[2] } * Scene::MILLIMETRES_PER_UNIT) *
			glm::mat4_cast(rotation) * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ object.scale[0], object.scale[1], object.scale[2] });
	}

	// Puts a value within its limits (a value that is not a number goes to the lower limit)
	float clamp_value(float value, float lower, float upper)
	{
		if (!(value >= lower))
			return lower;
		return value > upper ? upper : value;
	}

	std::array<float, 3> clamp_values(const std::array<float, 3>& values, float lower, float upper)
	{
		return { clamp_value(values[0], lower, upper), clamp_value(values[1], lower, upper), clamp_value(values[2], lower, upper) };
	}

	// Returns the radius of the sphere around the position of an object that contains it: the farthest vertex times the largest scale
	float get_bounding_radius(const Scene_Object& object)
	{
		float farthest{ 0.0f };
		for (std::size_t i = 0; i < object.shape.get_vertex_count(); ++i)
		{
			const float* vertex = &object.shape.vertices[i * Shape_Data::FLOATS_PER_VERTEX];
			farthest            = std::max(farthest, glm::length(glm::vec3{ vertex[0], vertex[1], vertex[2] }));
		}
		return farthest * std::max({ object.scale[0], object.scale[1], object.scale[2] });
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
void Scene::add_object(Scene_Object object)
{
	objects.push_back(std::move(object));
	++revision;
}

bool Scene::set_object_material(std::size_t object_index, std::size_t material_index)
{
	if (object_index >= objects.size() || material_index >= Scene_Material_Table::get_materials().size())
		return false;

	objects[object_index].material_index = material_index;
	++revision;
	return true;
}

bool Scene::remove_object(std::size_t object_index)
{
	if (object_index >= objects.size())
		return false;

	objects.erase(objects.begin() + static_cast<std::ptrdiff_t>(object_index));
	++revision;
	return true;
}

bool Scene::set_object_position(std::size_t object_index, const std::array<float, 3>& position)
{
	if (object_index >= objects.size())
		return false;

	const std::array<float, 3> clamped = clamp_values(position, MIN_POSITION, MAX_POSITION);
	if (objects[object_index].position != clamped)
	{
		objects[object_index].position = clamped;
		++revision;
	}
	return true;
}

bool Scene::set_object_rotation(std::size_t object_index, const std::array<float, 3>& rotation)
{
	if (object_index >= objects.size())
		return false;

	const std::array<float, 3> clamped = clamp_values(rotation, MIN_ROTATION, MAX_ROTATION);
	if (objects[object_index].rotation != clamped)
	{
		objects[object_index].rotation = clamped;
		++revision;
	}
	return true;
}

bool Scene::set_object_scale(std::size_t object_index, const std::array<float, 3>& scale)
{
	if (object_index >= objects.size())
		return false;

	const std::array<float, 3> clamped = clamp_values(scale, MIN_SCALE, MAX_SCALE);
	if (objects[object_index].scale != clamped)
	{
		objects[object_index].scale = clamped;
		++revision;
	}
	return true;
}

std::vector<Xray_Mesh> Scene::make_xray_local_meshes() const
{
	std::vector<Xray_Mesh> meshes;
	meshes.reserve(objects.size());

	const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
	for (const Scene_Object& object : objects)
	{
		Xray_Mesh mesh;
		mesh.name = object.name;
		mesh.positions.reserve(object.shape.get_vertex_count() * 3);
		for (std::size_t i = 0; i < object.shape.get_vertex_count(); ++i)
		{
			const float* vertex = &object.shape.vertices[i * Shape_Data::FLOATS_PER_VERTEX];
			mesh.positions.insert(
				mesh.positions.end(),
				{ vertex[0] * MILLIMETRES_PER_UNIT, vertex[1] * MILLIMETRES_PER_UNIT, vertex[2] * MILLIMETRES_PER_UNIT });
		}
		mesh.indices  = object.shape.indices;
		mesh.material = materials[object.material_index < materials.size() ? object.material_index : 0].xray;

		meshes.push_back(std::move(mesh));
	}

	return meshes;
}

std::array<float, 16> Scene::get_xray_transform(std::size_t object_index) const
{
	std::array<float, 16> transform;
	const glm::mat4       matrix = object_index < objects.size() ? make_xray_model_matrix(objects[object_index]) : glm::mat4{ 1.0f };
	std::copy_n(glm::value_ptr(matrix), transform.size(), transform.begin());
	return transform;
}

const Scene_Object* Scene::find_object_around_point(const std::array<float, 3>& point) const
{
	const glm::vec3 position{ point[0], point[1], point[2] };
	for (const Scene_Object& object : objects)
	{
		const glm::vec3 centre{ object.position[0], object.position[1], object.position[2] };
		if (glm::length(position - centre) < get_bounding_radius(object))
			return &object;
	}
	return nullptr;
}

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
