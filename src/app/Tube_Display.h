/*
 * Tube_Display.h
 * This file defines the Tube_Display class, which draws the X-ray setup in the Engine's 3D view: the housing of the tube around the
 * source, the detector plate at its real size and orientation, the four edges of the beam, from the source to the corners of the
 * detector, and the volume of the beam between them. They are nodes of the Engine only, outside of the X-ray scene: they are not
 * objects of the scene, so the X-ray image does not see them. The nodes follow the setup that the user chooses, and their look
 * (Tube_Display_Style: what is shown, the colours and the transparency, and the sizes) follows the user's display options; the whole
 * display is hidden when the user turns it off.
 * The lengths are in centimetres, like the rest of the scene (the nodes take them in the units of the view, see Scene_Builder).
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "scene/Xray_Setup.h"

// The look of the display: for each part, whether it is shown and its colour (red, green, blue, and opacity: below 1, the Engine draws
// it transparent), and the sizes, which only show the setup and carry no clinical meaning. The edges of the beam take the colour of
// the volume, opaque, unless the user unlinks them
struct Tube_Display_Style
{
	struct Part
	{
		bool                 is_shown{ true };
		std::array<float, 4> colour{ 1.0f, 1.0f, 1.0f, 1.0f };

		bool operator==(const Part&) const = default;
	};

	// Public Constants
	// ----------------
	// The limits of the sizes, in centimetres
	static constexpr float MIN_HOUSING_RADIUS{ 2.0f }, MAX_HOUSING_RADIUS{ 20.0f };
	static constexpr float MIN_HOUSING_LENGTH{ 5.0f }, MAX_HOUSING_LENGTH{ 60.0f };
	static constexpr float MIN_PLATE_THICKNESS{ 0.2f }, MAX_PLATE_THICKNESS{ 10.0f };
	static constexpr float MIN_EDGE_THICKNESS{ 0.1f }, MAX_EDGE_THICKNESS{ 3.0f };

	// Public Methods
	// --------------
	// Returns the colour of the edges: the colour of the volume, opaque, when they are linked to it, or their own colour
	std::array<float, 4> get_edge_colour() const;

	bool operator==(const Tube_Display_Style&) const = default;

	// Public Attributes
	// -----------------
	Part  housing{ true, { 0.30f, 0.30f, 0.34f, 1.0f } };
	Part  plate{ true, { 0.55f, 0.62f, 0.72f, 1.0f } };
	Part  edges{ true, { 1.0f, 0.85f, 0.2f, 1.0f } };
	Part  volume{ true, { 1.0f, 0.85f, 0.2f, 0.15f } };
	bool  are_edges_linked_to_volume{ true }; // true: the edges take the colour of the volume, opaque
	float housing_radius{ 7.0f };             // the cylinder around the source, its axis across the beam
	float housing_length{ 25.0f };
	float plate_thickness{ 1.5f }; // behind the plane of the detector
	float edge_thickness{ 0.4f };
};

class Tube_Display
{
public:
	// Public Constants
	// ----------------
	// The index of each node: the housing, the plate, the four edges from FIRST_EDGE, and the volume of the beam
	static constexpr std::size_t HOUSING{ 0 }, PLATE{ 1 }, FIRST_EDGE{ 2 }, VOLUME{ 6 }, NODE_COUNT{ 7 };

	// Public Methods
	// --------------
	// Shows the setup in the 3D view with a style, or hides every node. The nodes are created the first time that they are shown (the
	// Engine has to be initialised, and the context current), and again if one of them was deleted in the Engine; they are moved when
	// the geometry of the setup or a size of the style changed, and take their colours and visibility when the style changed. A
	// hidden node cannot be picked, and the selection is cleared if a node that becomes hidden was selected
	void update(const Xray_Setup& setup, bool is_shown, const Tube_Display_Style& style);

	// Returns the id of the Engine's node of each part (HOUSING, PLATE, the four edges from FIRST_EDGE, and VOLUME), or 0 for a node
	// that is not created
	const std::array<std::uint32_t, NODE_COUNT>& get_node_ids() const { return node_ids; }

	// The centre, the axes (three unit vectors: the columns of the rotation), and the size in centimetres of the box (or of the
	// cylinder, whose axis is the second one, or of the pyramid, whose apex is the centre and whose base is at the size along the third
	// axis) that a part shows for a setup and a style. The nodes are made of a cube, a cylinder, and a pyramid of 1 cm, so the size is
	// also their scale
	struct Part_Placement
	{
		std::array<float, 3>                centre{};
		std::array<std::array<float, 3>, 3> axes{};
		std::array<float, 3>                size{};
	};
	static Part_Placement get_placement(const Xray_Setup& setup, const Tube_Display_Style& style, std::size_t part);

	// Returns whether a part is shown, and its colour, for a style
	static bool                 get_is_part_shown(const Tube_Display_Style& style, std::size_t part);
	static std::array<float, 4> get_part_colour(const Tube_Display_Style& style, std::size_t part);

private:
	// Private Methods
	// ---------------
	// Creates the node of a part and adds it to the Engine
	void create_node(std::size_t part);

	// Gives the nodes the placement of the parts for a setup and a style
	void place_nodes(const Xray_Setup& setup, const Tube_Display_Style& style) const;

	// Gives the nodes their visibility (is_shown is the switch of the whole display) and their colours for a style, and clears the
	// selection if a node that is hidden is selected
	void apply_look(bool is_shown, const Tube_Display_Style& style) const;

	// Private Attributes
	// ------------------
	std::array<std::uint32_t, NODE_COUNT> node_ids{};
	std::array<std::array<float, 3>, 5>   placed_points{}; // the source and the corners of the detector that the nodes show
	Tube_Display_Style                    applied_style;   // the style that the nodes show
	bool                                  is_placed{ false };
	bool                                  is_look_applied{ false };
	bool                                  was_shown{ false };
};
