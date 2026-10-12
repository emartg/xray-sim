## CMakeLists.txt for the simulator application (XRSim)

# Collect header and source files
set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${HEADERS_DIR}/*.h*)

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCES_DIR}/*.cpp)

# Create the executable (headers and sources are listed so the IDEs display them), linked only against
# the Platform library of the Engine (which provides the Core library)
add_executable(XRSim ${SOURCES} ${HEADERS})
target_link_libraries(XRSim PRIVATE Engine::Platform)

# Copy the runtime dependencies (DLLs) beside the executable (MinGW only, see the function of the Engine)
engine_stage_runtime_dependencies(XRSim)

# Organize files into source groups for Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

# Smoke test: create a window, render a few frames, and exit.
# It fails if the executable exits with an error code or prints an error message (the pattern is written as
# a character class because the escape sequence \[ is lost when CMake stores the property, which leaves an
# invalid pattern that ctest ignores).
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
endif()
