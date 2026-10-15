/*
 * Scene_Builder.h
 * This file defines the Scene_Builder class, which fills the Engine's scene from the simulator's description of a scene:
 * the camera, the lights, and a shape node for each object, drawn with the Engine's forward rendering. The geometry,
 * the transform, and the colour of each node come from the Scene, which also feeds the X-ray meshes.
 */

#pragma once

#include "scene/Scene.h"

class Core;

class Scene_Builder
{
public:
	// Public Static Methods
	// ---------------------
	// Creates the camera (looking at the objects from the front and above), a directional light, and a node for each
	// object of the scene, and adds them to the Engine. The Engine has to be initialised
	static void build(const Scene& scene, Core* engine);
};
