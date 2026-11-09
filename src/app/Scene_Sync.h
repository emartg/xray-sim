/*
 * Scene_Sync.h
 * This file defines the Scene_Sync class, which keeps the Engine's nodes in step with the simulator's Scene. The Scene is the source
 * of truth: when the material of an object changes, the colour of its node follows; when its position, rotation, or scale changes, the
 * node moves; and when the user deletes a node in the Engine
 * (the Delete key deletes the selected node), the object is removed from the Scene, so that the 3D view and the X-ray image keep
 * showing the same objects. It also tells which object is selected, from the selection of the Engine (a click in the 3D view).
 * It uses the Engine's singleton, so it has to be used after the Engine is initialised, from the frame of the Engine.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "scene/Scene.h"

class Scene_Sync
{
public:
	// Constructors
	// ------------
	explicit Scene_Sync(std::shared_ptr<Scene> scene);

	// Public Methods
	// --------------
	// Sets the ids of the nodes of the Engine, one for each object of the scene, in the order of the objects (see Scene_Builder::build)
	void set_node_ids(std::vector<std::uint32_t> ids);

	// Brings the Engine's nodes and the scene in step: an object whose node does not exist any more is removed from the scene (a warning
	// is printed), the node of an object whose material changed takes the colour of the new material, and the node of an object that
	// moved, turned, or was scaled takes its position, rotation, and scale. Cheap when nothing changed
	void update();

	// Returns the index of the object whose node is selected in the Engine, or nothing if no node is selected or the selected node
	// is not an object of the scene (the camera and the light can be selected too)
	std::optional<std::size_t> get_selected_object() const;

	// Returns the id of the node of an object, or 0 if there is no such object
	std::uint32_t get_node_id(std::size_t object_index) const;

private:
	// Private Attributes
	// ------------------
	// What a node shows of its object
	struct Applied_State
	{
		std::size_t          material_index{ 0 };
		std::array<float, 3> position{};
		std::array<float, 3> rotation{};
		std::array<float, 3> scale{};
	};

	static Applied_State make_applied_state(const Scene_Object& object);

	std::shared_ptr<Scene>     scene;
	std::vector<std::uint32_t> node_ids;      // the id of the node of each object, in the order of the objects
	std::vector<Applied_State> applied_state; // what the node of each object shows
};
