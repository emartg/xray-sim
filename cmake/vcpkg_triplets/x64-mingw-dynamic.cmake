# The triplet x64-mingw-dynamic of vcpkg (MinGW, 64 bits, shared libraries), with one change: the PATH reaches the build of
# the packages (vcpkg needs it to find the MinGW compiler) but is not part of the hash (ABI) of the packages. The triplet of
# vcpkg tracks it, and then any difference in the PATH (another terminal, VS Code started from another shell, a runner of the
# continuous integration) makes vcpkg build every package again, because no stored package matches the new hash. This file
# has the name of vcpkg's triplet, so it replaces it when the folder is an overlay (VCPKG_OVERLAY_TRIPLETS of the presets)
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_ENV_PASSTHROUGH_UNTRACKED PATH)

set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)
