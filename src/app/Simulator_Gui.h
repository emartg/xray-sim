/*
 * Simulator_Gui.h
 * This file defines the Simulator_Gui class, which is the graphical user interface of the simulator.
 * It implements the Gui_Layer interface: the platform renderer owns the ImGui context and backends,
 * and calls this class to draw the windows every frame.
 * For now it draws a placeholder window, the list of the objects of the scene with the panel of the selected object (its data, and the
 * combo of its material), and the X-ray view of the scene (see Xray_View), it moves the camera of the 3D view with the mouse (see
 * update_camera), and
 * it can run the self-test of the X-ray library inside the first frame (see run_xray_self_test), until the panels of the
 * simulator are written.
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "platform/gui/Gui_Layer.h"
#include "scene/Scene.h"

class Scene_Sync;
#ifdef XRSIM_HAS_XRAY
class Xray_View;
#endif

class Simulator_Gui : public Gui_Layer
{
public:
	// Constructors
	// ------------
	// The scene is the one that the list of objects and the X-ray view show (it can be null). If run_xray_self_test is true,
	// the first frame computes the X-ray image of a test object with the X-ray library, on the context of the window, and
	// checks it (see run_xray_self_test). If run_xray_view_test is true, the first X-ray image of the scene is checked (see
	// Xray_View::check_image). Without the X-ray part of the simulator, they have no effect
	explicit Simulator_Gui(std::shared_ptr<Scene> scene = nullptr, bool run_xray_self_test = false, bool run_xray_view_test = false);

	// Destructor
	// ----------
	// Releases the X-ray view while the OpenGL context is current (the renderer destroys the GUI before the window)
	~Simulator_Gui() override;

	// Public Methods
	// --------------
	// Draws the ImGui windows of the simulator. Called every frame by the renderer
	void draw() override;

	// Sets the ids of the Engine's nodes of the objects of the scene, in the order of the objects (see Scene_Builder::build). Until
	// then the panel cannot tell which object is selected
	void set_node_ids(std::vector<std::uint32_t> node_ids);

private:
	// Private Methods
	// ---------------
	// Computes the X-ray image of a cube of water of 10 cm, with a monochromatic beam of 80 keV, and checks that the
	// image is valid, that the cube attenuates the beam as the Beer-Lambert law predicts (within a tolerance), and that
	// the state of OpenGL is the same before and after the computation. It prints the result with the format of the
	// Engine ("[SUCCESS::..." or "[ERROR::...")
	void run_xray_self_test();

	// Draws the table of the objects and the panel of the selected object, with the combo of its material
	void draw_scene_window();

	// Moves the camera of the 3D view with the mouse, over the 3D view (not over a window of the GUI, and not in the single X-ray view):
	// a drag with the right button orbits about the pivot (see get_camera_pivot), a drag with the middle button pans, and the wheel
	// dollies towards the pivot (see Orbit_Camera). The pivot of a drag is the one of the moment its button was pressed
	void update_camera();

	// Returns the point that the camera orbits about: the position of the selected object, or the centre of the scene (the origin) if
	// no object is selected
	std::array<float, 3> get_camera_pivot() const;

	// Moves the Engine's camera: it orbits about camera_pivot by orbit_x and orbit_y and pans across the view by pan_x and pan_y (the
	// movements of the mouse, in pixels), and dollies towards the pivot of the moment by the turns of the wheel, in notches
	void move_camera(float orbit_x, float orbit_y, float pan_x, float pan_y, float wheel);

	// Changes the material of an object and deletes the node of another one, as the user does, and checks that the colour of the node,
	// the scene, and the X-ray image follow. Runs once, after the checks of the first X-ray image (--xray-view-test). It prints the
	// result with the format of the Engine
	void run_scene_change_test();

	// Checks that a click in the 3D view selects an object: the first call queues a click at the centre of the window, where the default
	// scene shows the aluminium cube, and the next frames check which object is selected (the Engine resolves a click in the frame
	// after it). Runs after run_scene_change_test (--xray-view-test). It prints the result with the format of the Engine
	void run_selection_test();

	// Checks that the camera of the 3D view orbits about the selected object, pans, and dollies, as the mouse moves it: it changes the
	// Engine's camera through the same steps as update_camera, checks its position and its angles, and puts it back. Runs after
	// run_selection_test, with the aluminium cube selected (--xray-view-test). It prints the result with the format of the Engine
	void run_camera_test();

	// Private Attributes
	// ------------------
	std::shared_ptr<Scene>      scene;
	std::unique_ptr<Scene_Sync> scene_sync; // keeps the Engine's nodes and the scene in step, and tells which object is selected
	bool                        is_xray_self_test_requested{ false };
	bool                        is_scene_change_test_requested{ false };
	int                         selection_test_stage{ 0 }; // 0: not requested or done, 1: queue the click, 2: check the selection
	int                         selection_test_tries{ 0 };
	bool                        is_camera_test_requested{ false };
	bool                        is_orbiting{ false }; // a drag with the right button started over the 3D view, and goes on
	bool                        is_panning{ false };  // a drag with the middle button started over the 3D view, and goes on
	std::array<float, 3>        camera_pivot{};       // the pivot of the drag, in centimetres
#ifdef XRSIM_HAS_XRAY
	std::unique_ptr<Xray_View> xray_view; // the X-ray image of the scene (only with the X-ray part)
#endif
};
