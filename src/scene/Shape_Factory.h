/*
 * Shape_Factory.h
 * This file defines the Shape_Factory class, which creates the geometry of the simple shapes of the simulator's scenes
 * (a cube, a sphere, and a cylinder), centred at the origin. Every shape is a closed surface whose triangles are
 * counter-clockwise seen from outside, which is what the X-ray library needs to measure the thickness of an object.
 * It has no dependency on OpenGL, on the Engine, or on the X-ray library.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "scene/Shape_Data.h"

class Shape_Factory
{
public:
	// Public Static Methods
	// ---------------------
	// Creates a cube of the given side (24 vertices, so that every face has its own normal vectors)
	static Shape_Data make_cube(float side);

	// Creates a sphere of the given radius, made of the given number of slices (around the vertical axis) and stacks
	// (from pole to pole)
	static Shape_Data make_sphere(float radius, unsigned int slices = 32, unsigned int stacks = 16);

	// Creates a cylinder of the given radius and height, with its axis along the vertical axis and the given number
	// of slices around it
	static Shape_Data make_cylinder(float radius, float height, unsigned int slices = 32);

	// Returns the signed volume enclosed by triangles whose vertices are stored with the given number of floats each
	// (the position comes first). It is positive for a closed surface with its triangles counter-clockwise seen from
	// outside, which makes it a check of the orientation and of the closure of a shape
	static double
	compute_signed_volume(const std::vector<float>& vertices, unsigned int floats_per_vertex, const std::vector<std::uint32_t>& indices);
};
