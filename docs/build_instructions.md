# Build Instructions

This document explains how to build and run the simulator (`XRSim`). The simulator builds the engine as a
subdirectory, so the details of the prerequisites, the dependencies of the engine, and the troubleshooting of
the platforms are in the [engine's build instructions](../engine/docs/build_instructions.md); this document
only covers what is specific to this repository. For now, only the engine is built: the gVirtualXRay
submodule is not part of the build yet.

## Prerequisites

- **Operating System:** Windows or Linux. macOS is not supported, since it only provides OpenGL up to 4.1.
- **GPU and drivers:** OpenGL 4.5 core profile support (a recent Mesa on Linux is enough, including the `llvmpipe` software renderer).
- **Compiler:** MSVC from Visual Studio 2022/2026, or GCC from a MinGW installation on Windows; GCC or Clang on Linux.
- **CMake:** 3.25 or newer, to use the presets in `CMakePresets.json`.
- **vcpkg (Windows):** a Git clone of [vcpkg](https://github.com/microsoft/vcpkg), with the `VCPKG_ROOT` environment variable pointing to it.
- **Git:** to clone the repository and its submodules.

## Getting the Sources

The engine is a Git submodule, so clone the repository with its submodules (see the [README](../README.md)), or fetch them afterwards:

```
git submodule update --init engine
```

CMake stops with a message if the engine is missing.

## Dependencies

The dependencies are the ones of the engine, declared in the `vcpkg.json` manifest of this repository (vcpkg manifest mode):

- **Windows (every preset):** vcpkg builds and installs them into the build directory during the first configuration. The first one takes a long time, and vcpkg stores the built packages in its binary cache for the next ones.
- **Linux:** the system packages. On Ubuntu:

```
sudo apt-get install ninja-build libglfw3-dev libassimp-dev libglm-dev libgl-dev
```

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

`XRSim --frames N` renders `N` frames and exits (it is used by the automated tests). The executable is at `bin/` in the build directory (`bin/Debug/` or `bin/Release/` with the Visual Studio generators).

## Tests

The tests are built with `-DXRSIM_BUILD_TESTS=ON` (off by default). Both are labelled `smoke`:

- `XRSim_Smoke_Test` renders 60 frames, and fails if the exit code is not zero or if a line contains `[ERROR`. It needs an OpenGL 4.5 display (on Linux without one, run it with `xvfb-run -a`, which needs the `xvfb` and `libgl1-mesa-dri` packages).
- `XRSim_Invalid_Argument_Test` checks that an invalid argument is rejected. It needs no display.

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
