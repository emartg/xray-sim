/*
 * Scene.h
 * This file defines the Scene class, which describes what the simulator shows: a list of objects, each with a shape, a
 * transform, and a material. The Engine draws it with its forward rendering, and the X-ray library images it, both
 * from the same description, so that they cannot disagree. It has no dependency on OpenGL, on the Engine, or on the
 * X-ray library (it only uses the plain types of the X-ray adapter).
 * The unit of length of a scene is the centimetre.
 */

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "scene/Shape_Data.h"
#include "xray/Xray_Types.h"

enum class Shape_Type
{
	CUBE,
	SPHERE,
	CYLINDER
};

struct Scene_Object
{
	std::string          name;
	Shape_Type           type{ Shape_Type::CUBE };
	Shape_Data           shape;                        // geometry, in the local space of the object
	std::array<float, 3> position{ 0.0f, 0.0f, 0.0f }; // in centimetres
	std::array<float, 3> rotation{ 0.0f, 0.0f, 0.0f }; // Euler angles in degrees (the Engine's convention)
	std::array<float, 3> scale{ 1.0f, 1.0f, 1.0f };
	std::size_t          material_index{ 0 }; // in Scene_Material_Table
};

class Scene
{
public:
	// Public Constants
	// ----------------
	static constexpr float MILLIMETRES_PER_UNIT{ 10.0f }; // the unit of a scene is the centimetre

	// Public Static Methods
	// ---------------------
	// Returns the name of a shape type ("Cube", "Sphere", or "Cylinder")
	static const char* get_shape_type_name(Shape_Type type);

	// Creates the default scene: a sphere of water, a rotated cube of aluminium, a cylinder of bone, and a small
	// sphere of iron, side by side
	static Scene make_default_scene();

	// Public Methods
	// --------------
	// Adds an object to the scene
	void add_object(Scene_Object object) { objects.push_back(std::move(object)); }

	// Returns the triangle meshes of the objects for the X-ray library, in millimetres, with the transform of each
	// object applied to its vertices and the X-ray material of its material
	std::vector<Xray_Mesh> make_xray_meshes() const;

	// Getters
	const std::vector<Scene_Object>& get_objects() const { return objects; }

private:
	// Private Attributes
	// ------------------
	std::vector<Scene_Object> objects;
};
