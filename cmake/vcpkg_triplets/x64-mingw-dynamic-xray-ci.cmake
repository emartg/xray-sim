# The triplet x64-mingw-dynamic-xray for continuous integration: it is the same, without the compiler in the ABI hash of
# the packages (the runner images change the build of the compiler, and every package would be built again)
include("${CMAKE_CURRENT_LIST_DIR}/x64-mingw-dynamic-xray.cmake")
set(VCPKG_DISABLE_COMPILER_TRACKING ON)
