# The triplet x64-windows of vcpkg (MSVC, 64 bits, shared libraries), with GLEW as a static library, because gVirtualXRay
# links GLEW statically (it defines GLEW_STATIC). The other packages keep the linkage of the triplet of vcpkg
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_PROVIDED_FORTRAN ON)

if(PORT STREQUAL "glew")
	set(VCPKG_LIBRARY_LINKAGE static)
endif()
