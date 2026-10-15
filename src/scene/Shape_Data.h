/*
 * Shape_Data.h
 * This file defines the Shape_Data struct, which holds the geometry of a shape as the Engine expects it
 * (interleaved vertices) and as the X-ray library needs it (triangles with the outward orientation).
 * The same arrays feed the Engine's nodes and the X-ray meshes, so that what is drawn and what is imaged cannot differ.
 */

#pragma once

#include <cstdint>
#include <vector>

struct Shape_Data
{
	// Number of floats of a vertex: the position (3), the normal vector (3), and the texture coordinates (2)
	static constexpr unsigned int FLOATS_PER_VERTEX{ 8 };

	std::vector<float>         vertices; // interleaved: x, y, z, nx, ny, nz, u, v
	std::vector<std::uint32_t> indices;  // three indices per triangle, counter-clockwise seen from outside

	// Returns the number of vertices
	std::size_t get_vertex_count() const { return vertices.size() / FLOATS_PER_VERTEX; }
};
