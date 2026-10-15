/*
 * Scene_Test.cpp
 * This file is the test of the scene library: the shapes (closed surfaces with outward triangles and the volume they
 * should have), the table of materials, and the default scene with its conversion to X-ray meshes (the units and the
 * transforms). It needs no OpenGL context. It returns an error code if a check fails, and prints each failure with the
 * format of the Engine's messages.
 */

#include <array>
#include <cmath>
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
} // namespace

int main()
{
	test_shapes();
	test_materials();
	test_default_scene();

	if (failures != 0)
	{
		std::cerr << "[ERROR::SCENE_TEST::main] " << failures << " checks failed" << std::endl;
		return 1;
	}

	std::cout << "[SUCCESS::SCENE_TEST::main] The shapes, the materials, and the default scene are valid" << std::endl;
	return 0;
}
