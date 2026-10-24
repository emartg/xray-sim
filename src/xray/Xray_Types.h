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

// Times that the adapter spent in its calls, in milliseconds. The times of the guard, of the initialisation, and of the meshes are
// added up since the adapter was created, so for an adapter that is created for each image they are the cost of that image. The
// times of the computation and of the read-back are the ones of the last image
struct Xray_Timings
{
	double guard_ms{ 0.0 }; // saving, setting, and restoring the state of OpenGL in all the calls (it is not part of the last two times)
	double initialise_ms{ 0.0 };          // the whole initialise call
	double glew_ms{ 0.0 };                // loading the OpenGL functions of the library (part of the initialisation)
	double renderer_initialise_ms{ 0.0 }; // the initialisation of the renderer: shaders, framebuffers, and textures (part of it too)
	double add_meshes_ms{ 0.0 };          // all the add_mesh calls
	unsigned int mesh_count{ 0 };         // the meshes that were added
	double       computation_ms{ 0.0 };   // the computation of the last image, until the GPU has finished
	double       read_back_ms{ 0.0 };     // the copy of the last image from the GPU to the memory
};

// X-ray image: the energy fluence of each pixel, row by row (the first row is the bottom of the detector)
struct Xray_Image
{
	unsigned int       width{ 0 };
	unsigned int       height{ 0 };
	std::vector<float> pixels;
};
