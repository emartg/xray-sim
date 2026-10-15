## CMakeLists.txt for the scene library of the simulator (XRSim_Scene), which describes what the simulator shows: the
## shapes, the materials, and the objects of a scene. It does not depend on OpenGL, on the Engine, or on gVirtualXRay, so it
## is built and tested on every platform (it only uses the plain types of the adapter's header Xray_Types.h)

# Collect header and source files
set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${HEADERS_DIR}/*.h*)

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCES_DIR}/*.cpp)

# Create the static library (headers and sources are listed so the IDEs display them), and a namespaced alias
add_library(XRSim_Scene STATIC ${SOURCES} ${HEADERS})
add_library(XRSim::Scene ALIAS XRSim_Scene)

# The headers are included as "scene/<header>.h" (and "xray/Xray_Types.h") from the source directory of the simulator
target_include_directories(XRSim_Scene PUBLIC ${PROJECT_SOURCE_DIR}/src)

# GLM, which the Engine uses too, so that the transforms are composed in the same way (the target of the Engine's
# directory is not visible here, so the package is found again)
find_package(glm CONFIG REQUIRED)
target_link_libraries(XRSim_Scene PUBLIC glm::glm)

# Organize files into source groups for Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})
