# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html) (before v1.0.0, minor versions may include breaking changes, and the releases are pre-releases).

## [Unreleased]

### Added

- Top-level CMake project (CMake 3.26, C++20) that builds the Engine and the core library of gVirtualXRay as subdirectories, with a `vcpkg.json` manifest pinned to the Engine's baseline, and CMake presets for Ninja with MSVC, Visual Studio 2022 and 2026, MinGW Makefiles with GCC (Windows), and Ninja with GCC or Clang (Linux), adapted from the Engine's.
- `XRSim`, a minimal application on top of the Engine (a window with a placeholder panel) with the `--frames N` command-line option, and the tests `XRSim_Smoke_Test` and `XRSim_Invalid_Argument_Test`.
- GitHub Actions CI: formatting check with clang-format 23.1.1, Linux builds (GCC Debug and Clang Release, with the tests under a virtual display), and Windows builds (MSVC and MinGW, with a vcpkg binary cache).
- gVirtualXRay, as a Git submodule of the fork, built as its core library alone (option `XRSIM_BUILD_XRAY`, off by default with MSVC), with the system packages on Linux and vcpkg packages on Windows (triplets `x64-windows-xray` and `x64-mingw-dynamic-xray`, which build GLEW as a static library, as the fork requires).
- `XRSim_Xray`, the library that adapts gVirtualXRay to the simulator (it links the library privately and does not expose its headers), with the class `Gvxr_Adapter` and the test `Gvxr_Adapter_Test`.
- Build instructions, the credits of the third-party libraries and resources (`docs/sources.md`), and the formatting configuration of the Engine (`.clang-format` and `.editorconfig`).
- The adapter computes X-ray images: `Gvxr_Adapter` owns the X-ray renderer, the beam, the detector, and the meshes, and computes the image of the meshes on the OpenGL context of the Engine's window, inside the Engine's frame. It saves the state of OpenGL, sets the defaults that the library relies on, and restores the state after each call, and its interface uses plain types (`Xray_Types.h`), without headers of the library or of the Engine. A workaround is needed to register the current context with the library, which cannot adopt an existing one (it goes away when the library provides a function for it).
- `--xray-self-test` option and `XRSim_Xray_Self_Test`: in the first frame, the simulator computes the X-ray image of a cube of water of 10 cm with a beam of 80 keV, and checks that the transmission is the one of the Beer-Lambert law (0.1593, within 3 percent) and that the state of OpenGL is unchanged.

### Fixed

- The Windows jobs of the CI use their vcpkg binary cache when GitHub moves a job to another build of the runner image: the CI triplets leave the compiler out of the ABI hash of the packages.
