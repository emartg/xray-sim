## Top-level CMakeLists.txt for the X-ray simulator.
## It builds the simulator application (XRSim) on top of the Engine, which is a Git submodule
## consumed through the add_subdirectory() command (only its Core and Platform libraries are built)

cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.21)

# Project name and languages
project(XRaySimProject LANGUAGES C CXX)

# Set C++ standard (the same as the Engine's, which the simulator consumes)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(XRSIM_BUILD_TESTS "Build the simulator tests" OFF)

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

# Add the Engine (Core and Platform libraries; its sample App is not built when it is not the top-level project)
add_subdirectory(engine)

# The sample App of the Engine must not be built for the simulator
if(TARGET App)
	message(FATAL_ERROR "The Engine's sample App must not be built for the simulator (ENGINE_BUILD_APP must be OFF)")
endif()

# Add the simulator application
add_subdirectory(src)
