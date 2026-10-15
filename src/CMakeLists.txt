## CMakeLists.txt for the simulator application (XRSim)

# Collect header and source files of the application (the libraries have their own directories)
set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/app)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${HEADERS_DIR}/*.h*)

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/app)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCES_DIR}/*.cpp)

# The library that adapts gVirtualXRay to the simulator
if(XRSIM_BUILD_XRAY)
	add_subdirectory(xray)
endif()

# Create the executable (headers and sources are listed so the IDEs display them), linked only against
# the Platform library of the Engine (which provides the Core library)
add_executable(XRSim ${SOURCES} ${HEADERS})
target_link_libraries(XRSim PRIVATE Engine::Platform)

# Copy the runtime dependencies (DLLs) beside the executable (MinGW only, see the function of the Engine)
engine_stage_runtime_dependencies(XRSim)

# The X-ray part: the application uses the adapter of gVirtualXRay
if(XRSIM_BUILD_XRAY)
	target_link_libraries(XRSim PRIVATE XRSim_Xray)
	target_compile_definitions(XRSim PRIVATE XRSIM_HAS_XRAY)

	# The DLL of xraylib (a shared library on Windows, bundled with gVirtualXRay) is copied once beside the executables
	# of the simulator, by a target that they depend on. If each executable copied it in its own post-build step, the two
	# copies could run at the same time (the executables are in the same folder and are linked in parallel), and one of
	# them would fail with "permission denied" because the other has the file open
	if(WIN32 AND XRAYLIB_SHARED_LIB_PATH)
		add_custom_target(XRSim_Xraylib_Dll
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${XRAYLIB_SHARED_LIB_PATH} $<TARGET_FILE_DIR:XRSim>
			COMMENT "Copying the DLL of xraylib beside the executables")
		add_dependencies(XRSim XRSim_Xraylib_Dll)
	endif()
endif()

# Organize files into source groups for Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

# Smoke test: create a window, render a few frames, and exit.
# It fails if the executable exits with an error code or prints an error message (the pattern is a character
# class, so that it needs no escape sequence).
# It needs an OpenGL 4.5 display (e.g., xvfb-run with Mesa on Linux)
if(XRSIM_BUILD_TESTS)
	add_test(NAME XRSim_Smoke_Test COMMAND XRSim --frames 60)
	set_tests_properties(XRSim_Smoke_Test PROPERTIES
		LABELS smoke
		FAIL_REGULAR_EXPRESSION "[[]ERROR"
		TIMEOUT 300
	)

	# An invalid argument must be rejected before the window is created (no display is needed)
	add_test(NAME XRSim_Invalid_Argument_Test COMMAND XRSim --frames 0)
	set_tests_properties(XRSim_Invalid_Argument_Test PROPERTIES
		LABELS smoke
		WILL_FAIL TRUE
		TIMEOUT 60
	)

	# Self-test of the X-ray library: in the first frame, on the context of the window, it computes the X-ray image of
	# a cube of water and checks it and the state of OpenGL. It needs an OpenGL 4.5 display, like the smoke test
	if(XRSIM_BUILD_XRAY)
		add_test(NAME XRSim_Xray_Self_Test COMMAND XRSim --frames 3 --xray-self-test)
		set_tests_properties(XRSim_Xray_Self_Test PROPERTIES
			LABELS smoke
			PASS_REGULAR_EXPRESSION "SUCCESS::SIMULATOR_GUI::run_xray_self_test"
			FAIL_REGULAR_EXPRESSION "[[]ERROR"
			TIMEOUT 300
		)
	endif()
endif()
