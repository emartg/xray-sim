/*
 * Tube_Display.h
 * This file defines the Tube_Display class, which draws the X-ray setup in the Engine's 3D view: the housing of the tube around the
 * source, the detector plate at its real size and orientation, and the four edges of the beam, from the source to the corners of the
 * detector. They are nodes of the Engine only, outside of the X-ray scene: they are not objects of the scene, so the X-ray image does
 * not see them. The nodes follow the setup that the user chooses, and are hidden when the user turns the display off.
 * The lengths are in centimetres, like the rest of the scene (the nodes take them in the units of the view, see Scene_Builder).
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "scene/Xray_Setup.h"

class Tube_Display
{
public:
	// Public Constants
	// ----------------
	// The housing of the tube is a cylinder centred on the source, with its axis across the beam, along the right of the image (as the
	// axis of a real tube, from the cathode to the anode, is across the beam); the detector plate is a box whose front face is the plane
	// of the detector, and the edges of the beam are thin boxes. They only show the setup: their sizes carry no clinical meaning
	static constexpr float       HOUSING_RADIUS{ 7.0f };  // in centimetres
	static constexpr float       HOUSING_LENGTH{ 25.0f }; // in centimetres
	static constexpr float       PLATE_THICKNESS{ 1.5f }; // in centimetres, behind the plane of the detector
	static constexpr float       EDGE_THICKNESS{ 0.4f };  // in centimetres
	static constexpr std::size_t HOUSING{ 0 }, PLATE{ 1 }, FIRST_EDGE{ 2 }, NODE_COUNT{ 6 }; // the index of each node

	// Public Methods
	// --------------
	// Shows the setup in the 3D view, or hides its nodes. The nodes are created the first time that they are shown (the Engine has to be
	// initialised, and the context current), and again if one of them was deleted in the Engine; they are moved only when the geometry
	// of the setup changed. When they are hidden, they cannot be picked, and the selection is cleared if one of them was selected
	void update(const Xray_Setup& setup, bool is_shown);

	// Returns the id of the Engine's node of each part (HOUSING, PLATE, and the four edges from FIRST_EDGE), or 0 for a node that is not
	// created
	const std::array<std::uint32_t, NODE_COUNT>& get_node_ids() const { return node_ids; }

	// Returns the centre, the axes (three unit vectors: the columns of the rotation), and the size in centimetres of the box (or of the
	// cylinder, whose axis is the second one) that a part shows for a setup. The nodes are made of a cube and a cylinder of 1 cm, so the
	// size is also their scale
	struct Part_Placement
	{
		std::array<float, 3>                centre{};
		std::array<std::array<float, 3>, 3> axes{};
		std::array<float, 3>                size{};
	};
	static Part_Placement get_placement(const Xray_Setup& setup, std::size_t part);

private:
	// Private Methods
	// ---------------
	// Creates the node of a part and adds it to the Engine
	void create_node(std::size_t part);

	// Gives the nodes the placement of the parts for a setup
	void place_nodes(const Xray_Setup& setup) const;

	// Private Attributes
	// ------------------
	std::array<std::uint32_t, NODE_COUNT> node_ids{};
	std::array<std::array<float, 3>, 5>   placed_points{}; // the source and the corners of the detector that the nodes show
	bool                                  is_placed{ false };
	bool                                  was_shown{ false };
};
