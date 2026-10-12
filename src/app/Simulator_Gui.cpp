/*
 * Simulator_Gui.cpp
 * This file implements the Simulator_Gui class, which is the graphical user interface of the simulator.
 */

#include "Simulator_Gui.h"

#include <imgui.h>

// Public Methods
// --------------
void Simulator_Gui::draw()
{
	// set the position and size explicitly, so that the window is visible from the first frame
	ImGui::SetNextWindowPos(ImVec2{ 20.0f, 20.0f });
	ImGui::SetNextWindowSize(ImVec2{ 360.0f, 80.0f });
	ImGui::Begin("XRSim");
	ImGui::TextUnformatted("Projectional radiography simulator");
	ImGui::End();
}
