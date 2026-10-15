## CMakeLists.txt for the display library of the simulator (XRSim_Display), which converts an X-ray image to the grey values
## that the screen shows. It does not depend on OpenGL, on the Engine, or on gVirtualXRay (it only uses the plain types of the
## adapter's header Xray_Types.h), so it is built and tested on every platform

# Collect header and source files
set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${HEADERS_DIR}/*.h*)

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCES_DIR}/*.cpp)

# Create the static library (headers and sources are listed so the IDEs display them), and a namespaced alias
add_library(XRSim_Display STATIC ${SOURCES} ${HEADERS})
add_library(XRSim::Display ALIAS XRSim_Display)

# The headers are included as "display/<header>.h" (and "xray/Xray_Types.h") from the source directory of the simulator
target_include_directories(XRSim_Display PUBLIC ${PROJECT_SOURCE_DIR}/src)

# Organize files into source groups for Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})
