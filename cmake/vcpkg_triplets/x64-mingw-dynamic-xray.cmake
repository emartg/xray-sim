# The triplet x64-mingw-dynamic of vcpkg (MinGW, 64 bits, shared libraries), with GLEW as a static library, because
# gVirtualXRay links GLEW statically (it defines GLEW_STATIC). The other packages keep the linkage of the triplet of vcpkg.
# One more change: the PATH reaches the build of the packages (vcpkg needs it to find the MinGW compiler) but is not part of the
# hash (ABI) of the packages. The triplet of vcpkg tracks it, and then any difference in the PATH (another terminal, VS Code started
# from another shell, a runner of the continuous integration) makes vcpkg build every package again, because no stored package
# matches the new hash
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_ENV_PASSTHROUGH_UNTRACKED PATH)

set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)

if(PORT STREQUAL "glew")
	set(VCPKG_LIBRARY_LINKAGE static)
endif()
