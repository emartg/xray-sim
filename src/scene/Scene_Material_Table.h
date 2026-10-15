/*
 * Scene_Material_Table.h
 * This file defines the Scene_Material_Table class, which holds the small table of materials that the objects of a
 * scene can have: for each one, its name, the colour that the Engine draws it with, and its X-ray material (a chemical
 * compound or element and its density). It is a prototype choice: where the X-ray material of an object lives is a
 * decision of the owner that has not been taken yet.
 */

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "xray/Xray_Types.h"

struct Scene_Material
{
	std::string          name;
	std::array<float, 4> albedo; // colour of the object in the Engine (red, green, blue, alpha)
	Xray_Material        xray;
};

class Scene_Material_Table
{
public:
	// Public Static Methods
	// ---------------------
	// Returns the materials: water, aluminium, iron, and bone (calcium). The first one is water
	static const std::vector<Scene_Material>& get_materials();

	// Returns the index of the material with the given name, or the number of materials if there is none
	static std::size_t find(const std::string& name);
};
