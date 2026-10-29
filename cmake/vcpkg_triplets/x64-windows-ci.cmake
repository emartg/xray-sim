# The triplet x64-windows of vcpkg (MSVC, 64 bits, shared libraries) for the continuous integration: the same, without the
# compiler in the hash (ABI) of the packages. It is the host triplet of the CI jobs (both the MSVC job and the MinGW job): the
# tools that vcpkg builds for itself (vcpkg-cmake and others) are an input of the hash of every package, header-only ones
# included, so when the runner image changes the build of cl.exe (the toolset keeps its version) every package would be built
# again. The key of the cache of the workflow has the version of the compiler. Not used by the presets: a developer keeps the
# compiler in the hash, so that a package is built again when the compiler is upgraded
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_PROVIDED_FORTRAN ON)
set(VCPKG_DISABLE_COMPILER_TRACKING ON)