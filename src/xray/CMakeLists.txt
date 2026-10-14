## CMakeLists.txt for the X-ray library of the simulator (XRSim_Xray), which adapts gVirtualXRay to the simulator.
## It is the only library that links gVirtualXRay (whose target requires C++23): the Engine's headers are never
## included here, and the headers of this library do not include gVirtualXRay's, so that the rest of the simulator
## keeps the C++ standard of the Engine and the two OpenGL loaders (GLAD and GLEW) are never mixed

# Collect header and source files
set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${HEADERS_DIR}/*.h*)

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCES_DIR}/*.cpp)

# Create the static library (headers and sources are listed so the IDEs display them), and a namespaced alias
add_library(XRSim_Xray STATIC ${SOURCES} ${HEADERS})
add_library(XRSim::Xray ALIAS XRSim_Xray)

# The headers are included as "xray/<header>.h" from the source directory of the simulator
target_include_directories(XRSim_Xray PUBLIC ${PROJECT_SOURCE_DIR}/src)

# gVirtualXRay is linked privately: its include directories, definitions, and C++ standard do not reach the consumers
target_link_libraries(XRSim_Xray PRIVATE gVirtualXRay::gVirtualXRay)

# On Linux, gVirtualXRay is compiled with code coverage instrumentation in the Debug configuration, so the executables
# that use it must be linked with the coverage runtime
if(UNIX AND NOT APPLE)
	target_link_libraries(XRSim_Xray PRIVATE "$<$<CONFIG:Debug>:--coverage>")
endif()

# Organize files into source groups for Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})
