# Build Instructions

This document explains how to build and run the simulator (`XRSim`). The simulator builds the engine as a
subdirectory, so the details of the prerequisites, the dependencies of the engine, and the troubleshooting of
the platforms are in the [engine's build instructions](../engine/docs/build_instructions.md); this document
only covers what is specific to this repository. The simulator also builds the core library of the
gVirtualXRay fork (the X-ray part, option `XRSIM_BUILD_XRAY`, on by default except with MSVC). On Linux its
dependencies are the system packages, and on Windows they are built by vcpkg, like the engine's. With MSVC the
option is off by default until the fork fixes the way it forces the inclusion of a header (it breaks its own library
with MSVC); pass `-DXRSIM_BUILD_XRAY=ON` to try it anyway (MinGW on Windows is supported).

## Prerequisites

- **Operating System:** Windows or Linux. macOS is not supported, since it only provides OpenGL up to 4.1.
- **GPU and drivers:** OpenGL 4.5 core profile support (a recent Mesa on Linux is enough, including the `llvmpipe` software renderer).
- **Compiler:** MSVC from Visual Studio 2022/2026, or GCC from a MinGW installation on Windows; GCC or Clang on Linux.
- **CMake:** 3.26 or newer (the minimum of gVirtualXRay), which also covers the presets in `CMakePresets.json`.
- **vcpkg (Windows):** a Git clone of [vcpkg](https://github.com/microsoft/vcpkg), with the `VCPKG_ROOT` environment variable pointing to it.
- **Git and Git LFS:** to clone the repository and its submodules (the configuration of gVirtualXRay checks that Git LFS is installed).

## Getting the Sources

The engine and gVirtualXRay are Git submodules, so clone the repository with its submodules (see the [README](../README.md)), or fetch them afterwards:

```
git submodule update --init engine gvirtual-xray
```

CMake stops with a message if the engine, or gVirtualXRay when the X-ray part is on, is missing. The build does not need the Git LFS files of gVirtualXRay (data and assets): set the environment variable `GIT_LFS_SKIP_SMUDGE=1` before fetching the submodule to skip them.

## Dependencies

The dependencies are the ones of the engine, declared in the `vcpkg.json` manifest of this repository (vcpkg manifest mode), plus the ones of gVirtualXRay:

- **Windows (every preset):** vcpkg builds and installs them into the build directory during the first configuration, for the triplet of the preset (`x64-windows-xray` with MSVC and `x64-mingw-dynamic-xray` with MinGW). These triplets are in `cmake/vcpkg_triplets` and are the ones of vcpkg (`x64-windows` and `x64-mingw-dynamic`) with GLEW as a static library, which gVirtualXRay requires. The first configuration takes a long time, and vcpkg stores the built packages in its binary cache for the next ones.
- **Linux:** the system packages. On Ubuntu:

```
sudo apt-get install ninja-build libglfw3-dev libassimp-dev libglm-dev libgl-dev \
  libglu1-mesa-dev libegl-dev libglew-dev libtiff-dev libfreetype-dev zlib1g-dev libboost-dev libpng-dev libbz2-dev
```

gVirtualXRay is built with these system packages and with its own copy of xraylib. A missing package is not an error: gVirtualXRay would build that dependency from sources instead (and download it), so check that no line of the configuration log starts with `-- Build` followed by the name of a library. Pass `-DXRSIM_BUILD_XRAY=OFF` to build only the engine and the application.

The dependencies of the engine can change when the pinned version of the engine changes (the manifest and the package list above are kept in sync with the pinned version).

## Building and Running

Use one of the presets of `CMakePresets.json`. The names are the same as the engine's: `ninja-msvc-debug`, `ninja-msvc-release`, `vs2022-msvc-*`, `vs2026-msvc-*`, `mingw-gcc-vcpkg-*` (Windows), and `ninja-gcc-*`, `ninja-clang-*` (Linux). The Ninja presets with MSVC must run in an x64 Visual Studio Developer PowerShell (see the engine's instructions).

```powershell
# Windows (Ninja and MSVC, in an x64 developer environment)
cmake --preset ninja-msvc-debug
cmake --build --preset ninja-msvc-debug
.\out\build\ninja-msvc-debug\bin\XRSim.exe
```

```
# Linux
cmake --preset ninja-gcc-debug
cmake --build --preset ninja-gcc-debug
./out/build/ninja-gcc-debug/bin/XRSim
```

The build presets build every target (the application, the libraries, and the tests when they are on). `XRSim --frames N` renders `N` frames and exits (it is used by the automated tests). The executable is at `bin/` in the build directory (`bin/Debug/` or `bin/Release/` with the Visual Studio generators).

## Tests

The tests are built with `-DXRSIM_BUILD_TESTS=ON` (off by default). The first three are labelled `smoke`:

- `XRSim_Smoke_Test` renders 60 frames, and fails if the exit code is not zero or if a line contains `[ERROR`. It needs an OpenGL 4.5 display (on Linux without one, run it with `xvfb-run -a`, which needs the `xvfb` and `libgl1-mesa-dri` packages).
- `XRSim_Invalid_Argument_Test` checks that an invalid argument is rejected. It needs no display.
- `XRSim_Xray_Self_Test` (only with the X-ray part) runs `XRSim --frames 3 --xray-self-test`: in the first frame, on the OpenGL context of the Engine's window, it computes the X-ray image of a cube of water of 10 cm with a beam of 80 keV, and checks that the cube transmits the beam as the Beer-Lambert law predicts (within 3 percent), and that the state of OpenGL is the same before and after the computation. It needs an OpenGL 4.5 display, like the smoke test.
- `XRSim_Xray_View_Test` (only with the X-ray part) runs `XRSim --frames 3 --xray-view-test`: in the first frame the simulator computes the X-ray image of the default scene, and checks that the corners of the image are not attenuated, that the beam is attenuated where the centre of each object projects on the detector, and that the iron sphere attenuates more than the water sphere; then it changes the setup (a lower energy must attenuate more, and another geometry and detector must give a valid image of the new size) and goes back to the first setup. It needs an OpenGL 4.5 display too. The smoke test shows the image as well, without checking it.
- `Scene_Test` (label `unit`) checks the scene library without an OpenGL context: the shapes (closed surfaces, outward triangles, and the volume that each one should enclose), the table of materials, and that the default scene gives X-ray meshes in millimetres with the transform of each object applied.
- `Xray_Setup_Test` (label `unit`) checks the setup of the source and the detector (the default values, the conversion from centimetres to the millimetres of the library, the magnification, and the limits of every value) without an OpenGL context.
- `Xray_Display_Test` (label `unit`) checks the conversion of an X-ray image to grey values (the reference, the transmission, the linear and logarithmic scales, and the negative), without an OpenGL context.
- `Gvxr_Adapter_Test` (label `unit`, only with the X-ray part) checks that the simulator can use gVirtualXRay: the version is reported and the library creates a mixture of chemical elements, without an OpenGL context.

```
cmake --preset ninja-gcc-debug -DXRSIM_BUILD_TESTS=ON
cmake --build --preset ninja-gcc-debug
ctest --test-dir out/build/ninja-gcc-debug --output-on-failure
```

## Code Formatting

The code of `src/` is formatted with clang-format 23.1.1, using the `.clang-format` of the repository. The submodules are excluded (`.clang-format-ignore`). To check the formatting:

```
git ls-files '*.c' '*.cpp' '*.h' '*.hpp' | xargs clang-format --style=file --dry-run --Werror
```

## Continuous Integration

The workflow `.github/workflows/ci.yml` runs on every push to `main` and on every pull request:

- **Format:** the clang-format check above.
- **Linux:** the `ninja-gcc-debug` and `ninja-clang-release` presets, with the system packages; it builds and runs the tests under a virtual display (Xvfb) with Mesa's software OpenGL 4.5 renderer.
- **Windows:** the `ninja-msvc-debug` and `mingw-gcc-vcpkg-release` presets, with the dependencies built by vcpkg (stored in a binary cache, with the triplets `x64-windows-xray-ci` and `x64-mingw-dynamic-xray-ci` of `cmake/vcpkg_triplets`, which leave the compiler out of the ABI hash of the packages so that a new build of the compiler in the runner image does not discard the cache); it builds and runs only the tests that need no display (the invalid argument test, and the adapter test where the X-ray part is built, which is not the case with MSVC for now), since the runners do not support OpenGL 4.5.
