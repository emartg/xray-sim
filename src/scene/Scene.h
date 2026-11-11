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
#include <cstdint>
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

	// The limits of the transform of an object. They only keep the scene valid (the objects stay near the tube, and a scale is positive:
	// a negative one would turn the triangles inside out); they carry no clinical meaning
	static constexpr float MIN_POSITION{ -50.0f }, MAX_POSITION{ 50.0f };   // on each axis, in centimetres
	static constexpr float MIN_ROTATION{ -180.0f }, MAX_ROTATION{ 180.0f }; // on each axis, in degrees
	static constexpr float MIN_SCALE{ 0.1f }, MAX_SCALE{ 5.0f };            // on each axis

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
	void add_object(Scene_Object object);

	// Changes the material of an object. Returns false, and changes nothing, if the object or the material does not exist
	bool set_object_material(std::size_t object_index, std::size_t material_index);

	// Removes an object (the ones after it move one place down). Returns false, and changes nothing, if it does not exist
	bool remove_object(std::size_t object_index);

	// Change the position (in centimetres), the rotation (Euler angles in degrees), or the scale of an object. Every value is put
	// within its limits first (a value that is not a number goes to the lower limit). They return false, and change nothing, if the
	// object does not exist. The revision is raised only if the value changes
	bool set_object_position(std::size_t object_index, const std::array<float, 3>& position);
	bool set_object_rotation(std::size_t object_index, const std::array<float, 3>& rotation);
	bool set_object_scale(std::size_t object_index, const std::array<float, 3>& scale);

	// Returns the triangle meshes of the objects for the X-ray library, in millimetres, with the transform of each
	// object applied to its vertices and the X-ray material of its material
	std::vector<Xray_Mesh> make_xray_meshes() const;

	// Returns the triangle meshes of the objects in millimetres, in the local space of each object (no transform applied), for the X-ray
	// library to move them in place with get_xray_transform
	std::vector<Xray_Mesh> make_xray_local_meshes() const;

	// Returns the transform of an object for the X-ray library: 16 floats in the column-major order of OpenGL, with the translation in
	// the last column in millimetres, to apply to the positions of make_xray_local_meshes (the identity if the object does not exist)
	std::array<float, 16> get_xray_transform(std::size_t object_index) const;

	// Returns the first object whose bounding sphere (its centre is the position of the object, its radius the farthest vertex times the
	// largest scale) contains the point, in centimetres, or null if there is none. The tube must stay outside of the objects
	const Scene_Object* find_object_around_point(const std::array<float, 3>& point) const;

	// Returns the first object whose bounding sphere touches a rectangle (the detector plate), or null if there is none. The rectangle
	// is given by its centre, two perpendicular unit vectors along its sides, and its half width and half height, in centimetres
	const Scene_Object* find_object_touching_rectangle(
		const std::array<float, 3>& centre,
		const std::array<float, 3>& right,
		const std::array<float, 3>& up,
		float                       half_width,
		float                       half_height) const;

	// Getters
	const std::vector<Scene_Object>& get_objects() const { return objects; }

	// Returns a number that grows with every change of the scene, so that the views know that they have to look at it again
	std::uint64_t get_revision() const { return revision; }

private:
	// Private Attributes
	// ------------------
	std::vector<Scene_Object> objects;
	std::uint64_t             revision{ 0 };
};
