/*
 * Scene_Material_Table.cpp
 * This file implements the Scene_Material_Table class, which holds the materials that the objects of a scene can have.
 */

#include "scene/Scene_Material_Table.h"

// Public Static Methods
// ---------------------
const std::vector<Scene_Material>& Scene_Material_Table::get_materials()
{
	// densities in g/cm3
	static const std::vector<Scene_Material> materials{
		{ "Water", { 0.30f, 0.60f, 0.90f, 1.0f }, { "H2O", 1.0 } },
		{ "Aluminium", { 0.75f, 0.76f, 0.80f, 1.0f }, { "Al", 2.699 } },
		{ "Iron", { 0.50f, 0.36f, 0.32f, 1.0f }, { "Fe", 7.874 } },
		{ "Bone (calcium)", { 0.95f, 0.92f, 0.80f, 1.0f }, { "Ca", 1.55 } },
	};
	return materials;
}

std::size_t Scene_Material_Table::find(const std::string& name)
{
	const std::vector<Scene_Material>& materials = get_materials();
	for (std::size_t i = 0; i < materials.size(); ++i)
	{
		if (materials[i].name == name)
			return i;
	}
	return materials.size();
}
