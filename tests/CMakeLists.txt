## CMakeLists.txt for the tests of the libraries of the simulator

# Test of the pin guard (cmake/submodule_pins.cmake): it creates small Git repositories in the build directory and runs the guard on them, so it
# needs Git (it is skipped without it) and no display
add_test(NAME Submodule_Pin_Guard_Test COMMAND ${CMAKE_COMMAND}
	-DGUARD_MODULE=${PROJECT_SOURCE_DIR}/cmake/submodule_pins.cmake
	-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/submodule_pin_guard
	-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/submodule_pin_guard_test.cmake)
set_tests_properties(Submodule_Pin_Guard_Test PROPERTIES
	LABELS unit
	PASS_REGULAR_EXPRESSION "SUCCESS::SUBMODULE_PIN_GUARD_TEST"
	SKIP_REGULAR_EXPRESSION "SKIPPED"
	FAIL_REGULAR_EXPRESSION "[[]ERROR"
	TIMEOUT 120
)

# Test of the scene library (shapes, materials, and the default scene): it needs no OpenGL context, so it runs on any machine
add_executable(Scene_Test scene/SceneTest.cpp)
target_link_libraries(Scene_Test PRIVATE XRSim::Scene)

add_test(NAME Scene_Test COMMAND Scene_Test)
set_tests_properties(Scene_Test PROPERTIES
	LABELS unit
	FAIL_REGULAR_EXPRESSION "[[]ERROR"
	TIMEOUT 60
)

# Test of the setup of the source and the detector (conversion and limits): it needs no OpenGL context either
add_executable(Xray_Setup_Test scene/XraySetupTest.cpp)
target_link_libraries(Xray_Setup_Test PRIVATE XRSim::Scene)

add_test(NAME Xray_Setup_Test COMMAND Xray_Setup_Test)
set_tests_properties(Xray_Setup_Test PROPERTIES
	LABELS unit
	FAIL_REGULAR_EXPRESSION "[[]ERROR"
	TIMEOUT 60
)

# Test of the display library (the conversion of an X-ray image to grey values): it needs no OpenGL context either
add_executable(Xray_Display_Test display/XrayDisplayTest.cpp)
target_link_libraries(Xray_Display_Test PRIVATE XRSim::Display)

add_test(NAME Xray_Display_Test COMMAND Xray_Display_Test)
set_tests_properties(Xray_Display_Test PROPERTIES
	LABELS unit
	FAIL_REGULAR_EXPRESSION "[[]ERROR"
	TIMEOUT 60
)

# Test of the adapter of gVirtualXRay (only with the X-ray part): it needs no OpenGL context, so it runs on any machine
if(XRSIM_BUILD_XRAY)
	add_executable(Gvxr_Adapter_Test xray/Gvxr_AdapterTest.cpp)
	target_link_libraries(Gvxr_Adapter_Test PRIVATE XRSim::Xray)

	# Copy the runtime dependencies (DLLs) beside the executable (MinGW only, see the function of the Engine)
	engine_stage_runtime_dependencies(Gvxr_Adapter_Test)

	# The executables of the simulator and of its tests are in the same folder and copy the same DLLs beside themselves. The
	# DLL of xraylib is copied once, by a target (see src/CMakeLists.txt), and the test is built after the application, so that
	# the copies of the DLLs of vcpkg do not run at the same time either: a copy that finds the file open by another one fails
	# with "permission denied"
	if(TARGET XRSim_Xraylib_Dll)
		add_dependencies(Gvxr_Adapter_Test XRSim_Xraylib_Dll)
	endif()
	add_dependencies(Gvxr_Adapter_Test XRSim)

	add_test(NAME Gvxr_Adapter_Test COMMAND Gvxr_Adapter_Test)
	set_tests_properties(Gvxr_Adapter_Test PROPERTIES
		LABELS unit
		FAIL_REGULAR_EXPRESSION "[[]ERROR"
		TIMEOUT 60
	)
endif()
