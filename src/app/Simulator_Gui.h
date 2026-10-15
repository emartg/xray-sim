/*
 * Simulator_Gui.h
 * This file defines the Simulator_Gui class, which is the graphical user interface of the simulator.
 * It implements the Gui_Layer interface: the platform renderer owns the ImGui context and backends,
 * and calls this class to draw the windows every frame.
 * For now it draws a placeholder window, and it can run the self-test of the X-ray library inside the first frame
 * (see run_xray_self_test), until the panels of the simulator are written.
 */

#pragma once

#include "platform/gui/Gui_Layer.h"

class Simulator_Gui : public Gui_Layer
{
public:
	// Constructors
	// ------------
	// If run_xray_self_test is true, the first frame computes the X-ray image of a test object with the X-ray library,
	// on the context of the window, and checks it (see run_xray_self_test)
	explicit Simulator_Gui(bool run_xray_self_test = false);

	// Public Methods
	// --------------
	// Draws the ImGui windows of the simulator. Called every frame by the renderer
	void draw() override;

private:
	// Private Methods
	// ---------------
	// Computes the X-ray image of a cube of water of 10 cm, with a monochromatic beam of 80 keV, and checks that the
	// image is valid, that the cube attenuates the beam as the Beer-Lambert law predicts (within a tolerance), and that
	// the state of OpenGL is the same before and after the computation. It prints the result with the format of the
	// Engine ("[SUCCESS::..." or "[ERROR::...")
	void run_xray_self_test();

	// Private Attributes
	// ------------------
	bool is_xray_self_test_requested{ false };
};
