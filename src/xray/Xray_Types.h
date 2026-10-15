/*
 * Xray_Types.h
 * This file defines the plain data types that the simulator uses to talk to the X-ray library (the adapter of
 * gVirtualXRay): the material, mesh, beam, detector, and image structs. They only use standard types, so that
 * this header can be included by any part of the simulator without the headers, definitions, or OpenGL loader
 * of gVirtualXRay or of the Engine.
 * All the lengths are in millimetres, the energies in keV, and the densities in g/cm3.
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Material of a mesh: a chemical compound or an element by its name (e.g., "H2O", "Al", "Fe"), and its density
struct Xray_Material
{
	std::string compound{ "H2O" };
	double      density{ 1.0 }; // in g/cm3
};

// Triangle mesh of an object of the scene, in the world space of the X-ray scene
struct Xray_Mesh
{
	std::string                name;
	std::vector<float>         positions; // x, y, z of each vertex, one after the other
	std::vector<std::uint32_t> indices;   // three indices per triangle
	Xray_Material              material;
};

// Monochromatic point source: its position, and the energy and the number of photons of its beam
struct Xray_Beam
{
	std::array<double, 3> source_position{ 0.0, 0.0, -1000.0 };
	double                energy{ 80.0 };              // in keV
	double                photons_per_pixel{ 1000.0 }; // photons per pixel of the detector, at the detector
};

// Flat detector: the position of its centre, its up vector, and its size
struct Xray_Detector
{
	std::array<double, 3> position{ 0.0, 0.0, 500.0 };
	std::array<double, 3> up_vector{ 0.0, 1.0, 0.0 };
	unsigned int          width_in_pixels{ 128 };
	unsigned int          height_in_pixels{ 128 };
	double                pixel_size{ 2.0 }; // side of a pixel, in millimetres
};

// X-ray image: the energy fluence of each pixel, row by row (the first row is the bottom of the detector)
struct Xray_Image
{
	unsigned int       width{ 0 };
	unsigned int       height{ 0 };
	std::vector<float> pixels;
};
