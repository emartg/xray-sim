# The triplet x64-windows-xray for continuous integration: it is the same, without the compiler in the ABI hash of the
# packages. The runner images change the build of the compiler without changing the version of its toolset (for example,
# Visual Studio 18.10.2 and 18.10.3 both have the toolset 14.51.36231), and every package would be built again
include("${CMAKE_CURRENT_LIST_DIR}/x64-windows-xray.cmake")
set(VCPKG_DISABLE_COMPILER_TRACKING ON)
