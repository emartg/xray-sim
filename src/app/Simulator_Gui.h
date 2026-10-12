/*
 * Simulator_Gui.h
 * This file defines the Simulator_Gui class, which is the graphical user interface of the simulator.
 * It implements the Gui_Layer interface: the platform renderer owns the ImGui context and backends,
 * and calls this class to draw the windows every frame.
 * For now it only draws a placeholder window, until the panels of the simulator are written.
 */

#pragma once

#include "platform/gui/Gui_Layer.h"

class Simulator_Gui : public Gui_Layer
{
public:
	// Public Methods
	// --------------
	// Draws the ImGui windows of the simulator. Called every frame by the renderer
	void draw() override;
};
