/*
 * Scene_Test.cpp
 * This file is the test of the scene library: the shapes (closed surfaces with outward triangles and the volume they
 * should have), the table of materials, the default scene with its conversion to X-ray meshes (the units and the
 * transforms), and the changes that the panel makes (a material, a transform, a removal, and the bounding spheres). It needs
 * no OpenGL context. It returns an error code if a check fails, and prints each failure with the format of the Engine's messages.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <numbers>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "scene/Scene.h"
#include "scene/Scene_Material_Table.h"
#include "scene/Shape_Factory.h"

namespace
{
	int failures{ 0 };

	void check(bool condition, const std::string& what)
	{
		if (!condition)
		{
			++failures;
			std::cerr << "[ERROR::SCENE_TEST::check] " << what << std::endl;
		}
	}

	bool is_near(double value, double expected, double relative_tolerance)
	{
		return std::abs(value - expected) <= relative_tolerance * std::abs(expected);
	}

	// Checks the indices, the normal vectors, and that the surface is closed (every edge is used once in each direction)
	void check_shape(const Shape_Data& shape, const std::string& name)
	{
		check(shape.vertices.size() % Shape_Data::FLOATS_PER_VERTEX == 0, name + ": the vertices are not in groups of eight floats");
		check(!shape.indices.empty() && shape.indices.size() % 3 == 0, name + ": the indices are not in groups of three");

		for (std::uint32_t index : shape.indices) check(index < shape.get_vertex_count(), name + ": an index is outside of the vertices");

		for (std::size_t i = 0; i < shape.get_vertex_count(); ++i)
		{
			const float* vertex = &shape.vertices[i * Shape_Data::FLOATS_PER_VERTEX];
			const double length = std::sqrt(
				static_cast<double>(vertex[3]) * vertex[3] + static_cast<double>(vertex[4]) * vertex[4] +
				static_cast<double>(vertex[5]) * vertex[5]);
			check(is_near(length, 1.0, 1e-4), name + ": a normal vector is not a unit vector");
		}

		// the vertices that share a position (the copies of the faces and of the seam) are the same point
		const auto key = [&shape](std::uint32_t index) {
			const float* vertex = &shape.vertices[static_cast<std::size_t>(index) * Shape_Data::FLOATS_PER_VERTEX];
			return std::make_tuple(std::lround(vertex[0] * 1000.0f), std::lround(vertex[1] * 1000.0f), std::lround(vertex[2] * 1000.0f));
		};
		std::map<std::pair<std::tuple<long, long, long>, std::tuple<long, long, long>>, int> directed_edges;
		for (std::size_t i = 0; i + 2 < shape.indices.size(); i += 3)
		{
			for (int corner = 0; corner < 3; ++corner)
				++directed_edges[{ key(shape.indices[i + corner]), key(shape.indices[i + (corner + 1) % 3]) }];
		}
		bool is_closed{ true };
		for (const auto& [edge, count] : directed_edges)
		{
			const auto opposite = directed_edges.find({ edge.second, edge.first });
			if (count != 1 || opposite == directed_edges.end() || opposite->second != 1)
				is_closed = false;
		}
		check(is_closed, name + ": the surface is not closed (an edge is not shared by two triangles with opposite directions)");
	}

	double volume_of(const Shape_Data& shape)
	{
		return Shape_Factory::compute_signed_volume(shape.vertices, Shape_Data::FLOATS_PER_VERTEX, shape.indices);
	}

	// Returns the x coordinate of the centre of the volume enclosed by the triangles (positions, three floats per vertex): the
	// average of the centres of the tetrahedra that join the triangles to the origin, weighted by their signed volumes
	double compute_centroid_x(const std::vector<float>& positions, const std::vector<std::uint32_t>& indices)
	{
		double weighted_x{ 0.0 }, volume{ 0.0 };
		for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
		{
			const float* a           = &positions[static_cast<std::size_t>(indices[i]) * 3];
			const float* b           = &positions[static_cast<std::size_t>(indices[i + 1]) * 3];
			const float* c           = &positions[static_cast<std::size_t>(indices[i + 2]) * 3];
			const double tetrahedron = (static_cast<double>(a[0]) * (static_cast<double>(b[1]) * c[2] - static_cast<double>(b[2]) * c[1]) -
										static_cast<double>(a[1]) * (static_cast<double>(b[0]) * c[2] - static_cast<double>(b[2]) * c[0]) +
										static_cast<double>(a[2]) * (static_cast<double>(b[0]) * c[1] - static_cast<double>(b[1]) * c[0])) /
				6.0;
			weighted_x += tetrahedron * (static_cast<double>(a[0]) + b[0] + c[0]) / 4.0;
			volume += tetrahedron;
		}
		return weighted_x / volume;
	}

	void test_shapes()
	{
		constexpr double PI{ std::numbers::pi };

		const Shape_Data cube = Shape_Factory::make_cube(6.0f);
		check_shape(cube, "cube");
		check(cube.get_vertex_count() == 24 && cube.indices.size() == 36, "cube: it should have 24 vertices and 12 triangles");
		check(is_near(volume_of(cube), 216.0, 1e-4), "cube: the volume of a cube of 6 cm should be 216 cm3");

		const unsigned int slices{ 32 }, stacks{ 16 };
		const Shape_Data   sphere = Shape_Factory::make_sphere(4.0f, slices, stacks);
		check_shape(sphere, "sphere");
		const double sphere_volume = volume_of(sphere);
		check(
			sphere_volume > 0.97 * (4.0 / 3.0) * PI * 64.0 && sphere_volume < (4.0 / 3.0) * PI * 64.0,
			"sphere: the volume should be a little below 4/3 pi r3 (an inscribed polyhedron)");

		const Shape_Data cylinder = Shape_Factory::make_cylinder(2.0f, 10.0f, slices);
		check_shape(cylinder, "cylinder");
		const double polygon_area = 0.5 * slices * 4.0 * std::sin(2.0 * PI / slices);
		check(
			is_near(volume_of(cylinder), 10.0 * polygon_area, 1e-4),
			"cylinder: the volume should be the height times the area of the polygon of its section");
	}

	void test_materials()
	{
		const std::vector<Scene_Material>& materials = Scene_Material_Table::get_materials();
		check(materials.size() >= 4, "materials: there should be at least four");
		check(Scene_Material_Table::find("Water") == 0, "materials: water should be the first");
		check(
			Scene_Material_Table::find("Unobtainium") == materials.size(),
			"materials: an unknown name should give the number of materials");

		std::set<std::string> names;
		for (const Scene_Material& material : materials)
		{
			names.insert(material.name);
			check(
				!material.xray.compound.empty() && material.xray.density > 0.0,
				"materials: " + material.name + " has no compound or density");
		}
		check(names.size() == materials.size(), "materials: the names should be unique");
	}

	void test_default_scene()
	{
		const Scene scene = Scene::make_default_scene();
		check(scene.get_objects().size() == 4, "scene: the default scene should have four objects");

		std::set<std::string> names;
		for (const Scene_Object& object : scene.get_objects())
		{
			names.insert(object.name);
			check(
				object.material_index < Scene_Material_Table::get_materials().size(),
				"scene: " + object.name + " has an invalid material");
		}
		check(names.size() == scene.get_objects().size(), "scene: the names of the objects should be unique");

		// the X-ray meshes are in millimetres, with the transforms applied
		const std::vector<Xray_Mesh> meshes = scene.make_xray_meshes();
		check(meshes.size() == scene.get_objects().size(), "scene: there should be one X-ray mesh per object");
		for (std::size_t i = 0; i < meshes.size(); ++i)
		{
			const Scene_Object& object = scene.get_objects()[i];
			const Xray_Mesh&    mesh   = meshes[i];

			// a rotation and a translation keep the volume: cm3 to mm3 is a factor of 1000
			const double volume = Shape_Factory::compute_signed_volume(mesh.positions, 3, mesh.indices);
			check(
				is_near(volume, volume_of(object.shape) * 1000.0, 1e-3),
				"scene: the volume of the X-ray mesh of " + object.name + " should be the one of its shape in mm3");

			// the centre of the volume is on the position of the object, in millimetres (every shape is centred on its origin)
			check(
				std::abs(compute_centroid_x(mesh.positions, mesh.indices) - object.position[0] * 10.0) < 0.05,
				"scene: the X-ray mesh of " + object.name + " is not at its position (in millimetres)");

			check(
				mesh.material.compound == Scene_Material_Table::get_materials()[object.material_index].xray.compound,
				"scene: the X-ray material of " + object.name + " is wrong");
		}
	}

	// Checks the changes of the scene that the panel makes: the material of an object, and the removal of an object
	void test_scene_changes()
	{
		Scene scene = Scene::make_default_scene();
		check(scene.get_revision() > 0, "changes: a scene with objects should have a revision above zero");

		const std::size_t aluminium = Scene_Material_Table::find("Aluminium");
		check(aluminium < Scene_Material_Table::get_materials().size(), "changes: the table should have aluminium");

		// a new material changes the object, the revision, and the X-ray mesh
		std::uint64_t revision = scene.get_revision();
		check(scene.set_object_material(0, aluminium), "changes: a valid change of material should succeed");
		check(scene.get_objects()[0].material_index == aluminium, "changes: the object should have the new material");
		check(scene.get_revision() > revision, "changes: a change of material should raise the revision");
		check(
			scene.make_xray_meshes()[0].material.compound == Scene_Material_Table::get_materials()[aluminium].xray.compound,
			"changes: the X-ray mesh should take the new material");

		// an object or a material that does not exist changes nothing
		revision                        = scene.get_revision();
		const std::size_t material_here = scene.get_objects()[1].material_index;
		check(!scene.set_object_material(99, aluminium), "changes: a material of an object that does not exist should be refused");
		check(
			!scene.set_object_material(1, Scene_Material_Table::get_materials().size()),
			"changes: a material that does not exist should be refused");
		check(scene.get_objects()[1].material_index == material_here, "changes: a refused change should change nothing");
		check(scene.get_revision() == revision, "changes: a refused change should not raise the revision");

		// the removal moves the other objects down
		const std::string second_name = scene.get_objects()[1].name;
		const std::size_t count       = scene.get_objects().size();
		check(scene.remove_object(0), "changes: removing an object should succeed");
		check(scene.get_objects().size() == count - 1, "changes: the scene should have one object less");
		check(scene.get_objects()[0].name == second_name, "changes: the next object should take the place of the removed one");
		check(scene.get_revision() > revision, "changes: a removal should raise the revision");
		revision = scene.get_revision();
		check(!scene.remove_object(count), "changes: removing an object that does not exist should be refused");
		check(scene.get_revision() == revision, "changes: a refused removal should not raise the revision");
	}

	// Checks the changes of the transform of an object (the limits, the refusals, the revision), the transform that the X-ray library
	// applies to the local meshes (it must give the world meshes), and the bounding spheres that keep the tube outside of the objects
	void test_scene_transforms()
	{
		Scene scene = Scene::make_default_scene();

		// a value within the limits is set and raises the revision; the same value again changes nothing
		std::uint64_t revision = scene.get_revision();
		check(scene.set_object_position(0, { 3.0f, -4.0f, 5.0f }), "transform: a valid position should be accepted");
		check(
			scene.get_objects()[0].position == std::array<float, 3>{ 3.0f, -4.0f, 5.0f },
			"transform: the object should have the position");
		check(scene.get_revision() > revision, "transform: a new position should raise the revision");
		revision = scene.get_revision();
		check(scene.set_object_position(0, { 3.0f, -4.0f, 5.0f }), "transform: the same position should be accepted");
		check(scene.get_revision() == revision, "transform: the same position should not raise the revision");

		// the values are put within their limits (a value that is not a number goes to the lower limit)
		scene.set_object_position(0, { 100.0f, -100.0f, std::nanf("") });
		check(
			scene.get_objects()[0].position == std::array<float, 3>{ Scene::MAX_POSITION, Scene::MIN_POSITION, Scene::MIN_POSITION },
			"transform: the position should be put within its limits");
		scene.set_object_rotation(0, { 400.0f, -400.0f, 90.0f });
		check(
			scene.get_objects()[0].rotation == std::array<float, 3>{ Scene::MAX_ROTATION, Scene::MIN_ROTATION, 90.0f },
			"transform: the rotation should be put within its limits");
		scene.set_object_scale(0, { 0.0f, 100.0f, -1.0f });
		check(
			scene.get_objects()[0].scale == std::array<float, 3>{ Scene::MIN_SCALE, Scene::MAX_SCALE, Scene::MIN_SCALE },
			"transform: the scale should be put within its limits, and stay positive");

		// an object that does not exist is refused
		revision = scene.get_revision();
		check(
			!scene.set_object_position(99, { 0.0f, 0.0f, 0.0f }),
			"transform: the position of an object that does not exist should be refused");
		check(
			!scene.set_object_rotation(99, { 0.0f, 0.0f, 0.0f }),
			"transform: the rotation of an object that does not exist should be refused");
		check(!scene.set_object_scale(99, { 1.0f, 1.0f, 1.0f }), "transform: the scale of an object that does not exist should be refused");
		check(scene.get_revision() == revision, "transform: a refused change should not raise the revision");

		// the transform of the X-ray library moves the local meshes (millimetres) to the world meshes
		scene.set_object_position(1, { 4.0f, -3.0f, 2.0f });
		scene.set_object_rotation(1, { 20.0f, 30.0f, 40.0f });
		scene.set_object_scale(1, { 1.5f, 0.5f, 2.0f });
		const std::vector<Xray_Mesh> local_meshes = scene.make_xray_local_meshes();
		const std::vector<Xray_Mesh> world_meshes = scene.make_xray_meshes();
		check(local_meshes.size() == world_meshes.size(), "transform: there should be one local mesh per object");
		for (std::size_t i = 0; i < local_meshes.size() && i < world_meshes.size(); ++i)
		{
			const std::array<float, 16> m = scene.get_xray_transform(i);
			double                      largest_difference{ 0.0 };
			for (std::size_t j = 0; j + 2 < local_meshes[i].positions.size(); j += 3)
			{
				const double x = local_meshes[i].positions[j];
				const double y = local_meshes[i].positions[j + 1];
				const double z = local_meshes[i].positions[j + 2];
				largest_difference =
					std::max(largest_difference, std::abs(m[0] * x + m[4] * y + m[8] * z + m[12] - world_meshes[i].positions[j]));
				largest_difference =
					std::max(largest_difference, std::abs(m[1] * x + m[5] * y + m[9] * z + m[13] - world_meshes[i].positions[j + 1]));
				largest_difference =
					std::max(largest_difference, std::abs(m[2] * x + m[6] * y + m[10] * z + m[14] - world_meshes[i].positions[j + 2]));
			}
			check(
				largest_difference < 1e-3,
				"transform: the transform of " + scene.get_objects()[i].name + " should move its local mesh to its world mesh");
		}
		const std::array<float, 16> identity{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
		check(scene.get_xray_transform(99) == identity, "transform: the transform of an object that does not exist should be the identity");

		// the bounding sphere of an object (its farthest vertex times the largest scale) contains its surroundings, and nothing else
		Scene        single;
		Scene_Object object = Scene::make_default_scene().get_objects()[0]; // the water sphere of radius 4 cm at (-9, 0, 0)
		object.position     = { 0.0f, 0.0f, 0.0f };
		single.add_object(object);
		check(single.find_object_around_point({ 3.9f, 0.0f, 0.0f }) != nullptr, "bounds: a point inside the radius should be found");
		check(single.find_object_around_point({ 4.2f, 0.0f, 0.0f }) == nullptr, "bounds: a point outside the radius should not be found");
		check(single.find_object_around_point({ 0.0f, 0.0f, -100.0f }) == nullptr, "bounds: a far point should not be found");
		single.set_object_scale(0, { 2.0f, 1.0f, 1.0f });
		check(single.find_object_around_point({ 7.9f, 0.0f, 0.0f }) != nullptr, "bounds: the largest scale should enlarge the sphere");
		check(
			single.find_object_around_point({ 0.0f, 0.0f, 0.0f })->name == object.name,
			"bounds: the object found should be the one around the point");

		// a rectangle (the detector plate) touches the sphere through its inside, its edge, or its corner, not only through its centre
		single.set_object_scale(0, { 1.0f, 1.0f, 1.0f }); // radius 4 cm at the origin
		const std::array<float, 3> right{ 1.0f, 0.0f, 0.0f };
		const std::array<float, 3> up{ 0.0f, 1.0f, 0.0f };
		check(
			single.find_object_touching_rectangle({ 0.0f, 0.0f, 3.0f }, right, up, 1.0f, 1.0f) != nullptr,
			"bounds: a plate through the sphere touches it");
		check(
			single.find_object_touching_rectangle({ 0.0f, 0.0f, 4.5f }, right, up, 20.0f, 10.0f) == nullptr,
			"bounds: a plate in front of the sphere does not touch it");
		check(
			single.find_object_touching_rectangle({ 20.0f, 0.0f, 0.0f }, right, up, 17.0f, 5.0f) != nullptr,
			"bounds: a plate whose centre is outside of the sphere touches it with its side");
		check(
			single.find_object_touching_rectangle({ 20.0f, 0.0f, 0.0f }, right, up, 15.0f, 5.0f) == nullptr,
			"bounds: a plate that stops short of the sphere does not touch it");
		check(
			single.find_object_touching_rectangle({ 6.0f, 6.0f, 0.0f }, right, up, 3.5f, 3.5f) != nullptr,
			"bounds: a plate touches the sphere with its corner");
	}
} // namespace

int main()
{
	test_shapes();
	test_materials();
	test_default_scene();
	test_scene_changes();
	test_scene_transforms();

	if (failures != 0)
	{
		std::cerr << "[ERROR::SCENE_TEST::main] " << failures << " checks failed" << std::endl;
		return 1;
	}

	std::cout << "[SUCCESS::SCENE_TEST::main] The shapes, the materials, the default scene, and the changes of the scene are valid"
			  << std::endl;
	return 0;
}
