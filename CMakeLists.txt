## Top-level CMakeLists.txt for the X-ray simulator.
## It builds the simulator application (XRSim) on top of the Engine and gVirtualXRay, which are Git submodules
## consumed through the add_subdirectory() command (only the Core and Platform libraries of the Engine, and the
## core library of gVirtualXRay, are built)

# The minimum version of CMake is the one required by gVirtualXRay
cmake_minimum_required(VERSION 3.26)
cmake_policy(VERSION 3.26)

# Project name and languages
project(XRaySimProject LANGUAGES C CXX)

# Set C++ standard (the same as the Engine's, which the simulator consumes)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(XRSIM_BUILD_TESTS "Build the simulator tests" OFF)

# The X-ray part (gVirtualXRay and the library that adapts it). It is off by default with MSVC: the pinned version of
# gVirtualXRay passes an option to every Windows compiler that MSVC does not know, and then compiles a header as a source
# file, which leaves its library without some of its code
if(MSVC)
	set(XRSIM_BUILD_XRAY_DEFAULT OFF)
else()
	set(XRSIM_BUILD_XRAY_DEFAULT ON)
endif()
option(XRSIM_BUILD_XRAY "Build the X-ray part of the simulator (gVirtualXRay and its adapter)" ${XRSIM_BUILD_XRAY_DEFAULT})

# The folders of the submodules must be at the commits that the repository records for them (see cmake/submodule_pins.cmake):
# with this option, the stop becomes a warning
option(XRSIM_ALLOW_UNPINNED_SUBMODULES "Warn instead of stopping when engine/ or gvirtual-xray/ is not at the commit that the repository records for it" OFF)

# Set default build type to Debug if not specified (single-configuration generators only, like Makefile or Ninja)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Gather every binary in the same directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Enable testing with CTest (the tests are registered by the subdirectories, e.g., the smoke test of XRSim)
if(XRSIM_BUILD_TESTS)
	enable_testing()
endif()

# The Engine is a Git submodule, which is empty if the repository was cloned without --recurse-submodules
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/engine/CMakeLists.txt")
	message(FATAL_ERROR "The Engine was not found in ${PROJECT_SOURCE_DIR}/engine.\n"
		"Fetch the Git submodules with: git submodule update --init engine")
endif()

# The build uses the contents of the folders of the submodules, so they must be at the recorded commits (the pins): the Engine
# always, and gVirtualXRay when the X-ray part is built
include(cmake/submodule_pins.cmake)
xrsim_check_submodule_pin(engine)
if(XRSIM_BUILD_XRAY)
	xrsim_check_submodule_pin(gvirtual-xray)
endif()

# Add the Engine (Core and Platform libraries; its sample App is not built when it is not the top-level project)
add_subdirectory(engine)

# The sample App of the Engine must not be built for the simulator
if(TARGET App)
	message(FATAL_ERROR "The Engine's sample App must not be built for the simulator (ENGINE_BUILD_APP must be OFF)")
endif()

# Add the core library of gVirtualXRay, which provides the target gVirtualXRay::gVirtualXRay
if(XRSIM_BUILD_XRAY)
	if(NOT EXISTS "${PROJECT_SOURCE_DIR}/gvirtual-xray/CMakeLists.txt")
		message(FATAL_ERROR "gVirtualXRay was not found in ${PROJECT_SOURCE_DIR}/gvirtual-xray.\n"
			"Fetch the Git submodules with: git submodule update --init gvirtual-xray")
	endif()

	# Build the core library alone, without a GLFW of its own: the Engine provides the window and the OpenGL context,
	# and GLEW (never GLAD, which would duplicate the symbols of the Engine's loader) is initialized after the Engine's
	set(BUILD_SIMPLEGVXR OFF)
	set(BUILD_WRAPPERS OFF)
	set(USE_GLFW OFF)
	set(USE_GLEW ON)

	# Use the packages of the dependencies (the system packages on Linux, vcpkg on Windows), instead of building them from sources
	set(USE_SYSTEM_GLEW ON)
	set(USE_SYSTEM_ASSIMP ON)
	set(USE_SYSTEM_ZLIB ON)
	set(USE_SYSTEM_LIBTIFF ON)
	set(USE_SYSTEM_FREETYPE ON)
	set(USE_SYSTEM_BOOST ON)

	# The simulator does not use Vulkan, and gVirtualXRay only supports it together with its own GLFW, which is turned off
	# (the library would otherwise fail to compile on a machine with the Vulkan loader installed)
	set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan ON)

	add_subdirectory(gvirtual-xray)

	# The tests and SimpleGVXR of gVirtualXRay must not be built for the simulator
	if(TARGET SimpleGVXR OR TARGET TestMixture)
		message(FATAL_ERROR "SimpleGVXR and the tests of gVirtualXRay must not be built for the simulator")
	endif()
endif()

# Add the simulator application
add_subdirectory(src)

# Add the tests of the libraries of the simulator
if(XRSIM_BUILD_TESTS)
	add_subdirectory(tests)
endif()
