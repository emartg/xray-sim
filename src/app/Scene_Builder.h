/*
 * Scene_Builder.h
 * This file defines the Scene_Builder class, which fills the Engine's scene from the simulator's description of a scene:
 * the camera, the lights, and a shape node for each object, drawn with the Engine's forward rendering. The geometry,
 * the transform, and the colour of each node come from the Scene, which also feeds the X-ray meshes. The Engine's view is drawn in
 * decimetres (see VIEW_UNITS_PER_CENTIMETRE), while the scene is in centimetres.
 */

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "scene/Scene.h"

class Core;

class Scene_Builder
{
public:
	// Public Constants
	// ----------------
	// The scale of the Engine's 3D view: one unit of the Engine is 10 cm of the scene (a decimetre). The Engine (v0.9.0) draws only what
	// is between 0.1 and 100 units from the camera, so in centimetres it could not show anything farther than 1 m, and the X-ray tube
	// and its detector span 1.5 m or more; in decimetres the view reaches from 1 cm to 10 m. Only the Engine's view is scaled: the
	// scene, its controls, and the X-ray image stay in centimetres
	static constexpr float VIEW_UNITS_PER_CENTIMETRE{ 0.1f };

	// Public Static Methods
	// ---------------------
	// Converts a position or a scale of the scene (in centimetres) to the units of the Engine's view
	static glm::vec3 to_view(const std::array<float, 3>& values);

	// Converts a position of the Engine's view to centimetres of the scene
	static std::array<float, 3> from_view(const glm::vec3& position);

	// Creates the camera (looking at the objects from the front and above), a directional light, and a node for each
	// object of the scene, and adds them to the Engine. The Engine has to be initialised. Returns the id of the node of each object, in
	// the order of the objects of the scene (the camera and the light have ids too, and are not in the list)
	static std::vector<std::uint32_t> build(const Scene& scene, Core* engine);
};
