/*
 * Shape_Factory.cpp
 * This file implements the Shape_Factory class, which creates the geometry of the simple shapes of the simulator's scenes.
 */

#include "scene/Shape_Factory.h"

#include <cmath>
#include <numbers>

namespace
{
	// Adds a vertex (position, normal vector, and texture coordinates) to the shape
	void add_vertex(Shape_Data& shape, float x, float y, float z, float nx, float ny, float nz, float u, float v)
	{
		shape.vertices.insert(shape.vertices.end(), { x, y, z, nx, ny, nz, u, v });
	}

	// Adds the two triangles of a quad whose corners a, b, c, d are counter-clockwise seen from outside
	void add_quad(Shape_Data& shape, std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d)
	{
		shape.indices.insert(shape.indices.end(), { a, b, c, a, c, d });
	}
} // namespace

// Public Static Methods
// ---------------------
Shape_Data Shape_Factory::make_cube(float side)
{
	const float h{ side / 2.0f };

	Shape_Data shape;

	// each face: its normal vector, and its four corners counter-clockwise seen from outside
	struct Face
	{
		float nx, ny, nz;
		float corners[4][3];
	};
	const Face faces[6]{
		{ 0.0f, 0.0f, 1.0f, { { -h, -h, h }, { h, -h, h }, { h, h, h }, { -h, h, h } } },      // front (+z)
		{ 0.0f, 0.0f, -1.0f, { { h, -h, -h }, { -h, -h, -h }, { -h, h, -h }, { h, h, -h } } }, // back (-z)
		{ -1.0f, 0.0f, 0.0f, { { -h, -h, -h }, { -h, -h, h }, { -h, h, h }, { -h, h, -h } } }, // left (-x)
		{ 1.0f, 0.0f, 0.0f, { { h, -h, h }, { h, -h, -h }, { h, h, -h }, { h, h, h } } },      // right (+x)
		{ 0.0f, 1.0f, 0.0f, { { -h, h, h }, { h, h, h }, { h, h, -h }, { -h, h, -h } } },      // top (+y)
		{ 0.0f, -1.0f, 0.0f, { { -h, -h, -h }, { h, -h, -h }, { h, -h, h }, { -h, -h, h } } }, // bottom (-y)
	};
	const float uv[4][2]{ { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

	for (const Face& face : faces)
	{
		const auto first = static_cast<std::uint32_t>(shape.get_vertex_count());
		for (int i = 0; i < 4; ++i)
			add_vertex(shape, face.corners[i][0], face.corners[i][1], face.corners[i][2], face.nx, face.ny, face.nz, uv[i][0], uv[i][1]);
		add_quad(shape, first, first + 1, first + 2, first + 3);
	}

	return shape;
}

Shape_Data Shape_Factory::make_sphere(float radius, unsigned int slices, unsigned int stacks)
{
	constexpr float PI{ std::numbers::pi_v<float> };

	Shape_Data shape;

	// the rings of vertices, from the north pole (stack 0) to the south pole (the last stack), each with one extra
	// vertex that repeats the first one, so that the texture coordinates can wrap around
	for (unsigned int stack = 0; stack <= stacks; ++stack)
	{
		const float polar_angle = PI * static_cast<float>(stack) / static_cast<float>(stacks);
		for (unsigned int slice = 0; slice <= slices; ++slice)
		{
			const float azimuth = 2.0f * PI * static_cast<float>(slice) / static_cast<float>(slices);
			const float nx      = std::sin(polar_angle) * std::cos(azimuth);
			const float ny      = std::cos(polar_angle);
			const float nz      = std::sin(polar_angle) * std::sin(azimuth);
			add_vertex(
				shape,
				radius * nx,
				radius * ny,
				radius * nz,
				nx,
				ny,
				nz,
				static_cast<float>(slice) / static_cast<float>(slices),
				1.0f - static_cast<float>(stack) / static_cast<float>(stacks));
		}
	}

	// the quads between two rings (a triangle at each pole is degenerate in one of its two halves: it has no area, and it is skipped)
	const unsigned int ring = slices + 1;
	for (unsigned int stack = 0; stack < stacks; ++stack)
	{
		for (unsigned int slice = 0; slice < slices; ++slice)
		{
			const std::uint32_t north_a = stack * ring + slice, north_b = stack * ring + slice + 1;
			const std::uint32_t south_a = (stack + 1) * ring + slice, south_b = (stack + 1) * ring + slice + 1;
			if (stack != 0)
				shape.indices.insert(shape.indices.end(), { north_a, north_b, south_a });
			if (stack != stacks - 1)
				shape.indices.insert(shape.indices.end(), { north_b, south_b, south_a });
		}
	}

	return shape;
}

Shape_Data Shape_Factory::make_cylinder(float radius, float height, unsigned int slices)
{
	constexpr float PI{ std::numbers::pi_v<float> };
	const float     h{ height / 2.0f };

	Shape_Data shape;

	// the side: two vertices (bottom and top) for each slice, plus one that repeats the first slice
	for (unsigned int slice = 0; slice <= slices; ++slice)
	{
		const float azimuth = 2.0f * PI * static_cast<float>(slice) / static_cast<float>(slices);
		const float nx = std::cos(azimuth), nz = std::sin(azimuth);
		const float u = static_cast<float>(slice) / static_cast<float>(slices);
		add_vertex(shape, radius * nx, -h, radius * nz, nx, 0.0f, nz, u, 0.0f);
		add_vertex(shape, radius * nx, h, radius * nz, nx, 0.0f, nz, u, 1.0f);
	}
	for (unsigned int slice = 0; slice < slices; ++slice)
	{
		const std::uint32_t bottom_a = 2 * slice, top_a = 2 * slice + 1, bottom_b = 2 * slice + 2, top_b = 2 * slice + 3;
		shape.indices.insert(shape.indices.end(), { bottom_a, top_a, top_b, bottom_a, top_b, bottom_b });
	}

	// the two caps: a fan around a centre vertex
	for (const float sign : { 1.0f, -1.0f })
	{
		const auto centre = static_cast<std::uint32_t>(shape.get_vertex_count());
		add_vertex(shape, 0.0f, sign * h, 0.0f, 0.0f, sign, 0.0f, 0.5f, 0.5f);
		for (unsigned int slice = 0; slice <= slices; ++slice)
		{
			const float azimuth = 2.0f * PI * static_cast<float>(slice) / static_cast<float>(slices);
			const float x = std::cos(azimuth), z = std::sin(azimuth);
			add_vertex(shape, radius * x, sign * h, radius * z, 0.0f, sign, 0.0f, 0.5f + 0.5f * x, 0.5f + 0.5f * z);
		}
		for (unsigned int slice = 0; slice < slices; ++slice)
		{
			// the top cap is counter-clockwise seen from above (+y), and the bottom cap seen from below
			if (sign > 0.0f)
				shape.indices.insert(shape.indices.end(), { centre, centre + 1 + slice + 1, centre + 1 + slice });
			else
				shape.indices.insert(shape.indices.end(), { centre, centre + 1 + slice, centre + 1 + slice + 1 });
		}
	}

	return shape;
}

double Shape_Factory::compute_signed_volume(
	const std::vector<float>&         vertices,
	unsigned int                      floats_per_vertex,
	const std::vector<std::uint32_t>& indices)
{
	// the sum of the signed volumes of the tetrahedra that join the triangles to the origin
	double volume{ 0.0 };
	for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const float* a = &vertices[static_cast<std::size_t>(indices[i]) * floats_per_vertex];
		const float* b = &vertices[static_cast<std::size_t>(indices[i + 1]) * floats_per_vertex];
		const float* c = &vertices[static_cast<std::size_t>(indices[i + 2]) * floats_per_vertex];
		volume += (static_cast<double>(a[0]) * (static_cast<double>(b[1]) * c[2] - static_cast<double>(b[2]) * c[1]) -
				   static_cast<double>(a[1]) * (static_cast<double>(b[0]) * c[2] - static_cast<double>(b[2]) * c[0]) +
				   static_cast<double>(a[2]) * (static_cast<double>(b[0]) * c[1] - static_cast<double>(b[1]) * c[0])) /
			6.0;
	}
	return volume;
}
